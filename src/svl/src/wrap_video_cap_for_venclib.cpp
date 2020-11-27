//
// wrap_video_cap_for_venclib.cpp
//
//  Created on: 16 сент. 2020 г.
//      Author: devdistress
#include"wrap_video_cap_for_venclib.h"
#include"wrap_iframebuffer_for_venclib.h"

namespace svl
{
	VideoCaptureWrapper::VideoCaptureWrapper(
												const std::string 			&v_d,
												bool						fps_on_screen_,
												bool						run_v_bar_line_on_scr_,
												size_t 						camera_buf_queue_size_,
												bool						use_deinterlacer_,
												kms::Plane					*plane,
												VideoEncLib::Rectangle		display_,
												VideoEncLib::Rectangle		video_,
												bool 						use_50fps_,
												bool 						use_vpe_,
												bool 						use_displayed_,
												kms::PixelFormat			pf_out_of_vpe_
											):
											VideoCapture(
															v_d,
															fps_on_screen_,
															run_v_bar_line_on_scr_,
															camera_buf_queue_size_,
															use_deinterlacer_,
															plane,
															Rectangle<uint32_t>(display_.x, display_.y, display_.w, display_.h),
															Rectangle<uint32_t>(video_.x, video_.y, video_.w, video_.h),
															use_50fps_,
															use_vpe_,
															use_displayed_,
															pf_out_of_vpe_
														){}

	int VideoCaptureWrapper::frameWidth() const noexcept
	{
		return getFrameSize().first;
	}

	int VideoCaptureWrapper::frameHeight() const noexcept
	{
		return getFrameSize().second;
	}

	int VideoCaptureWrapper::fps() const noexcept
	{
		return (getUse50Fps())?50:25;
	}

	VideoEncLib::IFrameBuffer*	VideoCaptureWrapper::waitFrame()
	{
		auto *ptr = waitTheFrame();

		if(ptr)
		{
			wrap = FromKmsFramebuffer_ToVideoEncLibIFrameBuffer(*ptr);
			return dynamic_cast<VideoEncLib::IFrameBuffer*>(&wrap);
		}
		else
			return nullptr;
	}

	void VideoCaptureWrapper::endingProcessFrameNotify()
	{
		continueCapture();
	}

	bool VideoCaptureWrapper::start()
	{
		startCapture();
		return captureIsStarted();
	}

	void VideoCaptureWrapper::stop()
	{
		stopCapture();
	}

	bool VideoCaptureWrapper::isRun() const
	{
		return captureIsStarted();
	}
}
