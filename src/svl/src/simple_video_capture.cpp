//
// simple_video_capture.cpp
//
//  Created on: 16 июн. 2020 г.
//      Author: devdistress
//
#include "simple_video_capture.h"
#include "vpe_common.h"
#include "vip_common.h"
#include "kms_work.h"
#include "for_debug.h"

namespace svl
{
	SimpleVideoCapture::SimpleVideoCapture(
											const char *dev_name,
											int in_width_vpe, int in_high_vpe,
											const char *format_input_vpe,
											int out_width_vpe, int out_high_vpe,
											const char *format_output_vpe,
											bool use_deinterlace,
											bool need_display_frame
										  )
		:index(-1), need_display_frame_(need_display_frame)
	{
	  vpe_work = new VpeWork();

	  vpe = vpe_work->getVpe();

	  vpe->src.width = in_width_vpe;
	  vpe->src.height= in_high_vpe;
	  vpe_work->describeFormat(format_input_vpe, &vpe->src);

	// Force input format to be single plane
	  vpe->src.coplanar = 0;
	  vpe->dst.width = out_width_vpe;
	  vpe->dst.height= out_high_vpe;
	  vpe_work->describeFormat(format_output_vpe, &vpe->dst);

	  vpe->deint = static_cast<int>(use_deinterlace);
	  vpe->translen = 3; // ToDo: wf?

	  PRINTF("\nInput = %d x %d , %d\nOutput = %d x %d , %d\n",
			  vpe->src.width, vpe->src.height, vpe->src.fourcc,
			vpe->dst.width, vpe->dst.height, vpe->dst.fourcc);

	  if (	vpe->src.height < 0 || vpe->src.width < 0 || vpe->src.fourcc < 0 || \
			vpe->dst.height < 0 || vpe->dst.width < 0 || vpe->dst.fourcc < 0)
			PEXIT("Invalid parameters\n");

	  vip_work = new VipWork(dev_name);

	  vpe->disp =KmsWork::dispOpen();

	  vip_work->vipSetFormat(vpe->src.width, vpe->src.height, vpe->src.fourcc);

	  vip_work->vipReqbuf();

	  vpe_work->vpeInputInit(vpe);

	  allocateSharedBuffers(vpe);

	  VpeWork::vpeOutputInit(vpe);

	  for (int i = 0; i < NUMBUF; i++)
		vip_work->vipQbuf(vpe, i);

	  for (int i = 0; i < NUMBUF; i++)
		VpeWork::vpeOutputQbuf(vpe, i);

	  // Data is ready Now
	}

	// @brief:  allocates shared buffer for vip and vpe
	//
	// @param:  vpe struct vpe pointer
	//
	// @return: 0 on success
	int SimpleVideoCapture::allocateSharedBuffers(Vpe *vpe)
	{
		int i;

		shared_bufs = KmsWork::dispGetVidBuffers(vpe->disp, NUMBUF, vpe->src.fourcc,
						   vpe->src.width, vpe->src.height);
		if (!shared_bufs)
			PEXIT("allocating shared buffer failed\n");

			for (i = 0; i < NUMBUF; i++)
			{
				// Get DMABUF fd for corresponding buffer object
				vpe->input_buf_dmafd[i] = omap_bo_dmabuf(shared_bufs[i]->bo[0]);
				shared_bufs[i]->fd[0] = vpe->input_buf_dmafd[i];
				PRINTF("vpe->input_buf_dmafd[%d] = %d\n", i, vpe->input_buf_dmafd[i]);
			}

		return 0;
	}

