//
// simple_video_capture.h
//
//  Created on: 16 июн. 2020 г.
//      Author: devdistress
//
#pragma once

#include "videoenclib.h"

struct Vpe;
struct Buffer;
class  VipWork;
class  VpeWork;

namespace svl
{
	class SimpleVideoCapture
	{
		Vpe		*vpe;
		Buffer **shared_bufs;
		VipWork *vip_work;
		VpeWork *vpe_work;
		unsigned index;
		bool	need_display_frame_;
		bool 	was_start{false};

		int  allocateSharedBuffers(Vpe *vpe);
		bool startFrame();
		bool startReadFrame();
		bool endReadFrame();
		void cmpSize();

	public:
		explicit SimpleVideoCapture(
										const char *dev_name,
										int in_width_vpe, int in_high_vpe,
										const char *format_input_vpe,
										int out_width_vpe, int out_high_vpe,
										const char *format_output_vpe,
										bool use_deinterlace = true,
										bool need_display_frame = false
								   );

		bool getFrame(VideoEncLib::IFrameBuffer *ptr_to_mem);

		size_t getSizeYBuf();
		size_t getSizeUvBuf();

		void display();

		virtual ~SimpleVideoCapture();
	};
}
