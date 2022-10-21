#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>
#ifdef __cplusplus
extern "C" {
#endif

#define TUYA_DEMO_DEBUG_DUMP

#ifdef TUYA_DEMO_DEBUG_DUMP
#include <stddef.h>
#include <execinfo.h>
#include <signal.h>
#endif

#include "tuya_iot_base_api.h"
#include "tuya_iot_com_api.h"
#if defined(WIFI_GW) && (WIFI_GW == 1)
#include "tuya_iot_wifi_api.h"
#endif

#include "tuya_ipc_api.h"
///`#include "tuya_ipc_cloud_storage.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_ipc_motion_detect_demo.h"
#include "tuya_xvr_dev_demo.h"
#include "tuya_ipc_system_control_demo.h"
#include "tuya_ipc_upgrade_demo.h"
#include "tuya_xvr_dev.h"
#include "tuya_xvr_event.h"
#include "tuya_ipc_notify.h"
#include "tuya_preset_operation.h"
#include "tuya_xvr_sdk_init.h"
#include "tuya_ipc_skill.h"
#include "tuya_xvr_cloud_storage.h"
#include "tuya_gw_subdev_api.h"
//see tuya_ipc_common_demo.h
#ifdef DEMO_NVR_ONVIF_ENABLE
#include "tuya_ipc_onvif_client_demo.h"
#endif
//设置DB文件存放的位置。确保路径可读可写且断电重启后不能丢失数据的位置
#define IPC_APP_STORAGE_PATH "/tmp/" //Path to save sdk cfg ,need to read and write, doesn't loss when poweroff
#define IPC_APP_UPGRADE_FILE "/tmp/upgrade.file" //Path to save upgrade file when OTA upgrading
#define IPC_APP_SD_BASE_PATH "/tmp/" // SD Card Mount Directory,Local Storage Root Directory

IPC_MGR_INFO_S s_mgr_info = { 0 };
CHAR_T s_raw_path[128] = { 0 };
CHAR_T s_pid[64] = {0};
CHAR_T s_uuid[64] = {0};
CHAR_T s_authkey[64] = {0};
CHAR_T s_nvr_sub_pid[64]={0};

STATIC INT_T s_mqtt_status = 0;
STATIC VOID __IPC_APP_Get_Net_Status_cb(IN CONST BYTE_T stat)
{
    PR_DEBUG("Net status change to:%d", stat);
    switch (stat) {
#if defined(WIFI_GW) && (WIFI_GW == 1)
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
#ifdef TUYA_DEMO_DEBUG_DUMP
#define EXE_FILE_PATH "cmake-build/out/demo_ipc"
void addr2funcline(char *addr)
{
    char cmd[128] = {0};
    char path[128] = EXE_FILE_PATH;
    snprintf(cmd, 128, "addr2line -e %s %s", path, addr);
    system(cmd);
}
void parse_addr(char *backstrace)
{
    char *p = strstr(backstrace, "[");
    char *p1 = strstr(backstrace, "]");
    int len = (void *)p1 - (void *)p;
    char addr[64] = {0};
    snprintf(addr, len, "%s", (char *)p + 1);
    //printf("len%d %s\n",len-1, addr);
    addr2funcline(addr);
}
void dump(int signo)
{
    void* buffer[30] = { 0 };
    size_t size = 0;
    size_t i = 0;
    char** strings = NULL;
    size = backtrace(buffer, 30);
    strings = backtrace_symbols(buffer, size);
    if (strings == NULL) {
        return;
    }

    for (i = 0; i < size; i++) {
        //printf("%s\n", strings[i]);
        parse_addr(strings[i]);
    }

    free(strings);
    exit(0);
}
#endif
CHAR_T s_pToken[64]={0};
#if defined(WIFI_GW) && (WIFI_GW == 1)
OPERATE_RET IPC_APP_Init_SDK(WIFI_INIT_MODE_E init_mode, CHAR_T* p_token)
#else
OPERATE_RET IPC_APP_Init_SDK(VOID)
#endif
{
    //step1：打印SDK信息,尽量不要删除，用于定位问题，了解版本信息
    PR_DEBUG("SDK Version:%s\r\n", tuya_ipc_get_sdk_info());
    PR_DEBUG("IOT SDK Version: %s", tuya_iot_get_sdk_info());
    //step2：XVR SDK 运行环境的赋值：用于存储SDK运行的DB文件路径，接入涂鸦的PID,UUID等
    memset(&s_mgr_info, 0, sizeof(IPC_MGR_INFO_S));
    strcpy(s_mgr_info.storage_path, IPC_APP_STORAGE_PATH);
    strcpy(s_mgr_info.upgrade_file_path, IPC_APP_UPGRADE_FILE);
    strcpy(s_mgr_info.sd_base_path, IPC_APP_SD_BASE_PATH);
    strcpy(s_mgr_info.product_key, s_pid);
    strcpy(s_mgr_info.uuid, s_uuid);
    strcpy(s_mgr_info.auth_key, s_authkey);
    strcpy(s_mgr_info.dev_sw_version, IPC_APP_VERSION);
    s_mgr_info.max_p2p_user = 48; //TUYA P2P supports 5-way simultaneous preview,
    //and developers can consider scaling down based on hardware resources.
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

    //step:用户可以考虑在XVR SDK启动前，自身业务需求逻辑。比如demo中是通知设备启动完成前，设备设置一些灯光和声音
    IPC_APP_Notify_LED_Sound_Status_CB(IPC_BOOTUP_FINISH);
    //step：XVR SDK 初始化。
    TUYA_XVR_ENV_CTX_S context = {0};
    strcpy(context.storage_path, IPC_APP_STORAGE_PATH);
    OPERATE_RET op_ret = tuya_xvr_sdk_init(&context);
    if (OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_init err: %d PATH: %s\n", op_ret, s_mgr_info.storage_path);
        return -1;
    }
    //step:可以设置XVR SDK 运行等级，最高5，默认debug级别4
   // tuya_ipc_set_log_attr(5,NULL);
#if defined(WIFI_GW) && (WIFI_GW == 1)
    WF_GW_PROD_INFO_S prod_info;
    prod_info.uuid = s_mgr_info.uuid;
    prod_info.auth_key = s_mgr_info.auth_key;
    //TODO add ssid passwd
    //    prod_info.ap_ssid = ;
    //    prod_info.ap_passwd = ;

    op_ret = tuya_iot_set_wf_gw_prod_info(&prod_info);
#else
    GW_PROD_INFO_S prod_info;
    prod_info.uuid = s_mgr_info.uuid;
    prod_info.auth_key = s_mgr_info.auth_key;
    op_ret = tuya_iot_set_gw_prod_info(&prod_info);
#endif
    if (OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_set_gw_prod_info err: %d\n", op_ret);
        return -1;
    }
    //step:gate way init：主要初始化网关对子设备的管理的初始化
    /*gw init*/
    TY_IOT_CBS_S iot_cbs;
    TY_IOT_GW_CBS_S gw_cbk;
    GW_ATTACH_ATTR_T* pattr = NULL;
    UINT_T attr_num = 0;

    memset(&iot_cbs, 0, sizeof(TY_IOT_CBS_S));
    memset(&gw_cbk, 0, sizeof(TY_IOT_GW_CBS_S));

    iot_cbs.gw_status_cb = IPC_APP_status_change_cb;
    iot_cbs.gw_ug_cb = IPC_APP_Upgrade_Inform_cb;
    iot_cbs.gw_reset_cb = IPC_APP_Reset_System_CB;
    iot_cbs.dev_obj_dp_cb = IPC_APP_handle_dp_cmd_objs;//一个及其重要的回调函数。APP和云端对设备的一些操作，是通过此函回调出来的。
    iot_cbs.dev_raw_dp_cb = IPC_APP_handle_dp_cmd_raw;
    iot_cbs.dev_dp_query_cb = IPC_APP_handle_dp_query_objs;
    iot_cbs.dev_ug_cb = ty_gw_dev_ug_inform;
    iot_cbs.dev_reset_cb = ty_gw_dev_reset_ifm;
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
    extern void ty_qrcode_active_shorturl_get_cb(OUT CONST CHAR_T *shorturl);
    //如果用户想要不同APP的url，可以在此处通过接口tuya_qrcode_set_appid 设置appid
    //当前默认是涂鸦智能。如果想要智能生活，appid设置为“19”
    iot_cbs.active_shorturl = ty_qrcode_active_shorturl_get_cb;
#endif

    gw_cbk.gw_add_dev_cb = ty_gw_subdev_add_dev;
    gw_cbk.gw_del_cb = ty_gw_subdev_del_dev;
    gw_cbk.gw_dev_grp_cb = ty_gw_subdev_grp_inform_dev;
    gw_cbk.gw_dev_scene_cb = ty_gw_subdev_scene_inform_dev;
    gw_cbk.gw_ifm_cb = ty_gw_subdev_inform_dev;

#if defined(WIFI_GW) && (WIFI_GW == 1)
    int iot_wifi_mode = WF_START_SMART_ONLY;
    if (0 == init_mode) {
        iot_wifi_mode = WF_START_SMART_ONLY;
    } else if (1 == init_mode) {
        iot_wifi_mode = WF_START_AP_ONLY;
    } else if (2 == init_mode) {
        iot_wifi_mode = WF_START_AP_ONLY;
    } else {
        PR_ERR("tuya sdk wifi mode is err %d", iot_wifi_mode);
        return OPRT_INVALID_PARM;
    }
    op_ret = tuya_iot_wf_gw_dev_init(GWCM_OLD, iot_wifi_mode, &iot_cbs, &gw_cbk, s_mgr_info.product_key, s_mgr_info.dev_sw_version, pattr, attr_num);
#else
    op_ret = tuya_iot_gw_dev_init(&iot_cbs, &gw_cbk, s_mgr_info.product_key, s_mgr_info.dev_sw_version, pattr, attr_num);
#endif
    if (OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_soc_init err: %d\n", op_ret);
        return -1;
    }

    //step：网络回到函数注册：用户回调设备连接上网络和连接上TUYA的回调函数
#if defined(WIFI_GW) && (WIFI_GW == 1)
    op_ret = tuya_iot_reg_get_wf_nw_stat_cb(__IPC_APP_Get_Net_Status_cb);
#else
    op_ret = tuya_iot_reg_get_nw_stat_cb(__IPC_APP_Get_Net_Status_cb);
#endif
    if (OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_reg_get_nw_stat_cb err: %d\n", op_ret);
        return -1;
    }

    //step:xvr sdk 管理子设备.
#if defined(DEMO_USE_AS_NVR) && (DEMO_USE_AS_NVR == 1)
    tuya_xvr_dev_init(TUYA_XVR_NVR_DEV_MODE);
#else
    tuya_xvr_dev_init(TUYA_XVR_STATION_DEV_MODE);
#endif

   //setp:添加需要的报警类型。设备需要支持的报警类型，需要通过次步骤来注册。比如需要有上报动检，需要有以后步骤
    TUYA_ALARM_BITMAP_T alarm_of_events[3] = {0};
    tuya_ipc_add_alarm_types(&alarm_of_events[0], E_ALARM_MOTION);
    //如果要添加门铃报警，则需要有tuya_ipc_add_alarm_types E_ALARM_DOORBELL的操作
    //tuya_ipc_add_alarm_types(&alarm_of_events[1], E_ALARM_DOORBELL);
    op_ret = tuya_ipc_event_module_init(sizeof(alarm_of_events) / sizeof(TUYA_ALARM_BITMAP_T),alarm_of_events, 1);
    if (OPRT_OK != op_ret) {
        PR_ERR("tuya_ipc_event_module_init err: %d\n", op_ret);
        //return -1;
    }
    //if(strlen(s_pToken))
    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx==============\n");
    if(strlen(s_pToken))
    gw_user_token_bind( s_pToken);
    return OPRT_OK;
}

VOID usage(CHAR_T* app_name)
{
    printf("%s -m mode -t token -r raw path -h\n", (CHAR_T*)basename(app_name));
    printf("\t m: 0-WIFI_INIT_AUTO 1-WIFI_INIT_AP 2-WIFI_INIT_DEBUG, refer to WIFI_INIT_MODE_E\n"
           "\t t: token get form qrcode info\n"
           "\t r: raw source file path\n"
           "\t h: help info\n");

    return;
}
void fun(ty_cJSON *root)
{
    char * printforig = ty_cJSON_PrintUnformatted(root);
    printf("call  input %s\n",printforig);
    ty_cJSON_FreeBuffer(printforig);
}

int main(int argc, char* argv[])
{
    INT_T res = -1;
    CHAR_T token[30] = { 0 };
    WIFI_INIT_MODE_E mode = WIFI_INIT_AUTO;

    strcpy(s_raw_path, "./"); //The directory where the test audio and video folders needed by demo are located
	strcpy(s_pid, IPC_APP_PID);
    strcpy(s_uuid, IPC_APP_UUID);
    strcpy(s_authkey, IPC_APP_AUTHKEY);
    strcpy(s_nvr_sub_pid, IPC_APP_SUB_DEV_PID);
	

    while ((res = getopt(argc, argv, "?m:t:s:r:p:u:a:h")) != -1) {
        switch (res) {
        case 'm':
            mode = atoi(optarg);
            break;

        case 't':
            strcpy(s_pToken, optarg);
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
        case 's':
            strcpy(s_nvr_sub_pid, optarg);
            break;

        case 'h':
        default:
            usage(argv[0]);
            return -1;
        }
    }

#ifdef TUYA_DEMO_DEBUG_DUMP
    signal(SIGSEGV, dump);
#endif

    /*step: Init XVR SDK 。注：WIFI_GW 表示无线的XVR,但当前XVR SDK 以对接有线网络为主*/
#if defined(WIFI_GW) && (WIFI_GW == 1)
    IPC_APP_Init_SDK(mode, token);
#else
    //The token interaction with TUYA APP is implemented in SDK of wired equipment.
    IPC_APP_Init_SDK();
#endif

    //step：等待MQTT上线，确保MQTT上线后，才可以处理其他业务
    /* whether SDK is connected to MQTT */
    while (s_mqtt_status != 1) {
        sleep(1);
    }

#ifdef BETA_DEBUG_ONVIF
    TUYA_APP_Enable_Onvif_Client();
    char onvif_input[64] = {0};
    while (1) {
        memset(onvif_input,0,sizeof(onvif_input));
        scanf("%s",onvif_input);

        if (0 == strcmp(onvif_input,"probe_start"))
        {
            TUYA_APP_Onvif_Probe_Start();
        }
        else if (0 == strcmp(onvif_input,"probe_stop"))
        {
            TUYA_APP_Onvif_Probe_Stop();
        }
        else if (0 == strncmp(onvif_input,"onvif_add",strlen("onvif_add")))
        {
            INT_T device = 0;
            INT_T probe_index = 0;
            sscanf(onvif_input, "onvif_add_dev%d_idx%d", &device, &probe_index);
            TUYA_APP_Onvif_Add_Device(device, probe_index);
        }
        else if (0 == strcmp(onvif_input, "onvif_finish"))
        {
            break;
        }
        else if (0 == strncmp(onvif_input, "onvif_del",strlen("onvif_del")))
        {
            INT_T device = 0;
            sscanf(onvif_input, "onvif_del_dev%d", &device);
            TUYA_APP_Onvif_Delete_Device(device);
        }

        usleep(100 * 1000);
    }

    while (1)
    {
        if (TUYA_APP_Onvif_Device_Online())
        {
            break;
        }
        else
        {
            usleep(100 * 1000);
        }
    }
    PR_DEBUG("leave onvif add device proc");
#endif

    /*step：add xvr sub dev to main device */
    TUYA_APP_Add_XVR_Dev();
    
    /* step:init ring buffer for user to append av frame */
    TUYA_APP_Init_Ring_Buffer_all();
    
    /*step：At least one system time synchronization after networking*/
    IPC_APP_Sync_Utc_Time();
#ifdef DEMO_NVR_ONVIF_ENABLE
    TUYA_IPC_Onvif_Start_Stream();
#endif

#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__==1)
    /* Start local storage, since it involves synchronizing time from the server, 
    it is recommended that it be placed after networking */
    TUYA_APP_Init_Stream_Storage();
#endif
#if  !(defined(DEMO_USE_AS_NVR) && (DEMO_USE_AS_NVR == 1))
     TUYA_STATION_Dev_Fun_Start();
#endif
    /* step:Enable the p2p ,用于APP预览*/
    TUYA_APP_Enable_P2PTransfer(s_mgr_info.max_p2p_user);

    /*step: Upload all local configuration item (DP) status when MQTT connection is successful */
    IPC_APP_upload_all_status_XVR();
    
    /*step：用于子设备心跳在线处理*/
    TUYA_APP_Enable_SUB_DEV_HB();
    
    /*step:用户根据需要，选择云存功能。本接口是开启云存的过程 。如果不需要，可以注释此步骤。用于节约内存和性能*/
    TUYA_APP_Enable_SubDev_CloudStorage();

    /* step：用于模拟设备如何向XVR SDK 送音视频流的过程
     * send video/audio to ring buffer */
    /*Demo uses file simulate external audio and video input.
    The actual data acquisition needs to be realized by developers. */
#ifndef DEMO_NVR_ONVIF_ENABLE
#if  (defined(DEMO_USE_AS_NVR) && (DEMO_USE_AS_NVR == 1))
    TUYA_APP_Fake_Data_2_Ringbuffer();
#endif
#endif

    /*step：用户使用接口自对应有些能力集，默认情况下，不需要用户处理*/
//    tuya_ipc_skill_param_u skill_param = {.value = 50397699};
//    tuya_ipc_skill_enable(TUYA_IPC_SKILL_LOCALSTG,&skill_param);

    /*last step:更新XVR 设备的能力集。*/
    tuya_xvr_upload_skills(DEMO_NVR_SUB_DEV_NUM);

//===========================================================================================//
    //以下是模拟一些功能。用户可以参考
    /* Starting the detection tasks and trigger alarm reporting/local storage/cloud storage tasks through detection results  */
    pthread_t motion_detect_thread;
    pthread_create(&motion_detect_thread, NULL, thread_md_proc, NULL);
    pthread_detach(motion_detect_thread);

#if  defined(BETA_DYNAMIC_UPDATE_RING_BUFFER) && (BETA_DYNAMIC_UPDATE_RING_BUFFER==1)
    //模拟动态更新设备媒体信息
    pthread_t update_theard;
    extern VOID *TUYA_Media_Info_Update_Thread(VOID *arg);
    pthread_create(&update_theard, NULL, TUYA_Media_Info_Update_Thread, NULL);
    pthread_detach(update_theard);
#endif

    /* Manual simulation of related functions */
    char test_input[64] = { 0 };
    extern int fake_md_status;
    while (1) {
        char sub_dev_id[64] = { 0 };
        int ret = tuya_xvr_dev_devId_get_by_chan(0, sub_dev_id, 64);
        if (0 != ret) {
            printf("tuya_xvr_dev_devId_get_by_chan chan 0 err%d\n", ret);
        }        

        memset(test_input,0,sizeof(test_input));
        scanf("%s",test_input);
        /* get sdk work status */
        if (0 == strcmp(test_input, "status")) {
            IPC_REGISTER_STATUS status = tuya_ipc_get_register_status();
            printf("current register status %d[0:unregistered 1:registered 2:activated]\n", status);
        } else if (0 == strcmp(test_input, "suspend")) {
            //TUYA_APP_LOW_POWER_ENABLE();
        } else if (0 == strcmp(test_input, "showSubDevInfo")) {
            printf("showSubDevInfo\n");
            tuya_xvr_dev_info_show();
        } else if (0 == strcmp(test_input, "showSubCloudInfo")) {
            printf("showSubCloudInfo\n");
            tuya_xvr_cloud_storage_info_show();
        } 
        /*  Simulate the start of events */
        else if (0 == strncmp(test_input, "bell", 4)) { // chan 0 triggle door bell
            //Report the message and data of the doorbell
            //Using file simulation, you can actually use the JPEG address and size obtained from the chip encoder.
            char snapfile[256];
            snprintf(snapfile, 256, "%s/resource/media/demo_snapshot.jpg", s_raw_path);
            FILE* fp = fopen(snapfile, "r+");
            if (NULL == fp) {
                printf("fail to open snap.jpg\n");
                continue;
            }
            fseek(fp, 0, SEEK_END);
            int snapshot_size = ftell(fp);
            char* snapshot_buf = (char*)malloc(snapshot_size);
            if(!snapshot_buf){
                printf("malloc err\n");
                continue;
            }
            fseek(fp, 0, SEEK_SET);
            fread(snapshot_buf, snapshot_size, 1, fp);
            fclose(fp);

            char* p = (char*)((char*)&test_input + 4);
            int chanNo = atoi(p);
            printf("bell chanNo %d\n", chanNo);

            /* Push the detection message and the current snapshot image to the APP . 
    		Snapshot image acquisition needs to be implemented by the developer */
           // tuya_ipc_notify_door_bell_press(snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG);
            tuya_xvr_notify_door_bell_press(0, snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG, NOTIFICATION_NAME_DOORBELL);
           //  tuya_ipc_notify_door_bell_press_4_CVI(0 , snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG, NOTIFICATION_NAME_DOORBELL);

            free(snapshot_buf);
        } else if (0 == strncmp(test_input, "ac_bell", 7)) { // chan 0 triggle door bell
            //Report the message and data of the doorbell
            //Using file simulation, you can actually use the JPEG address and size obtained from the chip encoder.
            char snapfile[256];
            snprintf(snapfile, 256, "%s/resource/media/demo_snapshot.jpg", s_raw_path);
            FILE* fp = fopen(snapfile, "r+");
            if (NULL == fp) {
                printf("fail to open snap.jpg\n");
                continue;
            }
            fseek(fp, 0, SEEK_END);
            int snapshot_size = ftell(fp);
            char* snapshot_buf = (char*)malloc(snapshot_size);
            if(!snapshot_buf){
                printf("malloc err\n");
                continue;
            }
            fseek(fp, 0, SEEK_SET);
            fread(snapshot_buf, snapshot_size, 1, fp);
            fclose(fp);

            char* p = (char*)((char*)&test_input + 4);
            int chanNo = atoi(p);
            printf("bell chanNo %d\n", chanNo);

            /* Push the detection message and the current snapshot image to the APP . 
    		Snapshot image acquisition needs to be implemented by the developer */
           // tuya_ipc_notify_door_bell_press(snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG);
            tuya_xvr_notify_door_bell_press_generic(DOORBELL_AC, 0, snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG, NOTIFICATION_NAME_DOORBELL);
           //  tuya_ipc_notify_door_bell_press_4_CVI(0 , snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG, NOTIFICATION_NAME_DOORBELL);

            free(snapshot_buf);
        } else if (0 == strcmp(test_input, "move") ){
            //Report the message and data of the move detect
            //Using file simulation, you can actually use the JPEG address and size obtained from the chip encoder.
            char snapfile[256];
            snprintf(snapfile, 256, "%s/resource/media/demo_snapshot.jpg", s_raw_path);
            FILE* fp = fopen(snapfile, "r+");
            if (NULL == fp) {
                printf("fail to open snap.jpg path %s\n", snapfile);
                continue;
            }
            fseek(fp, 0, SEEK_END);
            int snapshot_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char* snapshot_buf = (char*)malloc(snapshot_size);
            if(!snapshot_buf){
                printf("malloc err\n");
                continue;
            }
            fread(snapshot_buf, snapshot_size, 1, fp);
            fclose(fp);

            printf("sub dev chan[1] id[%s] trggger move event\n", sub_dev_id);
            /* Push the detection message and the current snapshot image to the APP . 
            Snapshot image acquisition needs to be implemented by the developer */
            int event_id = tuya_xvr_cloud_storage_event_add((CHAR_T*)&sub_dev_id, EVENT_TYPE_MOTION_DETECT, 20);// event start
            printf("=================== sub dev chan[1] id[%s] trggger move event\n", sub_dev_id);
            ret = tuya_xvr_notify_with_event((CHAR_T*)sub_dev_id, snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG, NOTIFICATION_NAME_MOTION,TRUE);
            if (OPRT_OK != ret) {
                printf("tuya_ipc_dev_notify_with_event err %d\n", ret);
              //  free(snapshot_buf);
             //   continue;
            }
            sleep(120);
            ret = tuya_xvr_cloud_storage_event_delete((CHAR_T*)&sub_dev_id, event_id); // event stop
            free(snapshot_buf);
        } else if (0 == strcmp(test_input, "showcloud")) {
            tuya_xvr_cloud_storage_info_show();
        } else if (0 == strcmp(test_input, "showsub")) {
            extern void tuya_xvr_dev_info_show();
            tuya_xvr_dev_info_show();
        }else if(0 == strcmp(test_input,"offline"))
        {
            tuya_iot_dev_online_update(sub_dev_id,false,true);
            printf("offline test\n");
        }else if(0 == strcmp(test_input,"online"))
        {
            tuya_iot_dev_online_update(sub_dev_id,true,true);
            printf("offline test\n");
        }else if(0 == strcmp(test_input,"getptz"))
        {
            S_PRESET_CFG preset_cfg={0};
             tuya_xvr_preset_get(0,&preset_cfg);
            printf("getptz test\n");
        }
        else if (0==strcmp(test_input,"xvr_dev_info"))
        {
            int i =0;
            for(int i =0;i<4;i++)
            {
                char xvr_dev_id[64] = { 0 };
                int ret = tuya_xvr_dev_devId_get_by_chan(i, xvr_dev_id, 64);
                if (0 != ret) {
                    printf("tuya_xvr_dev_devId_get_by_chan chan 0 err%d\n", ret);
                }
            printf("chan %d get devid is %s\n",i,xvr_dev_id);
            }
        }
        else if (0 == strcmp(test_input,"xvr_br_event"))
        {
            char snapfile[256];
            snprintf(snapfile, 256, "%s/resource/media/demo_snapshot.jpg", s_raw_path);
            FILE* fp = fopen(snapfile, "r+");
            if (NULL == fp) {
                printf("fail to open snap.jpg path %s\n", snapfile);
                continue;
            }
            fseek(fp, 0, SEEK_END);
            int snapshot_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char* snapshot_buf = (char*)malloc(snapshot_size);
            if(!snapshot_buf){
                printf("malloc err\n");
                continue;
            }
            fread(snapshot_buf, snapshot_size, 1, fp);
            fclose(fp);

            char extend_data[128]={0};
            //用户按照需求填写 extend_data数据到SDK
            snprintf(extend_data,128,"\"eventId\":\"1111\",\"leftTopX\":100,\"leftTopY\":200,\"rightBtmX\":300,\"rightBtmY\":400");
            TUYA_XVR_BEHAVIOUR_ANALYSIS_CTX ctx ={0};

            ctx.eventTypeName = NOTIFICATION_NAME_AI_NO_HELMET;
            ctx.meidaType = NOTIFICATION_CONTENT_JPEG;
            ctx.notifyMessageCenter = 1;
            ctx.extendData = extend_data;
            ctx.mediaCnt =2;
            strncpy(ctx.devId,sub_dev_id,sizeof(ctx.devId));

            ctx.mediaCtx=malloc(2*sizeof(TUYA_XVR_MEDIA_CTX_S));
            ctx.mediaCtx[0].data=snapshot_buf;
            ctx.mediaCtx[0].len=snapshot_size;
            ctx.mediaCtx[0].speType=TUYA_XVR_BIG_SPEC_TYPE_IMAGE;
            ctx.mediaCtx[1].data=snapshot_buf;
            ctx.mediaCtx[1].len=snapshot_size;
            ctx.mediaCtx[1].speType=TUYA_XVR_SMALL_SPEC_TYPE_IMAGE;

            int ret = tuya_xvr_behaviour_analysis_event_upload(&ctx);
            printf("tuya_xvr_behaviour_analysis_event_upload ret is %d\n",ret);
            free(snapshot_buf);
            free(ctx.mediaCtx);

        }else if (0 == strcmp(test_input,"audio_set")){
            extern OPERATE_RET tuya_xvr_cloud_storage_audio_stat_set(INT_T chan,IN CONST BOOL_T isAudioOpen);
            tuya_xvr_cloud_storage_audio_stat_set(0,0);
        }else if (0 == strcmp(test_input,"move4")){
            //Report the message and data of the move detect
            //Using file simulation, you can actually use the JPEG address and size obtained from the chip encoder.
            char snapfile[256];
            snprintf(snapfile, 256, "%s/resource/media/demo_snapshot.jpg", s_raw_path);
            FILE* fp = fopen(snapfile, "r+");
            if (NULL == fp) {
                printf("fail to open snap.jpg path %s\n", snapfile);
                continue;
            }
            fseek(fp, 0, SEEK_END);
            int snapshot_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char* snapshot_buf = (char*)malloc(snapshot_size);
            if(!snapshot_buf){
                printf("malloc err\n");
                continue;
            }
            fread(snapshot_buf, snapshot_size, 1, fp);
            fclose(fp);

            char sub_dev_id_x[4][64] = { 0 };
            int event_id_x[4]={0};
            int i = 0;
            for (i = 0; i<4;i++){
                int ret = tuya_xvr_dev_devId_get_by_chan(i, (CHAR_T*)&sub_dev_id_x[i], 64);
                printf("sub dev chan[%d] id[%s] trggger move event\n",i,(CHAR_T*)&sub_dev_id_x[i]);
                event_id_x[i] = tuya_xvr_cloud_storage_event_add((CHAR_T*)&sub_dev_id_x[i], EVENT_TYPE_MOTION_DETECT, 120);// event start
                ret = tuya_xvr_notify_with_event((CHAR_T*)&sub_dev_id_x[i], snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG, NOTIFICATION_NAME_MOTION,TRUE);
                if (OPRT_OK != ret) {
                    printf("tuya_ipc_dev_notify_with_event err %d\n", ret);
                  //  free(snapshot_buf);
                 //   continue;
                }
            }
            sleep(120);
            for(i = 0; i<4;i++){
                 ret = tuya_xvr_cloud_storage_event_delete((CHAR_T*)&sub_dev_id_x[i], event_id_x[i]); // event stop
            }
            free(snapshot_buf);
            /* Push the detection message and the current snapshot image to the APP .
            Snapshot image acquisition needs to be implemented by the developer */

        }else if (0 == strcmp(test_input, "start")) {
            fake_md_status = 1;
        }else if (0 == strcmp(test_input, "stop")) {
            fake_md_status =0;

        } else if (0 == strcmp(test_input,"skill")){
            OPERATE_RET tuya_xvr_single_dev_skill_upload(CHAR_T *dev_id);
            tuya_xvr_single_dev_skill_upload(sub_dev_id);

        } else if (0 == strcmp(test_input,"memory")){
            int ret = tuya_ipc_ring_buffer_get_max_frame_size(0,0,0);
            printf("hmdg:max frame size %d\n",ret);
        }else if (0 == strcmp(test_input,"ai_event"))
        {
            char snapfile[256];
            snprintf(snapfile, 256, "%s/resource/media/demo_snapshot.jpg", s_raw_path);
            FILE* fp = fopen(snapfile, "r+");
            if (NULL == fp) {
                printf("fail to open snap.jpg path %s\n", snapfile);
                continue;
            }
            fseek(fp, 0, SEEK_END);
            int snapshot_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char* snapshot_buf = (char*)malloc(snapshot_size);
            if(!snapshot_buf){
                printf("malloc err\n");
                continue;
            }
            fread(snapshot_buf, snapshot_size, 1, fp);
            fclose(fp);
            char result[256];
            tuya_xvr_ai_image_upload(sub_dev_id,snapshot_buf,snapshot_size,result,sizeof(result));
            char extend_data[128]={0};
            //用户按照需求填写 extend_data数据到SDK
//            snprintf(extend_data,256,"\"bigImage\": {\"bucket\": \"ty-cn-storage30\",\"path\": \"/72c019-2930117-tuyadea58eed02c3eb14/detect/1551083405.jpeg\",\"secretKey\": \"112233445566778899aabbccddeeff00\",\"expirTime\": 1551083405 },\"p1\": \"p1\",\"p2\": \"p2\",\"p3\": \"p3\"");
            snprintf(extend_data,256,"\"bigImage\":%s,\"p1\": \"p1\",\"p2\": \"p2\",\"p3\": \"p3\"",result);
            TUYA_XVR_AI_EVENT_CTX ctx={0};
            strncpy(ctx.eventId,"11111",sizeof("11111"));
            strncpy(ctx.cmd,"ipc_ai",sizeof("ipc_ai"));
            strncpy(ctx.eventType,"sub_event_type",sizeof("sub_event_type"));
            strncpy(ctx.subVersion,"1.0",sizeof("1.0"));
            strncpy(ctx.devId,sub_dev_id,sizeof(ctx.devId));
            ctx.bodyData=extend_data;
            ctx.bodyLen=strlen(extend_data);

            int ret = tuya_xvr_ai_event_notify(&ctx);
            printf("tuya_xvr_ai_event_notify ret is %d\n",ret);

        }
#ifdef DEMO_NVR_ONVIF_ENABLE
        else if (0 == strcmp(test_input, "probe_start"))
        {
            TUYA_APP_Onvif_Probe_Start();
        }
        else if (0 == strcmp(test_input, "probe_stop"))
        {
            TUYA_APP_Onvif_Probe_Stop();
        }
        else if (0 == strncmp(test_input, "onvif_add", strlen("onvif_add")))
        {
            INT_T device = 0;
            INT_T probe_index = 0;
            sscanf(test_input, "onvif_add_dev%d_idx%d", &device, &probe_index);
            TUYA_APP_Onvif_Add_Device(device, probe_index);
        }
        else if (0 == strncmp(test_input, "onvif_del", strlen("onvif_del")))
        {
            INT_T device = 0;
            sscanf(test_input, "onvif_del_dev%d", &device);
            TUYA_APP_Onvif_Delete_Device(device);
        }
        else if (0 == strncmp(test_input, "ptz_start", strlen("ptz_start")))
        {
            INT_T device = 0;
            sscanf(test_input, "ptz_start_dev%d", &device);
            STATIC FLOAT_T velocity = 1.0;
            TUYA_IPC_Onvif_PTZ_Move_Start(device, velocity, velocity, 0.0);
            velocity = -velocity;
        }
        else if (0 == strncmp(test_input, "ptz_stop", strlen("ptz_stop")))
        {
            INT_T device = 0;
            sscanf(test_input, "ptz_stop_dev%d", &device);
            TUYA_IPC_Onvif_PTZ_Move_Stop(device);
        }
        else if (0 == strncmp(test_input, "get_presets", strlen("get_presets")))
        {
            INT_T device = 0;
            sscanf(test_input, "get_presets_dev%d", &device);
            TUYA_IPC_Onvif_Get_PTZ_Preset(device);
        }
        else if (0 == strncmp(test_input, "add_preset", strlen("add_preset")))
        {
            INT_T device = 0;
            CHAR_T preset_name[64] = {0};
            sscanf(test_input, "add_preset_dev%d_%s", &device, preset_name);
            TUYA_IPC_Onvif_Add_PTZ_Preset(device, preset_name);
        }
        else if (0 == strncmp(test_input, "set_preset", strlen("set_preset")))
        {
            INT_T device = 0;
            CHAR_T preset_token[64] = {0};
            sscanf(test_input, "set_preset_dev%d_%s", &device, preset_token);
            TUYA_IPC_Onvif_Set_PTZ_Preset(device, preset_token, "preset");
        }
        else if (0 == strncmp(test_input, "remove_preset", strlen("remove_preset")))
        {
            INT_T device = 0;
            CHAR_T preset_token[64] = {0};
            sscanf(test_input, "remove_preset_dev%d_%s", &device, preset_token);
            TUYA_IPC_Onvif_Remove_PTZ_Preset(device, preset_token);
        }
        else if (0 == strncmp(test_input, "goto_preset", strlen("goto_preset")))
        {
            INT_T device = 0;
            CHAR_T preset_token[64] = {0};
            sscanf(test_input, "goto_preset_dev%d_%s", &device, preset_token);
            TUYA_IPC_Onvif_Goto_PTZ_Preset(device, preset_token);
        }
#endif

#if defined(__USER_DO_NOT_OPEN__)&&(__USER_DO_NOT_OPEN__ == 1)
else if (0 == strcmp(test_input, "storstart")) {
          // tuya_xvr_dev_set_alive_by_chan(0, TRUE); // demo zxq
            printf("hmdg:xxxxxxxxxxxxxxxxxx\n");
            ty_hdd_start(sub_dev_id);
            ty_hdd_set_stat(sub_dev_id, 2,0);

           // tuya_ipc_stor_start_all();
        } else if (0 == strcmp(test_input, "storstop")) {
             ty_hdd_stop(sub_dev_id);
          //  tuya_ipc_stor_stop_all();
        } else if (0 == strcmp(test_input, "stordebug")) {
             tuya_ipc_ss_show_debug_info();
        }
#endif
        else if (0== strcmp(test_input,"update_media")) {
            extern INT_T g_update_meida_flag ;
            g_update_meida_flag = 1;
        } else if (0==strcmp(test_input,"update_version")) {
            tuya_xvr_dev_version_update(sub_dev_id,"1.2.0");
        } else if(0 == strcmp(test_input,"unbind")) {
            #define BIND_TEST_CNT 1
            INT_T unbind_chan[BIND_TEST_CNT] = {-1};
            tuya_xvr_dev_unbind(sub_dev_id,&unbind_chan[0]);
            TUYA_XVR_DEV_CUSTOM_BIND_CTX_T bind_ctx_ptr={0};
            bind_ctx_ptr.bind_dev_cnt = BIND_TEST_CNT;
            bind_ctx_ptr.tp=0;//参数表示子设备类型，通常可以10，或者0.建议些0，这边表示IPC主设备的类型，这样在升级时，可以复用对应IPC固件
            bind_ctx_ptr.uddd=(0x2 << 24);//以demo固定写死为准
            TUYA_XVR_DEV_BIND_INFO_T xvr_dev_list[BIND_TEST_CNT]={0};
            int i =0;
            for(i = 0;i < BIND_TEST_CNT;i++){
                memcpy(xvr_dev_list[i].product_id,"cjg5ilik41ufa1ag",strlen("cjg5ilik41ufa1ag"));
                memcpy(xvr_dev_list[i].version,"2.0.0",strlen("2.0.0"));
                xvr_dev_list[i].binded_chan = unbind_chan[i];
                xvr_dev_list[i].binded_chan = unbind_chan[i];
            }
            bind_ctx_ptr.dev_info = xvr_dev_list;
            tuya_xvr_dev_custom_binds(&bind_ctx_ptr);
        } else if(0 == strcmp(test_input,"virtual_num")) {
            extern INT_T test_client_online(void);
            printf("virtaul num is %d\n",test_client_online());
        } else if (0 == strcmp(test_input,"set_g_dev_0")) {
            extern void test_set_g_dev_cnt(int);
            test_set_g_dev_cnt(0);
        }else if (0 == strcmp(test_input,"set_g_dev_1")) {
            extern void test_set_g_dev_cnt(int);
            test_set_g_dev_cnt(1);
            /* aes128解密 */
        } else if(0 == strcmp(test_input,"space_s")) {
            char out[] = {"{\"passwd\":\"yyy yy\"}"};
            ty_cJSON *root = ty_cJSON_Parse(out);
            if(root == NULL) {
                PR_DEBUG("NOT SUPPORT");
            }
        }
        usleep(100 * 1000);
    }
    return 0;
}
#ifdef __cplusplus
}
#endif
