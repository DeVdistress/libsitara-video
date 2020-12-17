//
// video_capture.cpp
//
//  Created on: 7 сент. 2020 г.
//      Author: devdistress
//

#include "drm_visual.h"
#include "video_capture.h"
#include "basic_func_v4l2.h"

#include <math.h>

#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>

#include <chrono>
#include <thread>

#include <map>

namespace svl
{
	VideoCapture::VideoCapture(
								const std::string 			&v_d,
								bool						fps_on_screen_,
								bool						run_v_bar_line_on_scr_,
								size_t 						camera_buf_queue_size_,
								bool						use_deinterlacer_,
								kms::Plane					*plane,
								Rectangle<uint32_t>			display_,
								Rectangle<uint32_t>			video_,
								bool 						use_50fps_,
								bool 						use_vpe_,
								bool 						use_displayed_,
								kms::PixelFormat			pf_out_of_vpe_
							 )
	:VideoDevice(v_d),
	 fps_on_screen(fps_on_screen_),						run_v_bar_line_on_scr(run_v_bar_line_on_scr_),
	 camera_buf_queue_size(camera_buf_queue_size_),		use_deinterlacer(use_deinterlacer_),
	 ptr_plane(plane),									rect_disp_on_screen(display_),
	 rect_video_on_screen(video_),						use_50fps(use_50fps_),
	 use_vpe(use_vpe_),									use_displayed(use_displayed_),
	 pf_out_of_vpe(pf_out_of_vpe_)
	{
		v4l2_dev_name = v_d;

		auto lambdaPrintAboutDevice = [](const VideoDevice *ptr)
		{
			std::cout << "fd '" 		<< ptr->fd()			<< "'" << std::endl;
			std::cout << "has_capture '"<< ptr->has_capture()	<< "'" << std::endl;
			std::cout << "has_output '" << ptr->has_output()	<< "'" << std::endl;
			std::cout << "has_m2m '"	<< ptr->has_m2m()		<< "'" << std::endl;
		};

		lambdaPrintAboutDevice(this);

		if(use_vpe)
		{
			ptr_vpe.reset(new VideoDevice ("/dev/video0"));

			lambdaPrintAboutDevice(ptr_vpe.get());
		}

		if(ptr_plane && use_displayed)
		{
			std::cout << "id='" <<	(int)(ptr_plane->id()) << "' plane_type='" <<
			(int)(ptr_plane->plane_type()) << "' support_format='" <<
			(int)(ptr_plane->supports_format(svl::DRMvisual::getInstance().getPixFmt())) << "'" << std::endl;
		}

		in			= get_capture_streamer();

		if(use_vpe)
		{
			in_vpe	= ptr_vpe->get_capture_streamer(true);
			out_vpe	= ptr_vpe->get_output_streamer(true);
		}

		struct v4l2_frmsizeenum frm_size = svl::BasicFuncV4l2::getFrameSizeV4l2Dev(in->fd(), kms::PixelFormat::YUYV);

		frm_size_of_cam.first	= frm_size.discrete.width;
		frm_size_of_cam.second	= frm_size.discrete.height;

		std::cout << "width_cam ='" << frm_size_of_cam.first << "'" <<
		", height_cam ='" << frm_size_of_cam.second << "'" << std::endl;

		in->set_format(kms::PixelFormat::YUYV, frm_size_of_cam.first, frm_size_of_cam.second);
		in->set_queue_size(camera_buf_queue_size);

		if(use_vpe)
		{
			in_vpe->set_format(kms::PixelFormat::YUYV, frm_size_of_cam.first, frm_size_of_cam.second, use_deinterlacer);
			in_vpe->set_queue_size(camera_buf_queue_size);

			out_vpe->set_format(pf_out_of_vpe, frm_size_of_cam.first, frm_size_of_cam.second);
			out_vpe->set_queue_size(camera_buf_queue_size);
		}

		for (unsigned i = 0; i < camera_buf_queue_size; ++i)
		{
			kms::OmapCard *ptr = dynamic_cast<kms::OmapCard*>(svl::DRMvisual::getInstance().getCard());

			if(ptr != nullptr)
			{
				auto fb = new kms::OmapFramebuffer( *ptr, frm_size_of_cam.first, frm_size_of_cam.second, kms::PixelFormat::YUYV);
				in->queue(fb);
			}
		}

		if(use_vpe)
		{
			for (unsigned i = 0; i < camera_buf_queue_size; ++i)
			{
				kms::OmapCard *ptr = dynamic_cast<kms::OmapCard*>(svl::DRMvisual::getInstance().getCard());

				if(ptr != nullptr)
				{
					auto fb = new kms::OmapFramebuffer( *ptr, frm_size_of_cam.first, frm_size_of_cam.second, pf_out_of_vpe);
					out_vpe->queue(fb);
				}
			}
		}

		if(ptr_plane && use_displayed)
		{
			svl::DRMvisual::getInstance().initPlane(
													ptr_plane, (use_vpe)?out_vpe->getVideoBuffer()[0]:in->getVideoBuffer()[0],
													rect_disp_on_screen,
													rect_video_on_screen
													);
		}

		was_init = true;
	}

