#ifndef __ENCODER_CAPTURE_H__
#define __ENCODER_CAPTURE_H__

#include <rockchip/rk_mpi.h>
#include "buffer.h"
#include "common.h"
#include <pthread.h>

typedef struct
{
    //Encoder Config
    int                 width;
    int                 height;
    int                 verStride;  //Represents the distance between two adjacent rows in vertical direction, in units of bytes.
    int                 horStride;  //Represents the number of row spacing between image components, in units of 1.

    int                 birate;
    int                 fps;
    int                 gop;
}EncoderConfig_t;

typedef struct
{
    void (*NewPacket)(unsigned char * data, int len, void *udata);
}EncoderInterface_t;

typedef struct
{
    //Handles
    MppCtx              ctx;
    MppApi              *api;
    MppEncCfg           cfg;
    pthread_t           threadEnc;

    //Callbacks and udata
    EncoderInterface_t  *itf;
    void                *udata;

    //Library Settings
    int                 frameSize;
    int                 headerSize;
    MppCodingType       codecType;
    MppFrameFormat      frameFormat;
    MppEncRcMode        rcMode;
    MppEncHeaderMode    headerMode;
    MppEncSeiMode       seiMode;

    //Configs
    EncoderConfig_t     config;

    //State Varibles
    bool                isRunning;
}Encoder_t;

Encoder_t * encoderCreate(EncoderConfig_t *config, EncoderInterface_t *itf, void *udata);

void encoderDestroy(Encoder_t *enc);

CStatus_t encoderPutFrame(Encoder_t *enc, Buffer_t *buff);


#endif