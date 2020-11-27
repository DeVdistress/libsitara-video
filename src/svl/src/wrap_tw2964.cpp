//
// v4l2-sv.cpp
//
//  Created on: 10 июн. 2020 г.
//      Author: devdistress
//
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <iomanip>
#include <cstring>

#include "wrap_tw2964.h"
#include "for_debug.h"

namespace svl
{
	bool WrapTw2964::openDevice()
	{
		MY_LOCKER_MUTEX

		PRINTF("\nOpening Driver\n");

		fd = open("/dev/tw2964", O_RDWR);
		if(fd < 0)
		{
			PRINTF("Cannot open device file...\n");
			return false;
		}

		if(doItActualizationStd())
		{
			was_init = true;

			MY_LOCKER_MUTEX_UNLOCK

			if(!getCntOfVideoChannel())
			{
				PRINTF("Cannot get count of video device\n");
				return false;
			}

			if(!getVideoAccordance())
			{
				PRINTF("Cannot get accordance of video device\n");
				return false;
			}

			return true;
		}
		else
			return false;
	}

	bool WrapTw2964::doItActualizationStd()
	{
		long	res	= 0;

		res = ioctl(fd, ACTUALIZATION_STD_TW2964);
		if(res < 0)
		{
			PRINTF("Error'%d' ioctl ACTUALIZATION_STD_TW2964\n", res);

			return false;
		}
		else
		{
			PRINTF("tw2946 actualization std success\n");

			return true;
		}
	}

	bool WrapTw2964::getCntOfVideoChannel()
	{
		MY_LOCKER_MUTEX

		if(was_init)
		{
			long res = ioctl(fd, GET_COUNT_OF_CHANNEL_TW2964);
			if(res < 0)
			{
				PRINTF("Error'%d' ioctl GET_COUNT_IRQ_OF_CHANNEL\n", res);
				return false;
			}
			else
			{
				cnt_of_video_channel = static_cast<int>(res);
				PRINTF("tw2946 has a '%u' channels\n", static_cast<int>(cnt_of_video_channel));
				return true;
			}
		}

		return false;
	}

	bool WrapTw2964::readStd()
	{
		long	res	= 0;
		Tw2964ChanVideoStdTypeDef	for_read_chan_std;

		MY_LOCKER_MUTEX

		if(was_init)
		{
			std.clear();

			for(int tik = 0; tik < cnt_of_video_channel; ++tik)
			{
				for_read_chan_std.num_channel = (Tw2964VideoNumOfV4l2TypeDef)(tik + 1);

				res = ioctl(fd, GET_VIDEO_STD_TW2964, &for_read_chan_std);
				if(res < 0)
				{
					PRINTF("Error'%d' ioctl GET_VIDEO_STD_TW2964\n", res);

					return false;
				}
				else
				{
					PRINTF("tw2946 chan-'%u' std-'%u'\n", for_read_chan_std.num_channel, for_read_chan_std.video_std);
					std.push_back(std::move(for_read_chan_std));
				}
			}

			return true;
		}

		return false;
	}

	bool WrapTw2964::readRealStd()
	{
		long res	= 0;
		Tw2964ChanVideoStdTypeDef for_read_chan_std;

		MY_LOCKER_MUTEX

		if(was_init)
		{
			real_std.clear();

			for(int tik = 0; tik < cnt_of_video_channel; ++tik)
			{
				for_read_chan_std.num_channel = (Tw2964VideoNumOfV4l2TypeDef)(tik + 1);

				res = ioctl(fd, GET_REAL_VIDEO_STD_TW2964, &for_read_chan_std);
				if(res < 0)
				{
					PRINTF("Error'%d' ioctl GET_VIDEO_STD_TW2964\n", res);
					return false;
				}
				else
				{
					PRINTF("tw2946 chan-'%u' std-'%u'\n", for_read_chan_std.num_channel, for_read_chan_std.video_std);
					real_std.push_back(std::move(for_read_chan_std));
				}
			}

			return true;
		}

		return false;
	}

