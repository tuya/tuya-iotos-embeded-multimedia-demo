/*
 * tuya_ipc_sdk_simple_start.c
 *
 *  Created on: 2020年4月25日
 *      Author: 02426
 */
#include <errno.h>
#include <string.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "gw_intf.h"
#include "mqc_app.h"
#include "iot_httpc.h"
#include "ipc_httpc.h"
#include "app_agent.h"
#include "mem_pool.h"
#include "uni_thread.h"
#include "uni_log.h"

#include "tuya_iot_internal_api.h"
#include "tuya_iot_base_api.h"
#include "tuya_iot_com_api.h"
#include "tuya_iot_wifi_api.h"
#include "com_mmod.h"
#include "tuya_ipc_stream_storage.h"

#include "tuya_ipc_api.h"
#include "tuya_ipc_p2p.h"
#include "tuya_ring_buffer.h"
#include "tuya_ipc_ai_detect_storage.h"
#include "tuya_ipc_cloud_storage.h"

#include "tuya_ipc_sdk_simple_start.h"

#if defined(ENABLE_IPC_GW_BOTH) && (ENABLE_IPC_GW_BOTH==1)
#include "scene_linkage.h"
#endif

typedef struct
{
	TUYA_IPC_SDK_RUN_VAR_S sdk_run_info;
	C2C_TRANS_LIVE_CLARITY_PARAM_S live_clarity_info;
	THRD_HANDLE mqtt_status_change_handle;
}TUYA_IPC_SDK_RUN_HANDLE;

static TUYA_IPC_SDK_RUN_HANDLE s_ipc_sdk_run_handler = {0};

STATIC OPERATE_RET tuya_ipc_sdk_ring_buffer_create( CONST IPC_MEDIA_INFO_S *pMediaInfo)
{
	printf("create ring buffer para is NULL\n");
	if(NULL == pMediaInfo)
	{
		PR_ERR("create ring buffer para is NULL\n");
		return -1;
	}
	OPERATE_RET ret = OPRT_OK;

    STATIC BOOL_T s_ring_buffer_inited = FALSE;
    if(s_ring_buffer_inited == TRUE)
    {
        PR_DEBUG("The Ring Buffer Is Already Inited");
        return OPRT_OK;
    }

    IPC_STREAM_E ringbuffer_stream_type;
   // CHANNEL_E channel;
    Ring_Buffer_Init_Param_S param={0};
    for( ringbuffer_stream_type = E_IPC_STREAM_VIDEO_MAIN; ringbuffer_stream_type < E_IPC_STREAM_MAX; ringbuffer_stream_type++ )
    {
        PR_DEBUG("init ring buffer Channel:%d Enable:%d", ringbuffer_stream_type, pMediaInfo->channel_enable[ringbuffer_stream_type]);
        if(pMediaInfo->channel_enable[ringbuffer_stream_type] == TRUE)
        {
            if(ringbuffer_stream_type == E_IPC_STREAM_AUDIO_MAIN)
            {
                param.bitrate = pMediaInfo->audio_sample[E_IPC_STREAM_AUDIO_MAIN]*pMediaInfo->audio_databits[E_IPC_STREAM_AUDIO_MAIN]/1024;
                param.fps = pMediaInfo->audio_fps[E_IPC_STREAM_AUDIO_MAIN];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                PR_DEBUG("audio_sample %d, audio_databits %d, audio_fps %d",pMediaInfo->audio_sample[E_IPC_STREAM_AUDIO_MAIN],pMediaInfo->audio_databits[E_IPC_STREAM_AUDIO_MAIN],pMediaInfo->audio_fps[E_IPC_STREAM_AUDIO_MAIN]);
                ret = tuya_ipc_ring_buffer_init(0,0,ringbuffer_stream_type,&param);
            }
            else
            {
                param.bitrate = pMediaInfo->video_bitrate[ringbuffer_stream_type];
                param.fps = pMediaInfo->video_fps[ringbuffer_stream_type];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                PR_DEBUG("video_bitrate %d, video_fps %d",pMediaInfo->video_bitrate[ringbuffer_stream_type],pMediaInfo->video_fps[ringbuffer_stream_type]);
                ret = tuya_ipc_ring_buffer_init(0,0,ringbuffer_stream_type, &param);
            }
            if(ret != 0)
            {
                PR_ERR("init ring buffer fails. %d %d", ringbuffer_stream_type, ret);
                return OPRT_MALLOC_FAILED;
            }
            PR_DEBUG("init ring buffer success. channel:%d", ringbuffer_stream_type);
        }
    }

    s_ring_buffer_inited = TRUE;

    return OPRT_OK;
}

