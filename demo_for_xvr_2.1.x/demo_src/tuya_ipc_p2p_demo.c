/*********************************************************************************
  *Copyright(C),2015-2020, TUYA www.tuya.comm
  *FileName:    tuya_ipc_p2p_demo
**********************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_media.h"
#include "tuya_ipc_multi_storage.h"
#include "tuya_ipc_p2p.h"
#include "tuya_ipc_playback.h"
#include "tuya_ipc_media_demo.h"
#ifdef DEMO_NVR_ONVIF_ENABLE
#include "tuya_ipc_onvif_client_demo.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern IPC_MEDIA_INFO_S s_media_info;

typedef struct
{
    BOOL_T enabled;
    TRANSFER_VIDEO_CLARITY_TYPE_E live_clarity;
    UINT_T max_users;
    TUYA_CODEC_ID p2p_audio_codec;
}TUYA_APP_P2P_MGR;

STATIC TUYA_APP_P2P_MGR s_p2p_mgr = {0};

STATIC VOID __TUYA_APP_media_frame_TO_trans_video(IN CONST MEDIA_FRAME_S *p_in, INOUT TRANSFER_VIDEO_FRAME_S *p_out)
{
    p_out->video_codec = s_media_info.video_codec[E_IPC_STREAM_VIDEO_MAIN];
    p_out->video_frame_type = p_in->type == E_VIDEO_PB_FRAME ? TY_VIDEO_FRAME_PBFRAME:TY_VIDEO_FRAME_IFRAME;
    p_out->p_video_buf = p_in->p_buf;
    p_out->buf_len = p_in->size;
    p_out->pts = p_in->pts;
    p_out->timestamp = p_in->timestamp;
}

STATIC VOID __TUYA_APP_media_frame_TO_trans_audio(IN CONST MEDIA_FRAME_S *p_in, INOUT TRANSFER_AUDIO_FRAME_S *p_out)
{  
    p_out->audio_codec = s_media_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN];
    p_out->audio_sample = s_media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN];
    p_out->audio_databits = s_media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN];
    p_out->audio_channel = s_media_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN];
    p_out->p_audio_buf = p_in->p_buf;
    p_out->buf_len = p_in->size;
    p_out->pts = p_in->pts;
    p_out->timestamp = p_in->timestamp;
}

STATIC VOID __TUYA_APP_ss_pb_event_cb(IN CHAR_T *devId,IN UINT_T pb_idx, IN SS_PB_EVENT_E pb_event, IN PVOID_T args)
{
    PR_DEBUG("ss pb rev event: %u %d", pb_idx, pb_event);
    if(pb_event == SS_PB_FINISH)
    {
        tuya_xvr_playback_finish_status_send(devId,pb_idx);
    }
}

STATIC VOID __TUYA_APP_ss_pb_get_video_cb(IN CHAR_T *devId,IN UINT_T pb_idx, IN UINT_T map_index,IN CONST MEDIA_FRAME_S *p_frame)
{
    TRANSFER_VIDEO_FRAME_S video_frame = {0};
    __TUYA_APP_media_frame_TO_trans_video(p_frame, &video_frame);
    //tuya_ipc_playback_send_video_frame_with_channel(0 ,pb_idx, &video_frame);
    int ret = tuya_xvr_playback_video_frame_send(devId,pb_idx,&video_frame);

}


STATIC VOID __TUYA_APP_ss_pb_get_audio_cb(IN CHAR_T *devId, IN UINT_T pb_idx,IN UINT_T map_index, IN CONST MEDIA_FRAME_S *p_frame)
{
    TRANSFER_AUDIO_FRAME_S audio_frame = {0};
    __TUYA_APP_media_frame_TO_trans_audio(p_frame, &audio_frame);
    tuya_xvr_playback_audio_frame_send(devId,pb_idx,&audio_frame);
}

STATIC VOID __depereated_online_cb(IN TRANSFER_ONLINE_E status)
{

}

/* Callback functions for transporting events */
STATIC VOID __TUYA_APP_rev_audio_cb(IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame, IN CONST UINT_T frame_no)
{
    MEDIA_FRAME_S audio_frame = {0};
    audio_frame.p_buf = p_audio_frame->p_audio_buf;
    audio_frame.size = p_audio_frame->buf_len;

    PR_TRACE("Rev Audio. size:%u audio_codec:%d audio_sample:%d audio_databits:%d audio_channel:%d",p_audio_frame->buf_len,
             p_audio_frame->audio_codec, p_audio_frame->audio_sample, p_audio_frame->audio_databits, p_audio_frame->audio_channel);
    //PCM-Format 8K 16Bit MONO
    //TODO
    /* Developers need to implement the operations of voice playback*/

}

