/*********************************************************************************
  *Copyright(C),2015-2021, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_onvif_client_demo.h
**********************************************************************************/

#ifndef _TUYA_IPC_ONVIF_CLIENT_DEMO_H
#define _TUYA_IPC_ONVIF_CLIENT_DEMO_H

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEMO_NVR_ONVIF_ENABLE
OPERATE_RET TUYA_APP_Enable_Onvif_Client(VOID);

OPERATE_RET TUYA_APP_Onvif_Probe_Start(VOID);

OPERATE_RET TUYA_APP_Onvif_Probe_Stop(VOID);

OPERATE_RET TUYA_APP_Onvif_Add_Device(INT_T device, INT_T probe_index);

OPERATE_RET TUYA_APP_Onvif_Delete_Device(INT_T device);

BOOL_T TUYA_APP_Onvif_Device_Online(VOID);

VOID TUYA_APP_Onvif_Get_Media_Info(char *devId, IPC_MEDIA_INFO_S *pMedia);

VOID TUYA_IPC_Onvif_Start_Stream(VOID);

OPERATE_RET TUYA_IPC_Onvif_PTZ_Move_Start(INT_T device, FLOAT_T velocity_pan, FLOAT_T velocity_tilt, FLOAT_T velocity_zoom);

OPERATE_RET TUYA_IPC_Onvif_PTZ_Move_Stop(INT_T device);

OPERATE_RET TUYA_IPC_Onvif_Get_PTZ_Preset(INT_T device);

OPERATE_RET TUYA_IPC_Onvif_Add_PTZ_Preset(INT_T device, CHAR_T *preset_name);

OPERATE_RET TUYA_IPC_Onvif_Set_PTZ_Preset(INT_T device, CHAR_T *preset_token, CHAR_T *preset_name);

OPERATE_RET TUYA_IPC_Onvif_Remove_PTZ_Preset(INT_T device, CHAR_T *preset_token);

OPERATE_RET TUYA_IPC_Onvif_Goto_PTZ_Preset(INT_T device, CHAR_T *preset_token);

#endif

#ifdef __cplusplus
}
#endif

#endif