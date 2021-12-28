/*
 * tuya_ipc_upgrade_demo.h
 *Copyright(C),2017-2022, TUYA company www.tuya.comm
 *
 *FILE description:
  *
 *  Created on: 2020年12月28日
 *      Author: kuiba
 */

#ifndef __TUYA_IPC_UPGRADE_DEMO_H__
#define __TUYA_IPC_UPGRADE_DEMO_H__

#include<stdbool.h>
#include "tuya_ipc_sdk_init.h"

#ifdef __cplusplus
extern "C" {
#endif

INT_T IPC_APP_Upgrade_Inform_cb(IN CONST FW_UG_S *fw);


#ifdef __cplusplus
}
#endif
#endif /* __TUYA_IPC_UPGRADE_DEMO_H__ */
