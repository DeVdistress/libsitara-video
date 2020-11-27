// drm_visual.cpp
//
//  Created on: 31 авг. 2020 г.
//      Author: devdistress
//
#include "drm_visual.h"

#include <iostream>

namespace svl
{
	void DRMvisual::addInit(const kms::PixelFormat &pixfmt_)
	{
		ptr_conn = ptr_card->get_first_connected_connector();
		ptr_crt  = ptr_conn->get_current_crtc();

		// printed resolution of lcd
		std::cout << "Display:'" << ptr_crt->width() << "'x'" << ptr_crt->height() << "'" << std::endl;

		pixfmt = pixfmt_;
		getNecessaryPlane();
	}

	void DRMvisual::init(const kms::PixelFormat &pixfmt_)
	{
		if( pixfmt_ != pixfmt)
		{
			ptr_card.reset(new kms::Card());
			addInit(pixfmt_);
		}
	}

	void DRMvisual::initOmap(const kms::PixelFormat &pixfmt_)
	{
		if( pixfmt_ != pixfmt)
		{
			ptr_card.reset(new kms::OmapCard());
			addInit(pixfmt_);
		}
	}

	void DRMvisual::getNecessaryPlane()
	{
		available_planes.clear();

		for (kms::Plane *p : ptr_crt->get_possible_planes())
		{
			if (p->plane_type() != kms::PlaneType::Overlay)
				continue;

			if (!p->supports_format(pixfmt))
				continue;

			available_planes.push_back(p);

			std::cout << "id='" <<	(int)(p->id()) << "' plane_type='" <<
			(int)(p->plane_type()) << "' support_format='" <<
			(int)(p->supports_format(pixfmt)) << "'" << std::endl;
		}
	}

	// do initial plane
	bool DRMvisual::internalInitPlane(
										kms::Plane					*plane,
										kms::Framebuffer			*m_fb,
										const Rectangle<uint32_t>	&display,
										const Rectangle<uint32_t>	&video
									)
	{
		if(
			plane    == nullptr || ptr_conn == nullptr || ptr_crt == nullptr ||
			ptr_card == nullptr || m_fb     == nullptr
		)
			return false;

		kms::Plane	*m_plane = plane;

		// Do initial plane setup with first fb, so that we only need to
		// set the FB when page flipping
		kms::AtomicReq req(*ptr_card);

		kms::Framebuffer *fb = m_fb;

		std::cout << "CRTC_ID: " << ptr_crt->id() << std::endl;
		req.add(m_plane, "CRTC_ID", ptr_crt->id());

		std::cout << "FB_ID: " << fb->id() << std::endl;
		req.add(m_plane, "FB_ID", fb->id());

		std::cout << "CRTC_X: " << display.x << std::endl;
		req.add(m_plane, "CRTC_X", display.x);

		std::cout << "CRTC_Y: " << display.y << std::endl;
		req.add(m_plane, "CRTC_Y", display.y);

		std::cout << "CRTC_W: " << display.w << std::endl;
		req.add(m_plane, "CRTC_W", display.w);

		std::cout << "CRTC_H: " << display.h << std::endl;
		req.add(m_plane, "CRTC_H", display.h);

		std::cout << "SRC_X: " << video.x << std::endl;
		req.add(m_plane, "SRC_X", video.x);

		std::cout << "SRC_Y: " << video.y << std::endl;
		req.add(m_plane, "SRC_Y", video.y);

		std::cout << "SRC_W: " << video.w << std::endl;
		req.add(m_plane, "SRC_W", video.w  << 16);

		std::cout << "SRC_H: " << video.h  << std::endl;
		req.add(m_plane, "SRC_H", video.h  << 16);

		int r = req.commit_sync();
		FAIL_IF(r, "initial plane setup failed");

		history[plane] = std::make_pair(display, video);

		return true;
	}

	bool DRMvisual::internalAddFbIdToPlaneAndChangeGeometry(
													kms::Plane			*m_plane,
													kms::Framebuffer	*m_fb,
													const Rectangle<uint32_t>	&display,
													const Rectangle<uint32_t>	&video
													)
	{
		if(m_plane == nullptr && m_fb == nullptr)
			return false;

		kms::AtomicReq req(*ptr_card);

		req.add(m_plane, "FB_ID",	m_fb->id());
		req.add(m_plane, "CRTC_X",	display.x);
		req.add(m_plane, "CRTC_Y",	display.y);
		req.add(m_plane, "CRTC_W",	display.w);
		req.add(m_plane, "CRTC_H",	display.h);
		req.add(m_plane, "SRC_X",	video.x);
		req.add(m_plane, "SRC_Y",	video.y);
		req.add(m_plane, "SRC_W",	video.w  << 16);
		req.add(m_plane, "SRC_H",	video.h  << 16);

		int r = req.commit_sync();

		if(r<0)
		{
			return false;
		}
		else
		{
			history[m_plane] = std::make_pair(display, video);
			return true;
		}
	}

