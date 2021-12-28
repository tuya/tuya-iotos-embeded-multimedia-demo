/*
 * tuya_ipc_cloud_storage_demo.h
 *Copyright(C),2017-2022, TUYA company www.tuya.comm
 *
 *FILE description:
  *
 *  Created on: 2021年12月23日
 *      Author: dante
 */

#ifndef __TUYA_IPC_CLOUD_STORAGE_DEMO_H__
#define __TUYA_IPC_CLOUD_STORAGE_DEMO_H__
#include "tuya_cloud_types.h"
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ipc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	BOOL_T enable; /*enable == false ,other var is invalid*/
	BOOL_T en_audio_record; /* default enable */
	INT_T pre_record_time;
}TUYA_IPC_SDK_CLOUD_STORAGE_S;

/* TUYA_APP_Enable_CloudStorage */
OPERATE_RET TUYA_APP_Enable_CloudStorage(TUYA_IPC_SDK_CLOUD_STORAGE_S *p_cloud_storage_info);

#ifdef __cplusplus
}
#endif
#endif /* __TUYA_IPC_CLOUD_STORAGE_DEMO_H__ */