STATIC VOID tuya_ipc_sdk_audio_rec_cb(IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame, IN CONST UINT_T frame_no)
{
	PR_DEBUG("rev audio cb\n");
	if(s_ipc_sdk_run_handler.sdk_run_info.p2p_info.rev_audio_cb)
	{
	    s_ipc_sdk_run_handler.sdk_run_info.p2p_info.rev_audio_cb(p_audio_frame,frame_no);
	}
	return;
}
#if defined(TUYA_EXTEND_DEVELOP)
STATIC VOID tuya_ipc_sdk_app_ss_pb_event_cb(IN UINT_T pb_idx, IN SS_PB_EVENT_E pb_event, IN PVOID_T args)
{
    PR_DEBUG("ss pb rev event: %u %d", pb_idx, pb_event);
    if(pb_event == SS_PB_FINISH)
    {
        tuya_ipc_playback_send_finish(pb_idx);
    }
}

STATIC VOID tuya_ipc_sdk_app_pb_video_get_cb(IN UINT_T pb_idx, IN CONST MEDIA_FRAME_S *p_frame)
{
    TRANSFER_VIDEO_FRAME_S video_frame = {0};

    UINT_T codec_type = 0;
    codec_type = (p_frame->type && 0xff00) >> 8;
    video_frame.video_codec = (codec_type == 0 ? TUYA_CODEC_VIDEO_H264 : TUYA_CODEC_VIDEO_H265);
    video_frame.video_frame_type = (p_frame->type && 0xff) == E_VIDEO_PB_FRAME ? TY_VIDEO_FRAME_PBFRAME:TY_VIDEO_FRAME_IFRAME;
    video_frame.p_video_buf = p_frame->p_buf;
    video_frame.buf_len = p_frame->size;
    video_frame.pts = p_frame->pts;
    video_frame.timestamp = p_frame->timestamp;

    tuya_ipc_playback_send_video_frame(pb_idx, &video_frame);
}