/*
 * 用于接收APP端的对讲音频数据。
 * */
void __TUYA_APP_speaker_audio_callback(IN CONST INT_T chn, IN CHAR_T * devId,IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame, IN CONST UINT_T frame_no)
{
    /* Developers need to implement the operations of voice playback*/
    PR_DEBUG("dev[%s] audio data recv\n",devId);
    return ;
}

void __TUYA_APP_online_cb(IN CONST INT_T chn, IN CONST CHAR_T * devId,IN TRANSFER_ONLINE_E status)
{
    //todo 
    return;
}

/*用于APP或者web预览时，回调相关的事件到应用层。事件的作用参考头文件和指导文档
    参数event:不同事件类型
    参数args:根据不同事件类型，具体内容不同。但通常都有区分是哪个设备的内容在对应结构体中，通过里面channel或者subid等来区分。
 */
STATIC INT_T __TUYA_APP_event_cb(IN CONST TRANSFER_EVENT_E event, IN CONST PVOID_T args)
{
    PR_DEBUG("p2p rev event cb=[%d] \n", event);

    switch (event) {
    case TRANS_LIVE_VIDEO_START_GW:
    {
        C2C_TRANS_CTRL_GW_VIDEO_START * parm = (C2C_TRANS_CTRL_GW_VIDEO_START *)args;
        PR_DEBUG("chn[%u] devId[%s] video start\n",parm->channel, parm->subdid);
        break;    
    }

    case TRANS_LIVE_VIDEO_STOP_GW:
    {
        C2C_TRANS_CTRL_GW_VIDEO_STOP * parm = (C2C_TRANS_CTRL_GW_VIDEO_STOP *)args;
        PR_DEBUG("chn[%u] devId[%s] video stop\n",parm->channel, parm->subdid);
        break;
    }
    case TRANS_LIVE_AUDIO_START_GW:
    {
        C2C_TRANS_CTRL_GW_AUDIO_START * parm = (C2C_TRANS_CTRL_GW_AUDIO_START *)args;
        PR_DEBUG("chn[%u] devId[%s] audio start\n",parm->channel, parm->subdid);
        break;
    }
    case TRANS_LIVE_AUDIO_STOP_GW:
    {
        C2C_TRANS_CTRL_GW_AUDIO_STOP * parm = (C2C_TRANS_CTRL_GW_AUDIO_STOP *)args;
        PR_DEBUG("chn[%u] devId[%s] audio stop\n",parm->channel, parm->subdid);
        break;
    }
    case TRANS_SPEAKER_START_GW:
    {
        C2C_TRANS_CTRL_GW_SPEAKER_START *param = (C2C_TRANS_CTRL_GW_SPEAKER_START *)args;
        PR_DEBUG("devId[%s]enbale audio speaker\n",param->subdid);
        break;
    }
    case TRANS_SPEAKER_STOP_GW:
    {
        //C2C_TRANS_CTRL_GW_SPEAKER_STOP *param = (C2C_TRANS_CTRL_GW_SPEAKER_STOP *)args;
        //PR_DEBUG("devId[%s]disable audio speaker\n",param->subdid);
        break;
    }
    case TRANS_LIVE_LOAD_ADJUST_GW:
    {
        break;
    }
	case TRANS_PLAYBACK_LOAD_ADJUST_GW:
    {
        break;
    }
    case TRANS_ABILITY_QUERY_GW:
    {
        C2C_TRANS_QUERY_GW_FIXED_ABI_REQ * pAbiReq;
        pAbiReq = (C2C_TRANS_QUERY_GW_FIXED_ABI_REQ *)args;
        pAbiReq->ability_mask = TY_CMD_QUERY_IPC_FIXED_ABILITY_TYPE_VIDEO |
                                TY_CMD_QUERY_IPC_FIXED_ABILITY_TYPE_SPEAKER |
                                TY_CMD_QUERY_IPC_FIXED_ABILITY_TYPE_MIC;          
        break;
    }

    case TRANS_PLAYBACK_QUERY_MONTH_SIMPLIFY_GW:
    {
        C2C_TRANS_QUERY_GW_PB_MONTH_REQ *p = (C2C_TRANS_QUERY_GW_PB_MONTH_REQ *)args;
        PR_DEBUG("dev[%s] pb query by month: %d-%d\n",p->subdid, p->year, p->month);
#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)
        OPERATE_RET ret = tuya_ipc_pb_query_by_month(p->subdid,p->channel,(USHORT_T)p->year, (USHORT_T)p->month, &(p->day));
        if (OPRT_OK != ret)
        {
            PR_ERR("pb query by month: %d-%d ret:%d\n", p->year, p->month, ret);
        }
#endif
        break;
    }
    case TRANS_PLAYBACK_QUERY_EVENT_DAY_TS_PAGE_MODE:
    {
        C2C_TRANS_QUERY_EVENT_PB_DAY_RESP *pquery = (C2C_TRANS_QUERY_EVENT_PB_DAY_RESP *)args;
        PR_DEBUG("pb_ts query by day page id :%d idx[%d]%d-%d-%d\n", pquery->page_id,pquery->channel,pquery->year, pquery->month, pquery->day);
#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)
        pquery->total_cnt = 20;
        pquery->page_size = 3;
        int static_page_size =3;
        int event_cnt = 3;
        int total_cnt = 20;
        int page_id = pquery->page_id;
        int page_size = pquery->page_size ;
        //UPDATE info;
        if(total_cnt - (page_id+1)*page_size < 0) {
            event_cnt = total_cnt - page_id*page_size;
            page_size = event_cnt;
        }
        printf("hmdg:event_cnt %d page size = %d\n",event_cnt,page_size);
        C2C_PB_EVENT_INFO_ARR_S * pResult = (C2C_PB_EVENT_INFO_ARR_S*)malloc(sizeof(C2C_PB_EVENT_INFO_ARR_S)+event_cnt *sizeof(C2C_PB_EVENT_INFO_S));
        pResult->version =1;
        // event_cnt = ((pquery->total_cnt - pquery->page_size *pquery->page_id) > pquery->page_size) ?pquery->page_size :(pquery->total_cnt - pquery->page_size *pquery->page_id);
        pResult->event_cnt = event_cnt;
        pquery->page_size = page_size;

        C2C_PB_EVENT_INFO_S * event_list = pResult->event_info_arr;
            INT_T i;
            SS_QUERY_DAY_TS_ARR_S *p_day_ts = NULL;
            int gap =10;
            OPERATE_RET ret = tuya_ipc_pb_query_by_day(pquery->subid,pquery->channel,pquery->year, pquery->month, pquery->day, &p_day_ts);
        for (i = 0; i <event_cnt; i++){
            pResult->event_info_arr[i].start_timestamp = p_day_ts->file_arr[0].start_timestamp  + gap*(pquery->page_id*static_page_size+i);
            pResult->event_info_arr[i].end_timestamp = p_day_ts->file_arr[0].end_timestamp +gap*(pquery->page_id*static_page_size+i);
            pResult->event_info_arr[i].type = i;
            memset(pResult->event_info_arr[i].pic_id,0x0,20);
        }
        pquery->event_arr=pResult;
#endif
        break;
    }
    case TRANS_PLAYBACK_QUERY_DAY_TS_PAGE_MODE:
    {

        C2C_TRANS_QUERY_PB_DAY_V2_RESP *pquery = (C2C_TRANS_QUERY_PB_DAY_V2_RESP *)args;
        PR_DEBUG("pb_ts query by day id:%d idx[%d]%d-%d-%d page_id =%d\n", pquery->page_id,pquery->channel,pquery->year, pquery->month, pquery->day,pquery->page_id);
        SS_QUERY_DAY_TS_ARR_S *p_day_ts = NULL;
#ifdef DEMO_USER_STOR
                //custom do it self
                OPERATE_RET ret;
#endif
#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)
        OPERATE_RET ret = tuya_ipc_pb_query_by_day(pquery->subid,pquery->channel,pquery->year, pquery->month, pquery->day, &p_day_ts);
        if (OPRT_OK != ret)
        {
            PR_ERR("pb_ts chn[%d] query by day: %d-%d-%d Fail\n", pquery->channel,pquery->year, pquery->month, pquery->day);
            break;
        }
        if (p_day_ts){
            pquery->total_cnt = p_day_ts->file_count;
            if( pquery->total_cnt > 2) {
                pquery->page_size =2;
            }
            else {
                pquery->page_size = 1;
            }
            int static_page_size =2;
            int total_cnt = pquery->total_cnt;
            int page_id = pquery->page_id;
            int page_size = pquery->page_size ;
            //UPDATE info;
            if(total_cnt - (page_id+1)*page_size < 0) {
                page_size = total_cnt - page_id*page_size;
            }
            pquery->page_size = page_size ;
            printf("%s %d count = %d page size= %d\n",__FUNCTION__,__LINE__,p_day_ts->file_count,page_size);
            PLAY_BACK_ALARM_INFO_ARR * pResult = (PLAY_BACK_ALARM_INFO_ARR *)malloc(sizeof(PLAY_BACK_ALARM_INFO_ARR) + pquery->page_size*sizeof(PLAY_BACK_ALARM_FRAGMENT));
            if (NULL == pResult){
                printf("%s %d malloc failed \n",__FUNCTION__,__LINE__);
                free(p_day_ts);
                pquery->alarm_arr = NULL;
                return OPRT_NOT_FOUND;
            }
            pResult->file_count = pquery->page_size;
            INT_T i;

            for (i = 0; i < pquery->page_size; i++){
                pResult->file_arr[i].time_sect.start_timestamp = p_day_ts->file_arr[page_id*static_page_size+i].start_timestamp;
                pResult->file_arr[i].time_sect.end_timestamp = p_day_ts->file_arr[page_id*static_page_size +i].end_timestamp;
            }
            pquery->alarm_arr = pResult;


            free(p_day_ts);
        }else{
           pquery->alarm_arr = NULL;
        }
#endif
        break;


    }
    case TRANS_PLAYBACK_QUERY_DAY_TS_GW:

    {
        C2C_TRANS_QUERY_GW_PB_DAY_RESP *pquery = (C2C_TRANS_QUERY_GW_PB_DAY_RESP *)args;
        PR_DEBUG("pb_ts query by day: idx[%d]%d-%d-%d\n", pquery->channel,pquery->year, pquery->month, pquery->day);
        SS_QUERY_DAY_TS_ARR_S *p_day_ts = NULL;
#ifdef DEMO_USER_TO_DO
        //开发者 自定开发这块，主要是保存一些信息，以便有回放的其他事件下来，可以做对应的送流处理
#endif
#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)

        OPERATE_RET ret = tuya_ipc_pb_query_by_day(pquery->subdid,pquery->idx,pquery->year, pquery->month, pquery->day, &p_day_ts);
        if (OPRT_OK != ret)
        {
            PR_ERR("pb_ts chn[%d] query by day: %d-%d-%d Fail\n", pquery->idx,pquery->year, pquery->month, pquery->day);
            break;
        }
        if (p_day_ts){
            printf("%s %d count = %d\n",__FUNCTION__,__LINE__,p_day_ts->file_count);
            PLAY_BACK_ALARM_INFO_ARR * pResult = (PLAY_BACK_ALARM_INFO_ARR *)malloc(sizeof(PLAY_BACK_ALARM_INFO_ARR) + p_day_ts->file_count*sizeof(PLAY_BACK_ALARM_FRAGMENT));
            if (NULL == pResult){
                printf("%s %d malloc failed \n",__FUNCTION__,__LINE__);
                free(p_day_ts);
                pquery->alarm_arr = NULL;
                return OPRT_NOT_FOUND;
            }

            INT_T i;
            pResult->file_count = p_day_ts->file_count;
            for (i = 0; i < p_day_ts->file_count; i++){
                pResult->file_arr[i].time_sect.start_timestamp = p_day_ts->file_arr[i].start_timestamp;
                pResult->file_arr[i].time_sect.end_timestamp = p_day_ts->file_arr[i].end_timestamp;
            }
            pquery->alarm_arr = pResult;
            free(p_day_ts);
        }else{
           pquery->alarm_arr = NULL; 
        }
#endif
        break;
    }
    case TRANS_PLAYBACK_START_TS_GW:
    {
        /* 开始回放时client会带上开始时间点，这里简单起见，只进行了日志打印 */
        C2C_TRANS_CTRL_GW_PB_START *pParam = (C2C_TRANS_CTRL_GW_PB_START *)args;
        PR_DEBUG("PB StartTS idx:%d %u [%u %u]\n", pParam->channel, pParam->playTime, pParam->time_sect.start_timestamp, pParam->time_sect.end_timestamp);

        SS_FILE_TIME_TS_S pb_file_info;
        int ret;
        memset(&pb_file_info, 0x00, sizeof(SS_FILE_TIME_TS_S));
        pb_file_info.start_timestamp = pParam->time_sect.start_timestamp;
        pb_file_info.end_timestamp = pParam->time_sect.end_timestamp;
#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)

        ret = tuya_ipc_pb_start(pParam->subdid, pParam->idx,0, __TUYA_APP_ss_pb_event_cb, __TUYA_APP_ss_pb_get_video_cb, __TUYA_APP_ss_pb_get_audio_cb);

        if (0 != ret){
            printf("%s %d pb_start failed\n",__FUNCTION__,__LINE__);
           // tuya_ipc_playback_send_finish_with_channel(pParam->subdid,pParam->channel);//zxq
        }else{
            if (0 != tuya_ipc_pb_seek(pParam->subdid, pParam->idx, &pb_file_info, pParam->playTime)){
                printf("%s %d pb_seek failed\n",__FUNCTION__,__LINE__);

              tuya_xvr_playback_finish_status_send(pParam->subdid,pParam->idx);
            }
        }
#endif
        break;
    }
    case TRANS_PLAYBACK_PAUSE_GW:
    {
        C2C_TRANS_CTRL_GW_PB_PAUSE *pParam = (C2C_TRANS_CTRL_GW_PB_PAUSE *)args;
        PR_DEBUG("PB Pause idx:%d\n", pParam->channel);
#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)
        tuya_ipc_ss_pb_set_status(pParam->subdid,pParam->idx, SS_PB_PAUSE);
#endif
        break;
    }
    case TRANS_PLAYBACK_RESUME_GW:
    {
        C2C_TRANS_CTRL_GW_PB_RESUME *pParam = (C2C_TRANS_CTRL_GW_PB_RESUME *)args;
        PR_DEBUG("PB Resume idx:%d\n", pParam->channel);
#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)
        tuya_ipc_ss_pb_set_status(pParam->subdid, pParam->channel, SS_PB_RESUME);
#endif
        break;
    }
    case TRANS_PLAYBACK_MUTE_GW:
    {
        C2C_TRANS_CTRL_GW_PB_MUTE *pParam = (C2C_TRANS_CTRL_GW_PB_MUTE *)args;
        PR_DEBUG("PB idx:%d mute\n", pParam->channel);
#ifdef DEMO_USER_STOR
                        //custom do it self
#else
        tuya_ipc_ss_pb_set_status(pParam->subdid,pParam->idx, SS_PB_MUTE);
#endif
        break;
    }
    case TRANS_PLAYBACK_UNMUTE_GW:
    {
        C2C_TRANS_CTRL_GW_PB_UNMUTE *pParam = (C2C_TRANS_CTRL_GW_PB_UNMUTE *)args;
        PR_DEBUG("devId[%s]PB idx:%d unmute\n",pParam->subdid, pParam->idx);
#ifdef DEMO_USER_STOR
                        //custom do it self
#endif
#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)
        tuya_ipc_ss_pb_set_status(pParam->subdid, pParam->idx, SS_PB_UN_MUTE);
#endif
        break;
    }
    case TRANS_PLAYBACK_STOP_GW:
    {
        C2C_TRANS_CTRL_GW_PB_STOP *pParam = (C2C_TRANS_CTRL_GW_PB_STOP *)args;
        PR_DEBUG("devId [%s]PB Stop idx:%d\n", pParam->subdid,pParam->idx);
#ifdef DEMO_USER_STOR
                        //custom do it self
#endif
#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)
        tuya_ipc_pb_stop((CHAR_T *)pParam->subdid, pParam->idx);
#endif
        break;
    }
    case TRANS_DOWNLOAD_START_GW:
        {
#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)
            C2C_TRANS_CTRL_GW_DL_START *pParam = (C2C_TRANS_CTRL_GW_DL_START *)args;
            SS_FILE_TIME_TS_S strParm;
            strParm.start_timestamp = pParam->downloadStartTime;
            strParm.end_timestamp = pParam->downloadEndTime;

            static pthread_mutex_t g_donwload_lock = PTHREAD_MUTEX_INITIALIZER;
            pthread_mutex_lock(&g_donwload_lock);
            if (OPRT_OK == tuya_ipc_ss_donwload_pre(pParam->devid, pParam->channel, &strParm)){
                tuya_ipc_ss_download_set_status(pParam->devid, pParam->channel, SS_DL_START);
            }
            else{
                TYDEBUG("tuya_ipc_ss_donwload_pre err \n");
            }
            pthread_mutex_unlock(&g_donwload_lock);
#endif
            break;
        }

    case TRANS_LIVE_VIDEO_CLARITY_SET_GW:
    {
        C2C_TRANS_LIVE_GW_CLARITY_PARAM_S *pParam = (C2C_TRANS_LIVE_GW_CLARITY_PARAM_S *)args;
        PR_DEBUG("set chn[%d] clarity:%d\n",pParam->channel, pParam->clarity);
        if((pParam->clarity == TY_VIDEO_CLARITY_STANDARD)||(pParam->clarity == TY_VIDEO_CLARITY_HIGH))
        {
            PR_DEBUG("set clarity:%d OK\n", pParam->clarity);
        }
        break;
    }
    case TRANS_LIVE_VIDEO_CLARITY_QUERY_GW:
    {
        C2C_TRANS_LIVE_GW_CLARITY_PARAM_S *pParam = (C2C_TRANS_LIVE_GW_CLARITY_PARAM_S *)args;
        PR_DEBUG("query larity:%d\n", pParam->clarity);
        break;
    }
    default:
        break;
    }
    return TRANS_EVENT_SUCCESS;
}
/*
 * __TUYA_APP_get_media_info_cb :用于在预览或者回放是，获取设备的媒体参数信息。
 * 参数chn:设备索引
 * devId:设备id
 * */
