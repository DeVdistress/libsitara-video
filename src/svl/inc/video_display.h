//
//  video_display.h
//
//  Created on: 22 сент. 2020 г.'
//	Author: devdistress
#pragma once

#include <kms++/kms++.h>
#include <kms++util/kms++util.h>

#include <thread>
#include <atomic>
#include <memory>
#include <array>
#include <mutex>

#include "videoenclib.h"
#include "drm_visual.h"

namespace svl
{
	class VideoDisplay
	{
		static DRMvisual::ListOfPlanesDef 						lst_plane;
		static std::array<bool, 3>								flag_used_plan;
		static std::mutex 										g_lock;

		kms::Plane*							ptr_cur_plane		{ nullptr };
		int							cur_idx_in_mas_used_plan	{ -1	  };

		std::unique_ptr<std::thread> 		ph					{ nullptr };
		VideoEncLib::AbstractFrameSource 	*ptr_frameSource	{ nullptr };
		std::atomic<bool> 					is_video_cap		{ false   };
		std::atomic<bool> 					was_init			{ false   };
		std::atomic<bool> 					was_start			{ false   };
		std::atomic<bool> 					need_stop			{ false   };
		std::atomic<bool> 					displayed			{ false   };
		std::atomic<bool> 					new_rect			{ false   };

		VideoEncLib::Rectangle				display_rect;
		VideoEncLib::Rectangle				video_rect;

		void threadOfVideoDisplay(VideoEncLib::AbstractFrameSource *ptr);

	public:
		VideoDisplay(VideoEncLib::AbstractFrameSource* frame_source, int planeIndex = -1);

		bool setViewDisplayRect(const VideoEncLib::Rectangle &rect);
		VideoEncLib::Rectangle getViewDisplayRect();

		bool setViewVideoRect(const VideoEncLib::Rectangle &rect);
		VideoEncLib::Rectangle getViewVideoRect();

		void start();
		void stop()		{ need_stop = true; }
		bool isStart()	{ return was_start; }

		void on ()		{ displayed = true; }
		void off ()		{ displayed = false;}
		bool isOn ()	{ return displayed; }

		virtual ~VideoDisplay();
	};
}

