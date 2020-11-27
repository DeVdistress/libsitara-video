//
// cmem_buf.h
//
//  Created on: 12 нояб. 2020 г.
//      Author: devdistress
//
// This file defines function to allocate memory from CMEM pool
#pragma once
#include <cstdint>

namespace svl
{
	class BufObj
	{
	public:
		BufObj(uint32_t w, uint32_t h, uint32_t bpp, uint32_t fourcc, uint32_t align, uint32_t num_bufs);
		~BufObj();

		//If applications wants to do cache operations on buffer
		int dmaBufDoCacheOperation(int dma_buf_fd, uint32_t cache_operation);

		uint32_t m_fourcc;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_stride;

		int *m_fd;
		uint32_t *m_fb_id;
		void **m_buf;

	private:
		int allocCmemBuffer(unsigned int size, unsigned int align, void **cmem_buf);
		void freeCmemBuffer(void *cmem_buffer);
		uint32_t m_num_bufs;
	};
}
