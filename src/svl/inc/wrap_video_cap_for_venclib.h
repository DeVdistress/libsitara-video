//
// wrap_video_cap_for_venclib.h
//
//  Created on: 16 сент. 2020 г.
//      Author: devdistress
#pragma once

#include "video_capture.h"
#include "videoenclib.h"

namespace svl
{
	class VideoCaptureWrapper: public VideoCapture, public VideoEncLib::AbstractFrameSource
	{
		FromKmsFramebuffer_ToVideoEncLibIFrameBuffer wrap;

	public:
		// in case error will throw std::runtime_error("bad fd")
		explicit VideoCaptureWrapper(
										const std::string 			&v_d					= "/dev/video1",
										bool						fps_on_screen_			= false,
										bool						run_v_bar_line_on_scr_	= false,
										size_t 						camera_buf_queue_size_	= 8,
										bool						use_deinterlacer_		= true,
										kms::Plane					*plane					= nullptr,
										VideoEncLib::Rectangle		display_				= VideoEncLib::Rectangle(0, 0, 704, 560),
										VideoEncLib::Rectangle		video_					= VideoEncLib::Rectangle(0, 0, 704, 280),
										bool 						use_50fps_ 				= false,
										bool 						use_vpe_ 				= true,
										bool 						use_displayed_ 			= false,
										kms::PixelFormat			pf_out_of_vpe_			= kms::PixelFormat::NV12
									);

		int							frameWidth()				const noexcept	override final;
		int							frameHeight()				const noexcept	override final;
		int							fps()						const noexcept	override final;
		VideoEncLib::IFrameBuffer*	waitFrame()						 			override final;
		void						endingProcessFrameNotify()					override final;
		bool						start()										override final;
		void						stop()										override final;
		bool						isRun()						const			override final;
	};
}
