//
// v4l2-sv.h
//
//  Created on: 10 июн. 2020 г.
//      Author: devdistress
//
#pragma once

#include <string>
#include <list>
#include <vector>

#include <kms++/kms++.h>
#include <kms++util/kms++util.h>
#include "pattern_singleton.h"
#include <linux/videodev2.h>

namespace svl
{
 //------------------------------------------------------------------------------
 /// @brief class providing a list of all available devices in the system
 //------------------------------------------------------------------------------
 class ListOfV4l2Dev
 {
	const char *sysdir{"/dev/video*"};

 public:
	using VectStringV4l2DevDef = std::vector<std::string>;
    using VectIntV4l2DevDef    = std::vector<int>;

    struct DataAboutV4l2DeV
	{
		VectStringV4l2DevDef str_;
		VectIntV4l2DevDef    int_;
	};

	explicit ListOfV4l2Dev() = default;
	virtual ~ListOfV4l2Dev(){closeAllV4l2CaptureDevice(nullptr);}

	//------------------------------------------------------------------------------
	/// @brief function providing a list of all available devices in the system
	//------------------------------------------------------------------------------
	DataAboutV4l2DeV getV4l2DevList();

	void closeAllV4l2CaptureDevice(DataAboutV4l2DeV *dev = nullptr);

 private:
	DataAboutV4l2DeV his_dev;

	VectStringV4l2DevDef glob(const std::string &pat);
 };
}
