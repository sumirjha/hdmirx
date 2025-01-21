#include "encoder.h"
#include "encoder_priv.h"
#include "common.h"
#include <time.h>
#include <errno.h>
#include <rockchip/rk_mpi.h>


static void *recvThread(void *args)
{
    Encoder_t *enc = args;
    MppPacket packet = NULL;
    RK_S64 last_pkt_time = 0;
    RK_S64 first_pkt_time = 0;
    RK_U32 eoi = 1;
    MPP_RET ret = MPP_SUCCESS;

    while (enc->isRunning)
    {
        ret = enc->api->encode_get_packet(enc->ctx, &packet);
        if (ret || NULL == packet)
        {
            printf("Get Package error, %d\n", ret);
            usleep(1);
            continue;
        }

        last_pkt_time = mpp_time();
        uint8_t *data = (uint8_t*)mpp_packet_get_pos(packet);
        size_t len = mpp_packet_get_length(packet);
        
        if(first_pkt_time == 0)
        {
            first_pkt_time = mpp_time();
        }

        RK_U32 pkt_eos = mpp_packet_get_eos(packet);
        /* for low delay partition encoding */
        if (mpp_packet_is_partition(packet))
        {
            eoi = mpp_packet_is_eoi(packet);
        }

        if(len > 0)
        {
            enc->itf->NewPacket(data, len, enc->udata);
        }

        ret = mpp_packet_deinit(&packet);
        assert(ret == MPP_SUCCESS);
    }
}

