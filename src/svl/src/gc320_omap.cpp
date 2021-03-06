//
// gc320_omap.cpp
//
//  Created on: 12 нояб. 2020 г.
//      Author: devdistress
//
#include "gc320_omap.h"
#include <stdint.h>
#include "cmem_buf.h"
#include "for_debug.h"
#include "cmem_buf.h"

namespace svl
{

	// Configure one time the source and destination surface context for all buffers
	// as they cost some performance overhead if configured per frame. Since the
	// buffer parameters aren't changing in this application run time, we can save the overhead
	Gc320ObjOmap::Gc320ObjOmap(uint32_t num_src, uint32_t num_src_surf, uint32_t num_dst_surf,
					kms::OmapFramebuffer **src_bo, kms::OmapFramebuffer *dst_bo, kms::PixelFormat pix_format)
	{
		m_dst_width   = dst_bo-> width();
		m_dst_height  = dst_bo->height();

		if(pix_format == kms::PixelFormat::NV12)
		{
			m_dst_num_of_stride = 2;
			m_dst_stride[0]  = dst_bo->stride(0);
			m_dst_stride[1]  = dst_bo->stride(1);
			m_src_format = m_dst_format  = gcvSURF_NV12;
		}
		else
		{
			m_dst_num_of_stride = 1;
			m_dst_stride[0]  = dst_bo->stride(0);
			m_dst_stride[1]  = 0;
			m_src_format = m_dst_format  = gcvSURF_YUY2;
		}

		m_num_dst_ctx = num_dst_surf;

		m_num_src     		=	num_src;
		m_num_src_ctx 		=	num_src_surf;
		m_src_num_of_stride =	(uint32_t *)		malloc(sizeof(uint32_t));
		m_src_surf_ctxt 	=	(surf_context **)	malloc(m_num_src * sizeof(*m_src_surf_ctxt));
		m_src_width     	=	(uint32_t *)		malloc(m_num_src * sizeof(uint32_t));
		m_src_height    	=	(uint32_t *)		malloc(m_num_src * sizeof(uint32_t));
		m_src_stride[0]    	=	(uint32_t *)		malloc(m_num_src * sizeof(uint32_t));
		m_src_stride[1]    	=	(uint32_t *)		malloc(m_num_src * sizeof(uint32_t));

		for (uint32_t i = 0; i < m_num_src; i++)
		{
			m_src_width[i]  = src_bo[i]->width();
			m_src_height[i] = src_bo[i]->height();
			m_src_stride[0][i] = src_bo[i]->stride(0);

			if(pix_format == kms::PixelFormat::NV12)
				m_src_stride[1][i] = src_bo[i]->stride(1);
			else
				m_src_stride[1][i] = 0;

			m_src_surf_ctxt[i] = (surf_context *) malloc(m_num_src_ctx * sizeof(surf_context));
		}
		m_dst_surf_ctxt = (surf_context *) malloc(m_num_dst_ctx * sizeof(surf_context));

		initGc320();

// on developing
#if(0)
		for (uint32_t i = 0; i < m_num_src; i++)
		{
			configureSurface(m_src_surf_ctxt[i], m_num_src_ctx, src_bo[i], m_src_format);
		}
		configureSurface(m_dst_surf_ctxt, m_num_dst_ctx, dst_bo, m_dst_format);
#endif
	}

