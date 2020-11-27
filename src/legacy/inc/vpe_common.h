//
// vpe_common.h
//
//  Created on: 16 июн. 2020 г.
//      Author: devdistress
//
#pragma once

#include <linux/videodev2.h>
#include <cstdint>
#include "list.h"

#define NUMBUF						(6)
#define V4L2_CID_TRANS_NUM_BUFS		(V4L2_CID_PRIVATE_BASE)

struct Buffer
{
	uint32_t fourcc, width, height;
	int nbo;
	void *cmem_buf;
	struct omap_bo *bo[4];
	uint32_t pitches[4];
	struct list unlocked;
	bool multiplanar;	 // True when Y and U/V are in separate buffers.
	int fd[4];		     // dmabuf
	bool noScale;
};

// State variables, used to maintain the playback rate.
struct RateControl
{
	int fps;				// When > zero, we maintain playback rate.
	long last_frame_mark;	// The time when the last frame was displayed,
				 	 	 	// 	as returned by the mark() function.
	int usecs_to_sleep;		// Number of useconds we have slep last frame.
};

struct Display
{
	int fd;
	uint32_t width, height;
	struct omap_device *dev;
	struct list unlocked;
	struct RateControl rtctl;

	struct Buffer ** (*get_buffers)(struct Display *disp, uint32_t n);
	struct Buffer ** (*get_vid_buffers)(struct Display *disp,
			uint32_t n, uint32_t fourcc, uint32_t w, uint32_t h);
	int (*post_buffer)(struct Display *disp, struct Buffer *buf);
	int (*post_vid_buffer)(struct Display *disp, struct Buffer *buf,
			uint32_t x, uint32_t y, uint32_t w, uint32_t h);
	void (*close)(struct Display *disp);
	void (*disp_free_buf) (struct Display *disp, uint32_t n);

	bool multiplanar;	// True when Y and U/V are in separate buffers.
	struct Buffer **buf;
};

struct ImageParams
{
	int width;
	int height;
	int fourcc;
	int size;
	int size_uv;
	int coplanar;
	enum v4l2_colorspace colorspace;
	int numbuf;
};

struct Vpe
{
	int fd;
	int field;
	int deint;
	int translen;
	struct ImageParams src;
	struct ImageParams dst;
	struct v4l2_crop crop;
	int input_buf_dmafd[NUMBUF];
	int input_buf_dmafd_uv[NUMBUF];
	int output_buf_dmafd[NUMBUF];
	int output_buf_dmafd_uv[NUMBUF];
	struct Display *disp;
	struct Buffer **disp_bufs;
};

class VpeWork
{
	 Vpe* vpe_;

	 Vpe* vpeOpen();


public:
	static int  describeFormat (const char *format, ImageParams *image);
	static int  setCtrl(Vpe *vpe);
	static int  vpeInputInit(Vpe *vpe);
	static int  vpeOutputInit(Vpe *vpe);
	static int  vpeInputQbuf(Vpe *vpe, int index);
	static int  vpeOutputQbuf(Vpe *vpe, int index);
	static int  streamOn(int fd, int type);
	static int  streamOff(int fd, int type);
	static int  vpeInputDqbuf(Vpe *vpe);
	static int  vpeOutputDqbuf(Vpe *vpe);
	int vpeGetFd(){ return (vpe_ != nullptr)?vpe_->fd:-1;};

	explicit VpeWork(): vpe_(nullptr) 	{ vpe_ = vpeOpen(); 				};
	virtual ~VpeWork() 					{ vpeClose(vpe_); vpe_ = nullptr;	};
	Vpe* getVpe() 						{ return vpe_; };
	int  vpeClose(Vpe *vpe);

	VpeWork(const VpeWork& other)			= delete;
	VpeWork& operator=(const VpeWork& other)= delete;
};