	bool WrapTw2964::readWidthHigh()
	{
		long	res	= 0;
		Tw2964ChanWidthHighTypeDef	for_read;

		MY_LOCKER_MUTEX

		if(was_init)
		{
			w_h.clear();

			for(int tik = 0; tik < cnt_of_video_channel; ++tik)
			{
				for_read.num_channel = (Tw2964VideoNumOfV4l2TypeDef)(tik + 1);

				res = ioctl(fd, GET_VIDEO_WIDTH_HIGH_TW2964, &for_read);
				if(res < 0)
				{
					PRINTF("Error'%d' ioctl GET_VIDEO_STD_TW2964\n", res);
					return false;
				}
				else
				{
					PRINTF("tw2946 chan-'%u' width-'%u' high-'%u'\n", for_read.num_channel,
							for_read.width, for_read.high);
					w_h.push_back(std::move(for_read));
				}
			}

			return true;
		}

		return false;
	}

	bool WrapTw2964::getColorKiller()
	{
		long	res	= 0;

		MY_LOCKER_MUTEX

		if(was_init)
		{
			res = ioctl(fd, GET_COLOR_KILLER);
			if(res < 0)
			{
				PRINTF("Error'%d' ioctl GET_COLOR_KILLER\n", res);
				return false;
			}
			else
			{
				color_killer = static_cast<unsigned int>(res);
				PRINTF("tw2946 color_killer='%#x'\n", static_cast<unsigned int>(color_killer));
				return true;
			}
		}

		return false;
	}

	bool WrapTw2964::writeStd(Tw2964ChanVideoStdTypeDef &for_set_std)
	{
		long res = 0;

		MY_LOCKER_MUTEX

		if(was_init)
		{
			res = ioctl(fd, SET_VIDEO_STD_TW2964, &for_set_std);
			if(res < 0)
			{
				PRINTF("Error'%d' ioctl GET_COUNT_IRQ_OF_CHANNEL\n", res);
				return false;
			}
			else
			{
				PRINTF("tw2946 set std success\n");
				return true;
			}
		}

		return false;
	}

	bool WrapTw2964::isInit()
	{
		return was_init;
	}

	bool WrapTw2964::getSizeVideoChannel(size_t &cnt)
	{
		if(was_init)
		{
			cnt = static_cast<size_t>(cnt_of_video_channel);

			return true;
		}

		return false;
	}

	bool WrapTw2964::getVideoAccordance()
	{
		Tw2964AccordanceV4l2AndAnalogInTypeDef video_accordance[cnt_of_video_channel];

		long res = 0;

		MY_LOCKER_MUTEX

		if(was_init)
		{
			memset(&video_accordance, 0, sizeof(video_accordance));
			vid_accordance.clear();

			res = ioctl(fd, GET_ACCORDANCE_VIDEO_NUM_V4L2_AND_ANALOG_INPUT, &video_accordance);

			if(res != cnt_of_video_channel)
			{
				PRINTF("Error'%d' ioctl GET_STATE_VIDEO_DETECT\n", res);
				return false;
			}
			else
			{
				for(int tik = 0; tik < cnt_of_video_channel; ++tik)
				{
					PRINTF(
							"/dev/video%u <=> tw2964-ain%u\n",
							video_accordance[tik].v4l2_num,
							video_accordance[tik].ain_num
						);

					vid_accordance.push_back(std::move(video_accordance[tik]));
				}
			}

			return true;
		}

		return false;
	}

	bool WrapTw2964::getStateVideoDetect()
	{

		long res = 0;

		MY_LOCKER_MUTEX

		if(was_init)
		{
			vid_stat.clear();

			for(int tik = 0; tik < cnt_of_video_channel; ++tik)
			{
				Tw2964StatVideoTypeDef	state_video;

				memset(&state_video, 0, sizeof(state_video));

				state_video.num_channel = Tw2964VideoNumOfV4l2TypeDef (tik + 1);
				res = ioctl(fd, GET_STATE_VIDEO_DETECT, &state_video);

				if(res < 0)
				{
					PRINTF("Error'%d' ioctl GET_STATE_VIDEO_DETECT\n", res);

					return false;
				}
				else
				{
					PRINTF(
							"get state video['%u']=detect_video'%u'; mono_video'%u'; det50'%u'\n",
							 state_video.num_channel,
							 state_video.detect_video,
							 state_video.mono_video,
							 state_video.det50
							);

					vid_stat.push_back(std::move(state_video));
				}
			}

			return true;
		}

		return false;
	}