	// free all resources
	Gc320ObjOmap::~Gc320ObjOmap()
	{
		if (m_hal != gcvNULL)
		{
			gcoHAL_Commit(m_hal, true);
		}

//on developing
#if(0)
		for(uint32_t i = 0; i < m_num_dst_ctx; i++)
		{
			if (m_dst_surf_ctxt[i].surf != gcvNULL)
			{
				gceHARDWARE_TYPE type;
				gcoHAL_GetHardwareType(gcvNULL, &type);
				gcmVERIFY_OK(gcoSURF_Unlock(m_dst_surf_ctxt[i].surf, gcvNULL));
				gcmVERIFY_OK(gcoSURF_Destroy(m_dst_surf_ctxt[i].surf));
				gcmVERIFY_OK(gcoHAL_Commit(gcvNULL, true));
				gcoHAL_SetHardwareType(gcvNULL, type);
			}
		}

		for(uint32_t i = 0; i < m_num_src; i++)
		{
			for(uint32_t j = 0; j < m_num_src_ctx; j++)
			{
				if (m_src_surf_ctxt[i][j].surf != gcvNULL)
				{
					gceHARDWARE_TYPE type;
					gcoHAL_GetHardwareType(gcvNULL, &type);
					gcmVERIFY_OK(gcoSURF_Unlock(m_src_surf_ctxt[i][j].surf, gcvNULL));
					gcmVERIFY_OK(gcoSURF_Destroy(m_src_surf_ctxt[i][j].surf));
					gcmVERIFY_OK(gcoHAL_Commit(gcvNULL, true));
					gcoHAL_SetHardwareType(gcvNULL, type);
				}
			}
		}

		for (uint32_t i = 0; i < m_num_src; i++)
		{
			free(m_src_surf_ctxt[i]);
		}

		free(m_src_surf_ctxt);
		free(m_src_width);
		free(m_src_height);
		free(m_src_stride[0]);
		free(m_src_stride[1]);

		free(m_dst_surf_ctxt);

		if (m_hal != gcvNULL)
		{
			gcoHAL_Commit(m_hal, true);
			gcoHAL_Destroy(m_hal);
		}

		if (m_os != gcvNULL)
		{
			gcoOS_Destroy(m_os);
		}
#endif
	}
#if(0)
	int Gc320Obj::configureSurface(surf_context *surf_ctx,
			uint32_t num_ctx, kms::OmapFramebuffer *bo, gceSURF_FORMAT surf_format)
	{

		gceSTATUS status;
		gceHARDWARE_TYPE type;
		unsigned long phys = ~0U;

		gcoHAL_GetHardwareType(gcvNULL, &type);

		for(uint32_t i = 0; i < num_ctx; i++)
		{
			if (!bo->map[0][i])
			{
				ERROR("Received null pointer\n");
				return -1;
			}

			if(surf_format == gcvSURF_NV12)
			{
				if (!bo->map[1][i])
				{
					ERROR("Received null pointer\n");
					return -1;
				}
			}


			// create a wrapper gcoSURF surface object, because we are mapping the user
			//* allocated buffer pool should be gcvPOOL_USER
			status = gcoSURF_Construct(
				gcvNULL,
				bo->m_width,
				bo->m_height,
				1,
				gcvSURF_BITMAP,
				surf_format,
				gcvPOOL_USER,
				&surf_ctx[i].surf);

			if (status < 0)
			{
				ERROR(" Failed to create gcoSURF object\n");
				return false;
			}

			// calculate the size of the buffer needed. Use gcoSURF_QueryFormat API to
			// format parameters descriptor - this contains information on format as
			// needed by the Vivante driver
			//
			uint32_t width, height;
			int stride;
			gcmONERROR(gcoSURF_GetAlignedSize(surf_ctx[i].surf,
				&width,
				&height,
				&stride));

			if((width != bo->m_width) || (height != bo->m_height) || ((uint32_t) stride != bo->m_stride))
			{
				ERROR("Allocated Surf buffer width %d, height %d and stride %d doesn't meet GC320 aligned requirement on width %d, height %d and stride %d\n",
					bo->m_width, bo->m_height, bo->m_stride, width, height, stride);
			}

			uint32_t  p_adr_al;
			gcmONERROR(gcoSURF_GetAlignment(gcvSURF_BITMAP, surf_format,
				&p_adr_al,
				NULL, //&p_x_al,
				NULL //&p_y_al)
				));

			if((uint32_t)bo->m_buf[i] % p_adr_al)
			{
				ERROR("Buffer passed to GC320 doesn't meet the alignment requirement, GC320 alignment need is %x, received buffer with %x aligned address\n",
					p_adr_al, ((uint32_t)bo->m_buf[i]%p_adr_al));
			}

			/* Buffer provided meets all the requirement from GC320.
			Go ahead and set the underlying buffer */
			gcmONERROR(gcoSURF_SetBuffer(
				surf_ctx[i].surf,
				gcvSURF_BITMAP,
				surf_format,
				bo->m_stride,
				bo->m_buf[i],
				phys));

			if (status < 0)
			{
				ERROR("Failed to set buffer for gcoSURF object\n");
				return false;
			}

			/* set window size */
			gcmONERROR(gcoSURF_SetWindow(surf_ctx[i].surf, 0, 0, bo->m_width, bo->m_height));
			if (status < 0)
			{
				ERROR("Failed to set window for gcoSURF object\n");
				return false;
			}

			/* lock surface */
			gcmONERROR(gcoSURF_Lock(surf_ctx[i].surf, surf_ctx[i].phys_address, &bo->m_buf[i]));
			if (status < 0)
			{
				ERROR("Failed to lock gcoSURF object\n");
				return false;
			}

			gcoHAL_SetHardwareType(gcvNULL, type);
		}

		return true;

	OnError:
		ERROR("Failed: %s\n", gcoOS_DebugStatus2Name(status));

		return false;
	}

