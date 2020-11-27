//
// vpe_common.cpp
//
//  Created on: 16 июн. 2020 г.
//      Author: devdistress
//
#include "vpe_common.h"
#include "for_debug.h"
#include "kms_work.h"

#include <linux/v4l2-controls.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <xf86drm.h>

#ifdef __cplusplus
 extern "C" {
#endif
	#include <omap_drm.h>
	#include <omap_drmif.h>
#ifdef __cplusplus
            }
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <errno.h>

// @brief:  open the device
//
// @return: vpe  struct vpe pointer
Vpe* VpeWork::vpeOpen()
{
	char devname[20] = "/dev/video0";
	Vpe *vpe;

	vpe = static_cast<Vpe*>(calloc(1, sizeof(*vpe)));

	vpe->fd = open(devname, O_RDWR);

	if(vpe->fd < 0)
		PEXIT("Cant open %s\n", devname);

	PRINTF("vpe:%s open success!!!\n", devname);

	return vpe;
}

// @brief:  close the device and free memory
//
// @param:  vpe  struct vpe pointer
//
// @return: 0 on success
int VpeWork::vpeClose(Vpe *vpe)
{
	close(vpe->fd);
	free(vpe);

	return 0;
}

//@brief:  fills 4cc, size, coplanar, colorspace based on command line input
//
// @param:  format  char pointer
// @param:  image  struct image_params pointer
//
// @return: 0 on success
int VpeWork::describeFormat (const char *format, struct ImageParams *image)
{
        image->size   = -1;
        image->fourcc = -1;
        if (strcmp (format, "rgb24") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_RGB24;
          image->size = image->height * image->width * 3;
          image->coplanar = 0;
          image->colorspace = V4L2_COLORSPACE_SRGB;
        }
        else if (strcmp (format, "bgr24") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_BGR24;
          image->size = image->height * image->width * 3;
          image->coplanar = 0;
          image->colorspace = V4L2_COLORSPACE_SRGB;
        }
        else if (strcmp (format, "argb32") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_RGB32;
          image->size = image->height * image->width * 4;
          image->coplanar = 0;
          image->colorspace = V4L2_COLORSPACE_SRGB;
        }
        else if (strcmp (format, "abgr32") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_BGR32;
          image->size = image->height * image->width * 4;
          image->coplanar = 0;
          image->colorspace = V4L2_COLORSPACE_SRGB;
        }
        else if (strcmp (format, "yuv444") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_YUV444;
          image->size = image->height * image->width * 3;
          image->coplanar = 0;
          image->colorspace = V4L2_COLORSPACE_SMPTE170M;
        }
        else if (strcmp (format, "yvyu") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_YVYU;
          image->size = image->height * image->width * 2;
          image->coplanar = 0;
          image->colorspace = V4L2_COLORSPACE_SMPTE170M;
        }
        else if (strcmp (format, "yuyv") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_YUYV;
          image->size = image->height * image->width * 2;
          image->coplanar = 0;
          image->colorspace = V4L2_COLORSPACE_SMPTE170M;
        }
        else if (strcmp (format, "uyvy") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_UYVY;
          image->size = image->height * image->width * 2;
          image->coplanar = 0;
          image->colorspace = V4L2_COLORSPACE_SMPTE170M;
        }
        else if (strcmp (format, "vyuy") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_VYUY;
          image->size = image->height * image->width * 2;
          image->coplanar = 0;
          image->colorspace = V4L2_COLORSPACE_SMPTE170M;
        }
        else if (strcmp (format, "nv16") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_NV16;
          image->size = image->height * image->width * 2;
          image->coplanar = 1;
          image->colorspace = V4L2_COLORSPACE_SMPTE170M;
        }
        else if (strcmp (format, "nv61") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_NV61;
          image->size = image->height * image->width * 2;
          image->coplanar = 1;
          image->colorspace = V4L2_COLORSPACE_SMPTE170M;
        }
        else if (strcmp (format, "nv12") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_NV12;
          image->size = image->height * image->width * 1.5;
          image->coplanar = 1;
          image->colorspace = V4L2_COLORSPACE_SMPTE170M;
        }
        else if (strcmp (format, "nv21") == 0)
        {
          image->fourcc = V4L2_PIX_FMT_NV21;
          image->size = image->height * image->width * 1.5;
          image->coplanar = 1;
          image->colorspace = V4L2_COLORSPACE_SMPTE170M;
        }
        else
        {
                return 0;
        }

        return 1;
}

