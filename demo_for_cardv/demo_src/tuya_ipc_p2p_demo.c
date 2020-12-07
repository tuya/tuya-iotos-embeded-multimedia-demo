/*********************************************************************************
  *Copyright(C),2015-2020, TUYA www.tuya.comm
  *FileName:    tuya_ipc_p2p_demo
**********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_media.h"
#include "tuya_ipc_p2p.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_media_demo.h"
extern IPC_MEDIA_INFO_S s_media_info[IPC_CHAN_NUM];

typedef struct
{
    BOOL_T enabled;
    TRANSFER_VIDEO_CLARITY_TYPE_E live_clarity;
    UINT_T max_users;
    TUYA_CODEC_ID p2p_audio_codec;
} TUYA_APP_P2P_MGR;

STATIC TUYA_APP_P2P_MGR s_p2p_mgr = { 0 };

STATIC VOID __TUYA_APP_media_frame_TO_trans_video(IN CONST MEDIA_FRAME_S* p_in, INOUT TRANSFER_VIDEO_FRAME_S* p_out)
{
    UINT_T codec_type = 0;
    codec_type = (p_in->type & 0xff00) >> 8;
    p_out->video_codec = (codec_type == 0 ? TUYA_CODEC_VIDEO_H264 : TUYA_CODEC_VIDEO_H265);
    p_out->video_frame_type = (p_in->type && 0xff) == E_VIDEO_PB_FRAME ? TY_VIDEO_FRAME_PBFRAME : TY_VIDEO_FRAME_IFRAME;
    p_out->p_video_buf = p_in->p_buf;
    p_out->buf_len = p_in->size;
    p_out->pts = p_in->pts;
    p_out->timestamp = p_in->timestamp;
}

STATIC VOID __TUYA_APP_media_frame_TO_trans_audio(IN CONST MEDIA_FRAME_S* p_in, INOUT TRANSFER_AUDIO_FRAME_S* p_out)
{
    p_out->audio_codec = s_media_info[0].audio_codec[E_CHANNEL_AUDIO]; // notice audio para must be the same
    p_out->audio_sample = s_media_info[0].audio_sample[E_CHANNEL_AUDIO];
    p_out->audio_databits = s_media_info[0].audio_databits[E_CHANNEL_AUDIO];
    p_out->audio_channel = s_media_info[0].audio_channel[E_CHANNEL_AUDIO];
    p_out->p_audio_buf = p_in->p_buf;
    p_out->buf_len = p_in->size;
    p_out->pts = p_in->pts;
    p_out->timestamp = p_in->timestamp;
}

STATIC VOID __TUYA_APP_ss_pb_event_cb(IN UINT_T pb_idx, IN SS_PB_EVENT_E pb_event, IN PVOID_T args)
{
    PR_DEBUG("ss pb rev event: %u %d", pb_idx, pb_event);
    if (pb_event == SS_PB_FINISH) {
        tuya_ipc_playback_send_finish(pb_idx);
    }
}

STATIC VOID __TUYA_APP_ss_pb_get_video_cb(IN UINT_T pb_idx, IN CONST MEDIA_FRAME_S* p_frame)
{
    TRANSFER_VIDEO_FRAME_S video_frame = { 0 };
    __TUYA_APP_media_frame_TO_trans_video(p_frame, &video_frame);
    tuya_ipc_playback_send_video_frame(pb_idx, &video_frame);
}

STATIC VOID __TUYA_APP_ss_pb_get_audio_cb(IN UINT_T pb_idx, IN CONST MEDIA_FRAME_S* p_frame)
{
    TRANSFER_AUDIO_FRAME_S audio_frame = { 0 };
    __TUYA_APP_media_frame_TO_trans_audio(p_frame, &audio_frame);
    tuya_ipc_playback_send_audio_frame(pb_idx, &audio_frame);
}

STATIC VOID __depereated_online_cb(IN TRANSFER_ONLINE_E status)
{
}

/* Callback functions for transporting events */
STATIC VOID __TUYA_APP_p2p_event_cb(IN CONST TRANSFER_EVENT_E event, IN CONST PVOID_T args)
{
    PR_DEBUG("p2p rev event cb=[%d] ", event);
    switch (event) {
    case TRANS_LIVE_VIDEO_START: {
        C2C_TRANS_CTRL_VIDEO_START* parm = (C2C_TRANS_CTRL_VIDEO_START*)args;
        //   PR_DEBUG("chn[%u] video start type:%d",parm->channel,parm->type);
        break;
    }
    case TRANS_LIVE_VIDEO_STOP: {
        C2C_TRANS_CTRL_VIDEO_STOP* parm = (C2C_TRANS_CTRL_VIDEO_STOP*)args;
        //  PR_DEBUG("chn[%u] video stop type:%d",parm->channel,parm->type);
        break;
    }
    case TRANS_LIVE_AUDIO_START: {
        C2C_TRANS_CTRL_AUDIO_START* parm = (C2C_TRANS_CTRL_AUDIO_START*)args;
        PR_DEBUG("chn[%u] audio start", parm->channel);
        break;
    }
    case TRANS_LIVE_AUDIO_STOP: {
        C2C_TRANS_CTRL_AUDIO_STOP* parm = (C2C_TRANS_CTRL_AUDIO_STOP*)args;
        PR_DEBUG("chn[%u] audio stop", parm->channel);
        break;
    }
    case TRANS_SPEAKER_START: {
        PR_DEBUG("enbale audio speaker");
        TUYA_APP_Enable_Speaker_CB(TRUE);
        break;
    }
    case TRANS_SPEAKER_STOP: {
        PR_DEBUG("disable audio speaker");
        TUYA_APP_Enable_Speaker_CB(FALSE);
        break;
    }
    case TRANS_LIVE_LOAD_ADJUST: {
        C2C_TRANS_LIVE_LOAD_PARAM_S* quality = (C2C_TRANS_LIVE_LOAD_PARAM_S*)args;
        PR_DEBUG("live quality %d -> %d", quality->curr_load_level, quality->new_load_level);
        break;
    }
    case TRANS_PLAYBACK_LOAD_ADJUST: {
        C2C_TRANS_PB_LOAD_PARAM_S* quality = (C2C_TRANS_PB_LOAD_PARAM_S*)args;
        PR_DEBUG("pb idx:%d quality %d -> %d", quality->client_index, quality->curr_load_level, quality->new_load_level);
        break;
    }
    case TRANS_ABILITY_QUERY: {
        C2C_TRANS_QUERY_FIXED_ABI_REQ* pAbiReq;
        pAbiReq = (C2C_TRANS_QUERY_FIXED_ABI_REQ*)args;
        pAbiReq->ability_mask = TY_CMD_QUERY_IPC_FIXED_ABILITY_TYPE_VIDEO | TY_CMD_QUERY_IPC_FIXED_ABILITY_TYPE_SPEAKER | TY_CMD_QUERY_IPC_FIXED_ABILITY_TYPE_MIC;
        break;
    }
    case TRANS_PLAYBACK_QUERY_MONTH_SIMPLIFY: {
        C2C_TRANS_QUERY_PB_MONTH_REQ* p = (C2C_TRANS_QUERY_PB_MONTH_REQ*)args;
        PR_DEBUG("pb query by month: %d-%d", p->year, p->month);

        OPERATE_RET ret = tuya_ipc_pb_query_by_month_chan(p->ipcChan, p->year, p->month, &(p->day));
        if (OPRT_OK != ret) {
            PR_ERR("pb query by month: %d-%d ret:%d", p->year, p->month, ret);
        }

        break;
    }
    case TRANS_PLAYBACK_QUERY_DAY_TS: {
        C2C_TRANS_QUERY_PB_DAY_RESP* pquery = (C2C_TRANS_QUERY_PB_DAY_RESP*)args;
        PR_DEBUG("pb_ts query by day: idx[%d] chan[%d]%d-%d-%d", pquery->channel, pquery->ipcChan, pquery->year, pquery->month, pquery->day);
        SS_QUERY_DAY_TS_ARR_S* p_day_ts = NULL;
        OPERATE_RET ret = tuya_ipc_pb_query_by_day_chan(pquery->channel, pquery->ipcChan, pquery->year, pquery->month, pquery->day, &p_day_ts);
        if (OPRT_OK != ret) {
            PR_ERR("pb_ts query by day: chan[%d] %d-%d-%d Fail", pquery->channel, pquery->year, pquery->month, pquery->day);
            break;
        }
        if (p_day_ts) {
            printf("%s %d count = %d\n", __FUNCTION__, __LINE__, p_day_ts->file_count);
            PLAY_BACK_ALARM_INFO_ARR* pResult = (PLAY_BACK_ALARM_INFO_ARR*)malloc(sizeof(PLAY_BACK_ALARM_INFO_ARR) + p_day_ts->file_count * sizeof(PLAY_BACK_ALARM_FRAGMENT));
            if (NULL == pResult) {
                printf("%s %d malloc failed \n", __FUNCTION__, __LINE__);
                free(p_day_ts);
                pquery->alarm_arr = NULL;
                return;
            }

            INT_T i;
            pResult->file_count = p_day_ts->file_count;
            for (i = 0; i < p_day_ts->file_count; i++) {
                pResult->file_arr[i].type = p_day_ts->file_arr[i].type;
                pResult->file_arr[i].time_sect.start_timestamp = p_day_ts->file_arr[i].start_timestamp;
                pResult->file_arr[i].time_sect.end_timestamp = p_day_ts->file_arr[i].end_timestamp;
            }
            pquery->alarm_arr = pResult;
            free(p_day_ts);

        } else {
            pquery->alarm_arr = NULL;
        }
        break;
    }
    case TRANS_PLAYBACK_START_TS: {
        /* Client will bring the start time when playback.
            For the sake of simplicity, only log printing is done. */
        C2C_TRANS_CTRL_PB_START* pParam = (C2C_TRANS_CTRL_PB_START*)args;
        PR_DEBUG("PB StartTS idx:%d %u [%u %u]", pParam->channel, pParam->playTime, pParam->time_sect.start_timestamp, pParam->time_sect.end_timestamp);

        SS_FILE_TIME_TS_S pb_file_info;
        int ret;
        memset(&pb_file_info, 0x00, sizeof(SS_FILE_TIME_TS_S));
        //memcpy(&pb_file_info, &pParam->time_sect, sizeof(SS_FILE_TIME_TS_S));
        pb_file_info.start_timestamp = pParam->time_sect.start_timestamp;
        pb_file_info.end_timestamp = pParam->time_sect.end_timestamp;
        ret = tuya_ipc_ss_pb_start(pParam->channel, __TUYA_APP_ss_pb_event_cb, __TUYA_APP_ss_pb_get_video_cb, __TUYA_APP_ss_pb_get_audio_cb);
        if (0 != ret) {
            printf("%s %d pb_start failed\n", __FUNCTION__, __LINE__);
            tuya_ipc_playback_send_finish(pParam->channel);
        } else {
            if (0 != tuya_ipc_ss_pb_seek(pParam->channel, &pb_file_info, pParam->playTime)) {
                printf("%s %d pb_seek failed\n", __FUNCTION__, __LINE__);
                tuya_ipc_playback_send_finish(pParam->channel);
            }
        }

        break;
    }
    case TRANS_PLAYBACK_PAUSE: {
        C2C_TRANS_CTRL_PB_PAUSE* pParam = (C2C_TRANS_CTRL_PB_PAUSE*)args;
        PR_DEBUG("PB Pause idx:%d", pParam->channel);

        tuya_ipc_ss_pb_set_status(pParam->channel, SS_PB_PAUSE);
        break;
    }
    case TRANS_PLAYBACK_RESUME: {
        C2C_TRANS_CTRL_PB_RESUME* pParam = (C2C_TRANS_CTRL_PB_RESUME*)args;
        PR_DEBUG("PB Resume idx:%d", pParam->channel);

        tuya_ipc_ss_pb_set_status(pParam->channel, SS_PB_RESUME);
        break;
    }
    case TRANS_PLAYBACK_MUTE: {
        C2C_TRANS_CTRL_PB_MUTE* pParam = (C2C_TRANS_CTRL_PB_MUTE*)args;
        PR_DEBUG("PB idx:%d mute", pParam->channel);

        tuya_ipc_ss_pb_set_status(pParam->channel, SS_PB_MUTE);
        break;
    }
    case TRANS_PLAYBACK_UNMUTE: {
        C2C_TRANS_CTRL_PB_UNMUTE* pParam = (C2C_TRANS_CTRL_PB_UNMUTE*)args;
        PR_DEBUG("PB idx:%d unmute", pParam->channel);

        tuya_ipc_ss_pb_set_status(pParam->channel, SS_PB_UN_MUTE);
        break;
    }
    case TRANS_PLAYBACK_STOP: {
        C2C_TRANS_CTRL_PB_STOP* pParam = (C2C_TRANS_CTRL_PB_STOP*)args;
        PR_DEBUG("PB Stop idx:%d", pParam->channel);

        tuya_ipc_ss_pb_stop(pParam->channel);
        break;
    }
    case TRANS_LIVE_VIDEO_CLARITY_SET: {
        C2C_TRANS_LIVE_CLARITY_PARAM_S* pParam = (C2C_TRANS_LIVE_CLARITY_PARAM_S*)args;
        PR_DEBUG("set clarity:%d", pParam->clarity);
        if ((pParam->clarity == TY_VIDEO_CLARITY_STANDARD) || (pParam->clarity == TY_VIDEO_CLARITY_HIGH)) {
            PR_DEBUG("set clarity:%d OK", pParam->clarity);
            s_p2p_mgr.live_clarity = pParam->clarity;
        }
        break;
    }
    case TRANS_LIVE_VIDEO_CLARITY_QUERY: {
        C2C_TRANS_LIVE_CLARITY_PARAM_S* pParam = (C2C_TRANS_LIVE_CLARITY_PARAM_S*)args;
        pParam->clarity = s_p2p_mgr.live_clarity;
        PR_DEBUG("query larity:%d", pParam->clarity);
        break;
    }
#define SUPPORT_VIDOE_DOWNLOAD
#ifdef SUPPORT_VIDOE_DOWNLOAD
    case TRANS_DOWNLOAD_START: {
        C2C_TRANS_CTRL_DL_START* pParam = (C2C_TRANS_CTRL_DL_START*)args;
        SS_DOWNLOAD_FILES_TS_S strParm;
        strParm.file_count = pParam->fileNum;
        strParm.dl_start_time = pParam->downloadStartTime;
        strParm.dl_end_time = pParam->downloadEndTime;
        strParm.pFileInfoArr = (SS_FILE_INFO_S*)(pParam->pFileInfo);
        if (OPRT_OK == tuya_ipc_ss_donwload_pre(pParam->channel, &strParm)) {
            tuya_ipc_ss_download_set_status(pParam->channel, SS_DL_START);
        }
        break;
    }
    case TRANS_DOWNLOAD_STOP: {
        C2C_TRANS_CTRL_DL_STOP* pParam = (C2C_TRANS_CTRL_DL_STOP*)args;
        tuya_ipc_ss_download_set_status(pParam->channel, SS_DL_STOP);
        break;
    }
    case TRANS_DOWNLOAD_PAUSE: {
        C2C_TRANS_CTRL_DL_PAUSE* pParam = (C2C_TRANS_CTRL_DL_PAUSE*)args;
        tuya_ipc_ss_download_set_status(pParam->channel, SS_DL_PAUSE);
        break;
    }
    case TRANS_DOWNLOAD_RESUME: {
        C2C_TRANS_CTRL_DL_RESUME* pParam = (C2C_TRANS_CTRL_DL_RESUME*)args;
        tuya_ipc_ss_download_set_status(pParam->channel, SS_DL_RESUME);
        break;
    }
    case TRANS_DOWNLOAD_CANCLE: {
        C2C_TRANS_CTRL_DL_CANCLE* pParam = (C2C_TRANS_CTRL_DL_CANCLE*)args;
        tuya_ipc_ss_download_set_status(pParam->channel, SS_DL_CANCLE);
        break;
    }
    case TRANS_DOWNLOAD_IMAGE: {
        C2C_TRANS_CTRL_PB_DOWNLOAD_IMAGE_PARAM_S *pParam = (C2C_TRANS_CTRL_PB_DOWNLOAD_IMAGE_PARAM_S *)args;
        SS_FILE_INFO_S strParm;
        memset(&strParm, 0, sizeof(strParm));
        strParm.start_timestamp = pParam->time_sect.start_timestamp;
        strParm.end_timestamp = pParam->time_sect.end_timestamp;
        PR_DEBUG("TRANS_DOWNLOAD_IMAGE\n");
        if (OPRT_OK != tuya_ipc_download_image(pParam->channel, strParm, &(pParam->image_fileLength), (char**)&(pParam->pBuffer))) {
            PR_DEBUG("tuya_ipc_download_image err \n");
        }
        break;
    }
    case TRANS_PLAYBACK_DELETE: {
        C2C_TRANS_CTRL_PB_DELDATA_BYDAY_REQ *pParam = (C2C_TRANS_CTRL_PB_DELDATA_BYDAY_REQ *)args;
        if (OPRT_OK != tuya_ipc_ss_delete_video(pParam->channel, pParam->year, pParam->month, pParam->day)) {
            PR_DEBUG("tuya_ipc_ss_delete_video err\n");
        }
        break;
    }
#endif

    case TRANS_STREAMING_VIDEO_START: {
        TRANSFER_SOURCE_TYPE_E* pSrcType = (TRANSFER_SOURCE_TYPE_E*)args;
        PR_DEBUG("streaming start type %d", *pSrcType);
        break;
    }
    case TRANS_STREAMING_VIDEO_STOP: {
        TRANSFER_SOURCE_TYPE_E* pSrcType = (TRANSFER_SOURCE_TYPE_E*)args;
        PR_DEBUG("streaming stop type %d", *pSrcType);
        break;
    }

#define SUPPORT_ALBUM
#ifdef SUPPORT_ALBUM
    case TRANS_ALBUM_QUERY: /* query album */
    {
        C2C_QUERY_ALBUM_REQ* pSrcType = (C2C_QUERY_ALBUM_REQ*)args;
        // NOTICE: pIndexFile malloc/free in SDK
        int ret = tuya_ipc_album_query_by_name(pSrcType->albumName, pSrcType->channel, &pSrcType->fileLen, (SS_ALBUM_INDEX_HEAD**)&(pSrcType->pIndexFile));
        if (0 != ret) {
            PR_ERR("err %d", ret); 
        }

        if (pSrcType->pIndexFile) {
            SS_ALBUM_INDEX_HEAD* ptmp = (SS_ALBUM_INDEX_HEAD*)pSrcType->pIndexFile;
            PR_DEBUG("get album items %d", ptmp->itemCount);
        }
        break;
    }
    case TRANS_ALBUM_DOWNLOAD_START: /* start download album */
    {
        C2C_CMD_IO_CTRL_ALBUM_DOWNLOAD_START* pSrcType = (C2C_CMD_IO_CTRL_ALBUM_DOWNLOAD_START*)args;
        SS_DOWNLOAD_STATUS_E status = 0;
        SS_ALBUM_DOWNLOAD_START_INFO strStarInfo = { 0 };
        strStarInfo.session_id = pSrcType->channel;
        memcpy(strStarInfo.albumName, pSrcType->albumName, strlen(pSrcType->albumName));
        strStarInfo.fileTotalCnt = pSrcType->fileTotalCnt;
        strStarInfo.thumbnail = pSrcType->thumbnail;
        strStarInfo.pFileInfoArr = (SS_FILE_PATH*)pSrcType->pFileInfoArr;
        int ret = tuya_ipc_album_set_download_status(SS_DL_START, &strStarInfo);
        if (0 != ret) {
            PR_ERR("err %d", ret);
        }

        break;
    }
    case TRANS_ALBUM_DOWNLOAD_CANCEL: // cancel album
    {
        C2C_ALBUM_DOWNLOAD_CANCEL* pSrcType = (C2C_ALBUM_DOWNLOAD_CANCEL*)args;
        SS_ALBUM_DOWNLOAD_START_INFO strStarInfo = { 0 };
        strStarInfo.session_id = pSrcType->channel;
        memcpy(strStarInfo.albumName, pSrcType->albumName, strlen(pSrcType->albumName));
        int ret = tuya_ipc_album_set_download_status(SS_DL_CANCLE, &strStarInfo);
        if (0 != ret) {
            PR_ERR("err %d", ret);
        }
        PR_ERR("ok %d", ret);
        break;
    }
    case TRANS_ALBUM_DELETE: //delete
    {
        C2C_CMD_IO_CTRL_ALBUM_DELETE* pSrcType = (C2C_CMD_IO_CTRL_ALBUM_DELETE*)args;
        int ret = tuya_ipc_album_delete_by_file_info(pSrcType->channel, pSrcType->albumName, pSrcType->fileNum, (SS_FILE_PATH*)pSrcType->pFileInfoArr);
        if (0 != ret) {
            PR_ERR("err %d", ret);
            tuya_ipc_delete_video_finish(pSrcType->channel, TUYA_DOWNLOAD_ALBUM, 0);
        }
        break;
    }
#endif

    default:
        break;
    }
}

