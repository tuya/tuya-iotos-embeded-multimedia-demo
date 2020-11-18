/*********************************************************************************
  *Copyright(C),2015-2020, TUYA www.tuya.comm
  *FileName:    tuya_ipc_doorbell_demo
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
#include <time.h>
#include <signal.h>

#include "tuya_ipc_api.h"
#include "tuya_ipc_video_msg.h"

#define DOORBELL_ALARM_DURATION 25*1000          //arlarm time
#define DOORBELL_NOTIFY_MSG_DURATION 6*1000  //video msg trigger timeout
#define DOORBELL_MESSAGE_DURATION 6*1000     //video msg duration
#define DOORBELL_HEARTBEAT_DURATION 30*1000  //heart beat timeout
#define PIC_MAX_SIZE 100 * 1024

typedef void *(*TIMER_CB)(void *arg);

typedef enum
{
    DOORBELL_LISTEN = 0,
    DOORBELL_RECORD,
    DOORBELL_RECORDING,
    DOORBELL_TALKING,
    DOORBELL_MAX
}DOORBELL_STATUS_E;

typedef struct
{
    DOORBELL_STATUS_E status;
    int first_press; //15s timer only set once
    time_t press_time;
    char pic_buffer[PIC_MAX_SIZE];
    int pic_size;
    timer_t timerid;
}DOOR_BELL_MANAGER;


/*****************util API*************************/
//According to different chip platforms, users need to implement the interface of timer
static int ty_timer_create(TIMER_CB cb, timer_t *p_timer_id)
{
    struct sigevent evp;

    memset(&evp, 0, sizeof(struct sigevent));

    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = cb;
 
    if(timer_create(CLOCK_REALTIME, &evp, p_timer_id) == -1) {  
        printf(" fail to timer create\n");
        return -1;
    }

    return 0;
}

static int ty_timer_destory(timer_t timer_id)
{
    timer_delete(timer_id);
    return 0;
}

static int ty_timer_set_time(timer_t timer_id, unsigned long long value,  unsigned long long interval)
{
    int flags = 0;
    struct itimerspec it;

    it.it_value.tv_sec = value/1000;     // 最初开始时间间隔
    it.it_value.tv_nsec = (value%1000) * 1000;
    it.it_interval.tv_sec = interval/1000;  // 后续按照该时间间隔
    it.it_interval.tv_nsec = (interval%1000) * 1000;

    if(timer_settime(timer_id, flags, &it, NULL) == -1) {
        printf("fail to timer settime\n");
        return -1;
    }

    return 0;
}

static int ty_timer_unset_time(timer_t timer_id)
{
    struct itimerspec ts;

    ts.it_value.tv_sec  = 0;
    ts.it_value.tv_nsec = 0;
    ts.it_interval = ts.it_value;

    timer_settime(timer_id, 0, &ts, NULL);

    return 0;
}

/**************************************************/

//According to different chip platforms, users need to implement the interface of capture.
void get_snapshot(char *snap_addr, int *snap_size)
{
    //we use file to simulate
    char snapfile[128];
    *snap_size = 0;
    extern char s_raw_path[];
    printf("get one motion snapshot\n");
    snprintf(snapfile,64,"%s/resource/media/demo_snapshot.jpg",s_raw_path);
    FILE*fp = fopen(snapfile,"r+");
    if(NULL == fp)
    {
        printf("fail to open snap.jpg\n");
        return;
    }
    fseek(fp,0,SEEK_END);
    *snap_size = ftell(fp);
    if(*snap_size < 100*1024)
    {
        fseek(fp,0,SEEK_SET);
        fread(snap_addr,*snap_size,1,fp);
    }
    fclose(fp);
    return;
}


DOOR_BELL_MANAGER * doorbell_get_handler()
{
    static DOOR_BELL_MANAGER doorbell_manager;

    return &doorbell_manager;
}

