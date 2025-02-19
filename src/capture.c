#include "common.h"
#include "capture.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"
#include "list_common.h"
#include "network.h"
#include "websock.h"

//Packetiser
static void* tsAlloc(void* param, size_t bytes)
{
    
    App_t *app = (App_t *) param;
    if(bytes > TS_PACKET_SIZE || bytes <=0)
    {
        printf("TS h264 packetizer asked for buffer of size more than\
                    that can be given, ask - %d, possible - %d\n", bytes, TS_PACKET_SIZE);
        return NULL;
    }

    NetBuffer_t *buf = NULL;
	LIST_POP_FRONT(buf, &app->qFree, link);
	if(NULL == buf)
	{
		printf("failed to get any new free buffer\n");
		return NULL;
	}

	listInsert(&app->qSend, &buf->link);
	app->qSendCount++;

	buf->size = bytes;
	return buf->buffer;
}

static void tsFreePacket(void* param, void *packet)
{
    UNUSED_PARAMETER(param);
    UNUSED_PARAMETER(packet);
}


int tsWrite(void* param, const void* packet, size_t bytes)
{
    UNUSED_PARAMETER(param);
    UNUSED_PARAMETER(packet);
    UNUSED_PARAMETER(bytes);
    return 0;
}

static struct mpeg_ts_func_t mpegHandler = {
    .alloc = tsAlloc,
    .free = tsFreePacket,
    .write = tsWrite,
};


CStatus_t capCreateViewer(App_t *app)
{
	app->viewer = displayCreate(app->src.width, app->src.height);
	OKAY_RETURN(NULL == app->viewer, CSTATUS_FAIL, "failed to create the viewer\n");
	return CSTATUS_SUCCESS;
}

CStatus_t capDrawFrameFromBufferIndex(App_t *app, int index)
{
	Buffer_t *buf = &app->buffers[index];
	return displayDraw(app->viewer, buf);
}


CStatus_t capOpenAndConfigure(App_t *app)
{
	CStatus_t status = CSTATUS_FAIL;
	do
	{
		int ret = -1;

		//Open Device		
		app->v4l2fd = open(V4L2_DEVICE, O_RDWR, 0);
		OKAY_STOP(app->v4l2fd < 0, "failed to open : %s (%s)\n", V4L2_DEVICE, ERRSTR);

		//Query Capabilities
		struct v4l2_capability caps = {0};
		ret = ioctl(app->v4l2fd, VIDIOC_QUERYCAP, &caps);
		OKAY_STOP(ret, "failed to query cap\n");
		OKAY_STOP(!(caps.capabilities & (V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING)),
					"device doesn't support capturing\n");


		//Get and Set Supported Image format
		struct v4l2_format fmt = {0};
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		ret = ioctl(app->v4l2fd, VIDIOC_G_FMT, &fmt);
		OKAY_STOP(ret < 0, "VIDIOC_G_FMT failed: %s\n", ERRSTR);

		fmt.fmt.pix_mp.width = IMG_WIDTH;
		fmt.fmt.pix_mp.height = IMG_HEIGHT;
		fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
		fmt.fmt.pix_mp.num_planes = 1;


		ret = ioctl(app->v4l2fd, VIDIOC_S_FMT, &fmt);
		OKAY_STOP(ret < 0, "VIDIOC_S_FMT failed: %s\n", ERRSTR);

		//Store src value
		app->src.pixfmt = fmt.fmt.pix_mp.pixelformat;
		app->src.width = fmt.fmt.pix_mp.width;
		app->src.height = fmt.fmt.pix_mp.height;
		app->src.numPlanes = fmt.fmt.pix_mp.num_planes;
		app->src.bytesPerLine[0] =  fmt.fmt.pix_mp.plane_fmt[0].bytesperline;

		status = CSTATUS_SUCCESS;
	} while (0);

	return status;
}


