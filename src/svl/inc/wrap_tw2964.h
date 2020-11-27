//
// v4l2-sv.h
//
//  Created on: 10 июн. 2020 г.
//      Author: devdistress
//
#pragma once

#include <string>
#include <vector>

#include "pattern_singleton.h"
#include "tw2964_ioctl.h"

namespace svl
{
 class WrapTw2964: public SingletonPattern<WrapTw2964>
 {
	WrapTw2964(const WrapTw2964 &src) = delete;
	WrapTw2964& operator=(const WrapTw2964 &rhs) = delete;

	std::vector<unsigned> 								num_by_v4l2_from_index_cam;
	std::vector<unsigned> 								num_by_cam_from_index_v4l2;
	std::vector<Tw2964ChanVideoStdTypeDef>  			real_std;
	std::vector<Tw2964ChanVideoStdTypeDef>  			std;
	std::vector<Tw2964ChanWidthHighTypeDef> 			w_h;
	std::vector<Tw2964AccordanceV4l2AndAnalogInTypeDef> vid_accordance;
	std::vector<Tw2964StatVideoTypeDef> 				vid_stat;
	std::vector<Tw2964StatIrqTypeDef> 				    irq_cnt;

	std::atomic<int>          fd;
	std::atomic<int>          cnt_of_video_channel;
	std::atomic<unsigned int> color_killer;

	bool doItActualizationStd();
	bool getCntOfVideoChannel();
	bool readStd();
	bool readRealStd();
	bool readWidthHigh();
	bool getColorKiller();
	bool getVideoAccordance();
	bool getStateVideoDetect();
	bool getCountIrqOfChannel();
	bool sortByCamNum();
	bool writeStd(Tw2964ChanVideoStdTypeDef &for_set_std);

public:
	WrapTw2964() = default;

	bool openDevice();

	bool closeDevice();

	bool isInit();

	bool getSizeVideoChannel(size_t &cnt);

	unsigned getWidthVideoOfChannel(Tw2964AnalogVideoInputNumTypeDef num);

	unsigned getHighVideoOfChannel(Tw2964AnalogVideoInputNumTypeDef num);

	bool getBindNumCamToNumV4l2(std::vector<unsigned> &mas_of_v4l2_num);

	bool getBindNumV4l2ToNumCam(std::vector<unsigned> &mas_of_cam_num);

	virtual ~WrapTw2964() = default;
 };
}
