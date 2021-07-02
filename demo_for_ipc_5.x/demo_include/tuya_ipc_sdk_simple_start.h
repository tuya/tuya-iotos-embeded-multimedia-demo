/*
 * tuya_ipc_interanl_api.h
 *
 *  Created on: 2020年4月23日
 *      Author: 02426
 */

#ifndef INCLUDE_TUYA_IPC_SDK_TUYA_IPC_SDK_COM_DEFS_H_
#define INCLUDE_TUYA_IPC_SDK_TUYA_IPC_SDK_COM_DEFS_H_

#include "tuya_ipc_media.h"
#include "tuya_ipc_p2p.h"
#include "tuya_ipc_video_msg.h"
//#include "tuya_ipc_chromecast.h"
//#include "tuya_ipc_echo_show.h"
#include "tuya_cloud_com_defs.h"
#include "aes_inf.h"

#if defined(ENABLE_IPC_GW_BOTH) && (ENABLE_IPC_GW_BOTH==1)

#include "tuya_gw_init_api.h"

OPERATE_RET tuya_ipc_gw_set_gw_cbs(TY_IOT_GW_CBS_S *p_gw_cbs);

#endif


#if defined(WIFI_GW) && (WIFI_GW==1)
#include "tuya_cloud_wifi_defs.h"
#endif

#include "tuya_cloud_types.h"
#include "tuya_cloud_error_code.h"
#include "tuya_cloud_com_defs.h"
#include "cJSON.h"

#include "tuya_os_adapter.h"

typedef enum
{
    TUYA_IPC_LOW_POWER_MODE,
    TUYA_IPC_INVALID_MODE
}TUYA_IPC_SDK_STREAM_MODE_E;

typedef VOID (*TUYA_CMD_DISPATCH)(IN INT_T cmd,IN VOID *param);
typedef VOID (*TUYA_QUERY_DISPATCH)(IN INT_T cmd,IN VOID *param);
typedef VOID (*TUYA_RST_INFORM_CB)(GW_RESET_TYPE_E from);
typedef VOID (*TUYA_RESTART_INFORM_CB)(VOID);
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
typedef VOID (*TUAY_QRCODE_ACTIVE_CB)(CHAR_T* shorturl);
#endif
/**
* the fifth version api
*/
/*** separator begin  ****/
typedef struct
{
	CHAR_T product_key[IPC_PRODUCT_KEY_LEN + 1]; /* one product key(PID) for one product  */
	CHAR_T uuid[IPC_UUID_LEN + 1]; /* one uuid-authkey pair for one device, to register on TUYA coud */
	CHAR_T auth_key[IPC_AUTH_KEY_LEN + 1];
	CHAR_T dev_sw_version[IPC_SW_VER_LEN + 1]; /* software version with format x1.x2.x3.x4.x5.x6, five dots maximum, zero dots minimum */
	CHAR_T cfg_storage_path[IPC_STORAGE_PATH_LEN + 1];
    TUYA_RST_INFORM_CB gw_reset_cb;/* reset callback fucntion, triggered when user unbind device from a account */
    TUYA_RESTART_INFORM_CB gw_restart_cb;/* restart callback function */
    TUYA_IPC_DEV_TYPE_E   dev_type;/*dev type ,eg low power device*/
}TUYA_IPC_SDK_BASE_S;
typedef VOID (*TUYA_IPC_SDK_NET_STATUS_CHANGE_CB)(IN CONST BYTE_T stat);

typedef struct
{
	WIFI_INIT_MODE_E connect_mode;
	TUYA_IPC_SDK_NET_STATUS_CHANGE_CB net_status_change_cb;

}TUYA_IPC_SDK_NET_S;

