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
#include <sys/statfs.h>
#include <unistd.h>
#include <dirent.h>

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
extern int g_demo_sd_status ;

//Implementation Interface for Obtaining SD Card Status
E_SD_STATUS tuya_ipc_sd_get_status(VOID)
{
     return g_demo_sd_status;
	 //return SD_STATUS_NORMAL;

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
#if 0
    *p_total = 0;
    *p_used = 0;
    *p_free =  *p_total  - *p_used;
    return ;
#endif

    *p_total = 500*1024;//500M
    *p_used = 1024*20;
  // *p_used = 480*1024; // to triggle del file
    *p_free =  *p_total  - *p_used;
    return ;

#if 0
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
#endif
}

//get the path of mounting sdcard
CHAR_T *tuya_ipc_get_sd_mount_path(VOID)
{
    return SD_MOUNT_PATH;
}

//The maximum number of events per day, exceeding this value, there will be an exception when playback and can not play.
//Too much setting of this value will affect the query efficiency
#define MAX_EVENT_NUM_PER_DAY   (500)
extern IPC_MEDIA_INFO_S s_media_info[IPC_CHAN_NUM];

OPERATE_RET TUYA_APP_Init_Stream_Storage(IN CONST CHAR_T *p_sd_base_path)
{
    STATIC BOOL_T s_stream_storage_inited = FALSE;
    if(s_stream_storage_inited == TRUE)
    {
        PR_DEBUG("The Stream Storage Is Already Inited");
        return OPRT_OK;
    }
    printf_media_info(&s_media_info[0]);
    printf_media_info(&s_media_info[1]);

    PR_DEBUG("Init Stream_Storage SD:%s\n\n\n", p_sd_base_path);
    
  //  printf("###%p  %p  %p size%d\n" ,s_media_info, &s_media_info[0], &s_media_info[1],  sizeof(IPC_MEDIA_INFO_S));
    TUYA_IPC_STORAGE_VAR_S strStorage_var = {0};
    strStorage_var.base_path = p_sd_base_path;
    strStorage_var.media_setting = (IPC_MEDIA_INFO_S* )&s_media_info[0];
    strStorage_var.max_event_per_day =MAX_EVENT_NUM_PER_DAY;
    strStorage_var.aes_func = NULL;

    strStorage_var.album_info.cnt =IPC_CHAN_NUM;
    memcpy(&strStorage_var.album_info.albumName[0], TUYA_IPC_ALBUM_EMERAGE_FILE,strlen(TUYA_IPC_ALBUM_EMERAGE_FILE));
    memcpy(&strStorage_var.album_info.albumName[1], "testAlbum",strlen("testAlbum"));
    OPERATE_RET ret = tuya_ipc_ss_init(&strStorage_var);
    if(ret != OPRT_OK)
    {
        PR_ERR("Init Main Video Stream_Storage Fail. %d", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

#define ABS_PATH_LEN 512
int ty_cmd_exe(char* srccmd)
{
    return system(srccmd);
#if 0
CHAR_T cmd[ABS_PATH_LEN];
    FILE *pp = NULL;

    if(!srccmd){
        printf("pointer null\n");
        return -1;
    }
    
    memset(cmd,0,SIZEOF(cmd));
    memcpy(cmd, srccmd, strlen(srccmd));
    //printf("execute: %s\n\n",cmd);
    pp = popen(cmd, "r");
    if(NULL == pp)
    {
        printf("cmd: %s failed\n", cmd);
        return -1;
    }
    pclose(pp);
    pp=NULL;
#endif
}

extern CHAR_T s_raw_path[128];

static char s_test_pic_thumbNail[ABS_PATH_LEN] = "thumbnail/pic_thumbnail.jpg";
static char s_test_mp4_thumbNail[ABS_PATH_LEN] = "thumbnail/mp4_thumbnail.jpg";
static char s_raw_emergency_path[ABS_PATH_LEN] = { 0 };
static char s_raw_emergency_thumbnail[ABS_PATH_LEN] = { 0 };
static char s_emergency_path[ABS_PATH_LEN] = { 0 };
static char s_emergency_thunbnail[ABS_PATH_LEN] = { 0 };

void album_jpg_file_put(char *jpgFile)
{
    CHAR_T cmd[ABS_PATH_LEN];
    int curtime = uni_time_get_posix();
    
    //init target file path
    char targetFile[ABS_PATH_LEN] = { 0 };
    memset(targetFile, 0, sizeof(targetFile));
    sprintf(targetFile, "%d_ch1.jpg", curtime);
    
    //cp pic and thumbnail to emergency album
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cp %s/%s %s/%s -f", s_raw_emergency_path, jpgFile, s_emergency_path, targetFile);//TODO put jpg into the path
    ty_cmd_exe(cmd);
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cp %s/%s %s/%s -f", s_raw_emergency_path, s_test_pic_thumbNail, s_emergency_thunbnail, targetFile);//TODO put thumbnail into the path
    ty_cmd_exe(cmd);

    // write file info
    ALBUM_FILE_INFO strInfo;
    memset(&strInfo, 0, sizeof(ALBUM_FILE_INFO));
    strInfo.channel = 0;
    strInfo.type = SS_DATA_TYPE_PIC;
    memcpy(&strInfo.filename, targetFile, strlen(targetFile));
    strInfo.createTime = curtime;
    strInfo.duration = 0;
    tuya_ipc_album_write_file_info(TUYA_IPC_ALBUM_EMERAGE_FILE, &strInfo);

    return;
}

void album_mp4_file_put(char *mp4File)
{
    CHAR_T cmd[ABS_PATH_LEN];
    int curtime = uni_time_get_posix();

    //init target file path
    char targetFile[ABS_PATH_LEN] = { 0 };
    memset(targetFile, 0, sizeof(targetFile));
    sprintf(targetFile, "%d_ch2.mp4", curtime);
    char mp4Pic[ABS_PATH_LEN] = { 0 };
    memset(mp4Pic, 0, sizeof(mp4Pic));
    sprintf(mp4Pic, "%d_ch2.jpg", curtime);
    
    // cp video and thumbnail to emergency album
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cp %s/%s %s/%s -f", s_raw_emergency_path, mp4File, s_emergency_path, targetFile);//TODO put mp4 into the path
    ty_cmd_exe(cmd);
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cp %s/%s %s/%s -f", s_raw_emergency_path, s_test_mp4_thumbNail, s_emergency_thunbnail, mp4Pic);//TODO put thumbnail into the path
    ty_cmd_exe(cmd);
    
    // write file info
    ALBUM_FILE_INFO strInfo;
    memset(&strInfo, 0, sizeof(ALBUM_FILE_INFO));
    strInfo.channel = 1;
    strInfo.type = SS_DATA_TYPE_VIDOE;
    memcpy(&strInfo.filename, targetFile, strlen(targetFile));
    strInfo.createTime = curtime;
    char* p = mp4File + strlen("1234567890_");
    strInfo.duration = atoi(p);
    tuya_ipc_album_write_file_info(TUYA_IPC_ALBUM_EMERAGE_FILE, &strInfo);

    return;
}

/*
  本地相册文件名要求；
  ipc_emergency_record/
        A.mp4
        B.jpg
        thumbnail/
              A.jpg
              B.jpg
  缩略图文件名和源文件文件名必须相同(除后缀外);
  源文件的文件名不可重名;

  本地相册列表交互ver1:
  1.user获取某相册路径、某相册缩略图路径，
  2.user在相册路径写入源数据（mp4/图片）
  3.user在缩略图路径写入对应缩略图
  4.user通过tuya_ipc_album_write_file_info，更新文件信息
*/

/*
  存放紧急相册文件(jpg/mp4文件)
*/
void album_file_put()
{
    static int emerge_album_path_inited = 0;
    if (0 == emerge_album_path_inited){
        emerge_album_path_inited =1 ;
        tuya_ipc_album_get_path(TUYA_IPC_ALBUM_EMERAGE_FILE, s_emergency_path, s_emergency_thunbnail);
        printf("get album file path %s \nthumbNail %s\n", s_emergency_path, s_emergency_thunbnail);
    }
    CHAR_T cmd[ABS_PATH_LEN];
    
    sprintf(s_raw_emergency_path, "%s/resource/ipc_emergency_record/", s_raw_path);
    sprintf(s_raw_emergency_thumbnail, "%s/resource/ipc_emergency_record/thumbnail", s_raw_path);

    char* testSrcFile;
    DIR* p_root_dir = NULL;
    struct dirent* p_child_dir = NULL;
    /* copy mp4/jpg from src file to album path, and write file info */
    p_root_dir = opendir(s_raw_emergency_path);
    if (p_root_dir) {
        while ((p_child_dir = readdir(p_root_dir)) != NULL) {
            if (strcmp(p_child_dir->d_name, ".") == 0
                || strcmp(p_child_dir->d_name, "..") == 0) {
                continue;
            }
            sleep(2);//控制创建速度
            int curtime = uni_time_get_posix();
            testSrcFile = p_child_dir->d_name;
            if (strstr(testSrcFile, "jpg")) {
                album_jpg_file_put(testSrcFile);
            } else if (strstr(testSrcFile, "mp4")) {
                album_mp4_file_put(testSrcFile);
            }
        }
        closedir(p_root_dir);
    }
}