	bool DRMvisual::internalAddFbIdToPlane(kms::Plane *plane, kms::Framebuffer	*m_fb)
	{
		kms::AtomicReq req(*ptr_card);
		req.add(plane, "FB_ID", m_fb->id());
		int r = req.commit_sync();

		return (r<0)?false:true;
	}

	bool DRMvisual::clearPlane(kms::Plane *plane)
	{
		kms::AtomicReq req(*ptr_card);
		req.add(plane, "FB_ID", 0);
		req.add(plane, "CRTC_ID", 0);
		int r = req.commit_sync();

		return (r<0)?false:true;
	}

	bool DRMvisual::initPlane(
								kms::Plane					*plane,
								kms::Framebuffer			*m_fb,
								const Rectangle<uint32_t>	&display,
								const Rectangle<uint32_t>	&video
				  	  	  	  )
	{
		std::unique_lock<std::mutex> locker(g_lock);
		bool res = internalInitPlane( plane, m_fb, display, video);
		return res;
	}

	bool DRMvisual::initPlane(
								kms::Plane						*plane,
								VideoEncLib::IFrameBuffer		*m_fb,
								const VideoEncLib::Rectangle	&display,
								const VideoEncLib::Rectangle	&video
							)
	{
		std::unique_lock<std::mutex> locker(g_lock);
		bool res = false;

		if(m_fb != nullptr)
		{
			FromVideoEncLibIFrameBuffer_ToKmsFramebuffer buf(m_fb);

			res = internalInitPlane(
									plane, dynamic_cast<kms::Framebuffer*>(&buf),
									Rectangle<uint32_t>(display.x, display.y, display.w, display.h),
									Rectangle<uint32_t>(video.x, video.y, video.w, video.h)
								);
		}
		return res;
	}

	bool DRMvisual::addFbIdToPlaneAndChangeGeometry(
															kms::Plane			*m_plane,
															kms::Framebuffer	*m_fb,
															const Rectangle<uint32_t>	&display,
															const Rectangle<uint32_t>	&video
															)
	{
		std::unique_lock<std::mutex> locker(g_lock);

		bool res = internalAddFbIdToPlaneAndChangeGeometry( m_plane, m_fb, display, video);
		return res;
	}

	bool DRMvisual::addFbIdToPlaneAndChangeGeometry(
													kms::Plane					*m_plane,
													VideoEncLib::IFrameBuffer	*m_fb,
													const VideoEncLib::Rectangle &display,
													const VideoEncLib::Rectangle &video
													)
	{
		std::unique_lock<std::mutex> locker(g_lock);

		FromVideoEncLibIFrameBuffer_ToKmsFramebuffer buf(m_fb);

		bool res = internalAddFbIdToPlaneAndChangeGeometry(
															m_plane, &buf,
															Rectangle<uint32_t>(display.x,	display.y,	display.w,	display.h),
															Rectangle<uint32_t>(video.x,	video.y,	video.w,	video.h)
															);
		return res;
	}

	void DRMvisual::addFbIdToPlane(kms::Plane *plane, kms::Framebuffer	*m_fb)
	{
		std::unique_lock<std::mutex> locker(g_lock);

		internalAddFbIdToPlane(plane, m_fb);
	}

	void DRMvisual::addFbIdToPlane(kms::Plane *plane, VideoEncLib::IFrameBuffer *m_fb)
	{
		std::unique_lock<std::mutex> locker(g_lock);

		FromVideoEncLibIFrameBuffer_ToKmsFramebuffer buf(m_fb);
		internalAddFbIdToPlane(plane, dynamic_cast<kms::Framebuffer*>(&buf));
	}

	bool DRMvisual::getGeometry(kms::Plane *plane, Rectangle<uint32_t>	&display, Rectangle<uint32_t> &video)
	{
		if (history.find(plane) != history.end() && plane != nullptr)
		{
			display	= history[plane].first;
			video	= history[plane].second;

			return true;
		}
		return false;
	}

	bool DRMvisual::getGeometry(kms::Plane *plane, VideoEncLib::Rectangle &display,	VideoEncLib::Rectangle &video)
	{
		Rectangle<uint32_t> video_;
		Rectangle<uint32_t> display_;

		bool res = getGeometry(plane, video_, display_);

		if(!res)
			return false;

		display = VideoEncLib::Rectangle(display_.x,	display_.y,	display_.w,	display_.h);
		video   = VideoEncLib::Rectangle(video_.x,		video_.y,	video_.w,	video_.h);
	}
}