	bool Gc320Obj::mirroring(uint32_t src_surf_indx, uint32_t dst_surf_indx)
	{

		gcsRECT src_rect, dst_rect, clip_rect;
		gceSTATUS status = gcvSTATUS_OK;

		// render to dest surface
		// blit with 90 rotation
		// only have one surface and one destination
		src_rect.left   = 0;
		src_rect.top    = 0;
		src_rect.right  = m_src_width[0];
		src_rect.bottom = m_src_height[0];

		// set the clipping according to the rotation
		clip_rect.left 	  = 0;
		clip_rect.top 	  = 0;
		clip_rect.right   = m_dst_height;
		clip_rect.bottom  = m_dst_width;

		gcmONERROR(gco2D_SetClipping(m_engine2d, &clip_rect));

		//Since the buffer is getting rotated 90 degree, make sure the destination buffer width is larger than source buffer height */
		dst_rect.left   = 0;
	#if defined(MIRROR)
		dst_rect.top    = m_src_height[0];
		dst_rect.right  = dst_rect.left + m_src_width[0];
		dst_rect.bottom = dst_rect.top + m_src_height[0];
	#else
		dst_rect.top    = (m_dst_width - m_src_height[0]);
		dst_rect.right  = dst_rect.left + m_src_width[0];
		dst_rect.bottom = dst_rect.top + m_src_height[0];
	#endif

		gcmONERROR(gco2D_SetGenericSource(m_engine2d,
			m_src_surf_ctxt[0][src_surf_indx].phys_address, 1, &m_src_stride[0], 1,
			gcvLINEAR, m_src_format, gcvSURF_0_DEGREE,
			m_src_width[0], m_src_height[0]));

		gcmONERROR(gco2D_SetSource(m_engine2d, &src_rect));

		gcmONERROR(gco2D_SetTarget(m_engine2d,
			m_dst_surf_ctxt[dst_surf_indx].phys_address[0],
			m_dst_stride,
	#if defined(MIRROR)
			gcvSURF_FLIP_X,
	#else
			gcvSURF_90_DEGREE,
	#endif
			m_dst_width));

		gcmONERROR(gco2D_Blit(m_engine2d,
			1,
			&dst_rect,
			0xCC,
			0xCC,
			m_dst_format));
		gcmONERROR(gco2D_Flush(m_engine2d));
		gcmONERROR(gcoHAL_Commit(m_hal, true));

		return true;

	OnError:
		ERROR("Failed: %s\n", gcoOS_DebugStatus2Name(status));
		return false;
	}

