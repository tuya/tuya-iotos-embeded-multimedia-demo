/*********************************************************************************
  *Copyright(C),2015-2020, TUYA company www.tuya.comm
  *FileName: tuya_ipc_system_control_demo.c
  *
  * File description：
  * The demo shows how the SDK uses callback to achieve system control, such as：
  * 1. Setting local ID
  * 2. Restart System and Restart Process
  * 3. OTA upgrade
  * 4. Sound and LED prompts.
  *
**********************************************************************************/

#include <string.h>
#include <stdio.h>
#include "tuya_ipc_media.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_cloud_types.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_system_control_demo.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
#include "tuya_ipc_api.h"
#endif

extern IPC_MGR_INFO_S s_mgr_info;
/* 
Callback when the active stat change
*/
VOID IPC_APP_status_change_cb(IN CONST GW_STATUS_E status)
{
    //todo
    return;
}

/* 
Callback when the user clicks on the APP to remove the device
*/
VOID IPC_APP_Reset_System_CB(GW_RESET_TYPE_E type)
{
    /* Developers need to restart IPC operations */
    printf("reset ipc success. please restart the ipc %d\n", type);
    IPC_APP_Notify_LED_Sound_Status_CB(IPC_RESET_SUCCESS);
    //TODO
}

VOID IPC_APP_Restart_Process_CB(VOID)
{
    printf("sdk internal restart request. please restart the ipc\n");
    //TODO
    /* Developers need to implement restart operations. Restart the process or restart the device. */
}

/* Developers need to implement the corresponding prompt sound playback and LED prompts,
   you can refer to the SDK attached files, using TUYA audio files. */
VOID IPC_APP_Notify_LED_Sound_Status_CB(IPC_APP_NOTIFY_EVENT_E notify_event)
{
    printf("curr event:%d \r\n", notify_event);
    switch (notify_event)
    {
        case IPC_BOOTUP_FINISH: /* Startup success */
        {
            break;
        }
        case IPC_START_WIFI_CFG: /* Start configuring the network */
        {
            break;
        }
        case IPC_REV_WIFI_CFG: /* Receive network configuration information */
        {
            break;
        }
        case IPC_CONNECTING_WIFI: /* Start Connecting WIFI */
        {
            break;
        }
        case IPC_MQTT_ONLINE: /* MQTT on-line */
        {
            break;
        }
        case IPC_RESET_SUCCESS: /* Reset completed */
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

/* Callback of talkback mode,turn on or off speaker hardware*/
VOID TUYA_APP_Enable_Speaker_CB(BOOL_T enabled)
{
    printf("enable speaker %d \r\n", enabled);
    //TODO
    /* Developers need to turn on or off speaker hardware operations. 
    If IPC hardware features do not need to be explicitly turned on, the function can be left blank. */
}

/* Callback of talkback mode,turn on or off the sound */
VOID TUYA_APP_Rev_Audio_CB(IN CONST MEDIA_FRAME_S *p_audio_frame,
                           TUYA_AUDIO_SAMPLE_E audio_sample,
                           TUYA_AUDIO_DATABITS_E audio_databits,
                           TUYA_AUDIO_CHANNEL_E audio_channel)
{
    printf("rev audio cb len:%u sample:%d db:%d channel:%d\r\n", p_audio_frame->size, audio_sample, audio_databits, audio_channel);
    //PCM-Format 8K 16Bit MONO
    //TODO
    /* Developers need to implement the operations of voice playback*/

}

OPERATE_RET IPC_APP_Sync_Utc_Time(VOID)
{
    TIME_T time_utc;
    INT_T time_zone;
    PR_DEBUG("Get Server Time ");
    OPERATE_RET ret = tuya_ipc_get_service_time_force(&time_utc, &time_zone);

    if(ret != OPRT_OK)
    {
        return ret;
    }
    //The API returns OK, indicating that UTC time has been successfully obtained.
    //If it return not OK, the time has not been fetched.

    PR_DEBUG("Get Server Time Success: %lu %d", time_utc, time_zone);
//  TODO
//  Setting the time using the system interface such as settimeofday;

    return OPRT_OK;
}

VOID IPC_APP_Show_OSD_Time(VOID)
{
    struct tm localTime;
    extern OPERATE_RET tuya_ipc_get_tm_with_timezone_dls(OUT struct tm *localTime);
    tuya_ipc_get_tm_with_timezone_dls(&localTime);
    PR_DEBUG("show OSD [%04d-%02d-%02d %02d:%02d:%02d]",localTime.tm_year,localTime.tm_mon,localTime.tm_mday,localTime.tm_hour,localTime.tm_min,localTime.tm_sec);
}

#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
OPERATE_RET IPC_APP_get_qrcode_info(VOID) // todo 
{
    CHAR_T info[32] = {0};
   // tuya_ipc_get_qrcode(NULL,info, 32);
    printf("###info:%s\n", info);
    return OPRT_OK;
}
#endif
#ifdef __cplusplus
}
#endif
