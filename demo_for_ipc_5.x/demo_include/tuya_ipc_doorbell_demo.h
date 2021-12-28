/*
 * tuya_ipc_doorbell_demo.h
 *Copyright(C),2017-2022, TUYA company www.tuya.comm
 *
 *FILE description:
  *
 *  Created on: 2021年12月23日
 *      Author: dante
 */

#ifndef __TUYA_DOORBELL_DEMO_H_
#define __TUYA_DOORBELL_DEMO_H_
#include <stdio.h>

#include "tuya_ipc_video_msg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
	BOOL_T enable; /*enable == false ,other var is invalid*/
	MESSAGE_E type; /* video msg type: video+audio, video only, audio only*/
	INT_T msg_duration; /* the duration of video record */
}TUYA_IPC_SDK_VIDEO_MSG_S;

/* 模拟按键响应 */
VOID doorbell_handler();

/* 初始化sdk视频留言模块，实现门铃及留言功能 */
OPERATE_RET TUYA_APP_Enable_Video_Msg(TUYA_IPC_SDK_VIDEO_MSG_S* p_video_msg_info);

#ifdef __cplusplus
}
#endif

#endif
