// vip_common.h
//
//  Created on: 16 июн. 2020 г.
//      Author: devdistress
#pragma once

#include "vpe_common.h"

class VipWork
{
	 int vipfd;

	 int vipOpen(const char *devname);

public:
	int vipClose();
	int vipSetFormat(int width, int height, int fourcc);
	int vipReqbuf();
	int vipQbuf(Vpe *vpe, int index);
	int vipDqbuf(Vpe * vpe);
	int vipGetFd(){ return vipfd;};

	explicit VipWork(const char *devname) { vipOpen(devname); };
	virtual ~VipWork() { vipClose();};
};
