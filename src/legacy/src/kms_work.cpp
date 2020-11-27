// kms_work.cpp
//
//  Created on: 17 июн. 2020 г.
//      Author: devdistress
#include "kms_work.h"
#include "for_debug.h"

// init of static propertys
int 		KmsWork::global_fd 		= 0;
uint32_t 	KmsWork::used_planes 	= 0;
int 		KmsWork::ndisplays 		= 0;

void KmsWork::freeBuffers(Display *disp, uint32_t n)
{
	uint32_t i;
	for (i = 0; i < n; i++)
	{
		if (disp->buf[i])
		{
			close(disp->buf[i]->fd[0]);
			omap_bo_del(disp->buf[i]->bo[0]);

			if(disp->multiplanar)
			{
				close(disp->buf[i]->fd[1]);
				omap_bo_del(disp->buf[i]->bo[1]);
			}
		}
    }
	free(disp->buf);
}

Buffer** KmsWork::allocBuffers(Display *disp, uint32_t n, uint32_t fourcc, uint32_t w, uint32_t h)
{
	Buffer **bufs;
	uint32_t i = 0;

	bufs = (Buffer **)calloc(n, sizeof(*bufs));
	if (!bufs)
	{
		ERROR("allocation failed");
		goto fail;
	}

	for (i = 0; i < n; i++)
	{
		bufs[i] = allocBuffer(disp, fourcc, w, h);
		if (!bufs[i])
		{
			ERROR("allocation failed");
			goto fail;
		}
	}
	disp->buf=bufs;
	return bufs;

fail:
	// XXX cleanup
	return NULL;
}

Buffer* KmsWork::allocBuffer(Display *disp, uint32_t fourcc, uint32_t w, uint32_t h)
{
	BufferKms *buf_kms;
	Buffer *buf;
	uint32_t bo_handles[4] = {0}, offsets[4] = {0};
	int ret;

	buf_kms = (BufferKms*)calloc(1, sizeof(*buf_kms));
	if (!buf_kms) {
		ERROR("allocation failed");
		return NULL;
	}
	buf = &buf_kms->base;

	buf->fourcc = fourcc;
	buf->width = w;
	buf->height = h;
	buf->multiplanar = true;

	buf->nbo = 1;

	if (!fourcc)
		fourcc = FOURCC('A','R','2','4');

	switch(fourcc)
	{
	case FOURCC('A','R','2','4'):
		buf->nbo = 1;
		buf->bo[0] = allocBo(disp, 32, buf->width, buf->height,
				&bo_handles[0], &buf->pitches[0]);
		break;
	case FOURCC('U','Y','V','Y'):
	case FOURCC('Y','U','Y','V'):
		buf->nbo = 1;
		buf->bo[0] = allocBo(disp, 16, buf->width, buf->height,
				&bo_handles[0], &buf->pitches[0]);
		break;
	case FOURCC('N','V','1','2'):
		if (disp->multiplanar)
		{
			buf->nbo = 2;
			buf->bo[0] = allocBo(disp, 8, buf->width, buf->height,
					&bo_handles[0], &buf->pitches[0]);
			buf->fd[0] = omap_bo_dmabuf(buf->bo[0]);
			buf->bo[1] = allocBo(disp, 16, buf->width/2, buf->height/2,
					&bo_handles[1], &buf->pitches[1]);
			buf->fd[1] = omap_bo_dmabuf(buf->bo[1]);
		}
		else
		{
			buf->nbo = 1;
			buf->bo[0] = allocBo(disp, 8, buf->width, buf->height * 3 / 2,
					&bo_handles[0], &buf->pitches[0]);
			buf->fd[0] = omap_bo_dmabuf(buf->bo[0]);
			bo_handles[1] = bo_handles[0];
			buf->pitches[1] = buf->pitches[0];
			offsets[1] = buf->width * buf->height;
			buf->multiplanar = false;
		}
		break;
	case FOURCC('I','4','2','0'):
		buf->nbo = 3;
		buf->bo[0] = allocBo(disp, 8, buf->width, buf->height,
				&bo_handles[0], &buf->pitches[0]);
		buf->bo[1] = allocBo(disp, 8, buf->width/2, buf->height/2,
				&bo_handles[1], &buf->pitches[1]);
		buf->bo[2] = allocBo(disp, 8, buf->width/2, buf->height/2,
				&bo_handles[2], &buf->pitches[2]);
		break;
	default:
		ERROR("invalid format: 0x%08x", fourcc);
		goto fail;
	}

	ret = drmModeAddFB2(disp->fd, buf->width, buf->height, fourcc,
			bo_handles, buf->pitches, offsets, &buf_kms->fb_id, 0);
	if (ret)
	{
		ERROR("drmModeAddFB2 failed: %s (%d)", strerror(errno), ret);
		goto fail;
	}

	return buf;

fail:
	// XXX cleanup
	return NULL;
}

