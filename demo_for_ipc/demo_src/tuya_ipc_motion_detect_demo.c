/*********************************************************************************
  *Copyright(C),2015-2020,
  *TUYA
  *www.tuya.comm
  *FileName:    tuya_ipc_motion_detect_demo
**********************************************************************************/

#include <stdio.h>
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_api.h"


//AI detect should enable SUPPORT_AI_DETECT
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

#if SUPPORT_AI_DETECT
//According to different chip platforms, users need to implement the interface of capture.
VOID tuya_ipc_get_snapshot_cb(char* pjbuf, unsigned int* size)
{
    get_motion_snapshot(pjbuf,size);
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
    EVENT_ID event_id;

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
                //start Local SD Card Event Storage
                tuya_ipc_ss_start_event();
                get_motion_snapshot(snap_addr,&snap_size);
                if(snap_size > 0)
                {
                    //Report Cloud Storage Events
                    //whether order type is event or continuous, a list of events is presented on APP for quick jumps.
                    event_id = tuya_ipc_cloud_storage_event_add(snap_addr, snap_size, EVENT_TYPE_MOTION_DETECT, MAX_CLOUD_EVENT_DURATION);
                    if(event_id == INVALID_EVENT_ID)
                    {
                        printf("fail to add cloud storage event\n");
                        // abnormal process here
                    }
                    md_enable = IPC_APP_get_alarm_function_onoff();
                    if(md_enable)  //NOTE!! this md_enable is ONLY used for md notification message control. Not for cloud or local storage
                    {
                        #if SUPPORT_AI_DETECT
                        // ai detect process will send several snapshots to cloud AI server, 
                        // and automatically send alarm message only if target is detected.
                        if(0 != tuya_ipc_ai_detect_storage_start())  // return NON-0 if no ai detect service bill exists or any other failure, then use nomal md notification api 
                        {
                            tuya_ipc_notify_motion_detect(snap_addr,snap_size,NOTIFICATION_CONTENT_JPEG);
                        }
                        #else
                        tuya_ipc_notify_motion_detect(snap_addr,snap_size,NOTIFICATION_CONTENT_JPEG);
                        #endif
                    }
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
                    tuya_ipc_ss_start_event();
                }

                if(ClOUD_STORAGE_TYPE_EVENT == tuya_ipc_cloud_storage_get_store_mode()
                   && tuya_ipc_cloud_storage_get_event_status_by_id(event_id) == EVENT_NONE)
                {
                    get_motion_snapshot(snap_addr,&snap_size);
                    event_id = tuya_ipc_cloud_storage_event_add(snap_addr, snap_size, EVENT_TYPE_MOTION_DETECT, MAX_CLOUD_EVENT_DURATION);
                }
            }
        }
        else
        {
            //No motion detect for more than 10 seconds, stop the event
            if(current_time - last_md_time > 10 && motion_alarm_is_triggerd)
            {
                #if SUPPORT_AI_DETECT
                tuya_ipc_ai_detect_storage_stop();
                #endif
                tuya_ipc_ss_stop_event();
                tuya_ipc_cloud_storage_event_delete(event_id);
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