// @brief:  sets crop parameters
//
// @param:  vpe  struct vpe pointer
//
// @return: 0 on success
int VpeWork::setCtrl(Vpe *vpe)
{
	int ret;
	struct	v4l2_control ctrl;

	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.id = V4L2_CID_TRANS_NUM_BUFS;
	ctrl.value = vpe->translen;
	ret = ioctl(vpe->fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0)
		PEXIT("vpe: S_CTRL failed\n");

	return 0;
}

// @brief:  Intialize the vpe input by calling set_control, set_format,
//	    set_crop, refbuf ioctls
//
// @param:  vpe  struct vpe pointer
//
// @return: 0 on success
int VpeWork::vpeInputInit(Vpe *vpe)
{
	int ret;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers rqbufs;

	setCtrl(vpe);

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	fmt.fmt.pix_mp.width = vpe->src.width;
	fmt.fmt.pix_mp.height = vpe->src.height;
	fmt.fmt.pix_mp.pixelformat = vpe->src.fourcc;
	fmt.fmt.pix_mp.colorspace = vpe->src.colorspace;
	fmt.fmt.pix_mp.num_planes = vpe->src.coplanar ? 2 : 1;

	switch (vpe->deint)
	{
	case 1:
		fmt.fmt.pix_mp.field = V4L2_FIELD_ALTERNATE;
		break;
	case 2:
		fmt.fmt.pix_mp.field = V4L2_FIELD_SEQ_TB;
		break;
	case 0:
	default:
		fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
		break;
	}

	ret = ioctl(vpe->fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0)
	{
	  PEXIT( "vpe i/p: S_FMT failed: %s\n", strerror(errno));
	}
	else
	{
      vpe->src.size = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
      vpe->src.size_uv = fmt.fmt.pix_mp.plane_fmt[1].sizeimage;
    }

	ret = ioctl(vpe->fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0)
		PEXIT( "vpe i/p: G_FMT_2 failed: %s\n", strerror(errno));

	PRINTF("vpe i/p: G_FMT: width = %u, height = %u, 4cc = %.4s\n",
			fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
			(char*)&fmt.fmt.pix_mp.pixelformat);

	memset(&rqbufs, 0, sizeof(rqbufs));
	rqbufs.count = NUMBUF;
	rqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	rqbufs.memory = V4L2_MEMORY_DMABUF;

	ret = ioctl(vpe->fd, VIDIOC_REQBUFS, &rqbufs);
	if (ret < 0)
		PEXIT( "vpe i/p: REQBUFS failed: %s\n", strerror(errno));

	vpe->src.numbuf = rqbufs.count;
	PRINTF("vpe i/p: allocated buffers = %d\n", rqbufs.count);

	return 0;
}

