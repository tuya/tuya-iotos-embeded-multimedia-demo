/***********************************************************
*  File: ty_hdd_proc.c
*  Author: clb
*  Date: 20190521
*  Note:本地存储接入和其他触发逻辑在此处理
**************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h> 
#include <stdio.h> 

#include <sys/vfs.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#include "tuya_hdd_file_operate_demo.h"
#include "tuya_ipc_multi_storage.h"
#include "tuya_ipc_dp_utils.h"

#include "tuya_ipc_common_demo.h"
#if defined(__USER_DO_NOT_OPEN__) && (__USER_DO_NOT_OPEN__ == 1)
#define HDD_START_OFFSET    (64*1024*1024)  //默认起始偏移// todo zxq
#define SUPPORT_HDD_NUM     (2) //test one sdb1
static int s_inited = 0;
typedef struct {
    int enable;
    char devPath[STOR_HD_DEVICE_LEN];
}HDD_DEV_INFO_T;
static HDD_DEV_INFO_T s_hdd_dev[SUPPORT_HDD_NUM] = {
    {.devPath = "/dev/loop0"},
     {.devPath = "/dev/loop0"}
 };

static STOR_MEDIA_STAT_E s_hdd_stat = E_STOR_MEDIA_NORMAL;

INT_T __format_cb(INT_T percent)
{
    ty_dp_format_percent(percent);

    return 0;
}

void __update_hdd_device_info()
{
    int i = 0;

    for(i = 0; i < SUPPORT_HDD_NUM; i++){
        memset(s_hdd_dev[i].devPath, 0x00, STOR_HD_DEVICE_LEN);
    }
    strncpy(s_hdd_dev[0].devPath, "/dev/mmcblk0", STOR_HD_DEVICE_LEN);
   // strncpy(s_hdd_dev[1].devPath, "/dev/sda1", STOR_HD_DEVICE_LEN);

    return ;
}

#if 1

int __find_proc_str(char *dev_str)
{
	char *p = NULL;
	char line_buf[128];
	int cnt = 0;
	FILE *pf = fopen("/proc/partitions", "rb");
	if (NULL == pf)
	{
		perror("fopen");
		return -1;
	}

	memset(line_buf, 0, sizeof(line_buf));
	while(fgets(line_buf, sizeof(line_buf)-1, pf))
	{
		//printf(" %s, %s\n", mmcblock, line_buf);
		p = strstr(line_buf, dev_str);
		if (NULL != p)
		{
			//printf("p = %s, %s\n", p, line_buf);
			if (pf)
			{
				fclose(pf);
			}
			return 0;
		}
		memset(line_buf, 0, sizeof(line_buf));
		if (cnt++ > 100)
		{
			break;
		}
	}

	if (pf)
	{
		fclose(pf);
	}
	return -2;
}

void __get_device_volume(CHAR_T * dev,UINT64_T * pTotol, UINT64_T * pStartOff)
{
    //find block name 
    int fd;
    int found_ret;
    int i = 0;
    UINT64_T size = 0;
    
    *pStartOff = HDD_START_OFFSET;
    for (i = strlen(dev); i >0 ; i--){
        if('/' == dev[i]){
            break;
        }
    }
    TYDEBUG("block name [%s]\n",&dev[i+1]);

    found_ret = __find_proc_str(&dev[i+1]);
    if (found_ret != 0)
    {
        *pTotol = 0;
        return;
    }

    if ((fd = open(dev, O_RDONLY)) < 0)
    {
        perror("");
        printf("open error %d %s\n", errno, dev);
        *pTotol = 0;
        close(fd);
        return;
    }
     
    if ((ioctl(fd, BLKGETSIZE64, &size)) < 0)
    {
        perror("");
        printf("ioctl error %d\n", errno);
        *pTotol = 0;
        close(fd);
        return;
    }
    close(fd);

    if (size > 0)
    {
        *pTotol = size;
    }
    else
    {
        *pTotol = 0;
    }
    *pStartOff = HDD_START_OFFSET;

    return ;
}
#else
void __get_device_volume(CHAR_T * dev,UINT64_T * pTotol, UINT64_T * pStartOff)
{
    int volume = 0;
    //find block name 
    int i = 0;
    for (i = strlen(dev); i >0 ; i--){
        if('/' == dev[i]){
            break;
        }
    }
    TYDEBUG("block name [%s]\n",&dev[i+1]);
    char cmd[256];
    char result[32];
    int len = 32;
    memset(cmd , 0x00, 256);
    memset(result , 0x00, 32);
    snprintf(cmd , 256, "cat /proc/partitions | grep %s | awk '{print $3}'",&dev[i+1]);
    exec_cmd((const char *)cmd, result, &len);
    volume = atoi(result);
    TYDEBUG("volume totol [%d]\n",volume);
    *pTotol = (UINT64_T)volume*1024;
    *pStartOff = HDD_START_OFFSET;
    return ;
}
#endif

VOID __stor_get_media(IN CONST CHAR_T * devId,IN OUT IPC_MEDIA_INFO_S *param)
{
    IPC_APP_Get_Media_Info(0,(char *)devId, param);
}

int ty_hdd_set_dev_name(int type, char * dev_name)
{
    if (type >= SUPPORT_HDD_NUM){
        TYERROR("input type[%d] error\n",type);
        return -1;
    }

    snprintf(s_hdd_dev[type].devPath,STOR_HD_DEVICE_LEN,"/dev/%s",dev_name);
    TYDEBUG("set devPath[%s]\n",s_hdd_dev[type].devPath);

    return 0;
}

void ty_stor_media_stat_report_unormal()
{
    int val = 2;
    tuya_ipc_dp_report(NULL, TUYA_DP_STOR_STATUS,PROP_VALUE,&val,1);
}

void ty_stor_media_stat_report_normal()
{
    int val = 1;
    tuya_ipc_dp_report(NULL, TUYA_DP_STOR_STATUS,PROP_VALUE,&val,1);
}


int ty_hdd_init(int type) // type 0 sd, 1 hd
{
    TYDEBUG("__begin\n");
    STOR_HD_INIT_INFO_T strHdInfo;
    OPERATE_RET ret;

    if (1 == s_inited){
        TYERROR("hdd inited\n");
        return -1;
    }
    //tuya_ipc_stor_statu_cb(tuya_ipc_stor_media_stat);
    //memset(s_hdd_dev, 0x00, sizeof(HDD_DEV_INFO_T)*SUPPORT_HDD_NUM);
    //上报sd正常状态
    if (E_STOR_MEDIA_UNORMAL == s_hdd_stat){
        ty_stor_media_stat_report_normal();
        s_hdd_stat = E_STOR_MEDIA_NORMAL;
    }
    
    UINT64_T totol = 0;
    UINT64_T startOff = 0;
    __get_device_volume(s_hdd_dev[type].devPath, &totol, &startOff);
    TYDEBUG("dev[%s] volume[%llu-%llu]\n",s_hdd_dev[type].devPath,startOff, totol);
    memset(&strHdInfo, 0x00, sizeof(STOR_HD_INIT_INFO_T));
    memcpy(strHdInfo.hdName, s_hdd_dev[type].devPath, STOR_HD_DEVICE_LEN);
    strHdInfo.hdType = type;
    strHdInfo.hdStartOffset = startOff;
    strHdInfo.hdTotolLen = totol;
    strHdInfo.strHdOpCb.stor_hd_open = hd_open;
    strHdInfo.strHdOpCb.stor_hd_seek = hd_seek;
    strHdInfo.strHdOpCb.stor_hd_read = hd_read;
    strHdInfo.strHdOpCb.stor_hd_write = hd_write;
    strHdInfo.strHdOpCb.stor_hd_flush = hd_flush;
    strHdInfo.strHdOpCb.stor_hd_close = hd_close;
    strHdInfo.media_cb = __stor_get_media;
    ret = tuya_ipc_stor_init(&strHdInfo);
    if (OPRT_OK != ret){
        TYERROR("hdd inited failed\n");
        return -2;
    }
    s_inited = 1;
    return 0;
}

int ty_hdd_uninit()
{
    if (OPRT_OK != tuya_ipc_stor_uninit()){
        TYERROR("hdd inited failed\n");
        return -1;
    }
    s_inited = 0;
    hd_free();
    return 0;
}

/***********************************************************
*  Function: ty_hdd_start
*  Note:  sub dev hdd start
*  Input: devId  
*  Output: none
*  Return: 
***********************************************************/
int ty_hdd_start(char * devId)
{
    if (NULL == devId){
        TYDEBUG("input error\n");
        return -1;
    }
    if (0 == s_inited){
        TYERROR("hdd not init\n");
        return -1;
    }

    //end
    return tuya_ipc_stor_create(devId);
}
/***********************************************************
*  Function: ty_hdd_stop
*  Note:  sub dev hdd stop
*  Input: devId  
*  Output: none
*  Return: 
***********************************************************/
int ty_hdd_stop(char * devId)
{
    if (0 == s_inited){
        TYERROR("hdd not init\n");
        return -1;
    }
    return tuya_ipc_stor_release(devId);
}