	svl::ListOfV4l2Dev::DataAboutV4l2DeV	VideoCapture::getListDev()
	{
		svl::ListOfV4l2Dev lst;
		return lst.getV4l2DevList();
	}

	const svl::DRMvisual::ListOfPlanesDef&	VideoCapture::getAvailablePlanesOfDrm()
	{
		return svl::DRMvisual::getInstance().getAvailablePlanes();
	}

	unsigned VideoCapture::getBarPos(kms::Framebuffer* fb, unsigned frame_num)
	{
		return (frame_num * bar_speed) % (fb->width() - bar_width + 1);
	}

	void VideoCapture::countFrameVideo(Stopwatch &clock_, unsigned &cnt_frame, unsigned &history_cnt_frame, bool use_50_fps_)
	{
		++cnt_frame;

		if(clock_.elapsed_ms() >= 1000.0)
		{
			if(cnt_frame > 50)
				cnt_frame = 50;
			else if (!use_50_fps_ && cnt_frame > 25 && cnt_frame < 29)
				cnt_frame = 25;

			if(history_cnt_frame != cnt_frame)
			{
				std::cout << v4l2_dev_name <<" fps='" << cnt_frame << "'" << std::endl;
				history_cnt_frame = cnt_frame;
			}

			cnt_frame = 0;
			clock_.start();
		}
	}

	void VideoCapture::drawTextOnFrame(kms::Framebuffer* fb, unsigned cnt_frame)
	{
		static std::map<kms::Framebuffer*, int> s_bar_pos_map;

		int old_pos = -1;
		if (s_bar_pos_map.find(fb) != s_bar_pos_map.end())
			old_pos = s_bar_pos_map[fb];

		int pos = getBarPos(fb, incremental_frame_num++);

		if(run_v_bar_line_on_scr)
			kms::draw_color_bar(*fb, old_pos, pos, bar_width);

		if(fps_on_screen)
			kms::draw_text(*fb, fb->width() / 2, 0, std::to_string(cnt_frame), kms::RGB(255, 255, 255));

		s_bar_pos_map[fb] = pos;
	}

	void VideoCapture::startCapture()
	{
		if(!need_start)
		{
			need_start = true;
			need_stop  = false;
			once_after_start = true;
		}
	}

	void VideoCapture::stopCapture()
	{
		need_stop = true;
	}

	bool VideoCapture::onceAfterStartReadyForCaption()
	{
		bool res = false;

		if (!was_init || was_capture_started)
			return res;

		fds_c.resize(1);
		for (unsigned i = 0; i < fds_c.size(); ++i)
		{
			fds_c[i].fd = fd();
			fds_c[i].events =  POLLIN;
		}

		if(use_vpe)
		{
			fds_vpe.resize(1);
			for (unsigned i = 0; i < fds_vpe.size(); ++i)
			{
				fds_vpe[i].fd = ptr_vpe->fd();
				fds_vpe[i].events =  POLLIN;
			}
		}

		in->stream_on();

		if(use_vpe)
		{
			in_vpe->stream_on();

			uint32_t count = 0;

			if (!do_once)
			{
				uint32_t ind_of_buf = 0;
				uint32_t frame_order = 0;

				int r = poll(fds_c.data(), fds_c.size(), -1);
				ASSERT(r > 0);

				kms::OmapFramebuffer *dst_fb = dynamic_cast<kms::OmapFramebuffer*>(in->dequeue(&frame_order));

				in_vpe->queue(dst_fb, &frame_order);

				++count;

				for (unsigned i = 1; i <= camera_buf_queue_size; i++)
				{
					// To star deinterlace, minimum 3 frames needed
					if (count != 3 && use_deinterlacer)
					{
						r = poll(fds_c.data(), fds_c.size(), -1);
						ASSERT(r > 0);

						dst_fb = dynamic_cast<kms::OmapFramebuffer*>(in->dequeue(&frame_order));
						in_vpe->queue(dst_fb, &frame_order);
					}
					else
					{
						out_vpe->stream_on();
						do_once = true;
						std::cout << v4l2_dev_name << " streaming started..." << std::endl;
						was_capture_started = true;
						break;
					}
					++count;
				}
			}
		}

		if(fps_on_screen)
			clock.start();

		if(!was_capture_started)
			was_capture_started = true;

		res = was_capture_started;
		return res;
	}