struct omap_bo* KmsWork::allocBo(Display *disp, uint32_t bpp, uint32_t width, uint32_t height, uint32_t *bo_handle, uint32_t *pitch)
{
	struct DisplayKms *disp_kms = TO_DISPLAY_KMS(disp);
	struct omap_bo *bo;
	uint32_t bo_flags = disp_kms->bo_flags;

	if ((bo_flags & OMAP_BO_TILED) == OMAP_BO_TILED) {
		bo_flags &= ~OMAP_BO_TILED;
		if (bpp == 8) {
			bo_flags |= OMAP_BO_TILED_8;
		} else if (bpp == 16) {
			bo_flags |= OMAP_BO_TILED_16;
		} else if (bpp == 32) {
			bo_flags |= OMAP_BO_TILED_32;
		}
	}
	bo_flags |= OMAP_BO_WC;

	if (bo_flags & OMAP_BO_TILED) {
		bo = omap_bo_new_tiled(disp->dev, ALIGN2(width,7), height, bo_flags);
	} else {
		bo = omap_bo_new(disp->dev, width * height * bpp / 8, bo_flags);
	}

	if (bo) {
		*bo_handle = omap_bo_handle(bo);
		*pitch = width * bpp / 8;
		if (bo_flags & OMAP_BO_TILED)
			*pitch = ALIGN2(*pitch, PAGE_SHIFT);
	}

	return bo;
}

Display* KmsWork::dispOpen()
{
	Display *disp;
	int i, fps = 0, no_post = 0;

	disp = dispKmsOpen();

	if (!disp)
	{
		ERROR("unable to create display");
		return NULL;
	}

out:
	disp->rtctl.fps = fps;

	// If buffer posting is disabled from command line, override post
	// functions with empty ones.
	if (no_post)
	{
		disp->post_buffer = emptyPostBuffer;
		disp->post_vid_buffer = emptyPostVidBuffer;
	}

	return disp;
}

int KmsWork::emptyPostBuffer(Display *disp, Buffer *buf)
{
	return 0;
}

int KmsWork::emptyPostVidBuffer(Display *disp, Buffer *buf,
			uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	return 0;
}

