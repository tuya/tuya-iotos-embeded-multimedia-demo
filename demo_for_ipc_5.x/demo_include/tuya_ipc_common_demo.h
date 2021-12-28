/*
 * tuya_ipc_common_demo.h
 *Copyright(C),2017-2022, TUYA company www.tuya.comm
 *
 *FILE description:
  *
 *  Created on: 2021年12月23日
 *      Author: dante
 */

#ifndef __TUYA_COMMON_DEMO_H_
#define __TUYA_COMMON_DEMO_H_
#include <stdio.h>
#include "tuya_cloud_types.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_media.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_cloud_types.h"
#include "tuya_cloud_error_code.h"
#include "tuya_cloud_com_defs.h"
#include "cJSON.h"
#include "tuya_os_adapter.h"
#include "tuya_ipc_p2p_demo.h"
#include "tuya_ipc_stream_storage_demo.h"
#include "tuya_ipc_cloud_storage_demo.h"
#include "tuya_ipc_doorbell_demo.h"
#include "tuya_ipc_log_demo.h"

#if defined(WIFI_GW) && (WIFI_GW==1)
#include "tuya_cloud_wifi_defs.h"
#endif

#if defined(ENABLE_IPC_GW_BOTH) && (ENABLE_IPC_GW_BOTH==1)
#include "tuya_gw_init_api.h"
OPERATE_RET tuya_ipc_gw_set_gw_cbs(TY_IOT_GW_CBS_S *p_gw_cbs);
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct
{
	WIFI_INIT_MODE_E connect_mode;
	GW_STATUS_CHANGED_CB net_status_change_cb;
}TUYA_IPC_SDK_NET_S;

typedef struct
{
	BOOL_T enable; /*enable == false ,other var is invalid*/
}TUYA_IPC_SDK_CLOUD_AI_S;

typedef struct
{
	IPC_MEDIA_INFO_S media_info;
}TUYA_IPC_SDK_MEDIA_STREAM_S;
typedef struct
{
	DEV_DP_QUERY_CB dp_query;
	DEV_RAW_DP_CMD_CB  raw_dp_cmd_proc;
	DEV_OBJ_DP_CMD_CB common_dp_cmd_proc;
}TUYA_IPC_SDK_DP_S;
typedef struct
{
    BOOL_T enable ;
    TUYA_IPC_SDK_DEV_UPGRADE_INFORM_CB upgrade_cb;
}TUYA_IPC_SDK_DEMO_UPGRADE_S;
typedef struct
{
    BOOL_T enable;
}TUYA_IPC_SDK_QUICK_START_S;
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
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
	TUAY_QRCODE_ACTIVE_CB    qrcode_active_cb;
#endif
}TUYA_IPC_SDK_RUN_VAR_S;

#ifdef __cplusplus
}
#endif
#endif