// @brief:  Initialize vpe output by calling set_format, reqbuf ioctls.
//	    Also allocates buffer to display the vpe output.
//
// @param:  vpe  struct vpe pointer
//
// @return: 0 on success
int VpeWork::vpeOutputInit(Vpe *vpe)
{
	int ret, i;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers rqbufs;
	bool saved_multiplanar;

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width = vpe->dst.width;
	fmt.fmt.pix_mp.height = vpe->dst.height;
	fmt.fmt.pix_mp.pixelformat = vpe->dst.fourcc;
	fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
	fmt.fmt.pix_mp.colorspace = vpe->dst.colorspace;
	fmt.fmt.pix_mp.num_planes = vpe->dst.coplanar ? 2 : 1;

	ret = ioctl(vpe->fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0)
		PEXIT( "vpe o/p: S_FMT failed: %s\n", strerror(errno));

	ret = ioctl(vpe->fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0)
		PEXIT( "vpe o/p: G_FMT_2 failed: %s\n", strerror(errno));

	PRINTF("vpe o/p: G_FMT: width = %u, height = %u, 4cc = %.4s\n",
			fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
			(char*)&fmt.fmt.pix_mp.pixelformat);

	memset(&rqbufs, 0, sizeof(rqbufs));
	rqbufs.count = NUMBUF;
	rqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	rqbufs.memory = V4L2_MEMORY_DMABUF;

	ret = ioctl(vpe->fd, VIDIOC_REQBUFS, &rqbufs);
	if (ret < 0)
		PEXIT( "vpe o/p: REQBUFS failed: %s\n", strerror(errno));

	vpe->dst.numbuf = rqbufs.count;
	PRINTF("vpe o/p: allocated buffers = %d\n", rqbufs.count);

	// disp->multiplanar is used when allocating buffers to enable
	// allocating multiplane buffer in separate buffers.
	// VPE does handle mulitplane NV12 buffer correctly
	// but VIP can only handle single plane buffers
	// So by default we are setup to use single plane and only overwrite
	// it when allocating strictly VPE buffers.
	// Here we saved to current value and restore it after we are done
	// allocating the buffers VPE will use for output.
	saved_multiplanar = vpe->disp->multiplanar;
	vpe->disp->multiplanar = true;

	vpe->disp_bufs = KmsWork::dispGetVidBuffers(vpe->disp, NUMBUF, vpe->dst.fourcc,
					      vpe->dst.width, vpe->dst.height);

	if (!vpe->disp_bufs)
		PEXIT("allocating display buffer failed\n");

	vpe->disp->multiplanar = saved_multiplanar;

	KmsWork::dispGetFb(vpe->disp);

	struct omap_bo *bo[4];

	for (i = 0; i < NUMBUF; i++)
	{
		vpe->output_buf_dmafd[i] = omap_bo_dmabuf(vpe->disp_bufs[i]->bo[0]);
		vpe->disp_bufs[i]->fd[0] = vpe->output_buf_dmafd[i];

		if(vpe->dst.coplanar)
		{
			vpe->output_buf_dmafd_uv[i] = omap_bo_dmabuf(vpe->disp_bufs[i]->bo[1]);
			vpe->disp_bufs[i]->fd[1] = vpe->output_buf_dmafd_uv[i];
		}

		// No Scale back to display resolution
		vpe->disp_bufs[i]->noScale = true;
		PRINTF("vpe->disp_bufs_fd[%d] = %d\n", i, vpe->output_buf_dmafd[i]);
	}

	PRINTF("allocating display buffer success\n");
	return 0;
}

// @brief:  queue buffer to vpe input
//
// @param:  vpe  struct vpe pointer
// @param:  index  buffer index to queue
//
// @return: 0 on success
int VpeWork::vpeInputQbuf(Vpe *vpe, int index)
{
	int ret;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[2];

	PRINTF("vpe: src QBUF (%d):%s field", vpe->field,
		vpe->field==V4L2_FIELD_TOP?"top":"bottom");

	memset(&buf, 0, sizeof buf);
	memset(&planes, 0, sizeof planes);

	planes[0].length = planes[0].bytesused = vpe->src.size;
	if(vpe->src.coplanar)
		planes[1].length = planes[1].bytesused = vpe->src.size_uv;

	planes[0].data_offset = planes[1].data_offset = 0;

	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.memory = V4L2_MEMORY_DMABUF;
	buf.index = index;
	buf.m.planes = &planes[0];
	buf.field = vpe->field;
	if(vpe->src.coplanar)
		buf.length = 2;
	else
		buf.length = 1;

	buf.m.planes[0].m.fd = vpe->input_buf_dmafd[index];
	if(vpe->src.coplanar)
		buf.m.planes[1].m.fd = vpe->input_buf_dmafd_uv[index];

	ret = ioctl(vpe->fd, VIDIOC_QBUF, &buf);
	if (ret < 0)
		PEXIT( "vpe i/p: QBUF failed: %s, index = %d\n",
			strerror(errno), index);

	return 0;
}