STATIC VOID tuya_ipc_sdk_app_pb_audio_get_cb(IN UINT_T pb_idx, IN CONST MEDIA_FRAME_S *p_frame)
{
    TRANSFER_AUDIO_FRAME_S audio_frame = {0};

	audio_frame.audio_codec = s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN];
	audio_frame.audio_sample = s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN];
	audio_frame.audio_databits = s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN];
	audio_frame.audio_channel = s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN];
	audio_frame.p_audio_buf = p_frame->p_buf;
	audio_frame.buf_len = p_frame->size;
	audio_frame.pts = p_frame->pts;
	audio_frame.timestamp = p_frame->timestamp;

    tuya_ipc_playback_send_audio_frame(pb_idx, &audio_frame);
}
#endif
STATIC VOID tuya_ipc_sdk_p2p_event_proc_cb(IN CONST TRANSFER_EVENT_E event, IN CONST PVOID_T args)
{
    PR_DEBUG("p2p rev event cb=[%d] ", event);
    if(s_ipc_sdk_run_handler.sdk_run_info.p2p_info.transfer_event_cb)
    {
        s_ipc_sdk_run_handler.sdk_run_info.p2p_info.transfer_event_cb(event, args);
    }
}
STATIC VOID tuya_ipc_sdk_p2p_init()
{
    //P2P init
    TUYA_IPC_TRANSFER_VAR_S p2p_var = {0};
    //TODO:humin
    p2p_var.online_cb = NULL;//TODO:此接口似乎对外没有用途
    p2p_var.on_rev_audio_cb = tuya_ipc_sdk_audio_rec_cb;//TODO 此接口应该对外
    /*speak data format  app->ipc*/
    //TODO 这些变量是否可以使用media_info。从demo上来看，是可以不同的
    p2p_var.rev_audio_codec = s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN];
    p2p_var.audio_sample =  s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN];
    p2p_var.audio_databits =  s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN];
    p2p_var.audio_channel = s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN];
    /*end*/
    p2p_var.on_event_cb = tuya_ipc_sdk_p2p_event_proc_cb;//TODO 此接口应该部分对外
    p2p_var.defLiveMode = s_ipc_sdk_run_handler.sdk_run_info.p2p_info.live_mode;
    p2p_var.max_client_num = s_ipc_sdk_run_handler.sdk_run_info.p2p_info.max_p2p_client;
    memcpy(&p2p_var.AVInfo,&s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info,sizeof(IPC_MEDIA_INFO_S));
    int ret = tuya_ipc_tranfser_init(&p2p_var);
    PR_DEBUG("p2p transfer init result is %d\n",ret);

}
BOOL_T s_mqtt_online_status = FALSE;
STATIC VOID tuya_ipc_sdk_net_status_change_cb(IN CONST BYTE_T stat)
{

    PR_DEBUG("Net status change to:%d", stat);
    switch(stat)
    {
#if defined(WIFI_GW) && (WIFI_GW==1)
        case STAT_CLOUD_CONN:        //for wifi ipc
        PR_DEBUG("WIFI_GW is opend:%d", stat);
#endif
#if defined(WIFI_GW) && (WIFI_GW==0)
        case GB_STAT_CLOUD_CONN:     //for wired ipc
#endif
          //  break; CI上 ：上线是 7.ipc本地是11。所以要注释掉break
        case STAT_MQTT_ONLINE:       //for low-power wifi ipc
        {
        	//TODO 网络上线后，需要用户去做一些设备声音和灯光处理
         //   IPC_APP_Notify_LED_Sound_Status_CB(IPC_MQTT_ONLINE);
            PR_DEBUG("mqtt is online %d\r\n",stat);
            if(s_mqtt_online_status == FALSE)
            {
                s_mqtt_online_status = TRUE;
            }
            break;
        }
        default:
        {
            break;
        }
    }
    //开放部分对外接口给上层应用
    //TODO:
    //TODO 网络上线后，需要用户去做一些设备声音和灯光处理。可能存在服务还未开启的情况
    if(s_ipc_sdk_run_handler.sdk_run_info.net_info.net_status_change_cb)
    {
    	s_ipc_sdk_run_handler.sdk_run_info.net_info.net_status_change_cb(stat);
    }

}
STATIC OPERATE_RET tuya_ipc_sdk_sd_status_cb(int status)
{
	OPERATE_RET ret = 0;
	if(s_ipc_sdk_run_handler.sdk_run_info.local_storage_info.enable && s_ipc_sdk_run_handler.sdk_run_info.local_storage_info.sd_status_cb )
	{
		return s_ipc_sdk_run_handler.sdk_run_info.local_storage_info.sd_status_cb(status);
	}
    return -1;
}