int ty_hdd_set_stat(char * devId, int stat,int type)
{
    if (0 == s_inited){
        TYERROR("hdd not init\n");
        return -1;
    }
    return tuya_ipc_stor_set_stat(devId, (STOR_STATUS_E)stat, (STOR_HD_EVENT_TYPE_E)type);
}

int ty_hdd_get_volume(int type, UINT64_T * totol, UINT64_T * free)
{
    STOR_HD_INIT_INFO_T strHdInfo;
    UINT64_T totollen = 0;
    UINT64_T startOff = 0;
    __get_device_volume(s_hdd_dev[type].devPath, &totollen, &startOff);
    TYDEBUG("dev[%s] volume[%llu-%llu]\n",s_hdd_dev[type].devPath,startOff, totollen);
    memset(&strHdInfo, 0x00, sizeof(STOR_HD_INIT_INFO_T));
    memcpy(strHdInfo.hdName, s_hdd_dev[type].devPath, STOR_HD_DEVICE_LEN);
    strHdInfo.hdType = type;
    strHdInfo.hdStartOffset = startOff;
    strHdInfo.hdTotolLen = totollen;
    strHdInfo.strHdOpCb.stor_hd_open = hd_open;
    strHdInfo.strHdOpCb.stor_hd_seek = hd_seek;
    strHdInfo.strHdOpCb.stor_hd_read = hd_read;
    strHdInfo.strHdOpCb.stor_hd_write = hd_write;
    strHdInfo.strHdOpCb.stor_hd_flush = hd_flush;
    strHdInfo.strHdOpCb.stor_hd_close = hd_close;
    
    return tuya_ipc_stor_get_volume(&strHdInfo,totol,free);    
}

