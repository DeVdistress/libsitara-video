//
// basic_func_v4l2.h
//
//  Created on: 15 июн. 2020 г.
//      Author: devdistress
//
#pragma once

#include <cstdint>
#include <linux/videodev2.h>
#include <kms++/kms++.h>
#include <kms++util/kms++util.h>

namespace svl
{
 struct BasicFuncV4l2
 {
	 enum class BufferProvider
	 {
	 	DRM,
	 	V4L2,
	 };

 	static bool isCaptureV4l2Dev(int fd);
	static v4l2_format getFormatV4l2Dev(int fd);
	static void setFormatV4l2Dev(int fd, const v4l2_format& fmt);
	static int bufferExport(int v4lfd, enum v4l2_buf_type bt, uint32_t index, int *dmafd);
	static struct v4l2_frmsizeenum getFrameSizeV4l2Dev(int fd, kms::PixelFormat pixfmt);
	static int startStreaming(int fd);
	static int stopStreaming(int fd);
	static int queueBuf(int fd,
						BasicFuncV4l2::BufferProvider buffer_provider,
						kms::Framebuffer *const fb,
						kms::AtomicReq *const req = nullptr,
						kms::Plane *const plane = nullptr);
	static int dequeueBuf(int fd, BasicFuncV4l2::BufferProvider buffer_provider);

	explicit BasicFuncV4l2()								= delete;
	virtual ~BasicFuncV4l2()								= delete;
	BasicFuncV4l2(const BasicFuncV4l2& other)				= delete;
	BasicFuncV4l2& operator=(const BasicFuncV4l2& other)	= delete;
 };
}
