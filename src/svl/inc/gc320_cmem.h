//
// gc320_cmem.h
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

#define MAX_MINORFEATURE_NUMBER 10
#define MIRROR

namespace svl
{
	class BufObj;

	struct surf_context
	{
		gcoSURF surf;
		uint32_t phys_address[4];
	};

	class Gc320Obj
	{
	public:
		// work with cmem
		Gc320Obj(uint32_t num_src, uint32_t num_src_surf, uint32_t num_dst_surf, BufObj **src_bo, BufObj *dst_bo);

		~Gc320Obj();

		// Purpose: Construct source surface
		//
		// @return Return bool success or failure
		//
		int configureSurface(surf_context *surf_ctx, uint32_t num_ctx, BufObj *bo, gceSURF_FORMAT surf_format);

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
		uint32_t	   m_dst_stride;
		surf_context  *m_dst_surf_ctxt;
		uint32_t       m_num_dst_ctx;

		// Source surface. (1 or more Source Surface)
		gceSURF_FORMAT m_src_format;
		uint32_t	   *m_src_width;
		uint32_t	   *m_src_height;
		uint32_t	   *m_src_stride;
		surf_context  **m_src_surf_ctxt;
		uint32_t       m_num_src_ctx;
		uint32_t	   m_num_src; // Defines number of source surface
	};
}