Display* KmsWork::dispKmsOpen()
{
	DisplayKms *disp_kms = NULL;
	Display *disp;
	Buffer **bufs;
	int i;

	disp_kms = (DisplayKms*)calloc(1, sizeof(*disp_kms));
	if (!disp_kms)
	{
		ERROR("allocation failed");
		goto fail;
	}
	disp = &disp_kms->base;

	if (!global_fd)
	{
		global_fd = drmOpen("omapdrm", NULL);
		if (global_fd < 0)
		{
			ERROR("could not open drm device: %s (%d)", strerror(errno), errno);
			goto fail;
		}
	}

	disp->fd = global_fd;
	// increment the number of displays counter
	ndisplays++;

	disp->dev = omap_device_new(disp->fd);
	if (!disp->dev)
	{
		ERROR("couldn't create device");
		goto fail;
	}

//  Adds callback's
//	disp->get_buffers = get_buffers;
//	disp->get_vid_buffers = get_vid_buffers;
//	disp->post_buffer = post_buffer;
//	disp->post_vid_buffer = post_vid_buffer;
//	disp->close = close_kms;
//	disp->disp_free_buf = free_buffers ;
	disp_kms->resources = drmModeGetResources(disp->fd);
	if (!disp_kms->resources)
	{
		ERROR("drmModeGetResources failed: %s", strerror(errno));
		goto fail;
	}

	disp_kms->plane_resources = drmModeGetPlaneResources(disp->fd);
	if (!disp_kms->plane_resources)
	{
		ERROR("drmModeGetPlaneResources failed: %s", strerror(errno));
		goto fail;
	}

	disp->multiplanar = true;

	// note: set args to NULL after we've parsed them so other modules know
	// that it is already parsed (since the arg parsing is decentralized)
	{
		Connector *connector = &disp_kms->connector[disp_kms->connectors_count++];
		connector->crtc = -1;
		connector->id = 35;
		const char str_exmpl[] = "1024x768";
		memcpy(connector->mode_str, str_exmpl, sizeof(str_exmpl));
		disp_kms->bo_flags |= OMAP_BO_SCANOUT;
	}

	disp->width = 0;
	disp->height = 0;
	for (i = 0; i < (int)disp_kms->connectors_count; i++)
	{
		Connector *c = &disp_kms->connector[i];
		connectorFindMode(disp, c);
		if (c->mode == NULL)
			continue;
		// setup side-by-side virtual display
		disp->width += c->mode->hdisplay;
		if (disp->height < c->mode->vdisplay)
		{
			disp->height = c->mode->vdisplay;
		}
	}

	MSG("using %d connectors, %dx%d display, multiplanar: %d",
			disp_kms->connectors_count, disp->width, disp->height, disp->multiplanar);

	bufs = getBuffers(disp, 1);
    postBuffer(disp, bufs[0]);

	return disp;

fail:
	// XXX cleanup
	return NULL;
}

Buffer** KmsWork::getBuffers(Display *disp, uint32_t n)
{
	return allocBuffers(disp, n, 0, disp->width, disp->height);
}

void KmsWork::connectorFindMode(Display *disp, Connector *c)
{
	DisplayKms *disp_kms = TO_DISPLAY_KMS(disp);
	drmModeConnector *connector;
	int i, j;

	/* First, find the connector & mode */
	c->mode = NULL;
	for (i = 0; i < disp_kms->resources->count_connectors; i++)
	{
		connector = drmModeGetConnector(disp->fd,
				disp_kms->resources->connectors[i]);

		if (!connector)
		{
			ERROR("could not get connector %i: %s",
					disp_kms->resources->connectors[i], strerror(errno));
			drmModeFreeConnector(connector);
			continue;
		}

		if (!connector->count_modes)
		{
			drmModeFreeConnector(connector);
			continue;
		}

		if (connector->connector_id != c->id)
		{
			drmModeFreeConnector(connector);
			continue;
		}

		for (j = 0; j < connector->count_modes; j++)
		{
			c->mode = &connector->modes[j];
			if (!strcmp(c->mode->name, c->mode_str))
				break;
		}

		// Found it, break out
		if (c->mode)
			break;

		drmModeFreeConnector(connector);
	}

	if (!c->mode)
	{
		ERROR("failed to find mode \"%s\"", c->mode_str);
		return;
	}

	// Now get the encoder */
	for (i = 0; i < connector->count_encoders; i++)
	{
		c->encoder = drmModeGetEncoder(disp->fd,
				connector->encoders[i]);

		if (!c->encoder)
		{
			ERROR("could not get encoder %i: %s",
					disp_kms->resources->encoders[i], strerror(errno));
			continue;
		}

		// Take the fisrt one, if none is assigned
		if (!connector->encoder_id)
		{
			connector->encoder_id = c->encoder->encoder_id;
		}

		if (c->encoder->encoder_id  == connector->encoder_id)
		{
			// find the first valid CRTC if not assigned
			if (!c->encoder->crtc_id)
			{
				int k;
				for (k = 0; k < disp_kms->resources->count_crtcs; ++k) {
					// check whether this CRTC works with the encoder
					if (!(c->encoder->possible_crtcs & (1 << k)))
						continue;

					c->encoder->crtc_id = disp_kms->resources->crtcs[k];
					break;
				}

				if (!c->encoder->crtc_id)
				{
					ERROR("Encoder(%d): no CRTC found!\n", c->encoder->encoder_id);
					drmModeFreeEncoder(c->encoder);
					continue;
				}
			}
			break;
		}
		drmModeFreeEncoder(c->encoder);
	}

	if (c->crtc == -1)
		c->crtc = c->encoder->crtc_id;

	// and figure out which crtc index it is:
	for (i = 0; i < disp_kms->resources->count_crtcs; i++)
	{
		if (c->crtc == (int)disp_kms->resources->crtcs[i])
		{
			c->pipe = i;
			break;
		}
	}
}