static CStatus_t encoderSetMppCfg(Encoder_t *enc)
{
    mpp_enc_cfg_set_s32(enc->cfg, "prep:width",         enc->config.width);
    mpp_enc_cfg_set_s32(enc->cfg, "prep:height",        enc->config.height);
    mpp_enc_cfg_set_s32(enc->cfg, "prep:hor_stride",    enc->config.horStride);
    mpp_enc_cfg_set_s32(enc->cfg, "prep:ver_stride",    enc->config.verStride);
    mpp_enc_cfg_set_s32(enc->cfg, "prep:format",        enc->frameFormat);

    mpp_enc_cfg_set_s32(enc->cfg, "rc:mode", enc->rcMode);

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(enc->cfg, "rc:fps_in_flex", 0);
    mpp_enc_cfg_set_s32(enc->cfg, "rc:fps_in_num", enc->config.fps);
    mpp_enc_cfg_set_s32(enc->cfg, "rc:fps_in_denorm", 1);
    mpp_enc_cfg_set_s32(enc->cfg, "rc:fps_out_flex", 0);
    mpp_enc_cfg_set_s32(enc->cfg, "rc:fps_out_num", enc->config.fps);
    mpp_enc_cfg_set_s32(enc->cfg, "rc:fps_out_denorm", 1);
    mpp_enc_cfg_set_s32(enc->cfg, "rc:gop", enc->config.gop ? enc->config.gop : enc->config.fps * 2);

    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(enc->cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(enc->cfg, "rc:drop_thd", 20); /* 20% of max bps */
    mpp_enc_cfg_set_u32(enc->cfg, "rc:drop_gap", 1); /* Do not continuous drop frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(enc->cfg, "rc:bps_target", enc->config.birate);
    switch (enc->rcMode) {
    case MPP_ENC_RC_MODE_FIXQP: {
        /* do not setup bitrate on FIXQP mode */
    } break;
    case MPP_ENC_RC_MODE_CBR: {
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(enc->cfg, "rc:bps_max", enc->config.birate * 17 / 16);
        mpp_enc_cfg_set_s32(enc->cfg, "rc:bps_min", enc->config.birate * 15 / 16);
    } break;
    case MPP_ENC_RC_MODE_VBR:
    case MPP_ENC_RC_MODE_AVBR: {
        /* VBR mode has wide bound */
        mpp_enc_cfg_set_s32(enc->cfg, "rc:bps_max", enc->config.birate * 17 / 16);
        mpp_enc_cfg_set_s32(enc->cfg, "rc:bps_min", enc->config.birate * 1 / 16);
    } break;
    default: {
        /* default use CBR mode */
        mpp_enc_cfg_set_s32(enc->cfg, "rc:bps_max", enc->config.birate * 17 / 16);
        mpp_enc_cfg_set_s32(enc->cfg, "rc:bps_min",  enc->config.birate * 15 / 16);
    } break;
    }

    /* setup qp for different codec and rc_mode */
    switch (enc->codecType) {
    case MPP_VIDEO_CodingAVC:
    case MPP_VIDEO_CodingHEVC: {
        switch (enc->rcMode) {
        case MPP_ENC_RC_MODE_FIXQP: {
            RK_S32 fix_qp = 0;
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_init", fix_qp);
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_max", fix_qp);
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_min", fix_qp);
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_ip", 0);
        } break;
        case MPP_ENC_RC_MODE_CBR:
        case MPP_ENC_RC_MODE_VBR:
        case MPP_ENC_RC_MODE_AVBR: {
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_init", -1);
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_max", 51);
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_min", 10);
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_max_i", 51);
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_min_i", 10);
            mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_ip", 2);
        } break;
        default: {
            printf("unsupport encoder rc mode %d\n", enc->rcMode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8: {
        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_init", 40);
        mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_max", 127);
        mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_min", 0);
        mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_max_i", 127);
        mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_min_i", 0);
        mpp_enc_cfg_set_s32(enc->cfg, "rc:qp_ip", 6);
    } break;
    case MPP_VIDEO_CodingMJPEG: {
        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(enc->cfg, "jpeg:q_factor", 80);
        mpp_enc_cfg_set_s32(enc->cfg, "jpeg:qf_max", 99);
        mpp_enc_cfg_set_s32(enc->cfg, "jpeg:qf_min", 1);
    } break;
    default: {
    } break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(enc->cfg, "codec:type", enc->codecType);
    switch (enc->codecType) {
    case MPP_VIDEO_CodingAVC: {
        RK_U32 constraint_set;
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(enc->cfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(enc->cfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(enc->cfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(enc->cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(enc->cfg, "h264:trans8x8", 1);

        // mpp_env_get_u32("constraint_set", &constraint_set, 0);
        // if (constraint_set & 0x3f0000)
        //     mpp_enc_cfg_set_s32(cfg_, "h264:constraint_set", constraint_set);
    } break;
    case MPP_VIDEO_CodingHEVC:
    case MPP_VIDEO_CodingMJPEG:
    case MPP_VIDEO_CodingVP8: {
    } break;
    default: {
        printf("unsupport encoder coding type %d\n", enc->codecType);
    } break;
    }

    // p->split_mode = 0;
    // p->split_arg = 0;
    // p->split_out = 0;

    // mpp_env_get_u32("split_mode", &p->split_mode, MPP_ENC_SPLIT_NONE);
    // mpp_env_get_u32("split_arg", &p->split_arg, 0);
    // mpp_env_get_u32("split_out", &p->split_out, 0);

    // if (p->split_mode) {
    //     mpp_log_q(quiet, "%p split mode %d arg %d out %d\n", ctx,
    //         p->split_mode, p->split_arg, p->split_out);
    //     mpp_enc_cfg_set_s32(cfg_, "split:mode", p->split_mode);
    //     mpp_enc_cfg_set_s32(cfg_, "split:arg", p->split_arg);
    //     mpp_enc_cfg_set_s32(cfg_, "split:out", p->split_out);
    // }

    // mpp_env_get_u32("mirroring", &mirroring, 0);
    // mpp_env_get_u32("rotation", &rotation, 0);
    // mpp_env_get_u32("flip", &flip, 0);

    // mpp_enc_cfg_set_s32(cfg_, "prep:mirroring", mirroring);
    // mpp_enc_cfg_set_s32(cfg_, "prep:rotation", rotation);
    // mpp_enc_cfg_set_s32(cfg_, "prep:flip", flip);

    MPP_RET ret = enc->api->control(enc->ctx, MPP_ENC_SET_CFG, enc->cfg);
    if (ret != MPP_SUCCESS) {
        printf("mpi control enc set cfg failed ret %d\n", ret);
        return CSTATUS_FAIL;
    }
    return CSTATUS_SUCCESS;
}

Encoder_t * encoderCreate(EncoderConfig_t *config, EncoderInterface_t *itf, void *udata)
{
    Encoder_t *enc = calloc(1, sizeof(Encoder_t));
    memcpy(&enc->config, config, sizeof(EncoderConfig_t));

    enc->itf = itf;
    enc->udata = udata;

    enc->codecType = MPP_VIDEO_CodingAVC; // H264 Codec
    enc->frameFormat = MPP_FMT_YUV444SP;
    enc->rcMode = MPP_ENC_RC_MODE_VBR;
    enc->frameSize = GetFrameSize(enc->frameFormat, enc->config.horStride, enc->config.verStride);
    enc->headerSize = GetHeaderSize(enc->frameFormat, enc->config.width, enc->config.height);
    
    MPP_RET ret = MPP_SUCCESS;
    MppApi *api_ = NULL;
    MppCtx ctx_ = NULL;

    do
    {
        ret = mpp_check_support_format(MPP_CTX_ENC, enc->codecType);
        OKAY_RETURN(ret != MPP_SUCCESS, NULL, "Mpp don't support codec\n");

        ret = mpp_create(&enc->ctx, &enc->api);
        OKAY_RETURN(ret != MPP_SUCCESS, NULL, "failed to create mpp\n");

        api_ = enc->api;
        ctx_ = enc->ctx;

        MppPollType timeout = MPP_POLL_NON_BLOCK;

        ret = api_->control(ctx_, MPP_SET_INPUT_TIMEOUT, &timeout);
        OKAY_STOP(ret != MPP_SUCCESS, "mpi control set input timeout ret %d\n", ret);

        timeout = MPP_POLL_BLOCK;
        ret = api_->control(ctx_, MPP_SET_OUTPUT_TIMEOUT, &timeout);
        OKAY_STOP(ret != MPP_SUCCESS, "mpi control set output timeout ret %d\n", ret);

        ret = mpp_init(ctx_, MPP_CTX_ENC, enc->codecType);
        OKAY_STOP(ret != MPP_SUCCESS, "failed to init mpp\n");

        ret = mpp_enc_cfg_init(&enc->cfg);
        OKAY_STOP(ret != MPP_SUCCESS, "failed to init cfg\n");

        ret = api_->control(ctx_, MPP_ENC_GET_CFG, enc->cfg);
        OKAY_STOP(ret != MPP_SUCCESS, "get enc cfg failed ret %d\n", ret);

        OKAY_STOP(CSTATUS_SUCCESS != encoderSetMppCfg(enc), NULL, "failed to set enc cfg\n");
    } while (0);

    if(ret != MPP_SUCCESS)
    {
        if(ctx_ != NULL)
        {
            mpp_destroy(ctx_);
        }
        free(enc);
        return NULL;
    }

    enc->isRunning = true;

    //recv_thread_ = std::thread(&VideoEncoder::EncRecvThread, this);
    if(pthread_create(&enc->threadEnc, NULL, recvThread, enc))
    {
        printf("failed to create encoder thread : errno(%d)\n", errno);
        mpp_destroy(ctx_);
        free(enc);
        return NULL;
    }

    return enc;
}

void encoderDestroy(Encoder_t *enc)
{
    enc->isRunning = false;
    pthread_join(enc->threadEnc, NULL);
    mpp_destroy(enc->ctx);
    free(enc);
}

CStatus_t encoderPutFrame(Encoder_t *enc, Buffer_t *buff)
{
    MPP_RET ret = MPP_SUCCESS;
    MppBuffer cam_buf = NULL;
    MppFrame frame = NULL;

    ret = mpp_frame_init(&frame);
    OKAY_RETURN(ret != MPP_SUCCESS, CSTATUS_FAIL, "mpp_frame_init failed %d\n", ret);

    mpp_frame_set_width(frame, enc->config.width);
    mpp_frame_set_height(frame, enc->config.height);
    mpp_frame_set_hor_stride(frame, enc->config.horStride);
    mpp_frame_set_ver_stride(frame, enc->config.verStride);
    mpp_frame_set_fmt(frame, enc->frameFormat);
    mpp_frame_set_eos(frame, 0);


    MppBufferInfo info;
    memset(&info, 0, sizeof(MppBufferInfo));
    info.type = MPP_BUFFER_TYPE_EXT_DMA;
    info.fd =  buff->dmafd[0];
    info.size = (uint32_t)buff->size[0] & 0x07ffffff;
    info.index = ((uint32_t)buff->size[0] & 0xf8000000) >> 27;
    mpp_buffer_import(&cam_buf, &info);

    mpp_frame_set_buffer(frame, cam_buf);

    ret = enc->api->encode_put_frame(enc->ctx, frame);
    OKAY_RETURN(ret != MPP_SUCCESS, CSTATUS_FAIL, "frame encoding failed %d\n", ret);

    return CSTATUS_SUCCESS;
}