	bool Gc320Obj::composition(uint32_t src_surf_indx, uint32_t dst_surf_indx)
	{
		// src_rect tells GC320 which area to capture from the source image
		// dst_rect tells GC320 where to display the captured source image
		gcsRECT src_rect, dst_rect;
		gcsRECT all_image { (gctINT32)0, (gctINT32)0, (gctINT32)m_dst_width, (gctINT32)m_dst_height };
		gceSTATUS status = gcvSTATUS_OK;
		bool first_shoot = true;

		for(int i = 0; i < 2; i++)
		{
			gcmONERROR(gco2D_SetCurrentSourceIndex(m_engine2d, i));

			gcmONERROR(gco2D_SetGenericSource(m_engine2d,
					m_src_surf_ctxt[i][src_surf_indx].phys_address, 1,
					&m_src_stride[i], 1, gcvLINEAR, m_src_format, gcvSURF_0_DEGREE,
					m_src_width[i], m_src_height[i]));

			switch (i % 2)
			{
				case 0:
					src_rect.left   = 0;
					src_rect.top    = 0;
					src_rect.right  = m_src_width[i];
					src_rect.bottom = m_src_height[i];

					dst_rect.left   = 0;
					dst_rect.top    = 0;
					dst_rect.right  = m_dst_width /2;
					dst_rect.bottom = m_dst_height/2;
				break;

				case 1:
					src_rect.left   = 0;
					src_rect.top    = 0;
					src_rect.right  = m_src_width[i];
					src_rect.bottom = m_src_height[i];
					dst_rect.left   = m_dst_width /2;
					dst_rect.top    = m_dst_height/2;
					dst_rect.right  = m_dst_width;
					dst_rect.bottom = m_dst_height;
				break;
			}

			// filled to color of all frame area
			if(first_shoot)
			{
				gcmONERROR(gco2D_SetGenericTarget(m_engine2d, &m_dst_surf_ctxt[dst_surf_indx].phys_address[0],
					1, &m_dst_stride, 1, gcvLINEAR,
					gcvSURF_YUY2, gcvSURF_0_DEGREE,
					m_dst_width,m_dst_height));

				gcmONERROR(gco2D_SetClipping(m_engine2d, &all_image));

				// Clear the surface
				gcmONERROR(gco2D_Clear(
					m_engine2d, 1, &all_image, 0,
					0xCC, 0xCC, gcvSURF_YUY2));

				first_shoot = false;
			}

			gcmONERROR(gco2D_SetSource(m_engine2d, &src_rect));

			gcmONERROR(gco2D_SetClipping(m_engine2d, &dst_rect));

			if (i==1)
			{    // Second image is going to be rotated by 90 degrees
				gcmONERROR(gco2D_FilterBlitEx2(m_engine2d,
					m_src_surf_ctxt[i][src_surf_indx].phys_address, 1,
					&m_src_stride[i], 1,
					gcvLINEAR, m_src_format, gcvSURF_FLIP_X,
					m_src_width[i],  m_src_height[i], &src_rect,
					&m_dst_surf_ctxt[dst_surf_indx].phys_address[0], 1, &m_dst_stride, 1,
					gcvLINEAR, m_dst_format, gcvSURF_0_DEGREE,
					m_dst_width, m_dst_height, &dst_rect, gcvNULL));
			}
			else if (!i)
			{
				gcmONERROR(gco2D_FilterBlitEx2(m_engine2d,
					m_src_surf_ctxt[i][src_surf_indx].phys_address, 1,
					&m_src_stride[i], 1,
					gcvLINEAR, m_src_format, gcvSURF_0_DEGREE,
					m_src_width[i],  m_src_height[i], &src_rect,
					&m_dst_surf_ctxt[dst_surf_indx].phys_address[0], 1, &m_dst_stride, 1,
					gcvLINEAR, m_dst_format, gcvSURF_0_DEGREE,
					m_dst_width, m_dst_height, &dst_rect, gcvNULL));
			}
		}
		gcmONERROR(gco2D_Flush(m_engine2d));
		gcmONERROR(gcoHAL_Commit(m_hal, true));

		return true;
	OnError:
		ERROR("Failed: %s\n", gcoOS_DebugStatus2Name(status));
		return false;
	}
#endif