int KmsWork::postBuffer(Display *disp, Buffer *buf)
{
	DisplayKms *disp_kms = TO_DISPLAY_KMS(disp);
	BufferKms *buf_kms = TO_BUFFER_KMS(buf);
	int ret, last_err = 0, x = 0;
	uint32_t i;

	for (i = 0; i < disp_kms->connectors_count; i++)
	{
		Connector *connector = &disp_kms->connector[i];

		if (! connector->mode) {
			continue;
		}

		if (! disp_kms->current)
		{
			// first buffer we flip to, setup the mode (since this can't
			// be done earlier without a buffer to scanout)
			MSG("Setting mode %s on connector %d, crtc %d",
					connector->mode_str, connector->id, connector->crtc);

			ret = drmModeSetCrtc(disp->fd, connector->crtc, buf_kms->fb_id,
					x, 0, &connector->id, 1, connector->mode);

			x += connector->mode->hdisplay;
		}
		else
		{
			ret = drmModePageFlip(disp->fd, connector->crtc, buf_kms->fb_id,
					DRM_MODE_PAGE_FLIP_EVENT, disp);
			disp_kms->scheduled_flips++;
		}

		if (ret)
		{
			ERROR("Could not post buffer on crtc %d: %s (%d)",
					connector->crtc, strerror(errno), ret);
			last_err = ret;
			// well, keep trying the reset of the connectors..
		}
	}

	// if we flipped, wait for all flips to complete!
	while (disp_kms->scheduled_flips > disp_kms->completed_flips)
	{
		drmEventContext evctx =
		{
			.version = DRM_EVENT_CONTEXT_VERSION
		};
		evctx.page_flip_handler = pageFlipHandler;

		struct timeval timeout =
		{
			.tv_sec = 3,
			.tv_usec = 0,
		};

		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(disp->fd, &fds);

		ret = select(disp->fd + 1, &fds, NULL, NULL, &timeout);

		if (ret <= 0)
		{
			if (errno == EAGAIN)
			{
				continue;    // keep going
			}
			else
			{
				ERROR("Timeout waiting for flip complete: %s (%d)",
						strerror(errno), ret);
				last_err = ret;
				break;
			}
		}

		drmHandleEvent(disp->fd, &evctx);
	}

	disp_kms->current = buf;

	return last_err;
}

void KmsWork::pageFlipHandler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data)
{
	Display *disp = (Display*)data;
	DisplayKms *disp_kms = TO_DISPLAY_KMS(disp);

	disp_kms->completed_flips++;

	MSG("Page flip: frame=%d, sec=%d, usec=%d, remaining=%d", frame, sec, usec,
			disp_kms->scheduled_flips - disp_kms->completed_flips);
}

Buffer** KmsWork::dispGetVidBuffers(Display *disp, uint32_t n, uint32_t fourcc, uint32_t w, uint32_t h)
{
	Buffer **buffers;
	unsigned int i;

	// DeVdistress
	buffers = getVidBuffers(disp, n, fourcc, w, h);
	//buffers = disp->get_vid_buffers(disp, n, fourcc, w, h);
	if (buffers) {
		// if allocation succeeded, store in the unlocked
		// video buffer list
		list_init(&disp->unlocked);
		for (i = 0; i < n; i++)
			list_add(&buffers[i]->unlocked, &disp->unlocked);
	}

	return buffers;
}

