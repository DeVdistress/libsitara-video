//
// wrap_iframebuffer_for_venclib.h
//
//  Created on: 16 сент. 2020 г.
//      Author: devdistress
#pragma once

#include "videoenclib.h"

#include <kms++/kms++.h>
#include <array>
#include <cstddef>

namespace svl
{
	struct OmapBufferWrapper: public VideoEncLib::IOmapBuffer
	{
		OmapBufferWrapper(int prime_fd_, size_t size__, void* map_):
			prime_fd(prime_fd_), size_(size__), map(map_){}

		OmapBufferWrapper() = default;

		int		dmaBuf() 	const noexcept	override final	{ return prime_fd;	}
		size_t	size() 		const noexcept	override final	{ return size_;		}
		void*	dataMap() 	const noexcept	override final	{ return map;		}

		int			prime_fd	=	{ 0 };
		size_t		size_		=	{ 0 };
		void*		map			= 	{ nullptr };
	};

	class FromVideoEncLibIFrameBuffer_ToKmsFramebuffer: public kms::Framebuffer
	{
		std::array<OmapBufferWrapper, 2> buf{OmapBufferWrapper(), OmapBufferWrapper()};

	public:
		FromVideoEncLibIFrameBuffer_ToKmsFramebuffer(VideoEncLib::IFrameBuffer *ptr);

		kms::PixelFormat format() 		const	override final	{ return kms::PixelFormat::NV12; }
		unsigned num_planes() 			const	override final	{ return kms::get_pixel_format_info(kms::PixelFormat::NV12).num_planes;}
		uint32_t stride(unsigned plane) const	override final	{ return 0; }
		uint32_t size(unsigned plane) 	const	override final	{ return buf[plane].size();}
		uint32_t offset(unsigned plane) const	override final	{ return 0; }
		uint8_t* map(unsigned plane) 			override final	{ return (uint8_t*)buf[plane].map;}
		int prime_fd(unsigned plane)			override final	{ return buf[plane].prime_fd; }
	};

	class FromKmsFramebuffer_ToVideoEncLibIFrameBuffer:	public VideoEncLib::IFrameBuffer
	{
		std::array<OmapBufferWrapper, 2>	buf				{OmapBufferWrapper(), OmapBufferWrapper()};
		kms::Framebuffer					*ptr_frame_buf	{ nullptr };

	public:
		FromKmsFramebuffer_ToVideoEncLibIFrameBuffer(kms::Framebuffer &buf);
		FromKmsFramebuffer_ToVideoEncLibIFrameBuffer() = default;

		VideoEncLib::IOmapBuffer*	yBuff()		override final {
			return dynamic_cast<VideoEncLib::IOmapBuffer*>(&buf[0]);
		}
		VideoEncLib::IOmapBuffer*	uvBuff()	override final {
			return dynamic_cast<VideoEncLib::IOmapBuffer*>(&buf[1]);
		}
		uint32_t					drmBufId()	const noexcept override final {
			return (ptr_frame_buf)?ptr_frame_buf->id():0;
		}
	};
}
