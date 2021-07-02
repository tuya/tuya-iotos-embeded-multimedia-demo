/*
 * tuya_ipc_p2p_demo.h 
 *  Created on: 2020年5月19日
 *      Author: 02426
 * Copyright(C),2018-2020, 涂鸦科技 www.tuya.comm
 */

#ifndef DEMOS_DEMO_TUYA_IPC_INCLUDE_TUYA_IPC_P2P_DEMO_H_
#define DEMOS_DEMO_TUYA_IPC_INCLUDE_TUYA_IPC_P2P_DEMO_H_
#include<stdbool.h>
VOID __TUYA_APP_rev_audio_cb(IN CONST TRANSFER_AUDIO_FRAME_S *p_audio_frame, IN CONST UINT_T frame_no);
VOID __TUYA_APP_p2p_event_cb(IN CONST TRANSFER_EVENT_E event, IN CONST PVOID_T args);


#endif /* DEMOS_DEMO_TUYA_IPC_INCLUDE_TUYA_IPC_P2P_DEMO_H_ */
