/***********************************************************
*  File: ty_hdd_file_oper.c
*  Author: clb
*  Date: 20190521
*  Note: 文件操作接口
**************************************************************/
//#define _GNU_SOURCE

#include "tuya_hdd_file_operate_demo.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <stdint.h>
#include <fcntl.h> 
#include <errno.h>
#include <pthread.h>
#include "string.h"
//#include "base/ty_cmd.h"

#include "tuya_ipc_common_demo.h"
#if defined(__USER_DO_NOT_OPEN__) && (__USER_DO_NOT_OPEN__ == 1)

#ifdef _GNU_SOURCE
#define OPEN_MODE   (O_SYNC|O_RDWR|O_DIRECT)
#else
#define OPEN_MODE   (O_SYNC|O_RDWR)
#endif

//#define ADD_MUTEX_LOCK

#ifdef ADD_MUTEX_LOCK
//20190809 add mutex
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#endif

#define FD_LIST_MAX     (32)
static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
INT_T fd_list[FD_LIST_MAX] = {0};

VOID hd_insert(int fd)
{
    int i = 0;
    pthread_mutex_lock(&list_lock);
    for(i = 0; i< FD_LIST_MAX; i++){
        if (0 == fd_list[i]){
            fd_list[i] = fd;
            break;
        }
    }
    pthread_mutex_unlock(&list_lock);
    return;
}

