#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "tuya_cloud_base_defs.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_system_control_demo.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_ipc_motion_detect_demo.h"
#include "tuya_ipc_doorbell_demo.h"
#include "tuya_iot_config.h"


#define IPC_APP_STORAGE_PATH    "./"   //Path to save tuya sdk DB files, should be readable, writeable and storable
#define IPC_APP_UPGRADE_FILE    "./upgrade.file" //File with path to download file during OTA
#define IPC_APP_SD_BASE_PATH    "/mnt/sdcard/"      //SD card mount directory
#define IPC_APP_PID             "gpkguYNp7yI4k413"  //Product ID of TUYA device, this is for demo only.
                                                     //Contact tuya PM/BD for official pid.
#define IPC_APP_UUID            "tuyaOneUuidForOneDevice" //Unique identification of each device
                                                        //Contact tuya PM/BD for developing devices or BUY more
#define IPC_APP_AUTHKEY         "tuyaOneAuthkeyForOneUUID" //Authentication codes corresponding to UUID, one machine one code, paired with UUID. 
#define IPC_APP_VERSION         "1.2.3"     //Firmware version displayed on TUYA APP 

IPC_MGR_INFO_S s_mgr_info = {0};
STATIC INT_T s_mqtt_status = 0;

CHAR_T s_raw_path[128] = {0};
CHAR_T s_pid[64] = {0};
CHAR_T s_uuid[64] = {0};
CHAR_T s_authkey[64] = {0};

STATIC VOID __IPC_APP_Get_Net_Status_cb(IN CONST BYTE_T stat)
{
    PR_DEBUG("Net status change to:%d", stat);
    switch(stat)
    {
#if defined(WIFI_GW) && (WIFI_GW==1)
        case STAT_CLOUD_CONN:        //for wifi ipc
        case STAT_MQTT_ONLINE:       //for low-power wifi ipc
#endif
#if defined(WIFI_GW) && (WIFI_GW==0)
        case GB_STAT_CLOUD_CONN:     //for wired ipc
#endif
        {
            IPC_APP_Notify_LED_Sound_Status_CB(IPC_MQTT_ONLINE);
            PR_DEBUG("mqtt is online\r\n");
            s_mqtt_status = 1;
            break;
        }
        default:
        {
            break;
        }
    }
}

OPERATE_RET IPC_APP_Init_SDK(WIFI_INIT_MODE_E init_mode, CHAR_T *p_token)
{
    PR_DEBUG("SDK Version:%s\r\n", tuya_ipc_get_sdk_info());

    memset(&s_mgr_info, 0, sizeof(IPC_MGR_INFO_S));
    strcpy(s_mgr_info.storage_path, IPC_APP_STORAGE_PATH);
    strcpy(s_mgr_info.upgrade_file_path, IPC_APP_UPGRADE_FILE);
    strcpy(s_mgr_info.sd_base_path, IPC_APP_SD_BASE_PATH);
    strcpy(s_mgr_info.product_key, s_pid);
    strcpy(s_mgr_info.uuid, s_uuid);
    strcpy(s_mgr_info.auth_key, s_authkey);
    strcpy(s_mgr_info.dev_sw_version, IPC_APP_VERSION);
    s_mgr_info.max_p2p_user = 5; //TUYA P2P supports 5 users at the same time, including live preview and playback
    PR_DEBUG("Init Value.storage_path %s", s_mgr_info.storage_path);
    PR_DEBUG("Init Value.upgrade_file_path %s", s_mgr_info.upgrade_file_path);
    PR_DEBUG("Init Value.sd_base_path %s", s_mgr_info.sd_base_path);
    PR_DEBUG("Init Value.product_key %s", s_mgr_info.product_key);
    PR_DEBUG("Init Value.uuid %s", s_mgr_info.uuid);
    PR_DEBUG("Init Value.auth_key %s", s_mgr_info.auth_key);
    PR_DEBUG("Init Value.p2p_id %s", s_mgr_info.p2p_id);
    PR_DEBUG("Init Value.dev_sw_version %s", s_mgr_info.dev_sw_version);
    PR_DEBUG("Init Value.max_p2p_user %u", s_mgr_info.max_p2p_user);

    IPC_APP_Set_Media_Info();
    TUYA_APP_Init_Ring_Buffer();

    IPC_APP_Notify_LED_Sound_Status_CB(IPC_BOOTUP_FINISH);

    TUYA_IPC_ENV_VAR_S env;

    memset(&env, 0, sizeof(TUYA_IPC_ENV_VAR_S));

    strcpy(env.storage_path, s_mgr_info.storage_path);
    strcpy(env.product_key,s_mgr_info.product_key);
    strcpy(env.uuid, s_mgr_info.uuid);
    strcpy(env.auth_key, s_mgr_info.auth_key);
    strcpy(env.dev_sw_version, s_mgr_info.dev_sw_version);
    strcpy(env.dev_serial_num, "tuya_ipc");
    env.dev_obj_dp_cb = IPC_APP_handle_dp_cmd_objs;
    env.dev_dp_query_cb = IPC_APP_handle_dp_query_objs;
    env.status_changed_cb = __IPC_APP_Get_Net_Status_cb;
    env.gw_ug_cb = IPC_APP_Upgrade_Inform_cb;
    env.gw_rst_cb = IPC_APP_Reset_System_CB;
    env.gw_restart_cb = IPC_APP_Restart_Process_CB;
    env.mem_save_mode = FALSE;
    tuya_ipc_init_sdk(&env);
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
    tuya_ipc_set_region(REGION_CN);
    p_token = NULL;
#endif
    tuya_ipc_start_sdk(init_mode, p_token);
    return OPRT_OK;
}

