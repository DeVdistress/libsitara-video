// basic_func_v4l2.cpp
//
//  Created on: 15 июн. 2020 г.
//      Author: devdistress
#include "basic_func_v4l2.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <for_debug.h>
#include <cstring>
#include <kms++/kms++.h>

namespace svl
{
 bool BasicFuncV4l2::isCaptureV4l2Dev(int fd)
 {
 	struct v4l2_capability cap = { };
 	int r = ioctl(fd, VIDIOC_QUERYCAP, &cap);
 	ASSERT(r == 0);
 	return cap.capabilities & V4L2_CAP_VIDEO_CAPTURE;
 }

 v4l2_format BasicFuncV4l2::getFormatV4l2Dev(int fd)
 {
 	struct v4l2_format fmt = { };
 	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 	int r = ioctl(fd, VIDIOC_G_FMT, &fmt);
 	ASSERT(r == 0);
 	return fmt;
 }

void BasicFuncV4l2::setFormatV4l2Dev(int fd, const v4l2_format& fmt)
 {
 	int r = ioctl(fd, VIDIOC_S_FMT, &fmt);
 	ASSERT(r == 0);
 }

 int BasicFuncV4l2::bufferExport(int v4lfd, enum v4l2_buf_type bt, uint32_t index, int *dmafd)
 {
 	struct v4l2_exportbuffer expbuf;

 	memset(&expbuf, 0, sizeof(expbuf));
 	expbuf.type = bt;
 	expbuf.index = index;
 	if (ioctl(v4lfd, VIDIOC_EXPBUF, &expbuf) == -1)
 	{
 		PRINTF("Error bufferExport\n");
 		perror("VIDIOC_EXPBUF");
 		return -1;
 	}

 	*dmafd = expbuf.fd;

 	return 0;
 }

 struct v4l2_frmsizeenum BasicFuncV4l2::getFrameSizeV4l2Dev(int fd, kms::PixelFormat pixfmt)
 {
	struct v4l2_frmsizeenum v4lfrms = {};
	v4lfrms.pixel_format = static_cast<uint32_t>(pixfmt);

	// in my opinion this is unnecessary loop, but let it be
	while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &v4lfrms) == 0)
	{
		if (v4lfrms.type != V4L2_FRMSIZE_TYPE_DISCRETE)
		{
			v4lfrms.index++;
			continue;
		}

		v4lfrms.index++;
	}

 	return v4lfrms;
 }

 int BasicFuncV4l2::startStreaming(int fd)
 {
 	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

 	int r = ioctl(fd, VIDIOC_STREAMON, &type);
 	FAIL_IF(r, "Failed to enable camera stream: %d", r);

 	return r;
 }

 int BasicFuncV4l2::stopStreaming(int fd)
 {
 	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

 	int r = ioctl(fd, VIDIOC_STREAMOFF, &type);
 	FAIL_IF(r, "Failed to disable camera stream: %d", r);

 	return r;
 }

 int BasicFuncV4l2::queueBuf(int fd, BasicFuncV4l2::BufferProvider buffer_provider,
		 kms::Framebuffer *const fb, kms::AtomicReq *const req, kms::Plane *const plane)
 {
	uint32_t v4l_mem;

	if (buffer_provider == BasicFuncV4l2::BufferProvider::V4L2)
 		v4l_mem = V4L2_MEMORY_MMAP;
 	else
 		v4l_mem = V4L2_MEMORY_DMABUF;

	if(req != nullptr && plane != nullptr)
		req->add(plane, "FB_ID", fb->id());

	struct v4l2_buffer v4l2buf = { };

	memset(&v4l2buf, 0, sizeof(v4l2buf));
	v4l2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2buf.memory = v4l_mem;
	v4l2buf.index = 0;

	if (buffer_provider == BufferProvider::DRM)
		v4l2buf.m.fd = fb->prime_fd(0);

	int r = ioctl(fd, VIDIOC_QBUF, &v4l2buf);
	ASSERT(r == 0);

	return r;
 }

 int BasicFuncV4l2::dequeueBuf(int fd, BasicFuncV4l2::BufferProvider buffer_provider)
 {
	 uint32_t v4l_mem;

	if (buffer_provider == BasicFuncV4l2::BufferProvider::V4L2)
 		v4l_mem = V4L2_MEMORY_MMAP;
 	else
 		v4l_mem = V4L2_MEMORY_DMABUF;

 	struct v4l2_buffer v4l2buf = { };
 	v4l2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 	v4l2buf.memory = v4l_mem;
 	int r = ioctl(fd, VIDIOC_DQBUF, &v4l2buf);
 	if (r != 0)
 		printf("VIDIOC_DQBUF ioctl failed with %d\n", errno);

 	return r;
 }
}