//DOTO:线程处理完，则退出了。如果是设备离线，再上线，开启过的功能，是否需要重新上线。线程是否要退出？
STATIC VOID tuya_ipc_sdk_mqtt_online_proc(PVOID_T arg)
{
    PR_DEBUG("tuya_ipc_sdk_mqtt_online_proc thread start success\n");
    while(s_mqtt_online_status == FALSE)
    {
        // not use  Semaphore for compatibility
        sleep(1);
    }
    PR_DEBUG("tuya_ipc_sdk_mqtt_online_proc is start run\n");
    int ret;
    //同步服务器时间
    TIME_T time_utc;
    INT_T time_zone;
    do
    {
        //TODO:一直不成功 一直获取？
        ret = tuya_ipc_get_service_time_force(&time_utc, &time_zone);

    } while(ret != OPRT_OK);

    if(FALSE == s_ipc_sdk_run_handler.sdk_run_info.quick_start_info.enable)
    {
        tuya_ipc_sdk_p2p_init();
    }

    if(s_ipc_sdk_run_handler.sdk_run_info.local_storage_info.enable)
    {
        TUYA_IPC_STORAGE_VAR_S stg_var;
        memset(&stg_var, 0, SIZEOF(TUYA_IPC_STORAGE_VAR_S));
        memcpy(stg_var.base_path, s_ipc_sdk_run_handler.sdk_run_info.local_storage_info.storage_path, SS_BASE_PATH_LEN);
        stg_var.media_setting = s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info;
        stg_var.max_event_per_day = s_ipc_sdk_run_handler.sdk_run_info.local_storage_info.max_event_num_per_day;
        stg_var.skills = s_ipc_sdk_run_handler.sdk_run_info.local_storage_info.skills;
        stg_var.sd_status_changed_cb = tuya_ipc_sdk_sd_status_cb;

        ret = tuya_ipc_ss_init(&stg_var);
        PR_DEBUG("local storage init result is %d\n",ret);

    }

    if(s_ipc_sdk_run_handler.sdk_run_info.cloud_ai_detct_info.enable)
    {
        ret = tuya_ipc_ai_detect_storage_init(&s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info);
        PR_DEBUG("ai detect result is %d\n",ret);
    }

    if(s_ipc_sdk_run_handler.sdk_run_info.video_msg_info.enable)
    {
        ret =  tuya_ipc_video_msg_init(&s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info, s_ipc_sdk_run_handler.sdk_run_info.video_msg_info.type, s_ipc_sdk_run_handler.sdk_run_info.video_msg_info.msg_duration);
        PR_DEBUG("door bell init result is %d\n",ret);
    }

    if(s_ipc_sdk_run_handler.sdk_run_info.cloud_storage_info.enable)
    {
        ret = tuya_ipc_cloud_storage_init(&s_ipc_sdk_run_handler.sdk_run_info.media_info.media_info, &s_ipc_sdk_run_handler.sdk_run_info.aes_hw_info.aes_fun);
        PR_DEBUG("cloud storage init result is %d\n",ret);
    }


    IPC_APP_upload_all_status();

    tuya_ipc_upload_skills();
    PR_DEBUG("tuya_ipc_sdk_mqtt_online_proc is end run\n");
}
VOID tuya_ipc_sdk_dp_cmd_proc(IN CONST TY_RECV_OBJ_DP_S *dp_rev)
{
	if(s_ipc_sdk_run_handler.sdk_run_info.dp_info.common_dp_cmd_proc)
	{
		s_ipc_sdk_run_handler.sdk_run_info.dp_info.common_dp_cmd_proc(dp_rev);
	}
	return;

#if defined(TUYA_EXTEND_DEVELOP)
	if(NULL == dp_rev)
	{
		PR_ERR("dp cmd proc paramter error\n");
		return;
	}

    TY_OBJ_DP_S *dp_data = (TY_OBJ_DP_S *)(dp_rev->dps);
    UINT_T cnt = dp_rev->dps_cnt;
    INT_T index = 0;
    DP_PROP_TP_E dp_value_type = dp_data->type;
    PR_DEBUG("ipc sdk dp info dp_value_type = %d, dpCnt=%d\n",dp_value_type,cnt);

    for(index = 0; index < cnt; index++)
    {
        TY_OBJ_DP_S *p_dp_obj = dp_data + index;
        DP_PROP_TP_E dp_value_type = p_dp_obj->type;
        //类型判定,选择合适的回调函数给应用开发层
        switch(dp_value_type)
        {
            case PROP_BOOL:
            {
                if(s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_bool_cmd_proc)
                {
                    BOOL_T reportBool = FALSE;
                    s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_bool_cmd_proc(p_dp_obj->dpid,p_dp_obj->value.dp_bool,&reportBool);
                   // tuya_ipc_dp_report(NULL, p_dp_obj->dpid,dp_value_type,&reportBool,1);
//                    tuya_ipc_dp_report_async(NULL, p_dp_obj->dpid,dp_value_type,&reportBool,1);
                    tuya_ipc_dp_report_sync(NULL, p_dp_obj->dpid,dp_value_type,&reportBool,1,100);
//                    tuya_ipc_dp_report_async(NULL, p_dp_obj->dpid,dp_value_type,&reportBool,1);
                }
                break;
            }
            case PROP_VALUE:
            {
                if(s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_value_cmd_proc)
                {
                    INT_T reportValue = 0;
                    (s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_value_cmd_proc)(p_dp_obj->dpid,p_dp_obj->value.dp_value,&reportValue);
                    tuya_ipc_dp_report(NULL, p_dp_obj->dpid,dp_value_type,&reportValue,1);

                }
                break;
            }
            case PROP_STR:
            {
                if(s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_str_cmd_proc)
                {
                    char reportStr[256] = {0};
                    (s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_str_cmd_proc)(p_dp_obj->dpid,p_dp_obj->value.dp_str,reportStr);
                    tuya_ipc_dp_report(NULL, p_dp_obj->dpid,dp_value_type,reportStr,1);
                }

                break;
            }
            case PROP_ENUM:
            {
                if(s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_enum_cmd_proc)
                {
                    char reportEnum[64] = {0};
                    (s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_enum_cmd_proc)(p_dp_obj->dpid,p_dp_obj->value.dp_enum,reportEnum);
                    tuya_ipc_dp_report(NULL, p_dp_obj->dpid,dp_value_type,&reportEnum,1);
                }
                break;
            }
            case PROP_BITMAP:
            {
                if(s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_bitmap_cmd_proc)
                {
                    UINT_T reportBitMap = 0;
                    (s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_bitmap_cmd_proc)(p_dp_obj->dpid,p_dp_obj->value.dp_bitmap,&reportBitMap);
                    tuya_ipc_dp_report(NULL, p_dp_obj->dpid,dp_value_type,&reportBitMap,1);

                }
                break;
            }
            default:
            {
                PR_DEBUG("dp type %d is not support type \r\n",dp_value_type);
                break;
            }
        }
    }
#endif

}
STATIC VOID tuya_ipc_sdk_raw_dp_cmd_proc(TY_RECV_RAW_DP_S * raw_dp)
{
	if(s_ipc_sdk_run_handler.sdk_run_info.dp_info.raw_dp_cmd_proc)
	{
		s_ipc_sdk_run_handler.sdk_run_info.dp_info.raw_dp_cmd_proc(raw_dp);
	}
	return;
}
STATIC VOID tuya_ipc_sdk_dp_query(IN CONST TY_DP_QUERY_S *dp_query)
{
	if(dp_query && s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_query)
	{
		printf("hmdg::dp_query id= %s\n",dp_query->dpid);
		s_ipc_sdk_run_handler.sdk_run_info.dp_info.dp_query(dp_query);
	}
}
#if defined(TUYA_EXTEND_DEVELOP)
//To collect OTA files in fragments and write them to local files
OPERATE_RET tuya_ipc_sdk_upgrade_file_data_get(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len,IN CONST UINT_T offset,
                             IN CONST BYTE_T *data,IN CONST UINT_T len,OUT UINT_T *remain_len, IN PVOID_T pri_data)
{
    PR_DEBUG("Rev File Data");
    PR_DEBUG("total_len:%d  fw_url:%s", total_len, fw->fw_url);
    PR_DEBUG("Offset:%d Len:%d", offset, len);

    //report UPGRADE process, NOT only download percent, consider flash-write time
    //APP will report overtime fail, if uprgade process is not updated within 60 seconds

    int download_percent = (offset * 100) / (total_len+1);
    int report_percent = download_percent/2; // as an example, download 100% = 50%  upgrade work finished
    tuya_ipc_upgrade_progress_report(report_percent);

    if(offset == total_len) // finished downloading
    {
        //start write OTA file to flash by parts
        /* only for example:
        FILE *p_upgrade_fd = (FILE *)pri_data;
        fwrite(data, 1, len, p_upgrade_fd);
        *remain_len = 0;
        */
        // finish 1st part
        report_percent+=10;
        tuya_ipc_upgrade_progress_report(report_percent);
        // finish 2nd part
        sleep(5);
        report_percent+=10;
        tuya_ipc_upgrade_progress_report(report_percent);
        // finish all parts, set to 90% for example
        report_percent = 90;
        tuya_ipc_upgrade_progress_report(report_percent);
    }

    //APP will report "uprage success" after reboot and new FW version is reported inside SDK automaticlly

    return OPRT_OK;
}