	int Gc320ObjOmap::chipCheck()
	{
		gceSTATUS status;
		uint32_t i;
		int ret = 0;
		gceCHIPMODEL    chip_model;
		uint32_t       chip_revision;
		uint32_t       patch_revision;
		uint32_t       chip_features;
		uint32_t       chip_minor_features_number;
		uint32_t       chip_minor_features[MAX_MINORFEATURE_NUMBER];

		status = gcoHAL_QueryChipIdentity(m_hal, &chip_model, &chip_revision, &chip_features, gcvNULL);
		if (status < 0)
		{
			ERROR("Failed to query chip info (status = 0x%x)\n", status);
			ret = -1;
			goto EXIT;
		}

		status = gcoOS_ReadRegister(m_os, 0x98, &patch_revision);
		if (status < 0)
		{
			ERROR("Failed to read patch version (status = 0x%x)\n", status);
			ret = -2;
			goto EXIT;
		}

		MSG("=================== Chip Information ==================\n");
		MSG("Chip : GC%x\n", chip_model);
		MSG("Chip revision: 0x%08x\n", chip_revision);
		MSG( "Patch revision: 0x%08x\n", patch_revision);
		MSG("Chip Features: 0x%08x\n", chip_features);

		status = gcoHAL_QueryChipMinorFeatures(m_hal, &chip_minor_features_number, gcvNULL);
		if (status < 0)
		{
			ERROR("Failed to query minor feature count (status = 0x%x)\n", status);
			ret = -2;
			goto EXIT;
		}

		if (MAX_MINORFEATURE_NUMBER < chip_minor_features_number)
		{
			ERROR("no enough space for minor features\n");
			ret = -3;
			goto EXIT;
		}

		status = gcoHAL_QueryChipMinorFeatures(m_hal, &chip_minor_features_number, chip_minor_features);
		if (status < 0)
		{
			ERROR("Failed to query minor features (status = 0x%x)\n", status);
			ret = -4;
			goto EXIT;
		}

		for (i = 0; i < chip_minor_features_number; i++)
		{
			MSG("Chip MinorFeatures%d: 0x%08x\n", i, chip_minor_features[i]);
		}

		MSG("=======================================================\n");

	EXIT:
		return ret;
	}

	int Gc320ObjOmap::initGc320()
	{
		gceSTATUS status;

		/* Construct the gcoOS object. */
		status = gcoOS_Construct(gcvNULL, &m_os);
		if (status < 0)
		{
			ERROR("Failed to construct OS object (status = %d)\n", status);
			return false;
		}

		/* Construct the gcoHAL object. */
		status = gcoHAL_Construct(gcvNULL, m_os, &m_hal);
		if (status < 0)
		{
			ERROR("Failed to construct GAL object (status = %d)\n", status);
			return false;
		}

		if (!gcoHAL_IsFeatureAvailable(m_hal, gcvFEATURE_2DPE20))
		{

			switch (m_dst_format)
			{
				/* PE1.0 support. */
			case gcvSURF_X4R4G4B4:
			case gcvSURF_A4R4G4B4:
			case gcvSURF_X1R5G5B5:
			case gcvSURF_A1R5G5B5:
			case gcvSURF_X8R8G8B8:
			case gcvSURF_A8R8G8B8:
			case gcvSURF_R5G6B5:
				break;

			default:
				ERROR("the target format %d is not supported by the hardware.\n",
					m_dst_format);
				return false;
			}
		}

		status = gcoHAL_Get2DEngine(m_hal, &m_engine2d);
		MSG("****GC320 m_engine2d = %x\n", (uint32_t) m_engine2d);
		if (status < 0)
		{
			ERROR("Failed to get 2D engine object (status = %d)\n", status);
			return false;
		}

		if (chipCheck() < 0)
		{
			ERROR("Check chip info failed!\n");
			return false;
		}

		return true;
	}

}
