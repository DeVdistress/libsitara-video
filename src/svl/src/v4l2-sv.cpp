//
// v4l2-sv.cpp
//
//  Created on: 10 июн. 2020 г.
//      Author: devdistress
//
#include "v4l2-sv.h"
#include "for_debug.h"
#include "basic_func_v4l2.h"

#include <glob.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

namespace svl
{
	ListOfV4l2Dev::VectStringV4l2DevDef ListOfV4l2Dev::glob(const std::string &pat)
	{
		glob_t glob_result;
		::glob(pat.c_str(), 0, NULL, &glob_result);

		VectStringV4l2DevDef tmp_vect;
		tmp_vect.clear();

		for(unsigned i = 0; i < glob_result.gl_pathc; ++i)
			tmp_vect.push_back(std::string(glob_result.gl_pathv[i]));

		globfree(&glob_result);

		return tmp_vect;
	}

	ListOfV4l2Dev::DataAboutV4l2DeV ListOfV4l2Dev::getV4l2DevList()
	{
		DataAboutV4l2DeV dev;
		dev.str_.clear();
		dev.int_.clear();

		for (std::string vidpath : this->glob(sysdir))
		{
			int fd = ::open(vidpath.c_str(), O_RDWR | O_NONBLOCK);

			if (fd < 0)
				continue;

			if (!BasicFuncV4l2::isCaptureV4l2Dev(fd))
			{
				close(fd);
				continue;
			}

			dev.str_.push_back(vidpath.c_str());
			dev.int_.push_back(fd);

			PRINTF("was finded %s, fd=%d\n", vidpath.c_str(), fd);
		}

		FAIL_IF(dev.str_.size() == 0, "No cameras found");

		his_dev = dev;

		return dev;
	}

	void ListOfV4l2Dev::closeAllV4l2CaptureDevice(DataAboutV4l2DeV *dev)
	{
		if (dev != nullptr)
			for (auto fd : dev->int_)
				::close(fd);
		else
			for (auto fd : his_dev.int_)
				::close(fd);
	}
}
