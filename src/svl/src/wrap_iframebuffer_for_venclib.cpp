//
//wrap_iframebuffer_for_venclib.cpp
//
//  Created on: 16 сент. 2020 г.
//      Author: devdistress
#include"wrap_iframebuffer_for_venclib.h"
#include "drm_visual.h"

namespace svl
{
	static kms::Card& initAndGetCardPtr()
	{
		//svl::DRMvisual::getInstance().init(kms::PixelFormat::NV12);
		return *svl::DRMvisual::getInstance().getCard();
	}

	FromVideoEncLibIFrameBuffer_ToKmsFramebuffer::FromVideoEncLibIFrameBuffer_ToKmsFramebuffer(VideoEncLib::IFrameBuffer *ptr):
			kms::Framebuffer(initAndGetCardPtr(), ptr->drmBufId())
	{
		if (ptr != nullptr)
		{
			int i = 0;
			buf[i].prime_fd = ptr->yBuff()->dmaBuf();
			buf[i].size_    = ptr->yBuff()->size();
			buf[i].map   	= ptr->yBuff()->dataMap();

			buf[++i].prime_fd = ptr->uvBuff()->dmaBuf();
			buf[i].size_    = ptr->uvBuff()->size();
			buf[i].map   	= ptr->uvBuff()->dataMap();
		}
	}

	FromKmsFramebuffer_ToVideoEncLibIFrameBuffer::FromKmsFramebuffer_ToVideoEncLibIFrameBuffer(kms::Framebuffer &buf_)
	{
		ptr_frame_buf = &buf_;

		for (int i=0; i<buf_.num_planes(); ++i)
		{
			buf[i].prime_fd 	=buf_.prime_fd(i);
			buf[i].size_		=buf_.size(i);
			buf[i].map			=(void*)buf_.map(i);
		}
	}
}
