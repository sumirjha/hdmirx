#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include "common.h"
#include "buffer.h"
#include "display.h"

typedef struct
{
	int v4l2fd;
	enum v4l2_buf_type 		bufType;
	enum v4l2_memory 		memType;

	struct
	{
		uint32_t 				pixfmt;
		int 					numPlanes;
		enum v4l2_colorspace 	clrspc;
		uint32_t 				width;
		uint32_t 				height;
		int						bytesPerLine[NUM_PLANES];
		int						numBufs;
	}src;

	Display_t				*viewer;
    Buffer_t 				buffers[DMA_BUFF_COUNT];
	
}App_t;

#endif