	bool WrapTw2964::getCountIrqOfChannel()
	{
		long res = 0;

		MY_LOCKER_MUTEX

		if(was_init)
		{
			irq_cnt.clear();

			for(int tik = 0; tik < cnt_of_video_channel; ++tik)
			{
				Tw2964StatIrqTypeDef	irq_count;

				memset(&irq_count,   0, sizeof(irq_count));

				irq_count.num_channel = (Tw2964VideoNumOfV4l2TypeDef)(tik+1);
				res = ioctl(fd, GET_COUNT_IRQ_OF_CHANNEL, &irq_count);

				if(res < 0)
				{
					PRINTF("Error'%d' ioctl GET_COUNT_IRQ_OF_CHANNEL\n", res);
					return false;
				}
				else
				{
					PRINTF("Count IRQ['%u']='%u'\n", irq_count.num_channel, irq_count.int_count);
					irq_cnt.push_back(std::move(irq_count));
				}
			}

			return true;
		}

		return false;
	}

	bool WrapTw2964::sortByCamNum()
	{
		int tik_1, tik_2;

		std::vector<Tw2964AccordanceV4l2AndAnalogInTypeDef> vid_accordance_(vid_accordance);

		for(tik_1 = 0; tik_1 < vid_accordance_.size(); ++tik_1)
		{
			if(vid_accordance_[tik_1].ain_num != (tik_1+1)) // because counting from 1
			{
				for(tik_2 = tik_1+1; tik_2 < vid_accordance_.size(); ++tik_2)
				{
					if(vid_accordance_[tik_2].ain_num==(tik_1+1))
					{
						Tw2964AccordanceV4l2AndAnalogInTypeDef tmp;
						tmp = vid_accordance_[tik_2];
						vid_accordance_[tik_2]= vid_accordance_[tik_1];
						vid_accordance_[tik_1]= tmp;

						break;
					}
				}
			}
		}

		num_by_cam_from_index_v4l2.clear();
		num_by_v4l2_from_index_cam.clear();

		for(auto elem:vid_accordance_)
			num_by_cam_from_index_v4l2.push_back(static_cast<unsigned>(elem.ain_num));

		for(auto elem:vid_accordance)
			num_by_v4l2_from_index_cam.push_back(static_cast<unsigned>(elem.ain_num));

		return true;
	}

	bool WrapTw2964::getBindNumCamToNumV4l2(std::vector<unsigned> &mas_of_v4l2_num)
	{
		MY_LOCKER_MUTEX

		if(was_init)
		{
			sortByCamNum();

			mas_of_v4l2_num.clear();

			mas_of_v4l2_num = num_by_v4l2_from_index_cam;

			return true;
		}

		return false;
	}

	bool WrapTw2964::getBindNumV4l2ToNumCam(std::vector<unsigned> &mas_of_cam_num)
	{
		MY_LOCKER_MUTEX

		if(was_init)
		{
			sortByCamNum();

			mas_of_cam_num.clear();

			mas_of_cam_num = num_by_cam_from_index_v4l2;

			return true;
		}

		return false;
	}

	unsigned WrapTw2964::getWidthVideoOfChannel(Tw2964AnalogVideoInputNumTypeDef num)
	{
		MY_LOCKER_MUTEX

		if(was_init)
		{
			sortByCamNum();

			for(auto elem:w_h)
			{
			 if (elem.num_channel == num_by_cam_from_index_v4l2[num])
				 return elem.width;
			}
		}

		return 0;
	}

	unsigned WrapTw2964::getHighVideoOfChannel(Tw2964AnalogVideoInputNumTypeDef num)
	{
		MY_LOCKER_MUTEX

		if(was_init)
		{
			sortByCamNum();

			for(auto elem:w_h)
			{
			 if (elem.num_channel == num_by_cam_from_index_v4l2[num])
				 return elem.high;
			}
		}

		return 0;
	}

	bool WrapTw2964::closeDevice()
	{
		PRINTF("Closing Driver\n");
		close(fd);
		was_init = false;

		return true;
	}
}