void __TUYA_APP_get_media_info_cb(IN CONST INT_T chn, IN CONST CHAR_T * devId,IN OUT TRANSFER_MEDIA_INFO_S *param)
{
    //step:用户根据chn或者devId找到对应的设备媒体信息，填写到param中。
    //demo 所展示设备的信息是一样的。故没有区分设备
    memset(param, 0x00 ,sizeof(TRANSFER_MEDIA_INFO_S));
    //step：获取预览的媒体信息
    IPC_APP_Get_Media_Info(chn,devId,&param->strMedia);

    //step:获取对讲的音频信息
    param->rev_audio_codec = param->strMedia.audio_codec[E_IPC_STREAM_AUDIO_MAIN];
    param->audio_channel = param->strMedia.audio_channel[E_IPC_STREAM_AUDIO_MAIN];
    param->audio_databits = param->strMedia.audio_databits[E_IPC_STREAM_AUDIO_MAIN];
    param->audio_sample = param->strMedia.audio_sample[E_IPC_STREAM_AUDIO_MAIN];
    return ;
}

#ifdef DEMO_NVR_ONVIF_ENABLE
void __TUYA_APP_onvif_get_media_info_cb(IN CONST INT_T chn, IN CONST CHAR_T * devId,IN OUT TRANSFER_MEDIA_INFO_S *param)
{
    memset(param, 0x00, sizeof(TRANSFER_MEDIA_INFO_S));
    IPC_MEDIA_INFO_S media_info;
    memset(&media_info, 0x00, sizeof(IPC_MEDIA_INFO_S));
    TUYA_APP_Onvif_Get_Media_Info(devId, &media_info);

    INT_T i = 0;
    for (i = 0; i < E_IPC_STREAM_VIDEO_MAX; i++)
    {
        if (media_info.channel_enable[i])
        {
            param->strMedia.channel_enable[i] = TRUE;
            param->strMedia.video_fps[i] = media_info.video_fps[i];
            param->strMedia.video_gop[i] = media_info.video_gop[i];
            param->strMedia.video_bitrate[i] = media_info.video_bitrate[i];
            param->strMedia.video_width[i] = media_info.video_width[i];
            param->strMedia.video_height[i] = media_info.video_height[i];
            param->strMedia.video_codec[i] = media_info.video_codec[i];
        }
    }

    for (i = E_IPC_STREAM_AUDIO_MAIN; i < E_IPC_STREAM_MAX; i++)
    {
        if (media_info.channel_enable[i])
        {
            param->strMedia.channel_enable[i] = TRUE;
            param->strMedia.audio_codec[i] = media_info.audio_codec[i];
            param->strMedia.audio_sample[i] = media_info.audio_sample[i];
            param->strMedia.audio_databits[i] = media_info.audio_databits[i];
            param->strMedia.audio_channel[i] = media_info.audio_channel[i];
            param->strMedia.audio_fps[i] = media_info.audio_fps[i];
        }
    }

    //rev audio info 
    param->rev_audio_codec = param->strMedia.audio_codec[E_IPC_STREAM_AUDIO_MAIN];
    param->audio_channel = param->strMedia.audio_channel[E_IPC_STREAM_AUDIO_MAIN];
    param->audio_databits = param->strMedia.audio_databits[E_IPC_STREAM_AUDIO_MAIN];
    param->audio_sample = param->strMedia.audio_sample[E_IPC_STREAM_AUDIO_MAIN];
}
#endif