typedef int (*TUYA_IPC_SDK_SD_STATUS_CB)(int status);
typedef struct
{
	BOOL_T enable; /*enable == false ,other var is invalid*/
	CHAR_T storage_path[IPC_STORAGE_PATH_LEN + 1];
	UINT_T max_event_num_per_day ;
	UINT_T  preview_record_time;
	UINT_T  skills;//参考tuya_ipc_stream_storage.h TUYA_IPC_SKIL_BASIC
	TUYA_IPC_SDK_SD_STATUS_CB sd_status_cb;
}TUYA_IPC_SDK_LOCAL_STORAGE_S;

typedef struct
{
	BOOL_T enable; /*enable == false ,other var is invalid*/
	UINT_T max_event_per_day;
	UINT_T preview_record_time;
}TUYA_IPC_SDK_CLOUD_STORAGE_S;

typedef struct
{
	BOOL_T enable; /*enable == false ,other var is invalid*/
	//UINT_T max_event_per_day;

}TUYA_IPC_SDK_CLOUD_AI_S;

typedef struct
{
	IPC_MEDIA_INFO_S media_info;

}TUYA_IPC_SDK_MEDIA_STREAM_S;

typedef struct
{
	BOOL_T enable; /*enable == false ,other var is invalid*/
	MESSAGE_E type;
	INT_T msg_duration;
}TUYA_IPC_SDK_VIDEO_MSG_S;

typedef struct
{
	BOOL_T enable; /*enable == false ,other var is invalid*/
	UINT_T max_p2p_client; /* p2p clinet max connect number*/
    TRANS_DEFAULT_QUALITY_E live_mode;  /* for multi-streaming ipc, the default quality for live preview */
	TRANSFER_EVENT_CB transfer_event_cb; /*transfer event callback*/;
	TRANSFER_REV_AUDIO_CB rev_audio_cb;/*app to dev audio callback*/
}TUYA_IPC_SDK_P2P_S;

#if defined(TUYA_EXTENT_DEVELOP)
typedef OPERATE_RET (* TUYA_IPC_SDK_DP_BOOL_CMD_PROC)(const INT_T dp_id, const BOOL_T dp_value,  BOOL_T * dp_report_value);
typedef OPERATE_RET (* TUYA_IPC_SDK_DP_STR_CMD_PROC)(IN CONST INT_T dp_id, IN CONST CHAR_T * dp_value,OUT  CHAR_T * dp_report_value);
typedef OPERATE_RET (* TUYA_IPC_SDK_DP_VALUE_CMD_PROC)(IN CONST INT_T dp_id, IN CONST INT_T dp_value,OUT  INT_T * dp_report_value);
typedef OPERATE_RET (* TUYA_IPC_SDK_DP_ENUM_CMD_PROC)(IN CONST INT_T dp_id, IN CONST UINT_T dp_value,OUT  CHAR_T * dp_report_value);
typedef OPERATE_RET (* TUYA_IPC_SDK_DP_BITMAP_CMD_PROC)(IN CONST INT_T dp_id, IN CONST UINT_T dp_value,OUT  UINT_T * dp_report_value);
#endif

typedef VOID (*TUYA_IPC_SDK_ASYNC_START_CB)(VOID *context);

typedef OPERATE_RET (* TUYA_IPC_SDK_DP_QUERY)(TY_DP_QUERY_S *dp_query);
typedef OPERATE_RET (*TUYA_IPC_SDK_RAW_DP_CMD_PROC)(IN CONST TY_RECV_RAW_DP_S *dp);
typedef OPERATE_RET (*TUYA_IPC_SDK_COMMON_DP_CMD_PROC)(IN CONST TY_RECV_OBJ_DP_S *dp);

