/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_media_demo
**********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statfs.h>  
#include<fcntl.h>
#include<sys/prctl.h>
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ring_buffer.h"
#include "tuya_xvr_dev.h"
#include "tuya_xvr_dynamic_media_api.h"
#ifdef DEMO_NVR_ONVIF_ENABLE
#include "tuya_ipc_onvif_client_demo.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

IPC_MEDIA_INFO_S s_media_info = {0};
IPC_MEDIA_INFO_S s_new_media_info = {0};
Ring_Buffer_User_Handle_S v_handle_hevc[DEMO_NVR_SUB_DEV_NUM];
Ring_Buffer_User_Handle_S sub_v_handle[DEMO_NVR_SUB_DEV_NUM];
Ring_Buffer_User_Handle_S main_v_handle[DEMO_NVR_SUB_DEV_NUM];
Ring_Buffer_User_Handle_S a_handle[DEMO_NVR_SUB_DEV_NUM];
BOOL_T s_is_update[DEMO_NVR_SUB_DEV_NUM] ={0};
BOOL_T s_is_new[DEMO_NVR_SUB_DEV_NUM] = {0};
extern CHAR_T s_raw_path[128];

/* Set audio and video properties */
VOID IPC_APP_Set_Media_Info(VOID)
{
    memset(&s_media_info, 0 , sizeof(IPC_MEDIA_INFO_S));

    /* main stream(HD), video configuration*/
    /* NOTE
    FIRST:If the main stream supports multiple video stream configurations, set each item to the upper limit of the allowed configuration.
    SECOND:E_CHANNEL_VIDEO_MAIN must exist.It is the data source of SDK.
    please close the E_CHANNEL_VIDEO_SUB for only one stream*/
    s_media_info.channel_enable[E_IPC_STREAM_VIDEO_MAIN] = TRUE;    /* Whether to enable local HD video streaming 是否开启本地高清视频流*/
    s_media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN] = 30;  /* FPS */
    s_media_info.video_gop[E_IPC_STREAM_VIDEO_MAIN] = 30;  /* GOP */
    s_media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN] = TUYA_VIDEO_BITRATE_1M; /* Rate limit 码率上限*/
    s_media_info.video_width[E_IPC_STREAM_VIDEO_MAIN] = 1920; /* Single frame resolution of width 单帧分辨率 宽*/
    s_media_info.video_height[E_IPC_STREAM_VIDEO_MAIN] = 1080;/* Single frame resolution of height 单帧分辨率 高 */
    s_media_info.video_freq[E_IPC_STREAM_VIDEO_MAIN] = 90000; /* Clock frequency 时钟频率*/
    s_media_info.video_codec[E_IPC_STREAM_VIDEO_MAIN] = TUYA_CODEC_VIDEO_H264; /* Encoding format 编码格式*/

    /* substream(HD), video configuration */
    /* Please note that if the substream supports multiple video stream configurations, please set each item to the upper limit of the allowed configuration. */
    /* 请注意如果设备子码流支持多种视频流配置，这里请把各项设置为允许配置的上限值 */
    s_media_info.channel_enable[E_IPC_STREAM_VIDEO_SUB] = TRUE;     /* Whether to enable local SD video stream 是否开启本地标清视频流*/
    s_media_info.video_fps[E_IPC_STREAM_VIDEO_SUB] = 30;  /* FPS */
    s_media_info.video_gop[E_IPC_STREAM_VIDEO_SUB] = 30;  /* GOP */
    s_media_info.video_bitrate[E_IPC_STREAM_VIDEO_SUB] = TUYA_VIDEO_BITRATE_512K; /* Rate limit 码率上限*/
    s_media_info.video_width[E_IPC_STREAM_VIDEO_SUB] = 640; /* Single frame resolution of width 单帧分辨率 宽*/
    s_media_info.video_height[E_IPC_STREAM_VIDEO_SUB] = 360;/* Single frame resolution of height 单帧分辨率 高 */
    s_media_info.video_freq[E_IPC_STREAM_VIDEO_SUB] = 90000; /* Clock frequency 时钟频率 */
    s_media_info.video_codec[E_IPC_STREAM_VIDEO_SUB] = TUYA_CODEC_VIDEO_H264; /* Encoding format 编码格式 */

    /* Audio stream configuration. 
    Note: The internal P2P preview, cloud storage, and local storage of the SDK are all use E_CHANNEL_AUDIO data. */
    s_media_info.channel_enable[E_IPC_STREAM_AUDIO_MAIN] = TRUE;         /* Whether to enable local sound collection 是否开启本地声音采集 */
    s_media_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN] = TUYA_CODEC_AUDIO_PCM;/* Encoding format 编码格式 */
    s_media_info.audio_sample [E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_SAMPLE_8K;/* Sampling Rate 采样率 */
    s_media_info.audio_databits [E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_DATABITS_16;/* Bit width 位宽 */
    s_media_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_CHANNEL_MONO;/* channel 信道 */
    s_media_info.audio_fps[E_IPC_STREAM_AUDIO_MAIN] = 25;/* Fragments per second 每秒分片数 */

    PR_DEBUG("channel_enable:%d %d %d", s_media_info.channel_enable[E_IPC_STREAM_VIDEO_MAIN], s_media_info.channel_enable[E_IPC_STREAM_VIDEO_SUB], s_media_info.channel_enable[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("fps:%u", s_media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("gop:%u", s_media_info.video_gop[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("bitrate:%u kbps", s_media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_main_width:%u", s_media_info.video_width[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_main_height:%u", s_media_info.video_height[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_freq:%u", s_media_info.video_freq[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_codec:%d", s_media_info.video_codec[E_IPC_STREAM_VIDEO_MAIN]);

    PR_DEBUG("audio_codec:%d", s_media_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("audio_sample:%d", s_media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("audio_databits:%d", s_media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("audio_channel:%d", s_media_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN]);
}

VOID IPC_APP_Get_Media_Info(IN CONST INT_T chn, IN CONST CHAR_T * devId,IN OUT IPC_MEDIA_INFO_S *param)
{
    //demo 所展示设备的信息是一样的。故没有区分设备。用户根据chn或者devId区分不同设备多媒体信息
    //确保媒体信息和IPC_APP_Set_Media_Info函数设置是一致的
    param->channel_enable[E_IPC_STREAM_VIDEO_MAIN] = TRUE;    /* 是否开启本地高清视频流 */
    param->video_fps[E_IPC_STREAM_VIDEO_MAIN] = 30;  /* FPS */
    param->video_gop[E_IPC_STREAM_VIDEO_MAIN] = 30;  /* GOP */
    param->video_bitrate[E_IPC_STREAM_VIDEO_MAIN] = TUYA_VIDEO_BITRATE_1M; /* 码率上限 */
    param->video_width[E_IPC_STREAM_VIDEO_MAIN] = 1920; /* 单帧分辨率 宽 */
    param->video_height[E_IPC_STREAM_VIDEO_MAIN] = 1080;/* 单帧分辨率 高 */
    param->video_freq[E_IPC_STREAM_VIDEO_MAIN] = 90000; /* 时钟频率 */
    if(s_is_new[chn] == FALSE) {
        param->video_codec[E_IPC_STREAM_VIDEO_MAIN] = TUYA_CODEC_VIDEO_H264; /* 编码格式 */
    } else {
        param->video_codec[E_IPC_STREAM_VIDEO_MAIN] = TUYA_CODEC_VIDEO_H265; /* 编码格式 */

    }
    //只需要高清和音频
    /* 请注意如果设备子码流支持多种视频流配置，这里请把各项设置为允许配置的上限值 */
    param->channel_enable[E_IPC_STREAM_VIDEO_SUB] = TRUE;     /* 是否开启本地标清视频流 */
    param->video_fps[E_IPC_STREAM_VIDEO_SUB] = 30;  /* FPS */
    param->video_gop[E_IPC_STREAM_VIDEO_SUB] = 30;  /* GOP */
    param->video_bitrate[E_IPC_STREAM_VIDEO_SUB] = TUYA_VIDEO_BITRATE_512K; /* 码率上限 */
    param->video_width[E_IPC_STREAM_VIDEO_SUB] = 640; /* 单帧分辨率 宽 */
    param->video_height[E_IPC_STREAM_VIDEO_SUB] = 360;/* 单帧分辨率 高 */
    param->video_freq[E_IPC_STREAM_VIDEO_SUB] = 90000; /* 时钟频率 */
    if(s_is_new[chn]== FALSE) {
        param->video_codec[E_IPC_STREAM_VIDEO_SUB] = TUYA_CODEC_VIDEO_H264; /* 编码格式 */

    } else {
        param->video_codec[E_IPC_STREAM_VIDEO_SUB] = TUYA_CODEC_VIDEO_H265; /* 编码格式 */

    }

    /* 音频码流配置 */
    param->channel_enable[E_IPC_STREAM_AUDIO_MAIN] = TRUE;         /* 是否开启本地声音采集 */
    param->audio_codec[E_IPC_STREAM_AUDIO_MAIN] = TUYA_CODEC_AUDIO_PCM;/* 编码格式 */
    param->audio_sample[E_IPC_STREAM_AUDIO_MAIN] = TUYA_AUDIO_SAMPLE_8K;/* 采样率 */
    param->audio_databits[E_IPC_STREAM_AUDIO_MAIN] = TUYA_AUDIO_DATABITS_16;/* 位宽 */
    param->audio_channel[E_IPC_STREAM_AUDIO_MAIN] = TUYA_AUDIO_CHANNEL_MONO;/* 信道 */
    param->audio_fps[E_IPC_STREAM_AUDIO_MAIN] = 25;/* 每秒分片数 */

    param->channel_enable[E_IPC_STREAM_AUDIO_SUB] = TRUE;         /* 是否开启本地声音采集 */
    param->audio_codec[E_IPC_STREAM_AUDIO_SUB] = TUYA_CODEC_AUDIO_AAC_ADTS;/* 编码格式 */
    param->audio_sample[E_IPC_STREAM_AUDIO_SUB] = TUYA_AUDIO_SAMPLE_8K;/* 采样率 */
    param->audio_databits[E_IPC_STREAM_AUDIO_SUB] = TUYA_AUDIO_DATABITS_16;/* 位宽 */
    param->audio_channel[E_IPC_STREAM_AUDIO_SUB] = TUYA_AUDIO_CHANNEL_MONO;/* 信道 */
    param->audio_fps[E_IPC_STREAM_AUDIO_SUB] = 8;/* 每秒分片数 */

}

VOID IPC_APP_New_Set_Media_Info(VOID)
{
    memset(&s_new_media_info, 0 , sizeof(IPC_MEDIA_INFO_S));

    /* main stream(HD), video configuration*/
    /* NOTE
    FIRST:If the main stream supports multiple video stream configurations, set each item to the upper limit of the allowed configuration.
    SECOND:E_CHANNEL_VIDEO_MAIN must exist.It is the data source of SDK.
    please close the E_CHANNEL_VIDEO_SUB for only one stream*/
    s_new_media_info.channel_enable[E_IPC_STREAM_VIDEO_MAIN] = TRUE;    /* Whether to enable local HD video streaming 是否开启本地高清视频流*/
    s_new_media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN] = 30;  /* FPS */
    s_new_media_info.video_gop[E_IPC_STREAM_VIDEO_MAIN] = 30;  /* GOP */
    s_new_media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN] = 2048*2; /* Rate limit 码率上限*/
    s_new_media_info.video_width[E_IPC_STREAM_VIDEO_MAIN] = 640; /* Single frame resolution of width 单帧分辨率 宽*/
    s_new_media_info.video_height[E_IPC_STREAM_VIDEO_MAIN] = 360;/* Single frame resolution of height 单帧分辨率 高 */
    s_new_media_info.video_freq[E_IPC_STREAM_VIDEO_MAIN] = 90000; /* Clock frequency 时钟频率*/
    s_new_media_info.video_codec[E_IPC_STREAM_VIDEO_MAIN] = TUYA_CODEC_VIDEO_H265; /* Encoding format 编码格式*/

    /* substream(HD), video configuration */
    /* Please note that if the substream supports multiple video stream configurations, please set each item to the upper limit of the allowed configuration. */
    /* 请注意如果设备子码流支持多种视频流配置，这里请把各项设置为允许配置的上限值 */
    s_new_media_info.channel_enable[E_IPC_STREAM_VIDEO_SUB] = TRUE;     /* Whether to enable local SD video stream 是否开启本地标清视频流*/
    s_new_media_info.video_fps[E_IPC_STREAM_VIDEO_SUB] = 30;  /* FPS */
    s_new_media_info.video_gop[E_IPC_STREAM_VIDEO_SUB] = 30;  /* GOP */
    s_new_media_info.video_bitrate[E_IPC_STREAM_VIDEO_SUB] = 2048*2; /* Rate limit 码率上限*/
    s_new_media_info.video_width[E_IPC_STREAM_VIDEO_SUB] = 640; /* Single frame resolution of width 单帧分辨率 宽*/
    s_new_media_info.video_height[E_IPC_STREAM_VIDEO_SUB] = 360;/* Single frame resolution of height 单帧分辨率 高 */
    s_new_media_info.video_freq[E_IPC_STREAM_VIDEO_SUB] = 90000; /* Clock frequency 时钟频率 */
    s_new_media_info.video_codec[E_IPC_STREAM_VIDEO_SUB] = TUYA_CODEC_VIDEO_H265; /* Encoding format 编码格式 */

    /* Audio stream configuration.
    Note: The internal P2P preview, cloud storage, and local storage of the SDK are all use E_CHANNEL_AUDIO data. */
    s_new_media_info.channel_enable[E_IPC_STREAM_AUDIO_MAIN] = TRUE;         /* Whether to enable local sound collection 是否开启本地声音采集 */
    s_new_media_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN] = TUYA_CODEC_AUDIO_PCM;/* Encoding format 编码格式 */
    s_new_media_info.audio_sample [E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_SAMPLE_8K;/* Sampling Rate 采样率 */
    s_new_media_info.audio_databits [E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_DATABITS_16;/* Bit width 位宽 */
    s_new_media_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_CHANNEL_MONO;/* channel 信道 */
    s_new_media_info.audio_fps[E_IPC_STREAM_AUDIO_MAIN] = 25;/* Fragments per second 每秒分片数 */

    PR_DEBUG("channel_enable:%d %d %d", s_new_media_info.channel_enable[E_IPC_STREAM_VIDEO_MAIN], s_new_media_info.channel_enable[E_IPC_STREAM_VIDEO_SUB], s_new_media_info.channel_enable[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("fps:%u", s_new_media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("gop:%u", s_new_media_info.video_gop[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("bitrate:%u kbps", s_new_media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_main_width:%u", s_new_media_info.video_width[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_main_height:%u", s_new_media_info.video_height[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_freq:%u", s_new_media_info.video_freq[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_codec:%d", s_new_media_info.video_codec[E_IPC_STREAM_VIDEO_MAIN]);

    PR_DEBUG("audio_codec:%d", s_new_media_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("audio_sample:%d", s_new_media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("audio_databits:%d", s_new_media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("audio_channel:%d", s_new_media_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN]);
}


void IPC_APP_New_Get_Media_Info(IN CONST INT_T chn, IN CONST CHAR_T * devId,IN OUT IPC_MEDIA_INFO_S *pMedia)
{
    memset(pMedia, 0x00 ,sizeof(IPC_MEDIA_INFO_S));

    //UserTodo: get media info for sub dev
    memcpy(pMedia, &s_new_media_info ,sizeof(IPC_MEDIA_INFO_S));// for demo test

    return ;
}

OPERATE_RET TUYA_APP_Init_Ring_Buffer(INT_T devic_chan,CHAR_T * devId)
{
    IPC_STREAM_E channel;
    IPC_STREAM_E chnIdx;
    OPERATE_RET ret = OPRT_OK;

    if (NULL == devId){
        PR_ERR("input error");
        return -1;
    }

    IPC_MEDIA_INFO_S media_info;
    memset(&media_info, 0x00, sizeof(IPC_MEDIA_INFO_S));
#ifdef DEMO_NVR_ONVIF_ENABLE
    TUYA_APP_Onvif_Get_Media_Info(devId, &media_info);
#else
    IPC_APP_Get_Media_Info(devic_chan,devId, &media_info);
#endif
    //step：根据需求，循环初始化对应设备的通道。当前demo初始化了主码流和附码流及主音频的ring buffer
    Ring_Buffer_Init_Param_S param;
    for( chnIdx = E_IPC_STREAM_VIDEO_MAIN; chnIdx < E_IPC_STREAM_MAX; chnIdx++ ) {
        if(TRUE == media_info.channel_enable[chnIdx]) {
            if(chnIdx == E_IPC_STREAM_AUDIO_MAIN) {
                PR_DEBUG("audio_sample %d, audio_databits %d, audio_fps %d",media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN],media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN],media_info.audio_fps[E_IPC_STREAM_AUDIO_MAIN]);
                //通道号一定需要从这个接口获取。开发不可以自认为通道号是APP上显示的通道号顺序
                ret = tuya_xvr_dev_chan_get_by_devId(devId, (INT_T *)&channel);
                if (OPRT_OK != ret){
                    PR_ERR("get ring chn id failed");
                }
                param.bitrate = media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN]*media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN]/1024;
                param.fps = media_info.audio_fps[E_IPC_STREAM_AUDIO_MAIN];
                //这个数据，默认填写0，SDK内部是按照10秒处理，这样出图效果最合适。
                //当然用户根据自身设备，不端调优这个值。但建议不小于5秒
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                ret = tuya_ipc_ring_buffer_init(channel, 0,E_IPC_STREAM_AUDIO_MAIN,&param);
            } else if(chnIdx == E_IPC_STREAM_AUDIO_SUB) {
                ret = tuya_xvr_dev_chan_get_by_devId(devId, (INT_T *)&channel);
                if (OPRT_OK != ret){
                    PR_ERR("get ring chn id failed");
                }
                param.bitrate = media_info.audio_sample[E_IPC_STREAM_AUDIO_SUB]*media_info.audio_databits[E_IPC_STREAM_AUDIO_SUB]/1024;
                param.fps = media_info.audio_fps[E_IPC_STREAM_AUDIO_SUB];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                ret = tuya_ipc_ring_buffer_init(channel, 0,E_IPC_STREAM_AUDIO_SUB,&param);
            } else if (chnIdx == E_IPC_STREAM_VIDEO_MAIN){
                PR_DEBUG("video_bitrate %d, video_fps %d\n",media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN],media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN]);
                ret = tuya_xvr_dev_chan_get_by_devId(devId, (INT_T *)&channel);
                if (OPRT_OK != ret){
                    PR_ERR("get ring chn id failed");
                }
                param.bitrate = media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN];
                param.fps = media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                ret = tuya_ipc_ring_buffer_init(channel,0, E_IPC_STREAM_VIDEO_MAIN,&param);
            } else if (chnIdx == E_IPC_STREAM_VIDEO_SUB){
                ret = tuya_xvr_dev_chan_get_by_devId(devId, (INT_T *)&channel);
                if (OPRT_OK != ret){
                    PR_ERR("get ring chn id failed");
                }
                param.bitrate = media_info.video_bitrate[E_IPC_STREAM_VIDEO_SUB];
                param.fps = media_info.video_fps[E_IPC_STREAM_VIDEO_SUB];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                ret = tuya_ipc_ring_buffer_init(channel, 0,E_IPC_STREAM_VIDEO_SUB,&param);
            } else{
                PR_DEBUG("not support\n");

            }
        }
    }

    return ret;
}
OPERATE_RET TUYA_APP_Uninit_Ring_Buffer(INT_T device)
{
    IPC_STREAM_E channel;
    IPC_STREAM_E chnIdx;
    OPERATE_RET ret = OPRT_OK;
    CHAR_T devId[64]={0};
    tuya_xvr_dev_chan_get_by_devId(devId, (INT_T *)&channel);
    // step 1: close 所有对应device设备的ring buffer 句柄，防止内存泄漏
    tuya_ipc_ring_buffer_close(main_v_handle[device]);
    tuya_ipc_ring_buffer_close(sub_v_handle[device]);
    tuya_ipc_ring_buffer_close(a_handle[device]);
    main_v_handle[device] = NULL;
    sub_v_handle[device] = NULL;
    a_handle[device] = NULL;
    //step 2:uninit all 所有初始化过的设备
    IPC_MEDIA_INFO_S media_info;
    memset(&media_info, 0x00, sizeof(IPC_MEDIA_INFO_S));
#ifdef DEMO_NVR_ONVIF_ENABLE
    TUYA_APP_Onvif_Get_Media_Info(devId, &media_info);
#else
    IPC_APP_Get_Media_Info(device,devId,&media_info);
#endif
    Ring_Buffer_Init_Param_S param;
    for( chnIdx = E_IPC_STREAM_VIDEO_MAIN; chnIdx < E_IPC_STREAM_MAX; chnIdx++ ) {
        if(TRUE == media_info.channel_enable[chnIdx]) {
            if(chnIdx == E_IPC_STREAM_AUDIO_MAIN) {
                ret = tuya_ipc_ring_buffer_uninit(device, 0,E_IPC_STREAM_AUDIO_MAIN);
            } else if(chnIdx == E_IPC_STREAM_AUDIO_SUB) {
                ret = tuya_ipc_ring_buffer_uninit(device, 0,E_IPC_STREAM_AUDIO_SUB);
            } else if (chnIdx == E_IPC_STREAM_VIDEO_MAIN){
                ret = tuya_ipc_ring_buffer_uninit(device,0, E_IPC_STREAM_VIDEO_MAIN);
            } else if (chnIdx == E_IPC_STREAM_VIDEO_SUB){
                ret = tuya_ipc_ring_buffer_uninit(device, 0,E_IPC_STREAM_VIDEO_SUB);
            } else if(chnIdx == E_IPC_STREAM_VIDEO_3RD){
                ret = tuya_ipc_ring_buffer_uninit(device, 0,E_IPC_STREAM_VIDEO_3RD);
            }else{
                PR_DEBUG("not support\n");

            }
        }
    }

    return ret;
}

OPERATE_RET TUYA_APP_Init_New_Ring_Buffer(INT_T devic_chan,CHAR_T * devId)
{
    //step 1
    IPC_APP_New_Set_Media_Info();

    IPC_STREAM_E channel;
    IPC_STREAM_E chnIdx;
    OPERATE_RET ret = OPRT_OK;

    if (NULL == devId){
        PR_ERR("input error");
        return -1;
    }

    IPC_MEDIA_INFO_S media_info;
    memset(&media_info, 0x00, sizeof(IPC_MEDIA_INFO_S));
#ifdef DEMO_NVR_ONVIF_ENABLE
    TUYA_APP_Onvif_Get_Media_Info(devId, &media_info);
#else
    //step 2
    IPC_APP_New_Get_Media_Info(devic_chan,devId, &media_info);
#endif
    Ring_Buffer_Init_Param_S param;
    for( chnIdx = E_IPC_STREAM_VIDEO_MAIN; chnIdx < E_IPC_STREAM_MAX; chnIdx++ ) {
        if(TRUE == media_info.channel_enable[chnIdx]) {
            if(chnIdx == E_IPC_STREAM_AUDIO_MAIN) {
                PR_DEBUG("audio_sample %d, audio_databits %d, audio_fps %d",media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN],media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN],media_info.audio_fps[E_IPC_STREAM_AUDIO_MAIN]);

                //通道号一定需要从这个接口获取。开发不可以自认为通道号是APP上显示的通道号顺序。
                ret = tuya_xvr_dev_chan_get_by_devId(devId, (INT_T *)&channel);
                if (OPRT_OK != ret){
                    PR_ERR("get ring chn id failed");
                }

                param.bitrate = media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN]*media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN]/1024;
                param.fps = media_info.audio_fps[E_IPC_STREAM_AUDIO_MAIN];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                ret = tuya_ipc_ring_buffer_init(channel, 0,E_IPC_STREAM_AUDIO_MAIN,&param);
            } else if(chnIdx == E_IPC_STREAM_AUDIO_SUB) {
                ret = tuya_xvr_dev_chan_get_by_devId(devId, (INT_T *)&channel);
                if (OPRT_OK != ret){
                    PR_ERR("get ring chn id failed");
                }

                param.bitrate = media_info.audio_sample[E_IPC_STREAM_AUDIO_SUB]*media_info.audio_databits[E_IPC_STREAM_AUDIO_SUB]/1024;
                param.fps = media_info.audio_fps[E_IPC_STREAM_AUDIO_SUB];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                ret = tuya_ipc_ring_buffer_init(channel, 0,E_IPC_STREAM_AUDIO_SUB,&param);
            } else if (chnIdx == E_IPC_STREAM_VIDEO_MAIN){
                PR_DEBUG("video_bitrate %d, video_fps %d\n",media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN],media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN]);

                ret = tuya_xvr_dev_chan_get_by_devId(devId, (INT_T *)&channel);
                if (OPRT_OK != ret){
                    PR_ERR("get ring chn id failed");
                }

                param.bitrate = media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN];
                param.fps = media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                tuya_ipc_ring_buffer_set_max_frame_size(channel,0, E_IPC_STREAM_VIDEO_MAIN,500*1024);
                ret = tuya_ipc_ring_buffer_init(channel,0, E_IPC_STREAM_VIDEO_MAIN,&param);
                ret = tuya_ipc_ring_buffer_set_max_frame_size(channel,0, E_IPC_STREAM_VIDEO_MAIN,500*1024);
            } else if (chnIdx == E_IPC_STREAM_VIDEO_SUB){
                ret = tuya_xvr_dev_chan_get_by_devId(devId, (INT_T *)&channel);
                if (OPRT_OK != ret){
                    PR_ERR("get ring chn id failed");
                }

                param.bitrate = media_info.video_bitrate[E_IPC_STREAM_VIDEO_SUB];
                param.fps = media_info.video_fps[E_IPC_STREAM_VIDEO_SUB];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;

                ret = tuya_ipc_ring_buffer_init(channel, 0,E_IPC_STREAM_VIDEO_SUB,&param);
                /*用户可以调用设置最大帧大小。如果不调用，XVR SDK会按照码率大小的1.5来计算*/
                ret = tuya_ipc_ring_buffer_set_max_frame_size(channel,0, E_IPC_STREAM_VIDEO_SUB,500*1024);
            } else{
                PR_DEBUG("not support\n");
            }
        }
    }

    return ret;
}
/*
 * The sample code simulates audio and video by reading and writing files in rawfiles.tar.gz
 */
#define AUDIO_FRAME_SIZE 640
#define AUDIO_FPS 25
#define VIDEO_BUF_SIZE	(1024 * 400) //Maximum frame

typedef struct {
    int valid;    
    pthread_t h264_thread;
    pthread_t audio_thread;
    char devId[64]; /* id of sub dev */
    int chn;        /* chan of sub dev */
}FAKE_VIDEO_INFO_T;

#define DEMO_FAKE_VIDEO_MAX_NUM (6)
FAKE_VIDEO_INFO_T s_fake_video[DEMO_FAKE_VIDEO_MAX_NUM];

/*
Example: Simulate audio output by reading and writing files and push it to TUYA SDK

*/

void *thread_live_audio(void *arg)
{
    char fullpath[128] = {0};
    sprintf(fullpath, "%s/resource/media/demo_audio.raw",s_raw_path);
    FAKE_VIDEO_INFO_T * pParam = (FAKE_VIDEO_INFO_T *)arg;
    FILE *aFp = fopen(fullpath, "rb");
    if(aFp == NULL)
    {
        printf("can't read live audio files\n");
        pthread_exit(0);
    }
    char audioBuf[AUDIO_FRAME_SIZE];
    memset(audioBuf, 0, sizeof(audioBuf));
    MEDIA_FRAME_S pcm_frame = {0};
    pcm_frame.type = E_AUDIO_FRAME;

    INT_T aChn = 0;
    tuya_xvr_dev_chan_get_by_devId(pParam->devId,(INT_T *)&aChn);
    PR_DEBUG("start live audio using %s [%d]",fullpath,aChn);

    a_handle[aChn] = tuya_ipc_ring_buffer_open(aChn, 0, E_IPC_STREAM_AUDIO_MAIN, E_RBUF_WRITE);
    if(a_handle[aChn] == NULL)
    {
        pthread_exit(0);
    }

    while(1)
    {
        if(s_is_update[aChn] == TRUE) {
            sleep(1);
            continue;
        }
        //#define USER_NO_AUDIO  //若子设备无音频，可填充PCM的静音数据
        #ifdef USER_NO_AUDIO
        int size = AUDIO_FRAME_SIZE;

        #else
        if(a_handle[aChn] == NULL) {
            a_handle[aChn] = tuya_ipc_ring_buffer_open(aChn, 0, E_IPC_STREAM_AUDIO_MAIN, E_RBUF_WRITE);
            if(a_handle[aChn] == NULL) {
                continue;
            }
        }

        int size = fread(audioBuf, 1, AUDIO_FRAME_SIZE, aFp);
        if(size < AUDIO_FRAME_SIZE)
        {
            rewind(aFp);
            continue;
        }
        #endif
        pcm_frame.size = size;
        pcm_frame.p_buf = audioBuf;
        pcm_frame.pts = -1;//如果没有其他值，用户需要写-1。否则PUT会失败

        TUYA_APP_Put_Frame(a_handle[aChn],&pcm_frame);
        int frameRate = AUDIO_FPS;
        int sleepTick = 1000000/frameRate;
        usleep(sleepTick);
    }

    pthread_exit(0);
}

/* Simulate live video by reading and writing files and push it to TUYA SDK */
int TUYA_APP_Get_Video_Frame(unsigned char *pVideoBuf,unsigned int offset,unsigned int BufSize,unsigned int *IskeyFrame,unsigned int *FramLen,unsigned int *Frame_start)
{
    int pos = 0;
    int bNeedCal = 0;
    unsigned char NalType=0;
    int idx=0;
    if(BufSize<=5)
    {
        printf("bufSize is too small\n");
        return -1;
    }
    for(pos=0;pos <= BufSize-5;pos++)
    {
        if(pVideoBuf[pos]==0x00
            &&pVideoBuf[pos+1]==0x00
            &&pVideoBuf[pos+2]==0x00
            &&pVideoBuf[pos+3]==0x01)
        {
            NalType = pVideoBuf[pos + 4] & 0x1f;
            if(NalType == 0x7)
            {
                if(bNeedCal==1)
                {
                    *FramLen=pos-idx;
                    return 0;
                }
                
                *IskeyFrame = 1;
                *Frame_start = offset+pos;
                bNeedCal=1;
                idx=pos;
              //  PR_DEBUG("the nal type is sps pps SEI Frame");
            }
            /*
            else if(NalType == 0x8)
            {
               spslen = pos-frame_start;
               frame_start = pos;
               bNeedPPSLen = 1;
               PR_DEBUG("the nal type is pps");
            }
            else if(NalType == 0x5)
            {
               if(bNeedPPSLen)
               {
                  ppslen = pos-  frame_start;
                  bNeedPPSLen=0;
               }
               if(bNeedCal)
               {
                   *FramLen=spslen+ppslen+offset+pos-frame_start;
                   return 0;
               }
               frame_start = offset+pos;
               idx =pos;
               *IskeyFrame = 1;
               bNeedCal=1;
               PR_DEBUG("the nal type is i frame");
            }
            */
            else if(NalType ==0x1)                
            {
               if(bNeedCal)
               {
                  *FramLen=pos-idx;
                //  printf("len:%d %d %d %d\n",*FramLen,pos,*Frame_start,idx);
                  return 0;
               }
               *Frame_start=offset+pos;
               *IskeyFrame = 0;
               idx = pos;
               bNeedCal=1;
              // PR_DEBUG("the nal type is P frame frame_start:%d",frame_start);
            }
        }
    }
    
    return 0;
}


int read_one_frame_from_demo_video_file_hevc(unsigned char *pVideoBuf,unsigned int offset,unsigned int BufSize,unsigned int *IskeyFrame,unsigned int
*FramLen,unsigned int *Frame_start)
{
    int pos = 0;
    int bNeedCal = 0;
    unsigned char NalType=0;
    int idx=0;
    if(BufSize<=5)
    {
        printf("bufSize is too small\n");
        return -1;
    }
    for(pos=0;pos <= BufSize-5;pos++)
    {
        if(pVideoBuf[pos]==0x00
            &&pVideoBuf[pos+1]==0x00
            &&pVideoBuf[pos+2]==0x00
            &&pVideoBuf[pos+3]==0x01)
        {
            NalType = (pVideoBuf[pos + 4] & 0x7E) >> 1;
            if(NalType == 0x20 || NalType == 0x21 || NalType == 0x22 || NalType == 0x13)
            {
                if(bNeedCal==1)
                {
                    *FramLen=pos-idx;
                    return 0;
                }

                *IskeyFrame = 1;
                *Frame_start = offset+pos;
                bNeedCal=1;
                idx=pos;
            }
            else if(NalType ==0x1)
            {
               if(bNeedCal)
               {
                  *FramLen=pos-idx;
                  return 0;
               }
               *Frame_start=offset+pos;
               *IskeyFrame = 0;
               idx = pos;
               bNeedCal=1;
            }
            else
            {
                printf("chuishidebug %0#x\n", NalType);
            }
        }
    }

    *FramLen = BufSize;
    return 0;
}

void *thread_live_video(void *arg)
{
    prctl(PR_SET_NAME,"live_video");
    char raw_fullpath[128] = { 0 };
    char info_fullpath[128] = { 0 };
    unsigned int FrameLen = 0, Frame_start = 0;
    unsigned int offset = 0;
    unsigned int IsKeyFrame = 0;
    unsigned char *pVideoBuf = NULL;
    FAKE_VIDEO_INFO_T *pParam = (FAKE_VIDEO_INFO_T*) arg;
    sprintf(raw_fullpath, "%s/resource/media/demo_video.264", s_raw_path);

    PR_DEBUG("start live video using %s [%d]", raw_fullpath, pParam->chn);

    char raw_fullpath_hevc[128] = { 0 };
    unsigned int FrameLen_hevc = 0, Frame_start_hevc = 0;
    unsigned int offset_hevc = 0;
    unsigned int IsKeyFrame_hevc = 0;
    unsigned char *pVideoBuf_hevc = NULL;
    sprintf(raw_fullpath_hevc, "%s/resource/media/demo_video_265.hevc", s_raw_path);

    FILE *streamBin_fp = fopen(raw_fullpath, "rb");
    if ((streamBin_fp == NULL)) {
        PR_DEBUG("can't read live video files\n");
        pthread_exit(0);
    }
    fseek(streamBin_fp, 0, SEEK_END);
    UINT_T file_size = ftell(streamBin_fp);
    fseek(streamBin_fp, 0, SEEK_SET);
    pVideoBuf = malloc(file_size);
    fread(pVideoBuf, 1, file_size, streamBin_fp);

    FILE *streamBin_fp_hevc = fopen(raw_fullpath_hevc, "rb");
    if ((streamBin_fp_hevc == NULL)) {
        PR_DEBUG("can't read live video file %s\n", raw_fullpath);
        pthread_exit(0);
    }
    fseek(streamBin_fp_hevc, 0, SEEK_END);
    UINT_T file_size_hevc = ftell(streamBin_fp_hevc);
    fseek(streamBin_fp_hevc, 0, SEEK_SET);
    pVideoBuf_hevc = malloc(file_size_hevc);
    fread(pVideoBuf_hevc, 1, file_size_hevc, streamBin_fp_hevc);

    INT_T vChn = 0;
    tuya_xvr_dev_chan_get_by_devId(pParam->devId, (INT_T*) &vChn);
    main_v_handle[vChn] = tuya_ipc_ring_buffer_open(vChn, 0, E_IPC_STREAM_VIDEO_MAIN, E_RBUF_WRITE);
    if (main_v_handle[vChn] == NULL) {
        PR_DEBUG("can't read live video file %s\n", raw_fullpath);
        pthread_exit(0);
    }
    sub_v_handle[vChn] = tuya_ipc_ring_buffer_open(vChn, 0, E_IPC_STREAM_VIDEO_SUB, E_RBUF_WRITE);
    if (sub_v_handle[vChn] == NULL) {
        PR_DEBUG("can't read live video file %s\n", raw_fullpath);
        pthread_exit(0);
    }


    PR_DEBUG("really start live %s [%d],%d", raw_fullpath, pParam->chn,vChn);
    MEDIA_FRAME_S hevc_frame = { 0 };
    MEDIA_FRAME_S h264_frame = { 0 };
    while (1) {
        if(s_is_update[vChn] == TRUE) {
            sleep(1);
            continue;
        }
        if (s_is_new[vChn] == FALSE) {
            offset = Frame_start + FrameLen;
            if (offset >= file_size) {
                offset = 0;
            }
            TUYA_APP_Get_Video_Frame(pVideoBuf + offset, offset, file_size - offset, &IsKeyFrame, &FrameLen, &Frame_start);
            if (IsKeyFrame == 1) {
                h264_frame.type = E_VIDEO_I_FRAME;
                h264_frame.size = FrameLen;
            } else {
                h264_frame.type = E_VIDEO_PB_FRAME;
                h264_frame.size = FrameLen;
            }
            //  printf("FrameLen:%d 0x%x 0x%x 0x%x 0x%x\n",h264_frame.size,pVideoBuf[Frame_start],pVideoBuf[Frame_start+1],pVideoBuf[Frame_start+2],pVideoBuf[Frame_start+3]);
            h264_frame.p_buf = pVideoBuf + Frame_start;
           // PR_DEBUG(" alive test really start %d",vChn);
            int frameRate = 20;
            int sleepTick = 1000000 / frameRate;
            static UINT64_T pts = 0;
            pts += sleepTick;
            h264_frame.pts = pts;
            /* Send HD video data to the SDK */
            TUYA_APP_Put_Frame(main_v_handle[vChn], &h264_frame);
            /* Send SD video data to the SDK */
            TUYA_APP_Put_Frame(sub_v_handle[vChn], &h264_frame);

            //int sleepTick = 1000000/frameRate;
            usleep(sleepTick);
        } else if (s_is_new[vChn] == TRUE) {

            if(v_handle_hevc[vChn] == NULL) {
                v_handle_hevc[vChn]= tuya_ipc_ring_buffer_open(vChn, 0, E_IPC_STREAM_VIDEO_MAIN, E_RBUF_WRITE);
                if (v_handle_hevc[vChn] == NULL) {
                    PR_DEBUG("can't read live video file %s\n", raw_fullpath);
                    sleep(1);
                    continue;
                }
            }
                offset_hevc = Frame_start_hevc + FrameLen_hevc;
                if (offset_hevc >= file_size_hevc) {
                    offset_hevc = 0;
                }
                read_one_frame_from_demo_video_file_hevc(pVideoBuf_hevc + offset_hevc, offset_hevc, file_size_hevc - offset_hevc, &IsKeyFrame_hevc, &FrameLen_hevc, &Frame_start_hevc);
                //Note: For I frame of H264, SPS/PPS/SEI/IDR should be combined within one frame, and the NALU separator should NOT be deleted.
                if (IsKeyFrame_hevc == 1) {
                    hevc_frame.type = E_VIDEO_I_FRAME;
                    hevc_frame.size = FrameLen_hevc;
                } else {
                    hevc_frame.type = E_VIDEO_PB_FRAME;
                    hevc_frame.size = FrameLen_hevc;
                }

                int frameRate = 25;        //30;
                int sleepTick = 1000000 / frameRate;
                static UINT64_T pts = 0;
                pts += sleepTick;

                hevc_frame.p_buf = pVideoBuf_hevc + Frame_start_hevc;
                hevc_frame.pts = pts;
                /* Send HD video data to the SDK */
                TUYA_APP_Put_Frame(v_handle_hevc[vChn], &hevc_frame);
                /* Send SD video data to the SDK */
                // TUYA_APP_Put_Frame(E_IPC_STREAM_VIDEO_SUB, &hevc_frame);
                usleep(sleepTick);
        }
    }

    pthread_exit(0);
}



/*
---------------------------------------------------------------------------------
code related RingBuffer
---------------------------------------------------------------------------------
*/
void __TUYA_APP_force_key_frame(int chn)
{
    //UserTodo 
    PR_DEBUG("__TUYA_APP_force_key_frame ..");

    return ;
}

OPERATE_RET TUYA_APP_Init_Ring_Buffer_all()
{
    INT_T channel;

    OPERATE_RET ret = OPRT_OK;

    int i = 0;
    int bind_cnt = 0;
    int chanArr[SUB_DEV_MAX_NUM] = {0};
    if (0 == tuya_xvr_dev_chans_get(&bind_cnt, (INT_T*)&chanArr))
    {
        //step:循环初始化每一路设备的ringbuffer通道
        for (i = 0; i < bind_cnt; i++){
            TUYA_XVR_DEV_INFO_T sub_dev_info = {0};
            memset(&sub_dev_info, 0, sizeof(sub_dev_info));
            tuya_xvr_dev_info_get_by_chan(chanArr[i],&sub_dev_info);
            PR_DEBUG("sub dev chan[%d] devId[%s] vmChan[%d] ",sub_dev_info.chn, sub_dev_info.devId, sub_dev_info.vmChn);
            TUYA_APP_Init_Ring_Buffer(sub_dev_info.chn,(char *)sub_dev_info.devId);
        }
    }
    return ret;
}
#if 0
OPERATE_RET TUYA_APP_UnInit_Ring_Buffer_all()
{
    INT_T channel;
    OPERATE_RET ret = OPRT_OK;
    INT_T i = 0;
    int bind_cnt = 0;
    int chanArr[SUB_DEV_MAX_NUM] = {0};

    if (0 != tuya_xvr_dev_get_chans(&bind_cnt, &chanArr)){
        PR_ERR("tuya_xvr_dev_get_chans err");
        return -1;
    }
    for (i = 0; i < bind_cnt; i++){
        TUYA_XVR_DEV_INFO_T sub_dev_info = {0};
        memset(&sub_dev_info, 0, sizeof(sub_dev_info));
        tuya_xvr_dev_get_info_by_chan(chanArr[i],&sub_dev_info);
        PR_DEBUG("sub dev chan[%d] devId[%s] ",sub_dev_info.chn, sub_dev_info.devId);
        TUYA_APP_UnInit_Ring_Buffer((char*) sub_dev_info.devId);
    }
    return ret;
}
#endif


OPERATE_RET TUYA_APP_Put_Frame(Ring_Buffer_User_Handle_S handle, IN CONST MEDIA_FRAME_S *p_frame)
{
    PR_TRACE("Put Frame. type:%d size:%u pts:%llu ts:%llu",
             p_frame->type, p_frame->size, p_frame->pts, p_frame->timestamp);

    OPERATE_RET ret = tuya_ipc_ring_buffer_append_data(handle,p_frame->p_buf, p_frame->size,p_frame->type,p_frame->pts);

    if(ret != OPRT_OK)
    {
        PR_ERR("Put Frame Fail.%d  type:%d size:%u pts:%llu ts:%llu",ret,
                  p_frame->type, p_frame->size, p_frame->pts, p_frame->timestamp);
    }
    return ret;
}

/*
---------------------------------------------------------------------------------

---------------------------------------------------------------------------------
*/

int TUYA_APP_fake_video_start(char * devId)
{
    int i = 0;

    for (i = 0; i < DEMO_FAKE_VIDEO_MAX_NUM; i++){
        if (1 != s_fake_video[i].valid){
            break;
        }
    }
    if (i == DEMO_FAKE_VIDEO_MAX_NUM){
        printf("dev[%s] fake failed\n",devId);
        return -1;
    }
    s_fake_video[i].valid = 1;
     s_fake_video[i].chn = i;
    memset(s_fake_video[i].devId, 0x00,64);
    strncpy(s_fake_video[i].devId, devId, sizeof(s_fake_video[i].devId));
    pthread_create(&s_fake_video[i].h264_thread, NULL, thread_live_video, &s_fake_video[i]);
    pthread_detach(s_fake_video[i].h264_thread);
    
    pthread_create(&s_fake_video[i].audio_thread, NULL, thread_live_audio, &s_fake_video[i]);
    pthread_detach(s_fake_video[i].audio_thread);

    return 0;
}

int TUYA_APP_fake_video_stop(char * devId)
{
    int i = 0;

    for (i = 0; i < DEMO_NVR_SUB_DEV_NUM; i++){
        if (0 == strncmp(s_fake_video[i].devId, devId, strlen(devId))){
            break;
        }
    }
    
    if (i == DEMO_NVR_SUB_DEV_NUM){
        printf("dev[%s] fake stop failed\n",devId);
        return -1;
    }
    s_fake_video[i].valid = 0;
    memset(s_fake_video[i].devId, 0x00,64);
    pthread_join(s_fake_video[i].h264_thread, NULL);
    pthread_join(s_fake_video[i].audio_thread, NULL);

    return 0;
}

VOID TUYA_APP_Fake_Data_2_Ringbuffer()
{
    INT_T i = 0;
    int bind_cnt = 0;
    int chanArr[SUB_DEV_MAX_NUM] = {0};
    if (0 == tuya_xvr_dev_chans_get(&bind_cnt, (INT_T*)&chanArr))
    {
        for (i = 0; i < bind_cnt; i++){
            TUYA_XVR_DEV_INFO_T sub_dev_info = {0};
            memset(&sub_dev_info, 0, sizeof(sub_dev_info));
            tuya_xvr_dev_info_get_by_chan(chanArr[i],&sub_dev_info);
            PR_DEBUG("sub list devId[%s] ",sub_dev_info.devId);
            TUYA_APP_fake_video_start((char *)sub_dev_info.devId);
        }
    }
}

INT_T g_update_meida_flag = 0;
typedef OPERATE_RET (*XVR_MEDIA_UPDATE_CB)(INT_T device);
OPERATE_RET TUYA_Media_Info_Update_CB(INT_T device)
{
    //step 1:unint device ring buffer information
    s_is_update[device] = TRUE;
    TUYA_APP_Uninit_Ring_Buffer(device);

    CHAR_T devId[64];
    memset(&devId, 0, sizeof(devId));
    tuya_xvr_dev_devId_get_by_chan(device, devId, 64);
    //step 2:int device  new ring buffer information;
    TUYA_APP_Init_New_Ring_Buffer(device,(char *)devId);

    s_is_new[device] = TRUE;
    s_is_update[device] = FALSE;
    return 0;
}
VOID * TUYA_Media_Info_Update_Thread(VOID *args)
{
    while(1) {
        if(g_update_meida_flag == 0) {
            sleep(1);
        } else {
            printf("start meida update device 0\n");
            //通常调用此接口更新媒体信息是在是更换设备的情况下。demo更新通道0的设备
            tuya_xvr_media_update(0,TUYA_Media_Info_Update_CB);
            printf("end meida update device 0\n");
            g_update_meida_flag =0;
        }

    }
}

#ifdef __cplusplus
}
#endif

