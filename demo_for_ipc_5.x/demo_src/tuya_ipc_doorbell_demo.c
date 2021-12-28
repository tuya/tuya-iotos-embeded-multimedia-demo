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
#include "tuya_ipc_event.h"
#include "tuya_ipc_doorbell_demo.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_ipc_log_demo.h"

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
    INT_T first_press; //15s timer only set once
    time_t press_time;
    CHAR_T pic_buffer[PIC_MAX_SIZE];
    INT_T pic_size;
    timer_t timerid;
}DOOR_BELL_MANAGER;


/*****************util API*************************/
//According to different chip platforms, users need to implement the interface of timer
STATIC INT_T ty_timer_create(TIMER_CB cb, timer_t *p_timer_id)
{
    struct sigevent evp;

    memset(&evp, 0, sizeof(struct sigevent));

    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = cb;
 
    if(timer_create(CLOCK_REALTIME, &evp, p_timer_id) == -1) {  
        PR_ERR(" fail to timer create\n");
        return -1;
    }

    return 0;
}

STATIC INT_T ty_timer_destory(timer_t timer_id)
{
    timer_delete(timer_id);
    return 0;
}

STATIC INT_T ty_timer_set_time(timer_t timer_id, unsigned long long value,  unsigned long long interval)
{
    INT_T flags = 0;
    struct itimerspec it;

    it.it_value.tv_sec = value/1000;     // 最初开始时间间隔
    it.it_value.tv_nsec = (value%1000) * 1000;
    it.it_interval.tv_sec = interval/1000;  // 后续按照该时间间隔
    it.it_interval.tv_nsec = (interval%1000) * 1000;

    if(timer_settime(timer_id, flags, &it, NULL) == -1) {
        PR_ERR("fail to timer settime\n");
        return -1;
    }

    return 0;
}

STATIC INT_T ty_timer_unset_time(timer_t timer_id)
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
VOID get_snapshot(CHAR_T *snap_addr, INT_T *snap_size)
{
    //we use file to simulate
    CHAR_T snapfile[128];
    *snap_size = 0;
    extern CHAR_T s_raw_path[];
    PR_DEBUG("get one motion snapshot\n");
    snprintf(snapfile, sizeof(snapfile), "%s/resource/media/demo_snapshot.jpg",s_raw_path);
    FILE*fp = fopen(snapfile,"r+");
    if(NULL == fp)
    {
        PR_ERR("fail to open snap.jpg\n");
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
    STATIC DOOR_BELL_MANAGER doorbell_manager;

    return &doorbell_manager;
}

//IMPORTANT!!!!!! if needed , use mutex to protect!!!!!!
VOID* timer_proc(VOID* argv)
{
    DOOR_BELL_MANAGER * phdl = doorbell_get_handler();

    if(phdl->status == DOORBELL_LISTEN)
    {
        phdl->status = DOORBELL_RECORD;
        phdl->first_press = 0;
        PR_DEBUG("##############LEAVE msg?\n");
        ty_timer_set_time(phdl->timerid, DOORBELL_NOTIFY_MSG_DURATION, 0);

		TUYA_ALARM_T alarm = {0};
	    alarm.type = E_ALARM_UNCONNECTED;
	    alarm.resource_type = RESOURCE_PIC;
	    alarm.is_notify = 1;
	    alarm.pic_buf = phdl->pic_buffer;
	    alarm.pic_size = phdl->pic_size;
	    alarm.valid = 1;
 		tuya_ipc_trigger_alarm_without_event(&alarm);  
 	}
    else if(phdl->status == DOORBELL_RECORD)
    {
        PR_DEBUG("no message leave\n");
        phdl->status = DOORBELL_LISTEN;
    }
    else if(phdl->status == DOORBELL_RECORDING)
    {
        PR_DEBUG("upload message\n");
        phdl->status = DOORBELL_LISTEN;
    }
    else if(phdl->status == DOORBELL_TALKING)
    {
        //talking timeout, reset doorbell ms
        PR_DEBUG("talking timeout ,reset doorbell ms\n");
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
            TUYA_ALARM_T alarm = {0};
		    alarm.type = E_ALARM_CONNECTED;
		    alarm.resource_type = RESOURCE_PIC;
		    alarm.is_notify = 1;
		    alarm.pic_buf = phdl->pic_buffer;
		    alarm.pic_size = phdl->pic_size;
		    alarm.valid = 1;
     		tuya_ipc_trigger_alarm_without_event(&alarm);
			
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


VOID doorbell_handler()
{
    DOOR_BELL_MANAGER * phdl = doorbell_get_handler();
    INT_T ret = 0;
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
            tuya_ipc_door_bell_press(DOORBELL_AC, phdl->pic_buffer, phdl->pic_size, NOTIFICATION_CONTENT_JPEG);

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
			PR_ERR("#####tuya_ipc_doorbell_record_start failed \n");
		}
		else
		{
			phdl->status = DOORBELL_RECORDING;  
			ty_timer_set_time(phdl->timerid, DOORBELL_MESSAGE_DURATION, 0);
		}
        
    }
    else if(phdl->status == DOORBELL_RECORDING)
    {
        PR_DEBUG("recording...ignore!\n");
    }
  
    return;
}

/***********************************************************
*  Function: tuya_ipc_doorbell_event
*  Description: A mandatory API to be implemented
*  Input: CALLED by TUYA sdk
*  Output: none
*  Return: 
***********************************************************/

VOID tuya_ipc_doorbell_event(char* action)
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

OPERATE_RET TUYA_APP_Enable_Video_Msg(TUYA_IPC_SDK_VIDEO_MSG_S* p_video_msg_info)
{
    IPC_MEDIA_INFO_S* p_media_info = IPC_APP_Get_Media_Info();
    if(p_media_info == NULL) 
    {
        return OPRT_COM_ERROR;
    }

    return tuya_ipc_video_msg_init(p_media_info, p_video_msg_info->type, p_video_msg_info->msg_duration);
}

