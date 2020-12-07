#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>
#include "tuya_cloud_base_defs.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_doorbell_demo.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_ipc_motion_detect_demo.h"
#include "tuya_ipc_system_control_demo.h"
#include "ty_cJSON.h"

#include "tuya_ipc_sd_demo.h"

#define IPC_APP_STORAGE_PATH "/tmp/car"                 //Path to save tuya sdk DB files, should be readable, writeable and storable
#define IPC_APP_UPGRADE_FILE "/tmp/car/upgrade.file"    //File with path to download file during OTA
#define IPC_APP_SD_BASE_PATH "/mnt/sdcard/car/"         //SD card mount directory
#define IPC_APP_PID "phzexsryldsh3o5i"   //Product ID of TUYA device, this is for demo only. \
                                         //Contact tuya PM/BD for official pid.
#define IPC_APP_UUID "tuya5cd5e4a9dd695421" //Unique identification of each device \
                                         //Contact tuya PM/BD for developing devices or BUY more
#define IPC_APP_AUTHKEY "f7tCj8EeT8woJLNdnbRYvSBfNlhO41Zh" 
                                         //Authentication codes corresponding to UUID, one machine one code, paired with UUID.
#define IPC_APP_VERSION "1.0.1"         //Firmware version displayed on TUYA APP

STATIC INT_T s_mqtt_status = 0;
IPC_MGR_INFO_S s_mgr_info = { 0 };
CHAR_T s_raw_path[128] = { 0 };
extern IPC_MEDIA_INFO_S s_media_info[IPC_CHAN_NUM];
extern int g_demo_sd_status;
extern INT_T __tuya_app_read_INT(CHAR_T* key);
extern void usr_set_net_ap();
extern void usr_set_net_dev(char* dev);

//#define TUYA_DEMO_DEBUG_DUMP
#ifdef TUYA_DEMO_DEBUG_DUMP
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
        printf("%s\n", strings[i]);
    }

    free(strings);
    exit(0);
}
#endif

STATIC VOID __IPC_APP_Get_Net_Status_cb(IN CONST BYTE_T stat)
{
    PR_DEBUG("Net status change to:%d", stat);
    switch (stat) {
        case GB_STAT_CLOUD_CONN: //for wired ipc
            IPC_APP_Notify_LED_Sound_Status_CB(IPC_MQTT_ONLINE);
            PR_DEBUG("mqtt is online\r\n");
            s_mqtt_status = 1;
            break;
        default:
            break;
    }
}

int tuya_qrcode_printf(char* msg)
{
    // TODO: show qrode
    printf("\n\n#### qrcode:%s\n\n\n", msg);
    return 0;
}

void active_shourturl_cb(CONST char* shorturl)
{
    ty_cJSON* item = ty_cJSON_Parse(shorturl);
    tuya_qrcode_printf(ty_cJSON_GetObjectItem(item, "shortUrl")->valuestring);
    ty_cJSON_Delete(item);
}

OPERATE_RET IPC_APP_Init_SDK(WIFI_INIT_MODE_E init_mode, CHAR_T* p_token)
{
    PR_DEBUG("SDK Version:%s\r\n", tuya_ipc_get_sdk_info());

    memset(&s_mgr_info, 0, sizeof(IPC_MGR_INFO_S));
    strcpy(s_mgr_info.storage_path, IPC_APP_STORAGE_PATH);
    strcpy(s_mgr_info.upgrade_file_path, IPC_APP_UPGRADE_FILE);
    strcpy(s_mgr_info.sd_base_path, IPC_APP_SD_BASE_PATH);
    strcpy(s_mgr_info.product_key, IPC_APP_PID);
    strcpy(s_mgr_info.uuid, IPC_APP_UUID);
    strcpy(s_mgr_info.auth_key, IPC_APP_AUTHKEY);
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
    strcpy(env.product_key, s_mgr_info.product_key);
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
    env.active_shorturl = active_shourturl_cb;

    tuya_ipc_init_sdk(&env);
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE == 1)
    //set_region(E_REGION_CN);
    p_token = NULL;
#endif
    tuya_ipc_start_sdk(init_mode, p_token);
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

