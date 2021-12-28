/*
 * tuya_ipc_p2p_demo.h 
 *  Created on: 2020年5月19日
 *      Author: 02426
 * Copyright(C),2018-2020, 涂鸦科技 www.tuya.comm
 */

#ifndef __TUYA_IPC_P2P_DEMO_H__
#define __TUYA_IPC_P2P_DEMO_H__
#include<stdbool.h>
#include "tuya_ipc_p2p.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
	BOOL_T enable; /*enable == false ,other var is invalid*/
	BOOL_T is_lowpower; /* for pre-link feature */
	UINT_T max_p2p_client; /* p2p clinet max connect number*/
    TRANS_DEFAULT_QUALITY_E live_mode;  /* for multi-streaming ipc, the default quality for live preview */
	TRANSFER_EVENT_CB transfer_event_cb; /*transfer event callback*/;
	TRANSFER_REV_AUDIO_CB rev_audio_cb;/*app to dev audio callback*/
}TUYA_IPC_SDK_P2P_S;


VOID __TUYA_APP_rev_audio_cb(IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame, IN CONST UINT_T frame_no);
INT_T __TUYA_APP_p2p_event_cb(IN CONST TRANSFER_EVENT_E event, IN CONST PVOID_T args);

/* TUYA_APP_Enable_P2PTransfer */
OPERATE_RET TUYA_APP_Enable_P2PTransfer(IN TUYA_IPC_SDK_P2P_S *p2p_infos);
#ifdef __cplusplus
}
#endif
#endif /* __TUYA_IPC_P2P_DEMO_H__ */
