//
//  video_display.cpp
//
//  Created on: 22 сент. 2020 г.
//      Author: devdistress
#include <iostream>
#include <chrono>

#include "video_display.h"

#include "wrap_video_cap_for_venclib.h"

namespace svl
{
	// init of static property's
	DRMvisual::ListOfPlanesDef 	VideoDisplay::lst_plane(VideoCapture::getAvailablePlanesOfDrm());
	std::array<bool, 3>			VideoDisplay::flag_used_plan { false, false, false };
	std::mutex 					VideoDisplay::g_lock;

	VideoDisplay::VideoDisplay(VideoEncLib::AbstractFrameSource* frame_source, int planeIndex)
	{
		std::unique_lock<std::mutex> locker(g_lock);

		if(frame_source)
		{
			ptr_frameSource = frame_source;
			svl::VideoCaptureWrapper *ptr_tmp = dynamic_cast<svl::VideoCaptureWrapper*>(ptr_frameSource);

			if(ptr_tmp)
				is_video_cap = true;

			#if(1)
				kms::PixelFormat my_px_fmt = kms::PixelFormat::NV12;
			#else
				kms::PixelFormat my_px_fmt = kms::PixelFormat::YUYV;
			#endif

			DRMvisual::getInstance().initOmap(my_px_fmt);
			if(!lst_plane.size())
				lst_plane = svl::VideoCapture::getAvailablePlanesOfDrm();

			//find used plane
			unsigned tik = 0;
			if(is_video_cap && ptr_tmp->getPlane())
			{
				ptr_cur_plane = ptr_tmp->getPlane();

				for(auto wtf : lst_plane)
				{
					if(wtf == ptr_cur_plane)
					{
						flag_used_plan[tik] = true;
						cur_idx_in_mas_used_plan = tik;
						break;
					}
					++tik;
				}
			}
			else
			{
				if(planeIndex < 0)
				{
					bool was_succes_finded = false;
					for(auto &wtf : flag_used_plan)
					{
						if(!wtf)
						{
							ptr_cur_plane = lst_plane[tik];
							flag_used_plan[tik] = true;
							was_succes_finded = true;
							cur_idx_in_mas_used_plan = tik;
							break;
						}
						++tik;
					}

					if(!was_succes_finded)
						throw std::runtime_error("error not free plane, all planes is used");
				}
				else	//if set current number of planes
				{
					if(planeIndex < 3)
					{
						tik = planeIndex;
						if(!flag_used_plan[tik])
						{
							ptr_cur_plane = lst_plane[tik];
							flag_used_plan[tik] = true;
							cur_idx_in_mas_used_plan = tik;
						}
						else
							throw std::runtime_error(std::string("plane ") + std::to_string(planeIndex) + std::string(" is used"));
					}
					else
						throw std::runtime_error("invalid plane index");
				}
			}
				was_init = true;
		}
	}

	void VideoDisplay::start()
	{
		if(ptr_frameSource && !need_stop && !was_start)
		{
			if(ph != nullptr)
				ph->join();

			ph.reset(new std::thread([this]{ threadOfVideoDisplay(ptr_frameSource); }));
			was_start = true;
		}
	}

	void VideoDisplay::threadOfVideoDisplay(VideoEncLib::AbstractFrameSource *ptr)
	{
		VideoEncLib::AbstractFrameSource *ptr_source = ptr;
		bool first_shoot = true;

		if(ptr_source==nullptr)
			return;

		while(true)
		{
			if(is_video_cap && !ptr->isRun())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

				if(need_stop)
				{
				  need_stop = false;
				  was_start = false;
				  break;
				}

				continue;
			}

			VideoEncLib::IFrameBuffer *ptr_fr_buf = nullptr;
			ptr_fr_buf = ptr->waitFrame();

			if(need_stop || ptr_fr_buf == nullptr)
			{
			  need_stop = false;
			  was_start = false;
			  ptr->endingProcessFrameNotify();
			  break;
			}

			if(displayed)
			{
				if(first_shoot)
				{
					svl::DRMvisual::getInstance().initPlane(ptr_cur_plane, ptr_fr_buf, display_rect, video_rect);
					first_shoot = false;
				}
				else if(new_rect)
				{
					svl::DRMvisual::getInstance().addFbIdToPlaneAndChangeGeometry(ptr_cur_plane, ptr_fr_buf, display_rect, video_rect);
					new_rect = false;
				}
				else
				{
					svl::DRMvisual::getInstance().addFbIdToPlane(ptr_cur_plane, ptr_fr_buf);
				}
			}
			else
			{
				if(!first_shoot)
				{
					svl::DRMvisual::getInstance().clearPlane(ptr_cur_plane);
					first_shoot = true;
				}
			}

			ptr->endingProcessFrameNotify();
		}
		svl::DRMvisual::getInstance().clearPlane(ptr_cur_plane);
		std::cout << " thread of VideoDisplay was stoped " << std::endl;
	}

	bool VideoDisplay::setViewDisplayRect(const VideoEncLib::Rectangle &rect)
	{
		VideoEncLib::Rectangle tmp(rect);

		if(tmp != display_rect)
		{
			display_rect = tmp;
			new_rect = true;
		}

		return true;
	}

	VideoEncLib::Rectangle VideoDisplay::getViewDisplayRect()
	{
		return display_rect;
	}

	bool VideoDisplay::setViewVideoRect(const VideoEncLib::Rectangle &rect)
	{
		VideoEncLib::Rectangle tmp(rect);

		if(tmp != video_rect)
		{
			video_rect = tmp;
			new_rect = true;
		}

		return true;
	}

	VideoEncLib::Rectangle VideoDisplay::getViewVideoRect()
	{
		return video_rect;
	}

	VideoDisplay::~VideoDisplay()
	{
		std::unique_lock<std::mutex> locker(g_lock);
		flag_used_plan[cur_idx_in_mas_used_plan] = false;
		
		if(ph != nullptr)
		{
			if(was_start)
				need_stop = true;
			ph->join();
		}
	}
}
