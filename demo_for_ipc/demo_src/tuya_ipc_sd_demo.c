/*********************************************************************************
  *Copyright(C),2015-2020, TUYA www.tuya.comm
  *FileName:    tuya_ipc_sd_demo
**********************************************************************************/
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "tuya_ipc_api.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_common_demo.h"

/************************
Description: Using the TUYA SD card storage and playback function, 
the developers need to implement the relevant interface.
Note：Interface names cannot be changed, and these interfaces are declared and used in SDK.
This demo file gives the management implementation of SD card operation for typical embedded devices in Linux system.
Developers can modify it according to their practical application.

VOID tuya_ipc_sd_format(VOID);
VOID tuya_ipc_sd_remount(VOID);
E_SD_STATUS tuya_ipc_sd_get_status(VOID);
VOID tuya_ipc_sd_get_capacity(UINT_T *p_total, UINT_T *p_used, UINT_T *p_free);
CHAR_T *tuya_ipc_get_sd_mount_path(VOID);

************************/
#define MAX_MMC_NAME_LEN        16
#define MAX_MOUNTS_INFO_LEN     1024
#define LINUX_SD_DEV_FILE       "/dev/mmcblk0"
#define LINUX_MOUNT_INFO_FILE   "/proc/mounts"
#define SD_MOUNT_PATH           "/mnt/sdcard"
#define FORMAT_CMD              "mkfs.vfat"

STATIC CHAR_T s_mounts_info[MAX_MOUNTS_INFO_LEN];
STATIC CHAR_T s_mmcblk_name[MAX_MMC_NAME_LEN] = {0};

#define TYDEBUG printf
#define TYERROR printf

//Implementation Interface of Formatting Operation
VOID tuya_ipc_sd_format(VOID)
{
    CHAR_T format_cmd[256] = {0};
    char buffer[512] = {0};
    TYDEBUG("sd format begin\n");

    snprintf(format_cmd,256,"umount %s;%s %s;mkdir -p /mnt/sdcard;mount -t auto %s /mnt/sdcard;",s_mmcblk_name,FORMAT_CMD,s_mmcblk_name,s_mmcblk_name);
    TYDEBUG("execute: %s\n",format_cmd);
    FILE *pp = popen(format_cmd, "r");
    if(NULL != pp)
    {
        fgets(buffer,sizeof(buffer),pp);
        printf("%s\n",buffer);
        pclose(pp);
    }
    else
    {
        TYDEBUG("format_sd_card failed\n");
    }
    TYDEBUG("sd format end\n");
}

//Implementation Interface for Remounting
VOID tuya_ipc_sd_remount(VOID)
{
    CHAR_T format_cmd[128] = {0};
    char buffer[512] = {0};
    E_SD_STATUS status = SD_STATUS_UNKNOWN;

    status = tuya_ipc_sd_get_status();
    if(SD_STATUS_NORMAL == status)
    {
        TYDEBUG("sd don't need to remount!\n");
        return;
    }
    TYDEBUG("remount_sd_card ..... \n");

    snprintf(format_cmd,128,"umount %s;sleep 1;mount -t auto %s /mnt/sdcard;",s_mmcblk_name,s_mmcblk_name);
    FILE *pp = popen(format_cmd, "r");
    if(NULL != pp)
    {
        fgets(buffer,sizeof(buffer),pp);
        printf("%s\n",buffer);
        pclose(pp);
    }
    else
    {
        printf("remount_sd_card failed\n");
    }
}

STREAM_STORAGE_WRITE_MODE_E tuya_ipc_sd_get_mode_config(VOID)
{
    BOOL_T sd_on = IPC_APP_get_sd_record_onoff();
    if(sd_on)
    {
        UINT_T sd_mode = IPC_APP_get_sd_record_mode();
        if(0 == sd_mode)
        {
            return SS_WRITE_MODE_EVENT;
        }
        else
        {
            return SS_WRITE_MODE_ALL;
        }
    }
    else
    {
        return SS_WRITE_MODE_NONE;
    }
}