VOID usage(CHAR_T *app_name)
{
    printf("%s -m mode -t token -r raw path -h\n", (CHAR_T *)basename(app_name));
    printf("\t m: 0-WIFI_INIT_AUTO 1-WIFI_INIT_AP 2-WIFI_INIT_DEBUG, refer to WIFI_INIT_MODE_E\n"
        "\t t: token get form qrcode info\n"
        "\t r: raw source file path\n"
        "\t h: help info\n");

    return;
}

#ifdef __HuaweiLite__
int app_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif

{
    INT_T res = -1;
    CHAR_T token[30] = {0};
    WIFI_INIT_MODE_E mode = WIFI_INIT_AUTO;

#if defined(WIFI_GW) && (WIFI_GW==0)
    mode = WIFI_INIT_NULL;
#endif

    strcpy(s_raw_path, "../../demo_resource"); //Path where demo resources locates
    strcpy(s_pid, IPC_APP_PID);
    strcpy(s_uuid, IPC_APP_UUID);
    strcpy(s_authkey, IPC_APP_AUTHKEY);
#ifdef __HuaweiLite__
    if(argc != 2)
    {
        printf("%s <token>\n", argv[0]);
        return -1;
    }
    mode = WIFI_INIT_DEBUG; //The demo mode is set to debug, so before running this main process, 
                            //developers need to make sure that devices are connected to the Internet. 。
    strcpy(token, argv[1]); //Token field values scanned from APP QR-codes or broadcast packets
#else
    while((res = getopt(argc, argv, "?m:t:s:r:h:p:u:a:")) != -1) 
    {
        switch(res) {
        case 'm':
            mode = atoi(optarg);
            break;

        case 't':
            strcpy(token, optarg);
            break;

        case 'r':
            strcpy(s_raw_path, optarg);
            break;

        case 'p':
            strcpy(s_pid, optarg);
            break;

        case 'u':
            strcpy(s_uuid, optarg);
            break;

        case 'a':
            strcpy(s_authkey, optarg);
            break;

        case 'h':
        default:
            usage(argv[0]);
            return -1;
        }
    }
#endif
    /* Init SDK */
    IPC_APP_Init_SDK(mode, token);

#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
    /* demo: how to get qrcode from tuya server for display */
    sleep(2);
    CHAR_T info[32] = {0};
    tuya_ipc_get_qrcode(NULL,info, 32);
    printf("###info:%s\n", info);
#endif
    /*Demo uses files to simulate audio/video/jpeg inputs. 
    The actual data acquisition needs to be realized by developers. */
#ifdef __HuaweiLite__
    TSK_INIT_PARAM_S stappTask;
    int taskid = -1;
    memset(&stappTask, 0, sizeof(TSK_INIT_PARAM_S));
    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)thread_live_video;
    stappTask.uwStackSize  = 0x80000;
    stappTask.pcName = "live_video";
    stappTask.usTaskPrio = 10;
    stappTask.uwResved   = LOS_TASK_STATUS_DETACHED;
    LOS_TaskCreate((UINT32 *)&taskid, &stappTask);

    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)thread_live_audio;
    stappTask.pcName = "live_video";
    LOS_TaskCreate((UINT32 *)&taskid, &stappTask);
 #else   
    pthread_t h264_output_thread;
    pthread_create(&h264_output_thread, NULL, thread_live_video, NULL);
    pthread_detach(h264_output_thread);

    pthread_t pcm_output_thread;
    pthread_create(&pcm_output_thread, NULL, thread_live_audio, NULL);
    pthread_detach(pcm_output_thread);
