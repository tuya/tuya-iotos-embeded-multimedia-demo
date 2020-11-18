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

extern IPC_MGR_INFO_S s_mgr_info;

/* 
Callback when the user clicks on the APP to remove the device
*/
VOID IPC_APP_Reset_System_CB(GW_RESET_TYPE_E type)
{
    printf("reset ipc success. please restart the ipc %d\n", type);
    IPC_APP_Notify_LED_Sound_Status_CB(IPC_RESET_SUCCESS);
    //TODO
    /* Developers need to restart IPC operations */
}

VOID IPC_APP_Restart_Process_CB(VOID)
{
    printf("sdk internal restart request. please restart the ipc\n");
    //TODO
    /* Developers need to implement restart operations. Restart the process or restart the device. */
}

/* OTA */
//Callback after downloading OTA files
VOID __IPC_APP_upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{
    FILE *p_upgrade_fd = (FILE *)pri_data;
    fclose(p_upgrade_fd);

    PR_DEBUG("Upgrade Finish");
    PR_DEBUG("download_result:%d fw_url:%s", download_result, fw->fw_url);

    if(download_result == 0)
    {
        /* The developer needs to implement the operation of OTA upgrade, 
        when the OTA file has been downloaded successfully to the specified path. [ p_mgr_info->upgrade_file_path ]*/
    }
    //TODO
    //reboot system
}

//To collect OTA files in fragments and write them to local files
OPERATE_RET __IPC_APP_get_file_data_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len,IN CONST UINT_T offset,
                             IN CONST BYTE_T *data,IN CONST UINT_T len,OUT UINT_T *remain_len, IN PVOID_T pri_data)
{
    PR_DEBUG("Rev File Data");
    PR_DEBUG("total_len:%d  fw_url:%s", total_len, fw->fw_url);
    PR_DEBUG("Offset:%d Len:%d", offset, len);

    //report UPGRADE process, NOT only download percent, consider flash-write time
    //APP will report overtime fail, if uprgade process is not updated within 60 seconds

    int download_percent = (offset * 100) / (total_len+1);
    int report_percent = download_percent/2; // as an example, download 100% = 50%  upgrade work finished
    tuya_ipc_upgrade_progress_report(report_percent);

    if(offset == total_len) // finished downloading
    {
        //start write OTA file to flash by parts
        /* only for example: 
        FILE *p_upgrade_fd = (FILE *)pri_data;
        fwrite(data, 1, len, p_upgrade_fd);
        *remain_len = 0;
        */
        // finish 1st part
        report_percent+=10;
        tuya_ipc_upgrade_progress_report(report_percent);
        // finish 2nd part
        sleep(5);
        report_percent+=10;
        tuya_ipc_upgrade_progress_report(report_percent);
        // finish all parts, set to 90% for example
        report_percent = 90;
        tuya_ipc_upgrade_progress_report(report_percent);
    }

    //APP will report "uprage success" after reboot and new FW version is reported inside SDK automaticlly

    return OPRT_OK;
}

VOID IPC_APP_Upgrade_Inform_cb(IN CONST FW_UG_S *fw)
{
    PR_DEBUG("Rev Upgrade Info");
    PR_DEBUG("fw->fw_url:%s", fw->fw_url);
    PR_DEBUG("fw->fw_md5:%s", fw->fw_md5);
    PR_DEBUG("fw->sw_ver:%s", fw->sw_ver);
    PR_DEBUG("fw->file_size:%u", fw->file_size);

    FILE *p_upgrade_fd = fopen(s_mgr_info.upgrade_file_path, "w+b");
    tuya_ipc_upgrade_sdk(fw, __IPC_APP_get_file_data_cb, __IPC_APP_upgrade_notify_cb, p_upgrade_fd);
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
    return OPRT_OK;
}

VOID IPC_APP_Show_OSD_Time(VOID)
{
    struct tm localTime;
    tuya_ipc_get_tm_with_timezone_dls(&localTime);
    PR_DEBUG("show OSD [%04d-%02d-%02d %02d:%02d:%02d]",localTime.tm_year,localTime.tm_mon,localTime.tm_mday,localTime.tm_hour,localTime.tm_min,localTime.tm_sec);
}