	SimpleVideoCapture::~SimpleVideoCapture()
	{
	  // Driver cleanup
	  VpeWork::streamOff(vip_work->vipGetFd(), V4L2_BUF_TYPE_VIDEO_CAPTURE);
	  VpeWork::streamOff(vpe_work->vpeGetFd(), V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	  VpeWork::streamOff(vpe_work->vpeGetFd(), V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	  was_start = false;

	  KmsWork::closeKms(vpe->disp);
	  vpe_work->vpeClose(vpe);
	  vip_work->vipClose();

	  delete vip_work;
	  vip_work	= nullptr;

	  delete vpe_work;
	  vpe_work	= nullptr;
	  vpe		= nullptr;
	}

	bool SimpleVideoCapture::startFrame()
	{
		if(!was_start)
		{
			VpeWork::streamOn(vip_work->vipGetFd(), V4L2_BUF_TYPE_VIDEO_CAPTURE);
			VpeWork::streamOn(vpe_work->vpeGetFd(), V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
			vpe->field = V4L2_FIELD_ANY;
			was_start = true;
		}
	}

	bool SimpleVideoCapture::startReadFrame()
	{
		static bool do_once = false;
		unsigned count = 0;

		{
			index = vip_work->vipDqbuf(vpe);
			vpe_work->vpeInputQbuf(vpe, index);

			if (!do_once)
			{
				++count;
				for (unsigned i = 1; i <= NUMBUF; i++)
				{
					// To star deinterlace, minimum 3 frames needed
					if (vpe->deint && count != 3)
					{
						index = vip_work->vipDqbuf(vpe);
						vpe_work->vpeInputQbuf(vpe, index);
					}
					else
					{
						VpeWork::streamOn(vpe_work->vpeGetFd(), V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
						do_once = true;
						PRINTF("streaming started...\n");
						break;
					}
					++count;
				}
			}

			index = vpe_work->vpeOutputDqbuf(vpe);
			//display_buffer(vpe, index);
		}
	}

	bool SimpleVideoCapture::endReadFrame()
	{
		vpe_work->vpeOutputQbuf(vpe, index);

		index = vpe_work->vpeInputDqbuf(vpe);
		vip_work->vipQbuf(vpe, index);
	}

	bool SimpleVideoCapture::getFrame(VideoEncLib::IFrameBuffer *ptr_to_mem)
	{

		startFrame();
		startReadFrame();

		if(ptr_to_mem != nullptr)
		{
			memcpy(ptr_to_mem->yBuff()->dataMap(), omap_bo_map(vpe->disp_bufs[index]->bo[0]), omap_bo_size(vpe->disp_bufs[index]->bo[0]));		// omap_bo_size(vpe->disp_bufs[index]->bo[0]));
			memcpy(ptr_to_mem->uvBuff()->dataMap(), omap_bo_map(vpe->disp_bufs[index]->bo[1]), omap_bo_size(vpe->disp_bufs[index]->bo[1]));
		}

		if(need_display_frame_)
			KmsWork::displayBuffer(vpe, index);

		endReadFrame();

		return true;
	}

	void SimpleVideoCapture::cmpSize()
	{
		if (omap_bo_size(vpe->disp_bufs[index]->bo[0]) != vpe->dst.size)
			PRINTF ("Error omap_bo_size'%d' != vpe->dst.size'%d'" ,omap_bo_size(vpe->disp_bufs[index]->bo[0]), vpe->dst.size);

		if (omap_bo_size(vpe->disp_bufs[index]->bo[1]) != vpe->dst.size_uv)
			PRINTF ("Error omap_bo_size'%d' != vpe->dst.size_uv'%d'" ,omap_bo_size(vpe->disp_bufs[index]->bo[1]), vpe->dst.size_uv);
	}

	size_t SimpleVideoCapture::getSizeYBuf()
	{
		return omap_bo_size(vpe->disp_bufs[index]->bo[0]);
	}

	size_t SimpleVideoCapture::getSizeUvBuf()
	{
		return omap_bo_size(vpe->disp_bufs[index]->bo[1]);
	}

	void SimpleVideoCapture::display()
{
	startFrame();
	startReadFrame();
	KmsWork::displayBuffer(vpe, index);
	endReadFrame();
}
}