Buffer** KmsWork::getVidBuffers(Display *disp, uint32_t n, uint32_t fourcc, uint32_t w, uint32_t h)
{
	return allocBuffers(disp, n, fourcc, w, h);
}

Buffer* KmsWork::dispGetFb(Display *disp)
{
	Buffer **bufs = getBuffers(disp, 1);
	if (!bufs)
		return NULL;

//DeVdistress no fill
#if(0)
	fill(bufs[0], 42);
	disp_post_buffer(disp, bufs[0]);
#endif

	return bufs[0];
}

void KmsWork::closeKms(Display *disp)
{
	omap_device_del(disp->dev);
	disp->dev = NULL;

	if (used_planes)
	{
		used_planes >>= 1;
	}

	if (--ndisplays == 0)
	{
		close(global_fd);
	}
}

int KmsWork::displayBuffer(Vpe *vpe, int index)
{
	int ret;
	Buffer *buf;

	buf = vpe->disp_bufs[index];

//  centure video
	ret = dispPostVidBuffer(vpe->disp, buf, (vpe->disp->width - vpe->dst.width)/2,
			(vpe->disp->height - vpe->dst.height)/2, vpe->dst.width, vpe->dst.height);

	if (ret)
		PEXIT("disp post vid buf failed\n");

	return 0;
}

int KmsWork::dispPostVidBuffer(Display *disp, Buffer *buf,
		uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	int ret;

	ret = postVidBuffer(disp, buf, x, y, w, h);
	//if(!ret)
	//	maintain_playback_rate(&disp->rtctl);
	return ret;
}

int KmsWork::postVidBuffer(Display *disp, Buffer *buf,
		uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	DisplayKms *disp_kms = TO_DISPLAY_KMS(disp);
	BufferKms *buf_kms = TO_BUFFER_KMS(buf);
	drmModePlane *ovr;
	int ret = 0;
	uint32_t i, j;

	// ensure we have the overlay setup:
	for (i = 0; i < disp_kms->connectors_count; i++)
	{
		Connector *connector = &disp_kms->connector[i];
		drmModeModeInfo *mode = connector->mode;

		if (! mode)
		{
			continue;
		}

		if (! disp_kms->ovr[i])
		{

			for (j = 0; j < disp_kms->plane_resources->count_planes; j++) {
				if (used_planes & (1 << j)) {
					continue;
				}
				ovr = drmModeGetPlane(disp->fd,
						disp_kms->plane_resources->planes[j]);
				if (ovr->possible_crtcs & (1 << connector->pipe))
				{
					disp_kms->ovr[i] = ovr;
					used_planes |= (1 << j);
					break;
				}
			}
		}

		if (! disp_kms->ovr[i])
		{
			MSG("Could not find plane for crtc %d", connector->crtc);
			ret = -1;
			/* carry on and see if we can find at least one usable plane */
			continue;
		}

		if(buf->noScale)
		{
			ret = drmModeSetPlane(disp->fd, disp_kms->ovr[i]->plane_id,
				connector->crtc, buf_kms->fb_id, 0,
				// Use x and y as co-ordinates of overlay
				x, y, buf->width, buf->height,
				// Consider source x and y is 0 always
				0, 0, w << 16, h << 16);
		}
		else
		{
			ret = drmModeSetPlane(disp->fd, disp_kms->ovr[i]->plane_id,
				connector->crtc, buf_kms->fb_id, 0,
				// make video fullscreen:
				0, 0, mode->hdisplay, mode->vdisplay,
				// source/cropping coordinates are given in Q16
				x << 16, y << 16, w << 16, h << 16);
		}

		if (ret)
		{
			ERROR("failed to enable plane %d: %s",
					disp_kms->ovr[i]->plane_id, strerror(errno));
		}
	}

	if (disp_kms->no_master && disp_kms->mastership)
	{
		/* Drop mastership after the first buffer on each plane is
		 * displayed. This will lock these planes for us and allow
		 * others to consume remaining
		 */
		disp_kms->mastership = 0;
		drmDropMaster(disp->fd);
	}

	return ret;
}