	void VideoCapture::onceAfterStop()
	{
		in->stream_off();

		if(use_vpe)
		{
			in_vpe->stream_off();
			out_vpe->stream_off();
		}

		frame_num = 0;
		last_frame_num = 0;
		ind_of_buf = 0;
		frame_order = 0;
		once_get_frame = false;
		was_capture_started = false;
	}

	kms::OmapFramebuffer* VideoCapture::getFrame()
	{
		try
		{
			uint32_t ind_of_buf = 0;
			uint32_t frame_order = 0;

			int r = poll(fds_c.data(), fds_c.size(), -1);
			ASSERT(r > 0);

			dst_fb = dynamic_cast<kms::OmapFramebuffer*>(in->dequeue(&frame_order));
			dst_fb_out = nullptr;
			kms::OmapFramebuffer* out_buf = nullptr;

			if(use_vpe)
			{
				if(once_get_frame)
					in_vpe->dequeue();

				in_vpe->queue(dst_fb, &frame_order);

				r = poll(fds_vpe.data(), fds_vpe.size(), -1);
				ASSERT(r > 0);

				dst_fb_out = dynamic_cast<kms::OmapFramebuffer*>(out_vpe->dequeue());
			}

			if(use_50fps || frame_order != V4L2_FIELD_BOTTOM)
			{
				if(fps_on_screen)
					countFrameVideo(clock, frame_num, last_frame_num, use_50fps);

				out_buf = (use_vpe)?dst_fb_out:dst_fb;

				if (fps_on_screen || run_v_bar_line_on_scr)
					drawTextOnFrame(out_buf, last_frame_num);

				if(use_displayed && ptr_plane != nullptr)
					svl::DRMvisual::getInstance().addFbIdToPlane(ptr_plane, out_buf);

				if(!once_get_frame)
					once_get_frame = true;

				return out_buf;
			}

			return nullptr;
		}
		catch (std::system_error &se)
		{
			if (se.code() != std::errc::resource_unavailable_try_again)
				FAIL("dequeue failed: %s", se.what());
		}
	}

	kms::OmapFramebuffer* VideoCapture::waitTheFrame()
	{
		MY_LOCKER_MUTEX

		kms::OmapFramebuffer *ptr = nullptr;

		while(need_start)
		{
			if(once_after_start)
			{
				onceAfterStartReadyForCaption();
				once_after_start = false;
			}

			ptr = getFrame();

			if(ptr != nullptr)
				break;

			if(need_stop)
			{
				need_start = false;
				need_stop  = false;
				onceAfterStop();
				break;
			}

			continueCapture();
		}

		return ptr;
	}

	void VideoCapture::continueCapture()
	{
		try
		{
			in->queue(dst_fb);

			if(use_vpe)
				out_vpe->queue(dst_fb_out);
		}
		catch (std::system_error &se)
		{
			if (se.code() != std::errc::resource_unavailable_try_again)
				FAIL("dequeue failed: %s", se.what());
		}
	}

	kms::DmabufFramebuffer* VideoCapture::GetDmabufFrameBuffer(int m_fd, kms::Card& card, uint32_t i, kms::PixelFormat pixfmt,
																std::tuple<uint32_t, uint32_t>cam_w_and_h)
	{
		int r, dmafd;

		uint32_t cam_in_width = 0, cam_in_height = 0;
		std::tie(cam_in_width, cam_in_height) = cam_w_and_h;

		r = svl::BasicFuncV4l2::bufferExport(m_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, i, &dmafd);
		ASSERT(r == 0);

		const kms::PixelFormatInfo& format_info = get_pixel_format_info(pixfmt);
		ASSERT(format_info.num_planes == 1);

		std::vector<int> fds { dmafd };
		std::vector<uint32_t> pitches { cam_in_width * (format_info.planes[0].bitspp / 8) };
		std::vector<uint32_t> offsets { 0 };

		return new kms::DmabufFramebuffer(card, cam_in_width, cam_in_height, pixfmt,
					  fds, pitches, offsets);
	}
}
