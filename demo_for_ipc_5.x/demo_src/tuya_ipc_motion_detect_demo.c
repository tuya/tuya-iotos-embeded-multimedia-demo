/*********************************************************************************
  *Copyright(C),2015-2020,
  *TUYA
  *www.tuya.comm
  *FileName:    tuya_ipc_motion_detect_demo
**********************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_event.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_ipc_dp_handler.h"

//AI detect should enable SUPPORT_AI_DETECT
#define SUPPORT_AI_DETECT 1
#if SUPPORT_AI_DETECT
#include "tuya_ipc_ai_detect_storage.h"
#endif

//According to different chip platforms, users need to implement whether there is motion alarm in the current frame.
static int fake_md_status = 0;

VOID IPC_APP_set_motion_status(int status)
{
    fake_md_status = status;
}

static int get_motion_status()
{
    //if motion detected ,return 1
    return fake_md_status;
    //else return 0
    //return 0;
}

#if SUPPORT_AI_DETECT
//According to different chip platforms, users need to implement the interface of capture.
VOID tuya_ipc_get_snapshot_cb(char* pjbuf,  int* size)
{
    IPC_APP_get_snapshot(pjbuf,size);
}
#endif

VOID *thread_md_proc(VOID *arg)
{
    int motion_flag = 0;
    int motion_alarm_is_triggerd = FALSE;
    char snap_addr[MAX_SNAPSHOT_BUFFER_SIZE] = {0};
    int snap_size = 0;
    int md_enable = 0;
    int ret = 0;
    TIME_T current_time;
    TIME_T last_md_time;

    while (1)
    {
        usleep(100*1000);
        tuya_ipc_get_utc_time(&current_time);
        motion_flag = get_motion_status();
        if(motion_flag)
        {
            last_md_time = current_time;
            if(!motion_alarm_is_triggerd)
            {
                motion_alarm_is_triggerd = TRUE;
                //start Local SD Card Event Storage and Cloud Storage Events
                tuya_ipc_start_storage(E_ALARM_SD_STORAGE);
                tuya_ipc_start_storage(E_ALARM_CLOUD_STORAGE);
                IPC_APP_get_snapshot(snap_addr,&snap_size);
                if(snap_size > 0)
                {
                    md_enable = IPC_APP_get_alarm_function_onoff();
                    // md_enable is TRUE, upload message to message center
                    tuya_ipc_notify_alarm(snap_addr, snap_size, NOTIFICATION_NAME_MOTION, md_enable);
                }

                /*NOTE:
                ONE：Considering the real-time performance of push and storage, the above interfaces can be executed asynchronously in different tasks.
                TWO：When event cloud storage is turned on, it will automatically stop beyond the maximum event time in SDK.
                THREE:If you need to maintain storage for too long without losing it, you can use the interface (tuya_ipc_ss_get_status and tuya_ipc_cloud_storage_get_event_status).
                      to monitor whether there are stop event videos in SDK and choose time to restart new events
                */
            }
            else
            {
                //Storage interruption caused by maximum duration of internal events, restart new events
                if(SS_WRITE_MODE_EVENT == tuya_ipc_ss_get_write_mode() && E_STORAGE_STOP == tuya_ipc_ss_get_status())
                {
                    tuya_ipc_start_storage(E_ALARM_SD_STORAGE);
                }

                if(ClOUD_STORAGE_TYPE_EVENT == tuya_ipc_cloud_storage_get_store_mode()
                   && FALSE == tuya_ipc_cloud_storage_get_status())
                {
                    tuya_ipc_start_storage(E_ALARM_CLOUD_STORAGE);
                }
            }
        }
        else
        {
            //No motion detect for more than 10 seconds, stop the event
            if(current_time - last_md_time > 10 && motion_alarm_is_triggerd)
            {
                tuya_ipc_stop_storage(E_ALARM_SD_STORAGE);
                tuya_ipc_stop_storage(E_ALARM_CLOUD_STORAGE);
                motion_alarm_is_triggerd = FALSE;
            }
        }
    }

    return NULL;
}

OPERATE_RET TUYA_APP_Enable_Motion_Detect()
{
#ifdef __HuaweiLite__    
    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)thread_md_proc;
    stappTask.pcName = "motion_detect";
    LOS_TaskCreate((UINT32 *)&taskid, &stappTask);
#else
    pthread_t motion_detect_thread;
    pthread_create(&motion_detect_thread, NULL, thread_md_proc, NULL);
    pthread_detach(motion_detect_thread);    
#endif

    return OPRT_OK;
}

#if SUPPORT_AI_DETECT

OPERATE_RET TUYA_APP_Enable_AI_Detect()
{
    IPC_MEDIA_INFO_S* p_media_info = IPC_APP_Get_Media_Info();
    if(p_media_info == NULL) 
    {
        return OPRT_COM_ERROR;
    }    
    tuya_ipc_ai_detect_storage_init(p_media_info);

    return OPRT_OK;
}
#endif
