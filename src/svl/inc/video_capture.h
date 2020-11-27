//
// video_capture.h
//
//  Created on: 7 сент. 2020 г.
//      Author: devdistress
//
#pragma once

#include <string>
#include <tuple>
#include <atomic>
#include <kms++/kms++.h>
#include <kms++util/kms++util.h>
#include <kms++util/videodevice.h>
#include <vector>
#include <poll.h>

#include "v4l2-sv.h"
#include "drm_visual.h"

namespace svl
{
class VideoCapture: public VideoDevice
	{
		kms::Plane						*ptr_plane 				{ nullptr					};
		size_t 							camera_buf_queue_size	{ 6							};// note: for deinterlacer needed 6, without deinterlacer 4
		bool 							use_vpe 				{ true						};
		bool 							use_50fps				{ true						};
		bool 							use_displayed			{ true						};
		bool							use_deinterlacer		{ true						};
		Rectangle<uint32_t>				rect_disp_on_screen		{ 0, 0, 704, 560			};
		Rectangle<uint32_t>				rect_video_on_screen	{ 0, 0, 704, 280			};
		kms::PixelFormat				pf_out_of_vpe			{ kms::PixelFormat::NV12	};
		bool							fps_on_screen			{ false 					};
		bool							run_v_bar_line_on_scr	{ false						};

		bool 							was_capture_started		{ false						};
		bool							was_init				{ false						};
		std::atomic<bool>				need_stop				{ false						};
		std::atomic<bool>				need_start				{ false						};
		std::atomic<bool>				once_after_start		{ false						};

		std::unique_ptr <VideoDevice> 	ptr_vpe					{ nullptr					};

		VideoStreamer 					*in						{ nullptr					};
		VideoStreamer 					*in_vpe					{ nullptr					};
		VideoStreamer 					*out_vpe				{ nullptr					};

		svl::DRMvisual::OutWHDef		frm_size_of_cam			{ 0, 0						};

		std::vector<pollfd> 			fds_c;
		std::vector<pollfd> 			fds_vpe;

		bool							do_once					{ false };
		bool							once_get_frame			{ false };
		uint32_t						incremental_frame_num	{ 0 };
		uint32_t						frame_num				{ 0 };
		uint32_t						last_frame_num			{ 0 };
		uint32_t						ind_of_buf				{ 0 };
		uint32_t						frame_order				{ 0 };
		Stopwatch						clock;

		kms::OmapFramebuffer			*dst_fb 				{ nullptr					};
		kms::OmapFramebuffer			*dst_fb_out				{ nullptr					};

		std::string						v4l2_dev_name;

		static constexpr int 			bar_speed 				{ 4							};
		static constexpr int 			bar_width				{ 10						};

		// allocating special only for MMAP memory V4l2
		kms::DmabufFramebuffer* GetDmabufFrameBuffer(int m_fd, kms::Card& card, uint32_t i,
				kms::PixelFormat pixfmt, std::tuple<uint32_t, uint32_t> cam_w_and_h);

		unsigned getBarPos(kms::Framebuffer* fb, unsigned frame_num);
		void countFrameVideo(Stopwatch &clock_, unsigned &cnt_frame, unsigned &history_cnt_frame, bool use_50_fps_);
		void drawTextOnFrame(kms::Framebuffer* fb, unsigned cnt_frame);
		bool onceAfterStartReadyForCaption();
		void onceAfterStop();
		kms::OmapFramebuffer* getFrame();

	public:
		static svl::ListOfV4l2Dev::DataAboutV4l2DeV		getListDev();
		static const svl::DRMvisual::ListOfPlanesDef&	getAvailablePlanesOfDrm();

		bool 				getUseVpe() 			const{ return use_vpe;					}
		bool 				getUse50Fps()			const{ return use_50fps;				}
		bool 				getUseDisplayed()		const{ return use_displayed;			}
		kms::Plane* 		getPlane()				const{ return ptr_plane;				}
		Rectangle<uint32_t> getRectDisplayedOnScreen()	const{ return rect_disp_on_screen;		}
		Rectangle<uint32_t> getRectVideoOnScreen()		const{ return rect_video_on_screen;		}
		kms::PixelFormat	getPixelFormat()			const{ return pf_out_of_vpe;			}
		bool				getFpsOnScreen()			const{ return fps_on_screen;			}
		bool 				getRunVBarLineOnScr()		const{ return run_v_bar_line_on_scr;	}

		// in case error will throw std::runtime_error("bad fd")
		explicit VideoCapture(
								const std::string 			&v_d					= "/dev/video1",
								bool						fps_on_screen_			= false,
								bool						run_v_bar_line_on_scr_	= false,
								size_t 						camera_buf_queue_size_	= 8,
								bool						use_deinterlacer_		= true,
								kms::Plane					*plane					= nullptr,
								Rectangle<uint32_t>			display_				= Rectangle<uint32_t>(0, 0, 704, 560),
								Rectangle<uint32_t>			video_					= Rectangle<uint32_t>(0, 0, 704, 280),
								bool 						use_50fps_ 				= false,
								bool 						use_vpe_ 				= true,
								bool 						use_displayed_ 			= false,
								kms::PixelFormat			pf_out_of_vpe_			= kms::PixelFormat::NV12
							 );

		svl::DRMvisual::OutWHDef getFrameSize() const { return frm_size_of_cam; }

		void startCapture();
		bool captureIsStarted() const { return need_start; }
		void stopCapture();

		kms::OmapFramebuffer* waitTheFrame();

		void continueCapture();

		virtual ~VideoCapture() = default;
	};
}