int main(int argc, char* argv[])
{
    INT_T res = -1;
    CHAR_T token[30] = { 0 };
    WIFI_INIT_MODE_E mode = WIFI_INIT_AUTO;

#if defined(WIFI_GW) && (WIFI_GW == 0)
    mode = WIFI_INIT_NULL;
#endif

    strcpy(s_raw_path, "/tmp"); //Path where demo resources locates
    while ((res = getopt(argc, argv, "?m:t:s:r:h")) != -1) {
        switch (res) {
        case 'm':
            mode = atoi(optarg);
            break;

        case 't':
            strcpy(token, optarg);
            break;

        case 'r':
            strcpy(s_raw_path, optarg);
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

    // set net work
    int ap_onoff = __tuya_app_read_INT("tuya_is_ap");
    if (ap_onoff) {
        // TODO: start ap
        void usr_set_net_ap();
        system("sh ap_mode.sh start");
    } else {
        // TODO: start wire
        system("sh ap_mode.sh stop");
    }

    /* Init SDK */
    IPC_APP_Init_SDK(mode, token);

    /* whether SDK is connected to MQTT */
    while (s_mqtt_status != 1) {
        usleep(100000);
    }

    /*Demo uses files to simulate audio/video/jpeg inputs. 
    The actual data acquisition needs to be realized by developers. */
    int ipcChan = 0;
#define STREAM_NUM 4 // suppose 2 channel, each has main\sub streams, total 4 streams
    int streams[STREAM_NUM] = { 0, 1, 2, 3 };
    pthread_t h264_output_thread[STREAM_NUM];
    for (ipcChan = 0; ipcChan < STREAM_NUM; ipcChan++) {
        pthread_create(&h264_output_thread[ipcChan], NULL, thread_live_video, (void*)&streams[ipcChan]);
        pthread_detach(h264_output_thread[ipcChan]);
    }

    pthread_t pcm_output_thread[2];
    for (ipcChan = 0; ipcChan < 2; ipcChan++) {
        pthread_create(&pcm_output_thread[2], NULL, thread_live_audio, (void*)&streams[ipcChan]);
        pthread_detach(pcm_output_thread[2]);
    }

    /*At least one system time synchronization after networking*/
    IPC_APP_Sync_Utc_Time();

    printf_media_info(&s_media_info[0]);
    printf_media_info(&s_media_info[1]);

    /* Start local storage. Tt is recommended to be after ONLINE, or make sure the system time is correct */
    TUYA_APP_Init_Stream_Storage(s_mgr_info.sd_base_path);

    /* Enable TUYA P2P service after the network is CONNECTED. 
       Note: For low-power camera, invoke this API as early as possible(can be before mqtt online) */
    TUYA_APP_Enable_P2PTransfer(s_mgr_info.max_p2p_user);

    /* Upload all local configuration item (DP) status when MQTT connection is successful */
    IPC_APP_upload_all_status();

    /*!!!very important! After all module inited, update skill to tuya cloud */
    tuya_ipc_upload_skills();

#if 0
    /* Starting the detection tasks and trigger alarm reporting/local storage/cloud storage tasks through detection results  */
    pthread_t motion_detect_thread;
    pthread_create(&motion_detect_thread, NULL, thread_md_proc, NULL);
    pthread_detach(motion_detect_thread);
#endif
    /* Manual simulation of related functions */
    char test_input[64] = { 0 };
    int loop = 0;
    while (1) {
        scanf("%s", test_input);
        /* Simulation of getting device's activation status */
        if (0 == strcmp(test_input, "status")) {
            IPC_REGISTER_STATUS status = tuya_ipc_get_register_status();
            printf("current register status %d[0:unregistered 1:registered 2:activated]\n", status);
        }
        else if (0 == strcmp(test_input, "album")) {
            album_file_put();
        } else if (0 == strcmp(test_input, "sdcardout")) {
            g_demo_sd_status = 5;
            printf("g_demo_sd_status %d", g_demo_sd_status);
        } else if (0 == strcmp(test_input, "sdcardin")) {
            g_demo_sd_status = 1;
            printf("g_demo_sd_status %d", g_demo_sd_status);
        } else if (0 == strcmp(test_input, "sdcarderr")) {
            g_demo_sd_status = 2;
            printf("g_demo_sd_status %d", g_demo_sd_status);
        }
        
        if (loop++ > 1000) {
            album_file_put();
            loop = 0;
        }

        usleep(100 * 1000);
    }

    return 0;
}