// @brief:  queue buffer to vpe output
//
// @param:  vpe  struct vpe pointer
// @param:  index  buffer index to queue
//
// @return: 0 on success
int VpeWork::vpeOutputQbuf(Vpe *vpe, int index)
{
	int ret;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[2];

	PRINTF("vpe output buffer queue\n");

	memset(&buf, 0, sizeof buf);
	memset(&planes, 0, sizeof planes);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.memory = V4L2_MEMORY_DMABUF;
	buf.index = index;
	buf.m.planes = &planes[0];
	if(vpe->dst.coplanar)
		buf.length = 2;
	else
		buf.length = 1;

	buf.m.planes[0].m.fd = vpe->output_buf_dmafd[index];

	if(vpe->dst.coplanar)
		buf.m.planes[1].m.fd = vpe->output_buf_dmafd_uv[index];

	ret = ioctl(vpe->fd, VIDIOC_QBUF, &buf);
	if (ret < 0)
		PEXIT( "vpe o/p: QBUF failed: %s, index = %d\n",
			strerror(errno), index);

	return 0;
}

//@brief:  start stream
//
// @param:  fd  device fd
// @param:  type  buffer type (CAPTURE or OUTPUT)
//
// @return: 0 on success

int VpeWork::streamOn(int fd, int type)
{
	int ret;

	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if (ret < 0)
		PEXIT("STREAMON failed,  %d: %s\n", type, strerror(errno));

	PRINTF("stream ON: done! fd = %d,  type = %d\n", fd, type);

	return 0;
}


//@brief:  stop stream
//
// @param:  fd  device fd
// @param:  type  buffer type (CAPTURE or OUTPUT)
//
// @return: 0 on success
int VpeWork::streamOff(int fd, int type)
{
	int ret;

	ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	if (ret < 0)
		PEXIT("STREAMOFF failed, %d: %s\n", type, strerror(errno));

	PRINTF("stream OFF: done! fd = %d,  type = %d\n", fd, type);

	return 0;
}


// @brief:  dequeue vpe input buffer
//
// @param:  vpe  struct vpe pointer
//
// @return: buf.index index of dequeued buffer
int VpeWork::vpeInputDqbuf(Vpe *vpe)
{
	int ret;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[2];

	PRINTF("vpe input dequeue buffer\n");

	memset(&buf, 0, sizeof buf);
	memset(&planes, 0, sizeof planes);

	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.memory = V4L2_MEMORY_DMABUF;
        buf.m.planes = &planes[0];
        if (vpe->src.coplanar)
		buf.length = 2;
	else
		buf.length = 1;
	ret = ioctl(vpe->fd, VIDIOC_DQBUF, &buf);
	if (ret < 0)
		PEXIT("vpe i/p: DQBUF failed: %s\n", strerror(errno));

	PRINTF("vpe i/p: DQBUF index = %d\n", buf.index);

	return buf.index;
}


// @brief:  dequeue vpe output buffer
//
// @param:  vpe  struct vpe pointer
//
// @return: buf.index index of dequeued buffer
int VpeWork::vpeOutputDqbuf(Vpe *vpe)
{
	int ret;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[2];

	PRINTF("vpe output dequeue buffer\n");

	memset(&buf, 0, sizeof buf);
	memset(&planes, 0, sizeof planes);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.memory = V4L2_MEMORY_DMABUF;
        buf.m.planes = &planes[0];
	if(vpe->dst.coplanar)
		buf.length = 2;
	else
		buf.length = 1;
	ret = ioctl(vpe->fd, VIDIOC_DQBUF, &buf);
	if (ret < 0)
		PEXIT("vpe o/p: DQBUF failed: %s\n", strerror(errno));

	PRINTF("vpe o/p: DQBUF index = %d\n", buf.index);

	return buf.index;
}