int ty_hdd_format(int type)
{
    STOR_HD_INIT_INFO_T strHdInfo;
    UINT64_T totol = 0;
    UINT64_T startOff = 0;
    __get_device_volume(s_hdd_dev[type].devPath, &totol, &startOff);
printf("format %d\n", type);
    memset(&strHdInfo, 0x00, sizeof(STOR_HD_INIT_INFO_T));
    memcpy(strHdInfo.hdName, s_hdd_dev[type].devPath, STOR_HD_DEVICE_LEN);
    strHdInfo.hdType = type;
    strHdInfo.hdStartOffset = startOff;
    strHdInfo.hdTotolLen = totol;
    strHdInfo.strHdOpCb.stor_hd_open = hd_open;
    strHdInfo.strHdOpCb.stor_hd_seek = hd_seek;
    strHdInfo.strHdOpCb.stor_hd_read = hd_read;
    strHdInfo.strHdOpCb.stor_hd_write = hd_write;
    strHdInfo.strHdOpCb.stor_hd_flush = hd_flush;
    strHdInfo.strHdOpCb.stor_hd_close = hd_close;

    return tuya_ipc_stor_format(&strHdInfo, __format_cb);
}

VOID_T tuya_ipc_stor_media_stat(STOR_MEDIA_STAT_E stat)
{
    TYDEBUG("set stor media stat [%d]\n",stat);
    if (E_STOR_MEDIA_UNORMAL == stat){
        ty_stor_media_stat_report_unormal();
    }

    s_hdd_stat = stat;
    return;
}
extern pthread_t stor_format_thread;
extern  int stor_format_stat ;
void *thread_stor_format(void *arg)
{
    int stor_mode = SD_ENABLE_VALUE;
    //ty_gw_cfg_db_read(BASIC_IPC_STOR_MODE, &stor_mode);

    int val = 4;
    tuya_ipc_dp_report(NULL, TUYA_DP_STOR_STATUS,PROP_VALUE,&val,1);
    /* 先通知APP，进度0% */
    stor_format_stat = 0;
    tuya_ipc_dp_report(NULL, TUYA_DP_STOR_STATUS,PROP_VALUE,&stor_format_stat,1);
    sleep(1);
    stor_format_stat = 10;
    tuya_ipc_dp_report(NULL, TUYA_DP_STOR_STATUS,PROP_VALUE,&stor_format_stat,1);

    if (SD_ENABLE_VALUE == stor_mode){
        ty_hdd_format(0);
    }else if (HDD_ENABLE_VALUE == stor_mode){
        ty_hdd_format(1);
    }else{
        TYERROR("cur not set mode  please check\n");
    }

    UINT64_T totol = 0;
    UINT64_T free = 0;
    if (SD_ENABLE_VALUE == stor_mode){
        ty_hdd_get_volume(0, &totol, &free);
    }else if (HDD_ENABLE_VALUE == stor_mode){
        ty_hdd_get_volume(1, &totol, &free);
    }else{
        TYERROR("cur not set mode  please check\n");
    }
    CHAR_T tmp_str[100] = {0};
    snprintf(tmp_str, 100, "%llu|%llu|%llu", totol>>10, (totol - free)>>10, free>>10);
    tuya_ipc_dp_report(NULL, TUYA_DP_STOR_VOLUME,PROP_STR,tmp_str,1);
    /* 最后100% */
    stor_format_stat = 100;
    tuya_ipc_dp_report(NULL, TUYA_DP_STOR_STATUS,PROP_VALUE,&stor_format_stat,1);
    val =1;
    tuya_ipc_dp_report(NULL, TUYA_DP_STOR_STATUS,PROP_VALUE,&val,1);
    stor_format_thread = -1;
    pthread_exit(0);
    return NULL;
}

#endif

pthread_t stor_format_thread = -1;
int stor_format_stat = 100;
void tuya_stor_format_set()
{
    if (-1 == stor_format_thread){
#if defined(__USER_DO_NOT_OPEN__) && (__USER_DO_NOT_OPEN__ == 1)
        pthread_create(&stor_format_thread, NULL, thread_stor_format, NULL);
        pthread_detach(stor_format_thread);
#endif
    }else{
        TYERROR("STOR FORMATTING\n");
    }

}
