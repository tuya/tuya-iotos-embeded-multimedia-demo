/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
**********************************************************************************/

#ifndef _TUYA_IPC_MGR_HANDLER_H
#define _TUYA_IPC_MGR_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_com_defs.h"

typedef enum
{
    IPC_BOOTUP_FINISH,
    IPC_START_WIFI_CFG,
    IPC_REV_WIFI_CFG,
    IPC_CONNECTING_WIFI,
    IPC_MQTT_ONLINE,
    IPC_RESET_SUCCESS,
}IPC_APP_NOTIFY_EVENT_E;

/* Update local time */
OPERATE_RET IPC_APP_Sync_Utc_Time(VOID);
VOID IPC_APP_Show_OSD_Time(VOID);

VOID IPC_APP_Reset_System_CB(GW_RESET_TYPE_E reboot_type);

VOID IPC_APP_Upgrade_Inform_cb(IN CONST FW_UG_S *fw);

VOID IPC_APP_Restart_Process_CB(VOID);

VOID IPC_APP_Upgrade_Firmware_CB(VOID);

VOID IPC_APP_Notify_LED_Sound_Status_CB(IPC_APP_NOTIFY_EVENT_E notify_event);

#ifdef __cplusplus
}
#endif

#endif  /*_TUYA_IPC_MGR_HANDLER_H*/
