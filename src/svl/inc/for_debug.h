//
// for_debug.h
//
//  Created on: 10 июн. 2020 г.
//      Author: devdistress
//
#pragma once

#include "cstdio"
#include "cstdlib"
#include <stdio.h>
#include <errno.h>

#ifdef USE_DEBUG
  #define PRINTF(fmt, arg...)	printf( fmt, ##arg )

  #define PEXIT(fmt, arg...)	{ printf(fmt, ## arg); exit(1); }

  #define MSG(fmt, ...) \
		do { fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while (0)

  #define ERROR(fmt, ...) \
		do { fprintf(stderr, "ERROR:%s %s:%d: " fmt "\n", __FILE__, __func__, __LINE__,##__VA_ARGS__); } while (0)
#else
  #define PRINTF(fmt, arg...)
  #define PEXIT(fmt, arg...)
  #define MSG(fmt, ...) \
		do { fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while (0)

  #define ERROR(fmt, ...) \
		do { fprintf(stderr, "ERROR:%s %s:%d: " fmt "\n", __FILE__, __func__, __LINE__,##__VA_ARGS__); } while (0)
#endif