typedef struct
{
#if defined(TUYA_EXTENT_DEVELOP)
	TUYA_IPC_SDK_DP_BOOL_CMD_PROC dp_bool_cmd_proc;
	TUYA_IPC_SDK_DP_STR_CMD_PROC dp_str_cmd_proc;
	TUYA_IPC_SDK_DP_VALUE_CMD_PROC dp_value_cmd_proc;
	TUYA_IPC_SDK_DP_ENUM_CMD_PROC dp_enum_cmd_proc;
	TUYA_IPC_SDK_DP_BITMAP_CMD_PROC dp_bitmap_cmd_proc;
#endif
	TUYA_IPC_SDK_DP_QUERY dp_query;//TODO:not be used?
	TUYA_IPC_SDK_RAW_DP_CMD_PROC  raw_dp_cmd_proc;
	TUYA_IPC_SDK_COMMON_DP_CMD_PROC common_dp_cmd_proc;
}TUYA_IPC_SDK_DP_S;
#define TUYA_IPC_CI_SDK 1
typedef OPERATE_RET (*TUYA_IPC_SDK_UPGRADE_START)(UINT_T file_size);
typedef OPERATE_RET (*TUYA_IPC_SDK_UPGRADE_PROCESS)(CONST UINT_T total_len, CONST UINT_T offset, CONST UCHAR_T* data, CONST UINT_T len, UINT_T* remain_len, void* pri_data);
typedef OPERATE_RET (*TUYA_IPC_SDK_UPGRADE_END)(BOOL_T reset);
typedef int (*TUYA_IPC_SDK_DEV_UPGRADE_INFORM_CB)(IN CONST FW_UG_S *fw);
#if 1
typedef struct
{
    BOOL_T enable ;
    TUYA_IPC_SDK_DEV_UPGRADE_INFORM_CB upgrade_cb;
}TUYA_IPC_SDK_DEMO_UPGRADE_S;
#endif

typedef struct
{
    BOOL_T enable;
    TUYA_IPC_SDK_STREAM_MODE_E mode;
}TUYA_IPC_SDK_QUICK_START_S;

typedef struct
{
    AES_HW_CBC_FUNC aes_fun;

}TUYA_IPC_SDK_AES_HW_S;

typedef struct
{
    INT_T log_level;/*set log level default: error level*/
    CHAR_T qrcode_token[16]; //connect_method !=2 ,qrcode_token invalid
}TUYA_IPC_SDK_DEBUG_S;


/**
 * \brief IPC SDK run variable set
 * \struct TUYA_IPC_SDK_RUN_VAR_S
 */
typedef struct
{
	TUYA_IPC_SDK_BASE_S  iot_info;/*sdk base configure information*/
	TUYA_IPC_SDK_MEDIA_STREAM_S media_info;/*audio video configure information*/
	TUYA_IPC_SDK_NET_S net_info;/*net work info*/
	TUYA_IPC_SDK_DEMO_UPGRADE_S upgrade_info;/*fireware upgrade information*/
	TUYA_IPC_SDK_DP_S dp_info;/*date point information*/
	TUYA_IPC_SDK_P2P_S p2p_info;/*p2p information*/
	TUYA_IPC_SDK_LOCAL_STORAGE_S local_storage_info;/* sd card storage information*/
	TUYA_IPC_SDK_CLOUD_STORAGE_S cloud_storage_info;/*cloud storage information*/
	TUYA_IPC_SDK_CLOUD_AI_S cloud_ai_detct_info; /*ai detcet information*/
	TUYA_IPC_SDK_VIDEO_MSG_S  video_msg_info;/*door bell function information*/
	TUYA_IPC_SDK_QUICK_START_S  quick_start_info;/*start way for p2p  */
	TUYA_IPC_SDK_DEBUG_S debug_info;/*debug info sets*/
	TUYA_IPC_SDK_AES_HW_S   aes_hw_info; /*aes */
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
	TUAY_QRCODE_ACTIVE_CB    qrcode_active_cb;
#endif
}TUYA_IPC_SDK_RUN_VAR_S;


/**
 ******* the fifth version api separator end *******
*/


#endif /* INCLUDE_TUYA_IPC_SDK_TUYA_IPC_SDK_COM_DEFS_H_ */