//Implementation Interface for Obtaining SD Card Status
E_SD_STATUS tuya_ipc_sd_get_status(VOID)
{
    FILE *fp = fopen(LINUX_SD_DEV_FILE, "rb");
    if(!fp)
    {
        return SD_STATUS_NOT_EXIST;
    }
    fclose(fp);

    strcpy(s_mmcblk_name,LINUX_SD_DEV_FILE);
    if(0 == access(LINUX_SD_DEV_FILE"p1",F_OK)) //Default node name information
    {
        strcat(s_mmcblk_name,"p1");
    }
    
    fp = fopen(LINUX_MOUNT_INFO_FILE, "rb");
    if(fp)
    {
        memset(s_mounts_info,0,sizeof(s_mounts_info));
        fread(s_mounts_info,1,MAX_MOUNTS_INFO_LEN,fp);
        fclose(fp);
        CHAR_T *mmcblk_name_start = strstr(s_mounts_info,"/dev/mmcblk"); //Confirm the final node name in Mount information
        CHAR_T *mmcblk_name_end = strstr(s_mounts_info," /mnt/sdcard");
        if(mmcblk_name_start && mmcblk_name_end)
        {
            int mmcblk_name_len = mmcblk_name_end-mmcblk_name_start;
            if(mmcblk_name_len >= MAX_MMC_NAME_LEN)
            {
                 return SD_STATUS_ABNORMAL;
            }
            strncpy(s_mmcblk_name, mmcblk_name_start, mmcblk_name_len);
            s_mmcblk_name[mmcblk_name_len] = '\0';
        }
        //There are device nodes but no mount information. Generally, the card format is incorrect and report abnormal.
        else 
        {
            return SD_STATUS_ABNORMAL;
        } 
        //If the mount information of the SD card is not at the end and there is a ro mount behind it, there will be a problem.
        if(NULL != strstr(mmcblk_name_start,"ro,")) 
        {
            return SD_STATUS_ABNORMAL;
        }
        if(NULL == strstr(mmcblk_name_start,"vfat"))
        {
            return SD_STATUS_ABNORMAL;
        }
        if (access(s_mmcblk_name,0))
        {
            return SD_STATUS_ABNORMAL;
        }

        return SD_STATUS_NORMAL;
    }
    else
    {
        return SD_STATUS_UNKNOWN;
    }
}

//SD card capacity acquisition interface, unit: KB
VOID tuya_ipc_sd_get_capacity(UINT_T *p_total, UINT_T *p_used, UINT_T *p_free)
{
    *p_total = 0;
    *p_used = 0;
    *p_free = 0;

    struct statfs sd_fs;
    if (statfs("/mnt/sdcard", &sd_fs) != 0)
    {  
        TYERROR("statfs failed!/n");
        return;
    }

    *p_total = (UINT_T)(((UINT64_T)sd_fs.f_blocks * (UINT64_T)sd_fs.f_bsize) >> 10);
    *p_used = (UINT_T)((((UINT64_T)sd_fs.f_blocks - (UINT64_T)sd_fs.f_bfree) * (UINT64_T)sd_fs.f_bsize) >> 10);
    *p_free = (UINT_T)(((UINT64_T)sd_fs.f_bavail * (UINT64_T)sd_fs.f_bsize) >> 10);
    TYDEBUG("sd capacity: total: %d KB, used %d KB, free %d KB\n",*p_total,*p_used,*p_free);
    return;
}

//get the path of mounting sdcard
CHAR_T *tuya_ipc_get_sd_mount_path(VOID)
{
    return SD_MOUNT_PATH;
}

//The maximum number of events per day, exceeding this value, there will be an exception when playback and can not play.
//Too much setting of this value will affect the query efficiency
#define MAX_EVENT_NUM_PER_DAY   (500)
extern IPC_MEDIA_INFO_S s_media_info;

OPERATE_RET TUYA_APP_Init_Stream_Storage(IN CONST CHAR_T *p_sd_base_path)
{
    STATIC BOOL_T s_stream_storage_inited = FALSE;
    if(s_stream_storage_inited == TRUE)
    {
        PR_DEBUG("The Stream Storage Is Already Inited");
        return OPRT_OK;
    }

    PR_DEBUG("Init Stream_Storage SD:%s", p_sd_base_path);
    OPERATE_RET ret = tuya_ipc_ss_init(p_sd_base_path, &s_media_info,MAX_EVENT_NUM_PER_DAY, NULL);
    if(ret != OPRT_OK)
    {
        PR_ERR("Init Main Video Stream_Storage Fail. %d", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}


