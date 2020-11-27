// kms_work.h
//
//  Created on: 17 июн. 2020 г.
//      Author: devdistress
#pragma once

#include "vpe_common.h"

#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include <cstdlib>

#include <xf86drmMode.h>
#include <cstdint>

#ifdef __cplusplus
 extern "C" {
#endif
	#include <omap_drm.h>
	#include <omap_drmif.h>
#ifdef __cplusplus
            }
#endif

#ifdef FOURCC
	#undef FOURCC
#endif
#define FOURCC(a, b, c, d) ((uint32_t)(uint8_t)(a) | ((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) | ((uint32_t)(uint8_t)(d) << 24 ))
#ifdef FOURCC_STR
	#undef FOURCC_STR
#endif
#define FOURCC_STR(str)    FOURCC(str[0], str[1], str[2], str[3])

#ifndef MIN
#  define MIN(a,b)     (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#  define MAX(a,b)     (((a) > (b)) ? (a) : (b))
#endif

#ifndef PAGE_SHIFT
#  define PAGE_SHIFT 12
#endif

#ifndef PAGE_SIZE
#  define PAGE_SIZE (1 << PAGE_SHIFT)
#endif

// align x to next highest multiple of 2^n
#ifdef ALIGN2
	#undef ALIGN2
#endif
#define ALIGN2(x,n)   (((x) + ((1 << (n)) - 1)) & ~((1 << (n)) - 1))

#ifdef TO_BUFFER_KMS
	#undef TO_BUFFER_KMS
#endif
#define TO_BUFFER_KMS(x) container_of(x, struct BufferKms, base)

struct BufferKms
{
	struct Buffer base;
	uint32_t fb_id;
};

struct Connector
{
	uint32_t id;
	char mode_str[64];
	drmModeModeInfo *mode;
	drmModeEncoder *encoder;
	int crtc;
	int pipe;
};

#ifdef TO_DISPLAY_KMS
	#undef TO_DISPLAY_KMS
#endif
#define TO_DISPLAY_KMS(x) container_of(x, struct DisplayKms, base)
struct DisplayKms
{
	Display base;

	uint32_t connectors_count;
	Connector connector[10];
	drmModePlane *ovr[10];

	int scheduled_flips, completed_flips;
	uint32_t bo_flags;
	drmModeResPtr resources;
	drmModePlaneRes *plane_resources;
	Buffer *current;
	bool no_master;
	int mastership;
};

struct KmsWork
{
	static void     freeBuffers(Display *disp, uint32_t n);
	static Buffer** allocBuffers(Display *disp, uint32_t n, uint32_t fourcc, uint32_t w, uint32_t h);
	static Buffer*  allocBuffer(Display *disp, uint32_t fourcc, uint32_t w, uint32_t h);
	static struct omap_bo* allocBo(Display *disp, uint32_t bpp, uint32_t width,
			uint32_t height, uint32_t *bo_handle, uint32_t *pitch);
	static int      emptyPostBuffer(Display *disp, Buffer *buf);
	static int      emptyPostVidBuffer(Display *disp, Buffer *buf, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
	static Display* dispOpen();
	static void     connectorFindMode(Display *disp, Connector *c);
	static Display* dispKmsOpen();
	static Buffer** getBuffers(Display *disp, uint32_t n);
	static void     pageFlipHandler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);
	static int      postBuffer(Display *disp, Buffer *buf);
	static Buffer** dispGetVidBuffers(Display *disp, uint32_t n, uint32_t fourcc, uint32_t w, uint32_t h);
	static Buffer** getVidBuffers(Display *disp, uint32_t n, uint32_t fourcc, uint32_t w, uint32_t h);
	static Buffer*  dispGetFb(Display *disp);
	static void     closeKms(Display *disp);
	static int		displayBuffer(Vpe *vpe, int index);
	static int		dispPostVidBuffer(Display *disp, Buffer *buf,
										uint32_t x, uint32_t y, uint32_t w, uint32_t h);
	static int		postVidBuffer(Display *disp, Buffer *buf,
										uint32_t x, uint32_t y, uint32_t w, uint32_t h);


	static int global_fd;
	static uint32_t used_planes;
	static int ndisplays;

	KmsWork() 								= delete;
	KmsWork(const KmsWork &src) 			= delete;
	KmsWork& operator=(const KmsWork &rhs) 	= delete;
	virtual ~KmsWork() 						= delete;
};

