/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_motion_detect_demo
**********************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include "tuya_ipc_cloud_storage.h"
#include "tuya_xvr_cloud_storage.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_event.h"
#include "tuya_xvr_dev.h"
#include "tuya_ipc_dp_handler.h"
#ifdef __cplusplus
extern "C" {
#endif

//AI detect should SUPPORT_AI_DETECT
#define SUPPORT_AI_DETECT 1
#if SUPPORT_AI_DETECT
#include "tuya_ipc_ai_detect_storage.h"
#endif

//According to different chip platforms, users need to implement whether there is motion alarm in the current frame.
int fake_md_status = 0;
int get_motion_status()
{
    //if motion detected ,return 1
    return fake_md_status;
    //else return 0
    //return 0;
}

//According to different chip platforms, users need to implement the interface of capture.
void get_motion_snapshot(char *snap_addr, int *snap_size)
{
    //we use file to simulate
    char snapfile[256];
    *snap_size = 0;
    extern char s_raw_path[];
    printf("get one motion snapshot\n");
    snprintf(snapfile,256,"%s/rawfiles/tuya_logo.jpg",s_raw_path);
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

#if SUPPORT_AI_DETECT
//According to different chip platforms, users need to implement the interface of capture.
VOID tuya_ipc_get_snapshot_cb(char* pjbuf, int* size)
{

}
#endif

VOID *thread_md_proc(VOID *arg)
{
    int motion_flag = 0;
    int motion_alarm_is_triggerd = FALSE;
    char snapshot_buf[MAX_SNAPSHOT_BUFFER_SIZE] = {0};
    int snap_size = 0;
    int md_enable = 0;
    int ret = 0;
    TIME_T current_time;
    TIME_T last_md_time;
    int snapshot_size;
    char sub_dev_id[64] = { 0 };
    ret = tuya_xvr_dev_devId_get_by_chan(0, sub_dev_id, 64);
    int event_id;
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
                get_motion_snapshot(snapshot_buf,&snap_size);
                if(snap_size > 0)
                {
                    md_enable = IPC_APP_get_alarm_function_onoff();
                    // md_enable is TRUE, upload message to message center
                    //tuya_ipc_notify_alarm(snap_addr, snap_size, NOTIFICATION_NAME_MOTION, md_enable);
                    ret = tuya_xvr_notify_with_event((CHAR_T*)sub_dev_id, snapshot_buf, snap_size, NOTIFICATION_CONTENT_JPEG, NOTIFICATION_NAME_MOTION,TRUE);
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
                BOOL_T status=FALSE;
                ClOUD_STORAGE_TYPE_E type  ;
                tuya_xvr_cloud_storage_store_mode_get(0,&type);
                tuya_xvr_cloud_storage_status_get(0,&status);
                if((type == ClOUD_STORAGE_TYPE_EVENT )&&  (status == FALSE))
                {
                    event_id = tuya_xvr_cloud_storage_event_add(sub_dev_id, EVENT_TYPE_MOTION_DETECT, 300);
                }
            }
        }
        else
        {
            //No motion detect for more than 10 seconds, stop the event
            if(current_time - last_md_time > 10 && motion_alarm_is_triggerd)
            {
                printf("=====================================stop cloud storage\n");
                ret = tuya_xvr_cloud_storage_event_delete(sub_dev_id, event_id); // event stop
                motion_alarm_is_triggerd = FALSE;
            }
        }
    }

    return NULL;
}

#if SUPPORT_AI_DETECT
extern IPC_MEDIA_INFO_S s_media_info;
OPERATE_RET TUYA_APP_Enable_AI_Detect()
{
    tuya_ipc_ai_detect_storage_init(&s_media_info);

    return OPRT_OK;
}
#endif
#ifdef __cplusplus
}
#endif
