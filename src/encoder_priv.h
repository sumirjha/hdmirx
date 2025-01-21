#ifndef __ENCODER_CAPTURE_PRIV_H__
#define __ENCODER_CAPTURE_PRIV_H__

#include "encoder.h"

#define MPP_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define SZ_1K (1024)
#define SZ_2K (SZ_1K * 2)
#define SZ_4K (SZ_1K * 4)

int GetFrameSize(MppFrameFormat frame_format, int32_t hor_stride, int32_t ver_stride);
int GetHeaderSize(MppFrameFormat frame_format, uint32_t width, uint32_t height);
RK_S64 mpp_time();
#endif