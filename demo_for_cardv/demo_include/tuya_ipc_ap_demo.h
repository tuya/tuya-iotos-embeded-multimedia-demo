/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_media_demo.h
**********************************************************************************/
#ifndef _TUYA_IPC_AP_DEMO_H
#define _TUYA_IPC_AP_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif


#define WWM_LOWPOWER   0   ///< wifi work in lowpower mode
#define WWM_SNIFFER 1        ///< wifi work in sniffer mode
#define WWM_STATION 2        ///< wifi work in station mode
#define WWM_SOFTAP   3         ///< wifi work in ap mode
#define  WWM_STATIONAP 4      ///< wifi work in station+ap mode

void usr_set_net_ap();

OPERATE_RET user_wifi_get_mac(INOUT NW_MAC_S *mac);

int user_wifi_get_work_mode(int* mode);


#ifdef __cplusplus
}
#endif

#endif  /* _TUYA_IPC_AP_DEMO_H */