//Callback after downloading OTA files
VOID tuya_ipc_sdk_upgrade_file_end_download_proc(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{
    FILE *p_upgrade_fd = (FILE *)pri_data;
    fclose(p_upgrade_fd);

    PR_DEBUG("Upgrade Finish");
    PR_DEBUG("download_result:%d fw_url:%s", download_result, fw->fw_url);

    if(download_result == 0)
    {
        /* The developer needs to implement the operation of OTA upgrade,
        when the OTA file has been downloaded successfully to the specified path. [ p_mgr_info->upgrade_file_path ]*/
    }
    //TODO
    //reboot system
}

#endif
VOID tuya_ipc_sdk_sub_device_upgrade_proc_cb(IN CONST FW_UG_S *fw)
{
    PR_DEBUG("Rev Upgrade Info");
    PR_DEBUG("fw->fw_url:%s", fw->fw_url);
    PR_DEBUG("fw->sw_ver:%s", fw->sw_ver);
    PR_DEBUG("fw->file_size:%u", fw->file_size);

    if(s_ipc_sdk_run_handler.sdk_run_info.upgrade_info.upgrade_cb)
    {
        s_ipc_sdk_run_handler.sdk_run_info.upgrade_info.upgrade_cb(fw);
    }
}

/*
Callback when the user clicks on the APP to remove the device
*/
VOID tuya_ipc_sdk_app_reset_cb(GW_RESET_TYPE_E type)
{
    printf("reset ipc success. please restart the ipc %d\n", type);
    if(s_ipc_sdk_run_handler.sdk_run_info.iot_info.gw_reset_cb)
    {
        s_ipc_sdk_run_handler.sdk_run_info.iot_info.gw_reset_cb(type);
    }
 //   IPC_APP_Notify_LED_Sound_Status_CB(IPC_RESET_SUCCESS);
    //TODO
    /* Developers need to restart IPC operations */
}

VOID tuya_ipc_sdk_app_restart_cb(VOID)
{
    printf("sdk internal restart request. please restart the ipc\n");
    if(s_ipc_sdk_run_handler.sdk_run_info.iot_info.gw_restart_cb)
    {
        s_ipc_sdk_run_handler.sdk_run_info.iot_info.gw_restart_cb();
    }
    //TODO
    /* Developers need to implement restart operations. Restart the process or restart the device. */
}
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
void  tuya_ipc_qrcode_active_cb(char * shorturl)
{
    printf("tuya_ipc_qrcode_active_cb short url  is %s\n",shorturl);
    if(s_ipc_sdk_run_handler.sdk_run_info.qrcode_active_cb)
    {
        s_ipc_sdk_run_handler.sdk_run_info.qrcode_active_cb(shorturl);
    }
}
#endif
STATIC VOID * tuya_ipc_sdk_low_power_p2p_init_proc(VOID * args)
{
    PR_DEBUG("start low power p2p\n");
    //todo process fail
    tuya_ipc_sdk_p2p_init();
    return NULL;
}


/**
 *函数线程不安全
**/
OPERATE_RET tuya_ipc_sdk_start(IN CONST TUYA_IPC_SDK_RUN_VAR_S * pRunInfo)
{
    printf("sdk start\n");
	if(NULL == pRunInfo)
	{
		PR_ERR("start sdk para is NULL\n");
		return OPRT_INVALID_PARM;
	}

    OPERATE_RET ret = 0;
    STATIC BOOL_T s_ipc_sdk_started = FALSE;
    if(TRUE == s_ipc_sdk_started )
    {
        PR_DEBUG("IPC SDK has started\n");
        return ret;
    }

	s_ipc_sdk_run_handler.sdk_run_info = *pRunInfo;
    //TODO loglevel
    tuya_ipc_set_log_attr(pRunInfo->debug_info.log_level,NULL);

	//低功耗 优先开启P2P
    if(s_ipc_sdk_run_handler.sdk_run_info.quick_start_info.enable)
    {
        //TODO pthread attr set
//        pthread_attr_t attr;
//        pthread_attr_init(&attr);
//        pthread_attr_setstacksize(&attr, stack_size);
        pthread_t low_power_p2p_thread_handler;
        int op_ret = pthread_create(&low_power_p2p_thread_handler,NULL, tuya_ipc_sdk_low_power_p2p_init_proc, NULL);
        if(op_ret < 0)
        {
            PR_ERR("create p2p start thread is error\n");
            return -1;
        }
    }

    //setup:创建等待mqtt上线进程，mqtt上线后，再开启与网络相关的业务
    //TODO pthread attr set
    int op_ret = pthread_create(&s_ipc_sdk_run_handler.mqtt_status_change_handle,NULL, tuya_ipc_sdk_mqtt_online_proc, NULL);
    if(op_ret < 0)
    {
        PR_ERR("create tuya_ipc_sdk_mqtt_online_proc  thread is error\n");
        return -1;
    }

	//setup2:init sdk
    TUYA_IPC_ENV_VAR_S env;
    memset(&env, 0, sizeof(TUYA_IPC_ENV_VAR_S));
    strcpy(env.storage_path, pRunInfo->iot_info.cfg_storage_path);
    strcpy(env.product_key,pRunInfo->iot_info.product_key);
    strcpy(env.uuid, pRunInfo->iot_info.uuid);
    strcpy(env.auth_key, pRunInfo->iot_info.auth_key);
    strcpy(env.dev_sw_version, pRunInfo->iot_info.dev_sw_version);
    strcpy(env.dev_serial_num, "tuya_ipc");
    //TODO:raw
    env.dev_raw_dp_cb = tuya_ipc_sdk_raw_dp_cmd_proc;
    env.dev_obj_dp_cb = tuya_ipc_sdk_dp_cmd_proc;
    env.dev_dp_query_cb = tuya_ipc_sdk_dp_query;//TODO:这个接口无实际用途
    env.status_changed_cb = tuya_ipc_sdk_net_status_change_cb;//TODO:这个接口需要部分对外
    env.upgrade_cb_info.upgrade_cb = tuya_ipc_sdk_sub_device_upgrade_proc_cb;
    env.gw_rst_cb = tuya_ipc_sdk_app_reset_cb;//TODO 这个接口需要对外
    env.gw_restart_cb = tuya_ipc_sdk_app_restart_cb;//TODO:这个接口需要部分对外
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
    env.qrcode_active_cb = tuya_ipc_qrcode_active_cb;
#endif
    ret = tuya_ipc_init_sdk(&env);
	if(OPRT_OK != ret)
	{
		PR_ERR("init sdk is error\n");
		return ret;
	}

	//setup 1: ring buffer 创建。
	ret = tuya_ipc_sdk_ring_buffer_create(&pRunInfo->media_info.media_info);
	if(OPRT_OK != ret)
	{
		PR_ERR("create ring buffer is error\n");
		return ret;
	}
	//setup 3:
	ret = tuya_ipc_start_sdk(pRunInfo->net_info.connect_mode,pRunInfo->debug_info.qrcode_token);
	if(OPRT_OK != ret)
	{
		PR_ERR("start sdk is error\n");
		return ret;
	}

	s_ipc_sdk_started = true;
	PR_DEBUG("tuya ipc sdk start is complete\n");
	return ret;
}