CStatus_t capAllocateBuffers(App_t *app, int nums)
{
	int ret = 0;

	//Request DMA Buffer
	struct v4l2_requestbuffers rqbufs = {0};
	rqbufs.count = app->src.numBufs;
	rqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	rqbufs.memory = app->memType;
	ret = ioctl(app->v4l2fd, VIDIOC_REQBUFS, &rqbufs);
	OKAY_RETURN(ret < 0, CSTATUS_FAIL, "VIDIOC_REQBUFS failed: %s\n", ERRSTR);
	OKAY_RETURN(rqbufs.count < DMA_BUFF_COUNT, CSTATUS_FAIL, "video node allocated only "
				"%u of %u buffers\n", rqbufs.count, DMA_BUFF_COUNT);

	for(int i = 0; i < app->src.numBufs; i++)
	{
		struct v4l2_buffer buf = {0};
		buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory  = app->memType;
        buf.index   = i;
        struct v4l2_plane planes[NUM_PLANES];		//We are only going to use 1
        buf.m.planes = planes;
        buf.length = NUM_PLANES;

		ret = ioctl(app->v4l2fd, VIDIOC_QUERYBUF, &buf);
		OKAY_RETURN(ret < 0, CSTATUS_FAIL, "VIDIOC_QUERYBUF failed : index %d, %s\n", i, ERRSTR);

		for(int j = 0; j < NUM_PLANES; j++)
		{
			app->buffers[i].size[j] = buf.m.planes[j].length;
			app->buffers[i].ptr[j] = mmap(NULL,
										buf.m.planes[j].length,
										PROT_READ | PROT_WRITE,	//Required
										MAP_SHARED, 			//Recommended
										app->v4l2fd, buf.m.planes[0].m.mem_offset);
			OKAY_STOP(app->buffers[i].ptr == NULL, "failed to mmap : %i, %s\n", i, ERRSTR);

			struct v4l2_exportbuffer expbuf = (struct v4l2_exportbuffer) {0} ;
			expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			expbuf.index = i;
			expbuf.flags = O_CLOEXEC;
			ret = ioctl(app->v4l2fd, VIDIOC_EXPBUF, &expbuf);
			OKAY_RETURN(ret < 0, CSTATUS_FAIL, "VIDIOC_EXPBUF failed : index %d, %s\n", i, ERRSTR);
			
			app->buffers[i].dmafd[j] = expbuf.fd;
			app->buffers[i].offset[j] = buf.m.planes[j].data_offset;
		}

		// ret = displayInitBuffer(app->viewer, &app->buffers[i]);
		// OKAY_RETURN(CSTATUS_SUCCESS != ret, CSTATUS_FAIL, "failed to create gl texture\n");

		// MppBufferInfo info;
		// memset(&info, 0, sizeof(MppBufferInfo));
		// info.type = MPP_BUFFER_TYPE_EXT_DMA;
		// info.fd =  expbuf.fd;
		// info.size = buf_len & 0x07ffffff;
		// info.index = (buf_len & 0xf8000000) >> 27;
		// mpp_buffer_import(&ctx->fbuf[i].buffer, &info);

		
		app->buffers[i].index = i;
	}

	return CSTATUS_SUCCESS;
}

//Queue All the buffers
CStatus_t capQueueAllBuffers(App_t *app)
{
	struct v4l2_buffer	buffer;
	struct v4l2_plane	buf_planes[1];
	int					i = 0;
	int					ret = -1;

	for (i=0; i<app->src.numBufs; i++)
	{
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buffer.memory = app->memType;
		buffer.index = i;
		buffer.m.planes	= buf_planes;
		buffer.length	= app->src.numPlanes;

		ret = ioctl (app->v4l2fd, VIDIOC_QBUF, &buffer);
		OKAY_RETURN(ret < 0, CSTATUS_FAIL, "VIDIOC_QBUF failed : index %d, %s\n", i, ERRSTR);
	}
	return CSTATUS_SUCCESS;
}

//Enqueue Buffer
CStatus_t capQueueByIndex(App_t *app, int index)
{
	struct v4l2_buffer	buffer;
	struct v4l2_plane	buf_planes[NUM_PLANES] = {0};
	int					ret = -1;


	memset(&buffer, 0, sizeof(buffer));
	buffer.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buffer.memory	= app->memType;
	buffer.index	= index;
	buffer.m.planes	= buf_planes;
	buffer.length	= app->src.numPlanes;

	ret = ioctl (app->v4l2fd, VIDIOC_QBUF, &buffer);
	OKAY_RETURN(ret < 0, CSTATUS_FAIL, "VIDIOC_QBUF failed : index %d, %s\n", index, ERRSTR);
	return CSTATUS_SUCCESS;
}

