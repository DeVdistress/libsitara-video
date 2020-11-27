//
// cmem_buf.cpp
//
//  Created on: 12 нояб. 2020 г.
//      Author: devdistress
//
// This file defines function to allocate memory from CMEM pool
#include <iostream>
#include <ti/cmem.h>
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include "cmem_buf.h"
#include "for_debug.h"

#define CMEM_BLOCKID CMEM_CMABLOCKID

namespace svl
{
	CMEM_AllocParams cmem_alloc_params =
	{
		CMEM_HEAP,      /* type */
		CMEM_CACHED,    /* flags */
		1               /* alignment */
	};

	bool g_is_cmem_init=false;
	bool g_is_cmem_exit=false;

	int BufObj::allocCmemBuffer(uint32_t size, uint32_t align, void **cmem_buf)
	{
		cmem_alloc_params.alignment = align;

		*cmem_buf = CMEM_alloc2(CMEM_BLOCKID, size,
			&cmem_alloc_params);

		if(*cmem_buf == NULL)
		{
			ERROR("CMEM allocation failed");
			return -1;
		}

		return CMEM_export_dmabuf(*cmem_buf);
	}

	void BufObj::freeCmemBuffer(void *cmem_buffer)
	{
		CMEM_free(cmem_buffer, &cmem_alloc_params);
	}

	// If the user space need to access the CMEM buffer for CPU based processing, it can set
	//	the CMEM buffer cache settings using DMA_BUF IOCTLS.
	//	Cahche_operation setting for invalidation is (DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ))
	//	Cache operation settting for buffer read write is (DMA_BUF_SYNC_WRITE | DMA_BUF_SYNC_READ
	//	This piece of code is not used in the application and hence not tested
	int BufObj::dmaBufDoCacheOperation(int dma_buf_fd, uint32_t cache_operation)
	{
		int ret;
		struct dma_buf_sync sync;
		sync.flags = cache_operation;

		ret = ioctl(dma_buf_fd, DMA_BUF_IOCTL_SYNC, &sync);

		return ret;
	}

	BufObj::BufObj(uint32_t w, uint32_t h, uint32_t bpp,uint32_t fourcc,
				   uint32_t align, uint32_t num_bufs)
	{
		m_width = w;
		m_height = h;

		// Vivante HAL needs 16 pixel alignment in width and 4 pixel alignment in
		// height and hence putting that restriction for now on all buffer allocation through CMEM.
		//
		m_stride = ((w + 15) & ~15) * bpp;
		m_fourcc = fourcc;
		m_num_bufs = num_bufs;

		//Avoid calling CMEM init with every BufObj object
		if(g_is_cmem_init == false)
		{
			CMEM_init();
			g_is_cmem_init = true;
		}

		m_fd = (int *)malloc(num_bufs * sizeof(int));

		if(m_fd == NULL)
		{
			ERROR("DispObj: m_fd array allocation failure\n");
		}

		m_fb_id = (uint32_t *)malloc(num_bufs * sizeof(int));
		if(m_fb_id == NULL)
		{
			ERROR("DispObj: m_fb_id array allocation failure\n");
		}

		m_buf = (void **)malloc(num_bufs * sizeof(int));
		if(m_buf == NULL)
		{
			ERROR("DispObj: m_buf array allocation failure\n");
		}

		for(uint32_t i = 0; i < num_bufs; i++)
		{
			// Vivante HAL needs 16 pixel alignment in width and 4 pixel alignment in
			// height and hence adjust the buffer size accrodingly.
			//
			uint32_t size = m_stride * ((m_height + 3) & ~3);

			m_fd[i]  = allocCmemBuffer(size, align, &m_buf[i]);

			if(m_buf[i] == NULL)
			{
				ERROR("CMEM buffer allocation failed");
			}

			if(m_fd[i] < 0)
			{
				freeCmemBuffer(m_buf[i]);
				ERROR("Cannot export CMEM buffer");
			}
		}
	}

	BufObj::~BufObj()
	{
		uint32_t i;

		for(i = 0; i < m_num_bufs; i++)
		{
			//Free the buffer
			freeCmemBuffer(m_buf[i]);
		}

		free(m_fd);
		free(m_fb_id);
		free(m_buf);

		if(g_is_cmem_exit == false)
		{
			CMEM_exit();
			g_is_cmem_exit = true;
		}
	}
}