#endif
    /* whether SDK is connected to MQTT */
    while(s_mqtt_status != 1)
    {
        usleep(100000);
    }
#ifdef __HuaweiLite__
    tuya_ipc_report_p2p_msg();
#endif
    /*At least one system time synchronization after networking*/
    IPC_APP_Sync_Utc_Time();
    
    /* Start local storage. Tt is recommended to be after ONLINE, or make sure the system time is correct */
    TUYA_APP_Init_Stream_Storage(s_mgr_info.sd_base_path);

    /* Enable TUYA P2P service after the network is CONNECTED. 
       Note: For low-power camera, invoke this API as early as possible(can be before mqtt online) */
    TUYA_APP_Enable_P2PTransfer(s_mgr_info.max_p2p_user);
    
    /* Upload all local configuration item (DP) status when MQTT connection is successful */
    IPC_APP_upload_all_status();

    TUYA_APP_Enable_CloudStorage();

    TUYA_APP_Enable_AI_Detect();

    TUYA_APP_Enable_DOORBELL();

    /*!!!very important! After all module inited, update skill to tuya cloud */
    tuya_ipc_upload_skills();

    /* Starting the detection tasks and trigger alarm reporting/local storage/cloud storage tasks through detection results  */
#ifdef __HuaweiLite__
    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)thread_md_proc;
    stappTask.pcName = "motion_detect";
    LOS_TaskCreate((UINT32 *)&taskid, &stappTask);
#else
    pthread_t motion_detect_thread;
    pthread_create(&motion_detect_thread, NULL, thread_md_proc, NULL);
    pthread_detach(motion_detect_thread);
#endif

    /* Manual simulation of related functions */
    char test_input[64] = {0};
    extern int fake_md_status;
    while(1)
    {
        scanf("%s",test_input);
        /* Simulation of the start of motion detection events */
        if(0 == strcmp(test_input,"start"))
        {
            fake_md_status = 1;
        }
        /* Simulation of the end of event */
        else if(0 == strcmp(test_input,"stop"))
        {
            fake_md_status = 0;
        }
        /* Simulation of getting device's activation status */
        else if(0 == strcmp(test_input,"status"))
        {
            IPC_REGISTER_STATUS status = tuya_ipc_get_register_status();
            printf("current register status %d[0:unregistered 1:registered 2:activated]\n",status);
        }
        /* Simulation of doorbell press event */
        else if(0 == strcmp(test_input,"bell"))
        {
            //Using demo file for simulation, should be replaced by real snapshot when events happen.
            char snapfile[64];
            snprintf(snapfile,64,"%s/media/demo_snapshot.jpg",s_raw_path);
            FILE*fp = fopen(snapfile,"r+");
            if(NULL == fp)
            {
                printf("fail to open snap.jpg\n");
                continue;
            }
            fseek(fp,0,SEEK_END);
            int snapshot_size = ftell(fp);
            char *snapshot_buf = (char *)malloc(snapshot_size);
            fseek(fp,0,SEEK_SET);
            fread(snapshot_buf,snapshot_size,1,fp);
            fclose(fp);
            /* Push the detection message and the current snapshot image to the APP.
            Snapshot image acquisition needs to be implemented by the developer */
            tuya_ipc_door_bell_press(DOORBELL_NORMAL, snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG);
            free(snapshot_buf);
        }
        /* Simulation of low power ipc */
        else if (0 == strcmp(test_input, "suspend"))
        {
            TUYA_APP_LOW_POWER_ENABLE();
        }
        else if (0 == strcmp(test_input, "ac"))
        {
            doorbell_handler();
        }
        /* Simulation of get time for OSD */
        else if (0 == strcmp(test_input, "osd"))
        {
            IPC_APP_Show_OSD_Time();
        }

        usleep(100*1000);
    }

    return 0;
}