VOID hd_delete(int fd)
{
    int i = 0;
    pthread_mutex_lock(&list_lock);
    for(i = 0; i< FD_LIST_MAX; i++){
        if (fd == fd_list[i]){
            fd_list[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&list_lock);
    return;
}
static INT_T open_sd_failed_count = 0;

VOID hd_free()
{
    printf("%s %d begin\n",__FUNCTION__,__LINE__);
    int i = 0;
    pthread_mutex_lock(&list_lock);
    for(i = 0; i< FD_LIST_MAX; i++){
        if (0 != fd_list[i]){
            printf("%s %d close [%d]\n",__FUNCTION__,__LINE__,fd_list[i]);
            close(fd_list[i]);
            fd_list[i] = 0;
        }
    }
    open_sd_failed_count = 0;
    pthread_mutex_unlock(&list_lock);
}

INT_T hd_open(IN CONST BYTE_T * pHdName, IN CONST UINT_T fileType, IN CONST UINT_T fileNo, IN CONST INT_T mode, PVOID_T attr)
{
    if (NULL == pHdName){
        return -1;
    }
    INT_T fd = -1;
    //其他参数后续完善使用
    fd = open((char *)pHdName,OPEN_MODE);    
    if (fd > 0){
        printf("#####fd[%d] open\n",fd);
        hd_insert(fd);
        if(strcmp("/dev/mmcblk0",(char *)pHdName) == 0){//todo zxq
            open_sd_failed_count = 0;
        }
    } else {
        printf("ERR : %d,#####fd[%d] open %s open_sd_failed_count=%d\n",errno,fd,pHdName, open_sd_failed_count);
        if(strcmp("/dev/mmcblk0",(char *)pHdName) == 0){
            open_sd_failed_count++;
            if(open_sd_failed_count <= 5){
                printf("sd open failed ,retry,open_sd_failed_count %d\n",open_sd_failed_count);
                if(access("/data/tuya/core",F_OK) != 0){
                    INT_T sd_reinit_fd = -1;
                    INT_T write_ret = -1;
                    CHAR_T buf[256+1];
                    INT_T len = 256;
                    INT_T ret;
                    INT_T mount_flags = 0;
                    
                    if(open_sd_failed_count == 1) {
                        
                        memset(buf, 0, sizeof(buf));
                        ret = ty_cmd_excute_shell("mount | grep \"/dev/mmcblk0p1\"", buf, &len);
                        if (0 == ret) {
                            len = strlen(buf);
                            if (len > 0) {
                                mount_flags = 1;
                            }
                            printf("buf = %s, len =%d mount_flags=%d\n", buf, len, mount_flags);
                        }
                    
                        ty_cmd_excute_shell("umount /var/tmp/mmc/mmcblk0p1",NULL,NULL);
                        printf("umount /var/tmp/mmc/mmcblk0p1\n");
                        if (mount_flags)
                        {
                            open_sd_failed_count = 6;//直接跳出
                        }
                    }

                    if (0 == mount_flags) {
                        sd_reinit_fd = open("/proc/sd_reinit",OPEN_MODE);
                        if(sd_reinit_fd > 0){
                            printf("#####fd[%d] open\n",sd_reinit_fd);
                            write_ret = write(sd_reinit_fd,"1",1);
                            if(write_ret != 1){
                                printf("write_ret err /proc/sd_reinit %d\n",write_ret);
                            }
                            close(sd_reinit_fd);
                        }else{
                            printf("ERR : %d,#####sd_reinit_fd[%d] open %s\n",errno,sd_reinit_fd,"/proc/sd_reinit");
                        }
                    }
                    
                }
            }
        }
    }
    
    return fd;
}
INT_T hd_write(IN CONST PVOID_T hdHandle,IN CONST UINT64_T uOffset, IN CONST BYTE_T * pBuff, IN CONST UINT_T writeLen, PVOID_T attr)
{
    if (NULL == hdHandle || NULL == pBuff){
        return -1;
    }

    int len = 0;
    int ret = 0;
    int fd = *(int *)hdHandle;
#ifdef ADD_MUTEX_LOCK
    pthread_mutex_lock(&lock);
#endif
    //printf("%s %d write fd[%d ]len[%u] offset[%llu]\n",__FUNCTION__,__LINE__,fd,writeLen,uOffset);
    lseek(fd, uOffset, SEEK_SET);
    len = write(fd, pBuff, writeLen);
    if (len != writeLen){
        printf("%s %d write failed len[%u] [%d]\n",__FUNCTION__,__LINE__,writeLen,len);
        perror("write error");
        ret = -1;
    }
#ifdef ADD_MUTEX_LOCK
    pthread_mutex_unlock(&lock);
#endif
    return ret;
}
INT_T hd_read(IN CONST PVOID_T hdHandle,IN CONST UINT64_T uOffset, IN BYTE_T * pBuff, IN CONST UINT_T readLen, PVOID_T attr)
{
    if (NULL == hdHandle || NULL == pBuff){
        return -1;
    }

    int len = 0;
    int fd = *(int *)hdHandle;
#ifdef ADD_MUTEX_LOCK
    pthread_mutex_lock(&lock);
#endif
    lseek(fd, uOffset, SEEK_SET);
    len = read(fd, pBuff, readLen);
    if (len < readLen){
        printf("%s %d read failed len[%u] [%d]\n",__FUNCTION__,__LINE__,readLen,len);
    }
#ifdef ADD_MUTEX_LOCK
    pthread_mutex_unlock(&lock);
#endif
    return 0;
}
INT_T hd_seek(IN CONST PVOID_T hdHandle,IN CONST UINT64_T uOffset, IN CONST INT_T whence, PVOID_T attr)
{
    if (NULL == hdHandle){
        return -1;
    }

    int fd = *(int *)hdHandle;
#ifdef ADD_MUTEX_LOCK
    pthread_mutex_lock(&lock);
#endif
    lseek(fd, uOffset, whence);
#ifdef ADD_MUTEX_LOCK
    pthread_mutex_unlock(&lock);
#endif
    return 0;
}
INT_T hd_flush(IN CONST PVOID_T hdHandle,PVOID_T attr)
{
    if (NULL == hdHandle){
        return -1;
    }
    return fsync(*(INT_T *)hdHandle);
}
INT_T hd_close(IN CONST PVOID_T hdHandle,PVOID_T attr)
{
    if (NULL == hdHandle){
        return -1;
    }
    printf("#####fd[%d] close\n",*(INT_T *)hdHandle);
    hd_delete(*(INT_T *)hdHandle);
    return close(*(INT_T *)hdHandle);
}


#endif