//IMPORTANT!!!!!! if needed , use mutex to protect!!!!!!
void* timer_proc(void* argv)
{
    DOOR_BELL_MANAGER * phdl = doorbell_get_handler();

    if(phdl->status == DOORBELL_LISTEN)
    {
        phdl->status = DOORBELL_RECORD;
        phdl->first_press = 0;
        printf("##############LEAVE msg?\n");
        ty_timer_set_time(phdl->timerid, DOORBELL_NOTIFY_MSG_DURATION, 0);
        
        tuya_ipc_notify_with_event(phdl->pic_buffer, phdl->pic_size, NOTIFICATION_CONTENT_JPEG, NOTIFICATION_NAME_CALL_NOT_ACCEPT); 
    }
    else if(phdl->status == DOORBELL_RECORD)
    {
        printf("no message leave\n");
        phdl->status = DOORBELL_LISTEN;
    }
    else if(phdl->status == DOORBELL_RECORDING)
    {
        printf("upload message\n");
        phdl->status = DOORBELL_LISTEN;
    }
    else if(phdl->status == DOORBELL_TALKING)
    {
        //talking timeout, reset doorbell ms
        printf("talking timeout ,reset doorbell ms\n");
        phdl->first_press = 0;
        phdl->status = DOORBELL_LISTEN;
    }
    return NULL;
}


void doorbell_mqtt_handler(int status)
{
    DOOR_BELL_MANAGER * phdl = doorbell_get_handler();

    //mqtt_cb_status : <0:accept 1:stop 2:heartbeat>
    if(!status)
    {
        if(phdl->first_press && phdl->status == DOORBELL_LISTEN)
        {            
            tuya_ipc_notify_with_event(phdl->pic_buffer, phdl->pic_size, NOTIFICATION_CONTENT_JPEG, NOTIFICATION_NAME_CALL_ACCEPT);
            phdl->status = DOORBELL_TALKING;
            ty_timer_set_time(phdl->timerid, DOORBELL_HEARTBEAT_DURATION, 0);
        }
    }
    else if(status == 1)
    {
        if(phdl->status == DOORBELL_TALKING)
        {
            phdl->first_press = 0;
            phdl->status = DOORBELL_LISTEN;
            ty_timer_unset_time(phdl->timerid);
        }
    }
    else if(status == 2)
    {
        if(phdl->status == DOORBELL_TALKING)
        {
            ty_timer_set_time(phdl->timerid, DOORBELL_HEARTBEAT_DURATION, 0);
        }
    }
}


void doorbell_handler()
{
    DOOR_BELL_MANAGER * phdl = doorbell_get_handler();
    int ret = 0;
    time_t cur_time = 0;


    if(phdl->status == DOORBELL_LISTEN)
    {
        if(phdl->timerid == 0)
        {
            if(ty_timer_create(timer_proc, &phdl->timerid) < 0) {
                return;
            }
        }
 
        if(phdl->first_press == 0)
        {
            phdl->pic_size = PIC_MAX_SIZE;
            phdl->press_time = time(NULL);
            
            get_snapshot(phdl->pic_buffer,&(phdl->pic_size));
            tuya_ipc_notify_door_bell_press_generic(DOORBELL_AC, phdl->pic_buffer, phdl->pic_size, NOTIFICATION_CONTENT_JPEG);

            ty_timer_set_time(phdl->timerid, DOORBELL_ALARM_DURATION, 0);

            phdl->first_press = 1;
        }
    }
    else if(phdl->status == DOORBELL_RECORD)
    {
        phdl->pic_size = PIC_MAX_SIZE;
        get_snapshot(phdl->pic_buffer,&phdl->pic_size);
        
		ret = tuya_ipc_leave_video_msg(phdl->pic_buffer,phdl->pic_size);
		if(ret != 0)
		{
			printf("#####tuya_ipc_doorbell_record_start failed \n");
		}
		else
		{
			phdl->status = DOORBELL_RECORDING;  
			ty_timer_set_time(phdl->timerid, DOORBELL_MESSAGE_DURATION, 0);
		}
        
    }
    else if(phdl->status == DOORBELL_RECORDING)
    {
        printf("recording...ignore!\n");
    }
  
    return;
}

/***********************************************************
*  Function: tuya_ipc_cancel_doorbell_event
*  Description: A mandatory API to be implemented
*  Input: CALLED by TUYA sdk
*  Output: none
*  Return: 
***********************************************************/

VOID tuya_ipc_cancel_doorbell_event(char* action)
{
     int status = 0;

    if(0 == memcmp(action,"accept",6))
    {
        status = 0;
    }
    else if(0 == memcmp(action,"stop",4))
    {
        status = 1;
    }
    else if(0 == memcmp(action,"heartbeat",9))
    {
        status = 2;
    }
    doorbell_mqtt_handler(status);
    return;
}
extern IPC_MEDIA_INFO_S s_media_info;

OPERATE_RET TUYA_APP_Enable_DOORBELL(VOID)
{
    tuya_ipc_video_msg_init(&s_media_info, MSG_BOTH, 10);

    return OPRT_OK;
}