STATIC VOID __TUYA_APP_rev_audio_cb(IN CONST TRANSFER_AUDIO_FRAME_S* p_audio_frame, IN CONST UINT_T frame_no)
{
    MEDIA_FRAME_S audio_frame = { 0 };
    audio_frame.p_buf = p_audio_frame->p_audio_buf;
    audio_frame.size = p_audio_frame->buf_len;

    PR_TRACE("Rev Audio. size:%u audio_codec:%d audio_sample:%d audio_databits:%d audio_channel:%d", p_audio_frame->buf_len,
        p_audio_frame->audio_codec, p_audio_frame->audio_sample, p_audio_frame->audio_databits, p_audio_frame->audio_channel);

    TUYA_APP_Rev_Audio_CB(&audio_frame, TUYA_AUDIO_SAMPLE_8K, TUYA_AUDIO_DATABITS_16, TUYA_AUDIO_CHANNEL_MONO);
}

OPERATE_RET TUYA_APP_Enable_P2PTransfer(IN UINT_T max_users)
{
    if (s_p2p_mgr.enabled == TRUE) {
        PR_DEBUG("P2P Is Already Inited");
        return OPRT_OK;
    }

    PR_DEBUG("Init P2P With Max Users:%u", max_users);

    s_p2p_mgr.enabled = TRUE;
    s_p2p_mgr.max_users = max_users;
    s_p2p_mgr.p2p_audio_codec = s_media_info[0].audio_codec[E_CHANNEL_AUDIO];

    TUYA_IPC_TRANSFER_VAR_S p2p_var = { 0 };
    p2p_var.online_cb = __depereated_online_cb;
    p2p_var.on_rev_audio_cb = __TUYA_APP_rev_audio_cb;
    /*speak data format  app->ipc*/
    p2p_var.rev_audio_codec = TUYA_CODEC_AUDIO_G711U;
    p2p_var.audio_sample = TUYA_AUDIO_SAMPLE_8K;
    p2p_var.audio_databits = TUYA_AUDIO_DATABITS_16;
    p2p_var.audio_channel = TUYA_AUDIO_CHANNEL_MONO;
    /*end*/
    p2p_var.on_event_cb = __TUYA_APP_p2p_event_cb;
    p2p_var.live_quality = TRANS_LIVE_QUALITY_MAX;
    p2p_var.max_client_num = max_users;
    memcpy(&p2p_var.AVInfo, &s_media_info, sizeof(s_media_info));
    tuya_ipc_tranfser_init(&p2p_var);

    return OPRT_OK;
}
