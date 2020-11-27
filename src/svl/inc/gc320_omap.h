//
// gc320_omap.h
//
//  Created on: 12 нояб. 2020 г.
//      Author: devdistress
//
#pragma once

#include <gc_hal.h>
#include <gc_hal_raster.h>
#include <kms++/kms++.h>
#include <kms++util/kms++util.h>
#include <kms++/omap/omapkms++.h>
#include <gc320_cmem.h>

#define MAX_MINORFEATURE_NUMBER 10
#define MIRROR

namespace svl
{

	class Gc320ObjOmap
	{
	public:

		// work with kms::OmapFramebuffer
		Gc320ObjOmap(uint32_t num_src, uint32_t num_src_surf, uint32_t num_dst_surf,
				kms::OmapFramebuffer **src_bo, kms::OmapFramebuffer *dst_bo,
				kms::PixelFormat pix_format = kms::PixelFormat::NV12);
		~Gc320ObjOmap();

		// Purpose: Construct source surface work with cmem
		//
		// @return Return bool success or failure
		// work with cmem
		int configureSurface(surf_context *surf_ctx, uint32_t num_ctx,
				kms::OmapFramebuffer *bo, gceSURF_FORMAT surf_format);
#if(0)
		// Purpose: Program GC320 to mirroring the surface flip on x
		//
		// @param index to source surface
		// @param index to desitination surface
		//
		// @return bool true/success or false/failure
		//
		bool mirroring(uint32_t a, uint32_t b);

		// Purpose: Program GC320 to stitch two images together
		// One image will be mirrored and the second image will be without changed
		//
		// @param index to source surface
		// @param index to desitination surface
		//
		// @return bool true/success or false/failure
		//
		bool composition(uint32_t src_surf_indx, uint32_t dst_surf_indx);
#endif
	private:
		// Purpose: Checkout the GC320 Hardware and print info on it
		//
		//
		// @return 0 on success < 0 on failure
		//
		int chipCheck();

		// Purpose: Probe and initialize the GC320 Chip
		//
		// @return 0 on Success and -1 on Failure
		//
		int initGc320();

		// Runtime parameters
		gcoOS           m_os;
		gcoHAL          m_hal;
		gco2D           m_engine2d;

		// Dest surface. (1 Dest. Surface)
		gceSURF_FORMAT m_dst_format;
		uint32_t	   m_dst_width;
		uint32_t	   m_dst_height;
		uint32_t	   m_dst_stride[2];		//where [x] is num of planes. for nv12 is 2, for yuy2 is 1
		uint32_t	   m_dst_num_of_stride;	//is num of planes. for nv12 is 2, for yuy2 is 1
		surf_context  *m_dst_surf_ctxt;
		uint32_t       m_num_dst_ctx;		//this is a count of queue video buf

		// Source surface. (1 or more Source Surface)
		gceSURF_FORMAT  m_src_format;
		uint32_t	   *m_src_width;
		uint32_t	   *m_src_height;
		uint32_t	   *m_src_stride[2];	//where [x] is num of planes. for nv12 is 2, for yuy2 is 1
		uint32_t       *m_src_num_of_stride;//is num of planes. for nv12 is 2, for yuy2 is 1
		surf_context  **m_src_surf_ctxt;
		uint32_t        m_num_src_ctx;		//this is a count of queue video buf
		uint32_t	    m_num_src; // Defines number of source surface
	};
}
