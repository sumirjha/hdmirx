#include "encoder_priv.h"

int GetFrameSize(MppFrameFormat frame_format, int32_t hor_stride, int32_t ver_stride)
{
    int32_t frame_size = 0;
    switch (frame_format & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P: {
        frame_size = MPP_ALIGN(hor_stride, 64) * MPP_ALIGN(ver_stride, 64) * 3 / 2;
    } break;

    case MPP_FMT_YUV422_YUYV:
    case MPP_FMT_YUV422_YVYU:
    case MPP_FMT_YUV422_UYVY:
    case MPP_FMT_YUV422_VYUY:
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV422SP: {
        frame_size = MPP_ALIGN(hor_stride, 64) * MPP_ALIGN(ver_stride, 64) * 2;
    } break;
    case MPP_FMT_RGB444:
    case MPP_FMT_BGR444:
    case MPP_FMT_RGB555:
    case MPP_FMT_BGR555:
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR565:
    case MPP_FMT_RGB888:
    case MPP_FMT_BGR888:
    case MPP_FMT_RGB101010:
    case MPP_FMT_BGR101010:
    case MPP_FMT_ARGB8888:
    case MPP_FMT_ABGR8888:
    case MPP_FMT_BGRA8888:
    case MPP_FMT_RGBA8888: {
        frame_size = MPP_ALIGN(hor_stride, 64) * MPP_ALIGN(ver_stride, 64);
    } break;

    default: {
        frame_size = MPP_ALIGN(hor_stride, 64) * MPP_ALIGN(ver_stride, 64) * 4;
    } break;
    }
    return frame_size;
}


int GetHeaderSize(MppFrameFormat frame_format, uint32_t width, uint32_t height)
{
    int header_size = 0;
    if (MPP_FRAME_FMT_IS_FBC(frame_format)) {
        if ((frame_format & MPP_FRAME_FBC_MASK) == MPP_FRAME_FBC_AFBC_V1)
            header_size = MPP_ALIGN(MPP_ALIGN(width, 16) * MPP_ALIGN(height, 16) / 16, SZ_4K);
        else
            header_size = MPP_ALIGN(width, 16) * MPP_ALIGN(height, 16) / 16;
    } else {
        header_size = 0;
    }
    return header_size;
}

RK_S64 mpp_time()
{
    struct timespec time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (RK_S64)time.tv_sec * 1000000 + (RK_S64)time.tv_nsec / 1000;
}