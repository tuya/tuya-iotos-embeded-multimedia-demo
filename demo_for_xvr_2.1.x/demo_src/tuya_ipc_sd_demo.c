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
#include <unistd.h>
#include <fcntl.h> 
#include <errno.h>

#include "tuya_hdd_proc_demo.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_multi_storage.h"
#include "tuya_ipc_common_demo.h"
#if   defined(__USER_DO_NOT_OPEN__) && (__USER_DO_NOT_OPEN__ == 1)
#ifdef __cplusplus
extern "C" {
#endif

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
#define LINUX_SD_DEV_FILE       "/dev/loop0"
#define LINUX_MOUNT_INFO_FILE   "/proc/mounts"
#define SD_MOUNT_PATH           "/tmp" // keep same to IPC_APP_SD_BASE_PATH
#define FORMAT_CMD              "mkfs.vfat"

STATIC CHAR_T s_mounts_info[MAX_MOUNTS_INFO_LEN];
STATIC CHAR_T s_mmcblk_name[MAX_MMC_NAME_LEN] = {0};



INT_T TUYA_hd_open(IN CONST BYTE_T * pHdName, IN CONST UINT_T fileType, IN CONST UINT_T fileNo, IN CONST INT_T mode, PVOID_T attr)
{
    if (NULL == pHdName){
        return -1;
    }

    //其他参数后续完善使用
    return open((char *)pHdName,O_SYNC|O_RDWR);    
}
INT_T TUYA_hd_write(IN CONST PVOID_T hdHandle,IN CONST UINT64_T uOffset, IN CONST BYTE_T * pBuff, IN CONST UINT_T writeLen, PVOID_T attr)
{
    if (NULL == hdHandle || NULL == pBuff){
        return -1;
    }

    int len = 0;
    int fd = *(int *)hdHandle;
    lseek(fd, uOffset, SEEK_SET);
    len = write(fd, pBuff, writeLen);
    if (len != writeLen){
        printf("%s %d write failed len[%u] [%u]\n",__FUNCTION__,__LINE__,writeLen,len);
        perror("write error");
        return -1;
    }

    return 0;
}
INT_T TUYA_hd_read(IN CONST PVOID_T hdHandle,IN CONST UINT64_T uOffset, IN BYTE_T * pBuff, IN CONST UINT_T readLen, PVOID_T attr)
{
    if (NULL == hdHandle || NULL == pBuff){
        return -1;
    }

    int len = 0;
    int fd = *(int *)hdHandle;
    //printf("%s %d fd[%d] offset[%llu-%u]\n",__FUNCTION__,__LINE__,fd,uOffset, readLen);
    lseek(fd, uOffset, SEEK_SET);
    len = read(fd, pBuff, readLen);
    if (len < readLen){
        printf("%s %d read failed len[%u] [%u]\n",__FUNCTION__,__LINE__,readLen,len);
        perror("read error");
        return -1;
    }

    return 0;
}
INT_T TUYA_hd_seek(IN CONST PVOID_T hdHandle,IN CONST UINT64_T uOffset, IN CONST INT_T whence, PVOID_T attr)
{
    if (NULL == hdHandle){
        return -1;
    }

    int fd = *(int *)hdHandle;
    lseek(fd, uOffset, whence);

    return 0;
}
INT_T TUYA_hd_flush(IN CONST PVOID_T hdHandle,PVOID_T attr)
{
    if (NULL == hdHandle){
        return -1;
    }
    return fsync(*(INT_T *)hdHandle);
}
INT_T TUYA_hd_close(IN CONST PVOID_T hdHandle,PVOID_T attr)
{
    if (NULL == hdHandle){
        return -1;
    }
    return close(*(INT_T *)hdHandle);
}

//Implementation Interface of Formatting Operation
VOID tuya_ipc_sd_format(VOID)
{
    CHAR_T format_cmd[256] = {0};
    char buffer[512] = {0};
    PR_DEBUG("sd format begin\n");

    snprintf(format_cmd,256,"umount %s;%s %s;mkdir -p /mnt/sdcard;mount -t auto %s /mnt/sdcard;",s_mmcblk_name,FORMAT_CMD,s_mmcblk_name,s_mmcblk_name);
    PR_DEBUG("execute: %s\n",format_cmd);
    FILE *pp = popen(format_cmd, "r");
    if(NULL != pp)
    {
        fgets(buffer,sizeof(buffer),pp);
        printf("%s\n",buffer);
        pclose(pp);
    }
    else
    {
        PR_DEBUG("format_sd_card failed\n");
    }
    PR_DEBUG("sd format end\n");
}

//Implementation Interface for Remounting
VOID tuya_ipc_sd_remount(VOID)
{
    //stop all dev
    //tuya_ipc_stor_stop(NULL);
    tuya_ipc_stor_uninit();
    //do unmount
    return ;
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
        PR_ERR("statfs failed!/n");
        return;
    }

    *p_total = (UINT_T)(((UINT64_T)sd_fs.f_blocks * (UINT64_T)sd_fs.f_bsize) >> 10);
    *p_used = (UINT_T)((((UINT64_T)sd_fs.f_blocks - (UINT64_T)sd_fs.f_bfree) * (UINT64_T)sd_fs.f_bsize) >> 10);
    *p_free = (UINT_T)(((UINT64_T)sd_fs.f_bavail * (UINT64_T)sd_fs.f_bsize) >> 10);
    PR_DEBUG("sd capacity: total: %d KB, used %d KB, free %d KB\n",*p_total,*p_used,*p_free);
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

OPERATE_RET TUYA_APP_Init_Stream_Storage()
{
    return ty_hdd_init(0);
}
#ifdef __cplusplus
}
#endif
#endif