OPERATE_RET TUYA_APP_Enable_P2PTransfer(IN UINT_T max_users)
{
    if(s_p2p_mgr.enabled == TRUE)
    {
        PR_DEBUG("P2P Is Already Inited");
        return OPRT_OK;
    }
    PR_DEBUG("Init P2P With Max Users:%u", max_users);

    TUYA_IPC_TRANSFER_VAR_S p2p_var = {0};
#ifdef DEMO_NVR_ONVIF_ENABLE
    p2p_var.on_get_dev_media_cb = __TUYA_APP_onvif_get_media_info_cb;
#else
    p2p_var.on_get_dev_media_cb = __TUYA_APP_get_media_info_cb;
#endif
    p2p_var.on_event_cb = __TUYA_APP_event_cb;//用于APP或者web预览时，回调相关的事件到应用层。具体实现参见函数
    p2p_var.on_gw_rev_audio_cb = __TUYA_APP_speaker_audio_callback;
    p2p_var.live_quality = TRANS_LIVE_QUALITY_MAX;
    p2p_var.max_client_num = max_users;//这个参数表示最多可以預覽多少個視頻
    p2p_var.defLiveMode = TRANS_DEFAULT_HIGH;

    OPERATE_RET ret = tuya_ipc_tranfser_init(&p2p_var);
    if(ret != OPRT_OK) {
        PR_DEBUG("p2p init is error %d",ret);
        return ret;
    }

    s_p2p_mgr.enabled = TRUE;
    return ret;
}

#ifdef __cplusplus
}
#endif
