/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
**********************************************************************************/

#ifndef _TUYA_IPC_UPGRADE_DEMO_H
#define _TUYA_IPC_UPGRADE_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_com_defs.h"

INT_T IPC_APP_Upgrade_Inform_cb(IN CONST FW_UG_S *fw);

VOID IPC_APP_Upgrade_Firmware_CB(VOID);

INT_T ty_gw_dev_ug_inform(IN CONST CHAR_T *pdevId, IN CONST FW_UG_S* pfw);

#ifdef __cplusplus
}
#endif

#endif  /*_TUYA_IPC_MGR_HANDLER_H*/