//Deuque Buffer
CStatus_t capDequeue(App_t *app, Buffer_t ** buff)
{
	struct v4l2_buffer	buf;
	struct v4l2_plane	buf_planes[NUM_PLANES] = {0};
	int			ret = -1;

	memset(&buf, 0, sizeof(buf));
	memset(&buf_planes[0], 0, sizeof(struct v4l2_plane));

	buf.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.memory		= app->memType;
	buf.m.planes	= buf_planes;
	buf.length		= app->src.numPlanes;
	
	ret = ioctl (app->v4l2fd, VIDIOC_DQBUF, &buf);
	if(ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
	{
		*buff = NULL;
		return CSTATUS_AGAIN;
	}
	
	OKAY_RETURN(ret < 0, CSTATUS_FAIL, "VIDIOC_DBUF failed : %s\n", ERRSTR);

	Buffer_t *b = &app->buffers[buf.index];
	b->len[0] = buf.m.planes[0].bytesused;
	*buff = b;

	return CSTATUS_SUCCESS;
}

//Start capturing
CStatus_t capStreamStart(App_t *app)
{
	int	ret = -1;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	ret = ioctl (app->v4l2fd, VIDIOC_STREAMON, &type);
	OKAY_RETURN(ret < 0, CSTATUS_FAIL , "VIDIOC_STREAMON failed : %s\n", ERRSTR);
	return CSTATUS_SUCCESS;
}

// Stop Capture
CStatus_t capStreamStop(App_t *app)
{
	int	ret = -1;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	ret = ioctl (app->v4l2fd, VIDIOC_STREAMOFF, &type);
	OKAY_RETURN(ret < 0, CSTATUS_FAIL, "VIDIOC_STREAMOFF failed : %s\n", ERRSTR);
	return CSTATUS_SUCCESS;
}

static void encoderHandler_NewPacket(unsigned char * data, int len, void *udata)
{
	App_t *app = udata;
	//printf("new encoded packet received : %d bytes\n", len);
	app->qSendCount = 0;
	int retVal =  mpeg_ts_write(app->ts, app->tsStreamId, 0, 0, 0, (const void *)data, len);
	if(retVal != 0)
	{
		printf("failed to packetize buffer (%d bytes) into ts payload. error : %d\n", len, retVal);
	}
	else
	{
		//Successfull
		NetConWrapper_t *w = NULL, *_w = NULL;
		LIST_FOR_EACH_SAFE(w, _w, &app->lConnections, link)
		{
			NetBuffer_t * buf = NULL, *_buf = NULL;
			LIST_FOR_EACH_SAFE(buf, _buf, &app->qSend, link)
			{
				if(-1 == netConSend(w->con, buf))
				{
					break;
				}
			}
		}
	}

	while (1)
	{
		NetBuffer_t * buf = NULL;
		LIST_POP_FRONT(buf, &app->qSend, link);

		//If NULL then there are no more packet left in queue
		if(NULL == buf)
		{
			break;
		}	

		listInsertBack(&app->qFree, &buf->link);
	}

}

EncoderInterface_t encInterface = {
	.NewPacket = encoderHandler_NewPacket,
};


// static void netConHandler_Close(NetCon_t *con, void *udata)
// {
// 	App_t *app = udata;
// 	NetConWrapper_t *w = NULL, *_w = NULL;
// 	LIST_FOR_EACH_SAFE(w, _w, &app->lConnections, link)
// 	{
// 		if(w->con == con)
// 		{
// 			listRemove(&w->link);
// 		}
// 	}
// }

// NetConInterface_t netConInterface = {
// 	.Close = netConHandler_Close,
// };

// static void netHandler_NewClient(NetCon_t *con, void *udata)
// {
// 	App_t *app = udata;
// 	netConnInit(con, &netConInterface, app);
// 	NetConWrapper_t *w = calloc(1, sizeof(NetConWrapper_t));
// 	listInsert(&app->lConnections, &w->link);
// 	w->con = con;
// }

// NetInterface_t netInterface = {
// 	.NewClient = netHandler_NewClient,
// };

int main()
{
	App_t app = {0};
	int ret = 0;

	//Image Setting
	app.src.width = IMG_WIDTH;
	app.src.height = IMG_HEIGHT;
	app.src.pixfmt = V4L2_PIX_FMT_NV24;
	app.src.numBufs = DMA_BUFF_COUNT;
	app.memType = V4L2_MEMORY_MMAP;
	app.bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	clock_gettime(CLOCK_REALTIME, &app.tsLastTick);
	app.frameCount = 0;

	listInit(&app.qSend);
	listInit(&app.qFree);
	listInit(&app.lConnections);

	// NetConfig_t nConfig = {
	// 	.port = 6700,
	// };
	
	// app.net = netCreate(&nConfig, &netInterface, &app);
	// OKAY_RETURN(app.net == NULL, 0, "failed to create network\n");

	Websock_t *sock = websockCreate()

	app.netBuffer = calloc(sizeof(NetBuffer_t), TS_TOTAL_PACKET);
	OKAY_RETURN(app.netBuffer == NULL, 0, "failed to allocate network buffer\n");
	for(int i =0; i < TS_TOTAL_PACKET; i++)
	{
		NetBuffer_t *buf = (NetBuffer_t *)((uint8_t *)app.netBuffer + (i * sizeof(NetBuffer_t)));
		buf->size = 0;
		buf->capacity = TS_PACKET_SIZE;
		listInsert(&app.qFree, &buf->link);
	}

	//Setup TS Mxer
	app.ts = mpeg_ts_create(&mpegHandler, &app);
    if(app.ts == NULL)
    {
        printf("failed to create TS packetizer\n");
		free(app.netBuffer);
        return 0;
    }

    if((app.tsStreamId = mpeg_ts_add_stream(app.ts, PSI_STREAM_H264, NULL, 0)) <= 0)
    {
        printf("failed to add ts stream at packetizer\n");
        return 0;
    }

	EncoderConfig_t encConfig = {
		.birate = 1000000,
		.fps = 60,
		.gop = 60,
		.width = app.src.width,
		.height = app.src.height,
		.horStride = app.src.width,
		.verStride = app.src.height,
	};

	app.enc = encoderCreate(&encConfig, &encInterface, &app);
	OKAY_RETURN(app.enc == NULL, 0, "failed to create encoder device\n");

	do{
		CStatus_t status;

		//Open and Configure
		status = capOpenAndConfigure(&app);
		OKAY_STOP(status != CSTATUS_SUCCESS, "failed to open and configure device.\n");

		//Create Window
		status = capCreateViewer(&app);
		OKAY_STOP(status != CSTATUS_SUCCESS, "failed to create the viewer\n");
		
		//Allocate buffers
		status = capAllocateBuffers(&app, app.src.numBufs);
		OKAY_STOP(status != CSTATUS_SUCCESS, "failed to allocate buffers\n");

		//Start Capture Thread
		for(int i = 0; i < app.src.numBufs; i++)
		{
			status = capQueueByIndex(&app, i);
			OKAY_STOP(status != CSTATUS_SUCCESS, "failed to queue buffer %d\n", i);
		}

		OKAY_STOP(status != CSTATUS_SUCCESS, "failed to queue all buffers\n");

		//Start the stream
		capStreamStart(&app);

		int fd_flags = fcntl(app.v4l2fd, F_GETFL);
		fcntl(app.v4l2fd, F_SETFL, fd_flags | O_NONBLOCK);
		bool quit = false;
		while (!quit)
		{
			fd_set read_fds[2];
			fd_set exception_fds;
			struct timeval tv = { 2, 0 };
			int r;

			FD_ZERO(&exception_fds);
			FD_SET(app.v4l2fd, &exception_fds);
			FD_ZERO(read_fds);
			FD_SET(app.v4l2fd, read_fds);
			int netFd = netGetFd(app.net);
			FD_SET(netFd, read_fds);
			r = select(((app.v4l2fd > netFd)?app.v4l2fd:netFd) + 1, read_fds, NULL, &exception_fds, &tv);
			
			if (FD_ISSET(app.v4l2fd, &exception_fds))
			{
				printf("Exception is set\n");
				break;
			}

			if (FD_ISSET(app.v4l2fd, read_fds))
			{
				Buffer_t *buf1 = NULL;
				status = capDequeue(&app, &buf1);
				if(CSTATUS_AGAIN == status)
				{ continue; }
				else if(CSTATUS_SUCCESS == status)
				{

					//TODO: Check return of Update Texture
					//capDrawFrameFromBufferIndex(&app, buf1->index);
					app.frameCount++;

					struct timespec now;
					double start_sec, end_sec, elapsed_sec;
					clock_gettime(CLOCK_REALTIME, &now);

					end_sec = now.tv_sec + now.tv_nsec / NANO_PER_SEC;
					start_sec = app.tsLastTick.tv_sec + app.tsLastTick.tv_nsec / NANO_PER_SEC;
					elapsed_sec = end_sec - start_sec;

					if(elapsed_sec >= 1)
					{
						app.tsLastTick = now;
						printf("Frames/Sec : %d\n", app.frameCount);
						app.frameCount = 0;;
					}
					encoderPutFrame(app.enc, buf1);
					status = capQueueByIndex(&app, buf1->index);
					OKAY_STOP(status != CSTATUS_SUCCESS, "failed to enqueue : %d\n", buf1->index);
				}
				else
				{
					printf("Dequeue Failed\n");
				}
			}
			else if (FD_ISSET(netFd, read_fds))
			{
				netDispatch(app.net);
			}
		}
		capStreamStop(&app);

	}while(0);

	//TODO: Cleanup code
	if(app.v4l2fd >= 0)
	{
		close(app.v4l2fd);
	}

	if(app.enc != NULL)
	{
		encoderDestroy(app.enc);
	}

	if(app.net != NULL)
	{
		NetConWrapper_t *w = NULL, *_w = NULL;
		LIST_FOR_EACH_SAFE(w, _w, &app.lConnections, link)
		{
			listRemove(&w->link);
			netConClose(w->con);
			free(w);
		}
		netDestroy(app.net);
	}

	if(app.netBuffer != NULL)
	{
		free(app.netBuffer);
	}

	return 0;
}
