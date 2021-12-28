/*
 * tuya_ipc_sd_demo.h
 *Copyright(C),2017-2022, TUYA company www.tuya.comm
 *
 *FILE description:
  *
 *  Created on: 2021年1月11日
 *      Author: kuiba
 */

#ifndef __TUYA_IPC_SD_DEMO_H__
#define __TUYA_IPC_SD_DEMO_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "tuya_cloud_types.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_api.h"
typedef struct
{
	BOOL_T enable; /*enable == false ,other var is invalid*/
	CHAR_T storage_path[IPC_STORAGE_PATH_LEN + 1];
	UINT_T max_event_num_per_day ;
	UINT_T  preview_record_time;
	UINT_T  skills;//参考tuya_ipc_stream_storage.h TUYA_IPC_SKIL_BASIC
	SS_SD_STATUS_CHANGED_CB sd_status_cb;
}TUYA_IPC_SDK_LOCAL_STORAGE_S;

INT_T tuya_ipc_sd_status_upload(INT_T status);
VOID tuya_ipc_sd_format(VOID);
/* TUYA_APP_Init_Stream_Storage */
OPERATE_RET TUYA_APP_Init_Stream_Storage(TUYA_IPC_SDK_LOCAL_STORAGE_S * p_local_storage_info);
#ifdef __cplusplus
}
#endif
#endif /* __TUYA_IPC_SD_DEMO_H__ */
