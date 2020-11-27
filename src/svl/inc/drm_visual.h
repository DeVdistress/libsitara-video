// drm_visual.h
//
//  Created on: 31 авг. 2020 г.
//      Author: devdistress
//
#pragma once

#include "pattern_singleton.h"
#include "wrap_iframebuffer_for_venclib.h"

#include <kms++/kms++.h>
#include <kms++util/kms++util.h>
#include <kms++/omap/omapkms++.h>

#include <map>
#include <utility>
#include <mutex>
#include <tuple>

namespace svl
{
	template <class T>
	struct Rectangle
	{
		Rectangle(T x_, T y_, T w_, T h_):
			x(x_), y(y_), w(w_), h(h_){}
		Rectangle():x(0), y(0), w(0), h(0){}
		T x;
		T y;
		T w;
		T h;
	};

	class DRMvisual: public SingletonPattern<DRMvisual>
	{
	 public:
		using ListOfPlanesDef				= std::vector<kms::Plane*>;
		using OutXYDef						= std::pair<uint32_t , uint32_t>;
		using OutWHDef						= OutXYDef;
		using InWHDef						= OutXYDef;			//display				//video
		using DisplayAndVideoGeometryDef	= std::pair< Rectangle<uint32_t>, Rectangle<uint32_t>>;

	 private:
		DRMvisual			(const DRMvisual &src)	= delete;
		DRMvisual& operator=(const DRMvisual &rhs)	= delete;

		kms::PixelFormat			pixfmt	 { kms::PixelFormat::YUYV };
		std::unique_ptr<kms::Card>	ptr_card { nullptr };
		kms::Connector				*ptr_conn{ nullptr };
		kms::Crtc					*ptr_crt { nullptr };
		ListOfPlanesDef				available_planes;

		std::map<kms::Plane*, DisplayAndVideoGeometryDef> history;

		std::mutex 					g_lock;

		void getNecessaryPlane();
		void addInit(const kms::PixelFormat &pixfmt_);

		bool internalAddFbIdToPlaneAndChangeGeometry(
													kms::Plane					*m_plane,
													kms::Framebuffer			*m_fb,
													const Rectangle<uint32_t>	&display,
													const Rectangle<uint32_t>	&video
													);

		bool internalAddFbIdToPlane(kms::Plane *plane, kms::Framebuffer	*m_fb);

		bool internalInitPlane(
								kms::Plane					*plane,
								kms::Framebuffer			*m_fb,
								const Rectangle<uint32_t>	&display,
								const Rectangle<uint32_t>	&video
							  );

	 public:
		DRMvisual() 		= default;
		virtual ~DRMvisual()= default;
		void init(const kms::PixelFormat &pixfmt_ = kms::PixelFormat::YUYV);
		void initOmap(const kms::PixelFormat &pixfmt_ = kms::PixelFormat::YUYV);

		bool initPlane(
						kms::Plane					*plane,
						kms::Framebuffer			*m_fb,
						const Rectangle<uint32_t>	&display,
						const Rectangle<uint32_t>	&video
					  );
		bool initPlane(
						kms::Plane						*plane,
						VideoEncLib::IFrameBuffer		*m_fb,
						const VideoEncLib::Rectangle	&display,
						const VideoEncLib::Rectangle	&video
					  );

		bool addFbIdToPlaneAndChangeGeometry(
											kms::Plane					*m_plane,
											kms::Framebuffer			*m_fb,
											const Rectangle<uint32_t>	&display,
											const Rectangle<uint32_t>	&video
											);
		bool addFbIdToPlaneAndChangeGeometry(
											kms::Plane						*m_plane,
											VideoEncLib::IFrameBuffer		*m_fb,
											const VideoEncLib::Rectangle	&display,
											const VideoEncLib::Rectangle	&video
											);

		void addFbIdToPlane(kms::Plane *plane, kms::Framebuffer	*m_fb);
		void addFbIdToPlane(kms::Plane *plane, VideoEncLib::IFrameBuffer *m_fb);

		bool clearPlane(kms::Plane *plane);

		bool getGeometry(kms::Plane *plane, Rectangle<uint32_t>		&display,	Rectangle<uint32_t>		&video);
		bool getGeometry(kms::Plane *plane, VideoEncLib::Rectangle	&display,	VideoEncLib::Rectangle	&video);

		kms::Card *const			getCard()			const { return ptr_card.get();}
		const kms::Connector *const	getConnector()		const { return ptr_conn;}
		const kms::Crtc *const		getCrtc()			const { return ptr_crt;}
		const kms::PixelFormat&		getPixFmt()			const { return pixfmt;}
		const ListOfPlanesDef&		getAvailablePlanes()const { return available_planes;}
	};
}
