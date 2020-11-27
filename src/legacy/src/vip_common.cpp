// vip_common.cpp
//
//  Created on: 16 июн. 2020 г.
//      Author: devdistress
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <cstdint>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "vip_common.h"
#include "for_debug.h"

int VipWork::vipOpen(const char *devname)
{
	vipfd = open(devname, O_RDWR);

	if (vipfd < 0)
		PEXIT("Can't open camera: /dev/video1\n");

	PRINTF("vip open success!!!\n");

	return vipfd;
}

int VipWork::vipClose()
{
	close(vipfd);

	return 0;
}

// @brief:  set format for vip
//
// @param:  width  int
// @param:  height int
// @param:  fourcc int
//
// @return: 0 on success
int VipWork::vipSetFormat(int width, int height, int fourcc)
{
	int ret;
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = fourcc;
	fmt.fmt.pix.field = V4L2_FIELD_ALTERNATE;

	// to change the parameters
	ret = ioctl(vipfd, VIDIOC_S_FMT, &fmt);
	if (ret < 0)
		PEXIT( "vip: S_FMT failed: %s\n", strerror(errno));

	// to query the current parameters
	ret = ioctl(vipfd, VIDIOC_G_FMT, &fmt);
	if (ret < 0)
		PEXIT( "vip: G_FMT after set format failed: %s\n", strerror(errno));

	PRINTF("vip: G_FMT(start): width = %u, height = %u, 4cc = %.4s\n",
			fmt.fmt.pix.width, fmt.fmt.pix.height,
			(char*)&fmt.fmt.pix.pixelformat);

	return 0;
}

//@brief:  request buffer for vip
//  just configures the driver into DMABUF
// @return: 0 on success
int VipWork::vipReqbuf(void)
{
	int ret;
	struct v4l2_requestbuffers rqbufs;

	memset(&rqbufs, 0, sizeof(rqbufs));
	rqbufs.count = NUMBUF;
	rqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rqbufs.memory = V4L2_MEMORY_DMABUF;

	ret = ioctl(vipfd, VIDIOC_REQBUFS, &rqbufs);
	if (ret < 0)
		PEXIT( "vip: REQBUFS failed: %s\n", strerror(errno));

	PRINTF("vip: allocated buffers = %d\n", rqbufs.count);

	return 0;
}

//@brief:  queue shared buffer to vip
//
// @param:  vpe struct vpe pointer
// @param:  index int
//
// @return: 0 on success
int VipWork::vipQbuf(Vpe *vpe, int index)
{
	int ret;
	struct v4l2_buffer buf;

	PRINTF("vip buffer queue\n");

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_DMABUF;
	buf.index = index;
	buf.m.fd = vpe->input_buf_dmafd[index];

	ret = ioctl(vipfd, VIDIOC_QBUF, &buf);
	if (ret < 0)
		PEXIT( "vip: QBUF failed: %s, index = %d\n", strerror(errno), index);

	return 0;
}

// @brief:  dequeue shared buffer from vip
//
// @return: buf.index int
int VipWork::vipDqbuf(Vpe *vpe)
{
	int ret;
	struct v4l2_buffer buf;

	PRINTF("vip dequeue buffer\n");

	memset(&buf, 0, sizeof(buf));

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_DMABUF;
	ret = ioctl(vipfd, VIDIOC_DQBUF, &buf);
	if (ret < 0)
		PEXIT("vip: DQBUF failed: %s\n", strerror(errno));

	PRINTF("vip: DQBUF idx = %d, field = %s\n", buf.index,
		buf.field == V4L2_FIELD_TOP? "Top" : "Bottom");
	vpe->field = buf.field;

	return buf.index;
}
