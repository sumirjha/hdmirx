#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include "common.h"
#include "buffer.h"
#include "display.h"
#include "encoder.h"
#include "list_common.h"
#include "network.h"
#include "websock.h"

typedef struct
{
	NetCon_t	*con;
	List_t		link;
}NetConWrapper_t;

typedef struct
{
	NetCon_t	*sock;
	List_t		link;
}SockConWrapper_t;

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
	
	//Frame stats
	int						frameCount;
	struct timespec			tsLastTick;

	//Encoder
	Encoder_t 				*enc;

	//Muxer
	void					*ts;
	int						tsStreamId;

	//Network Buffers and Queue
	void					*netBuffer;

	int						qSendCount;
	List_t					qSend;
	List_t					qFree;

	//Websock
	Websock_t				*sockServer;
	List_t					lSocks;

	//Network
	Net_t					*net;
	List_t					lConnections;
	
}App_t;

#endif