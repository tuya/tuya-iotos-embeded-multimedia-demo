/*
 * tuya_ipc_motion_detect_demo.h
 *Copyright(C),2017-2022, TUYA company www.tuya.comm
 *
 *FILE description:
  *
 *  Created on: 2021年12月23日
 *      Author: dante
 */

#ifndef __TUYA_MD_DEMO_H_
#define __TUYA_MD_DEMO_H_
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
    motion detect 
*/
OPERATE_RET TUYA_APP_Enable_Motion_Detect();

VOID IPC_APP_set_motion_status(int status);

OPERATE_RET TUYA_APP_Enable_AI_Detect();

#ifdef __cplusplus
}
#endif
#endif
