#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
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
#include "tuya_ipc_sdk_simple_start.h"
#include "tuya_ipc_p2p_demo.h"
#include "tuya_ipc_upgrade_demo.h"
#include "tuya_ipc_sd_demo.h"
#include "tuya_ipc_upgrade_demo.h"
#include <stddef.h>
#include <execinfo.h>
#include <signal.h>
#include "tuya_ipc_video_proc.h"
#define IPC_APP_STORAGE_PATH    "/tmp/"   //Path to save tuya sdk DB files, should be readable, writeable and storable
#define IPC_APP_UPGRADE_FILE    "/tmp/upgrade.file" //File with path to download file during OTA
#define IPC_APP_SD_BASE_PATH    "/tmp/"      //SD card mount directory
//#define IPC_APP_PID             "tuya_pid"  //Product ID of TUYA device, this is for demo only.
//#define IPC_APP_UUID            "tuya_uuid" //Unique identification of each device//Contact tuya PM/BD for developing devices or BUY more
//#define IPC_APP_AUTHKEY         "tuya_authkey" //Authentication codes corresponding to UUID, one machine one code, paired with UUID.
#define IPC_APP_VERSION         "1.2.3"     //Firmware version displayed on TUYA APP


IPC_MGR_INFO_S s_mgr_info = {0};

STATIC INT_T s_mqtt_status = 0;

STATIC CHAR_T s_token[30] = {0};

CHAR_T s_raw_path[128] = {0};
CHAR_T s_ipc_pid[64]="tuya_pid";//Product ID of TUYA device, this is for demo only.
CHAR_T s_ipc_uuid[64]="tuya_uuid";//Unique identification of each device//Contact tuya PM/BD for developing devices or BUY more
CHAR_T s_ipc_authkey[64]="tuya_authkey";//Authentication codes corresponding to UUID, one machine one code, paired with UUID.

#define TUYA_DEMO_DEBUG_DUMP
#ifdef TUYA_DEMO_DEBUG_DUMP
#define EXE_FILE_PATH "cmake-build/out/demo-ipc"
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
    snprintf(addr, len, "%s", (void *)p + 1);
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

void signal_handle(int sig)
{
    char cThreadName[32] = {0};
    prctl(PR_GET_NAME, (unsigned long)cThreadName);
    printf("[%s, %d] get signal(%d) thread(%d) name(%s)\n", __FUNCTION__, __LINE__, sig, syscall(__NR_gettid), cThreadName);

    switch(sig) {
    case SIGINT:
    case SIGKILL:
    case SIGTERM:
    case SIGSEGV:
    case SIGABRT:
        exit(1);
        break;

    default:
        break;
    }

    return;
}


#endif

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


OPERATE_RET TUYA_IPC_SDK_START(WIFI_INIT_MODE_E connect_mode, CHAR_T *p_token)
{
    PR_DEBUG("SDK Version:%s\r\n", tuya_ipc_get_sdk_info());
	TUYA_IPC_SDK_RUN_VAR_S ipc_sdk_run_var ={0};
	memset(&ipc_sdk_run_var,0,sizeof(ipc_sdk_run_var));

	/*certification information(essential)*/
	strcpy(ipc_sdk_run_var.iot_info.product_key,s_ipc_pid);
	strcpy(ipc_sdk_run_var.iot_info.uuid,s_ipc_uuid);
	strcpy(ipc_sdk_run_var.iot_info.auth_key,s_ipc_authkey);
	strcpy(ipc_sdk_run_var.iot_info.dev_sw_version,IPC_APP_VERSION);
	strcpy(ipc_sdk_run_var.iot_info.cfg_storage_path,IPC_APP_STORAGE_PATH);
	//low power  device
	ipc_sdk_run_var.iot_info.dev_type= LOW_POWER_DEV;

	/*connect mode (essential)*/
	ipc_sdk_run_var.net_info.connect_mode = connect_mode;
	ipc_sdk_run_var.net_info.net_status_change_cb = __IPC_APP_Get_Net_Status_cb;
	if(p_token)
	{
	    strcpy(ipc_sdk_run_var.debug_info.qrcode_token,p_token);
	}

	/*media info (essential)*/
    /* main stream(HD), video configuration*/
    /* NOTE
    FIRST:If the main stream supports multiple video stream configurations, set each item to the upper limit of the allowed configuration.
    SECOND:E_IPC_STREAM_VIDEO_MAIN must exist.It is the data source of SDK.
    please close the E_IPC_STREAM_VIDEO_SUB for only one stream*/
    ipc_sdk_run_var.media_info.media_info.channel_enable[E_IPC_STREAM_VIDEO_MAIN] = TRUE;    /* Whether to enable local HD video streaming */
    ipc_sdk_run_var.media_info.media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN] = 30;  /* FPS */
    ipc_sdk_run_var.media_info.media_info.video_gop[E_IPC_STREAM_VIDEO_MAIN] = 30;  /* GOP */
    ipc_sdk_run_var.media_info.media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN] = TUYA_VIDEO_BITRATE_1M; /* Rate limit */
    ipc_sdk_run_var.media_info.media_info.video_width[E_IPC_STREAM_VIDEO_MAIN] = 640; /* Single frame resolution of width*/
    ipc_sdk_run_var.media_info.media_info.video_height[E_IPC_STREAM_VIDEO_MAIN] = 360;/* Single frame resolution of height */
    ipc_sdk_run_var.media_info.media_info.video_freq[E_IPC_STREAM_VIDEO_MAIN] = 90000; /* Clock frequency */
    ipc_sdk_run_var.media_info.media_info.video_codec[E_IPC_STREAM_VIDEO_MAIN] = TUYA_CODEC_VIDEO_H264; /* Encoding format */

    /* substream(HD), video configuration */
    /* Please note that if the substream supports multiple video stream configurations, please set each item to the upper limit of the allowed configuration. */
    ipc_sdk_run_var.media_info.media_info.channel_enable[E_IPC_STREAM_VIDEO_SUB] = TRUE;     /* Whether to enable local SD video stream */
    ipc_sdk_run_var.media_info.media_info.video_fps[E_IPC_STREAM_VIDEO_SUB] = 30;  /* FPS */
    ipc_sdk_run_var.media_info.media_info.video_gop[E_IPC_STREAM_VIDEO_SUB] = 30;  /* GOP */
    ipc_sdk_run_var.media_info.media_info.video_bitrate[E_IPC_STREAM_VIDEO_SUB] = TUYA_VIDEO_BITRATE_512K; /* Rate limit */
    ipc_sdk_run_var.media_info.media_info.video_width[E_IPC_STREAM_VIDEO_SUB] = 640; /* Single frame resolution of width */
    ipc_sdk_run_var.media_info.media_info.video_height[E_IPC_STREAM_VIDEO_SUB] = 360;/* Single frame resolution of height */
    ipc_sdk_run_var.media_info.media_info.video_freq[E_IPC_STREAM_VIDEO_SUB] = 90000; /* Clock frequency */
    ipc_sdk_run_var.media_info.media_info.video_codec[E_IPC_STREAM_VIDEO_SUB] = TUYA_CODEC_VIDEO_H264; /* Encoding format */

    /* Audio stream configuration.
    Note: The internal P2P preview, cloud storage, and local storage of the SDK are all use E_IPC_STREAM_AUDIO_MAIN data. */
    ipc_sdk_run_var.media_info.media_info.channel_enable[E_IPC_STREAM_AUDIO_MAIN] = TRUE;         /* Whether to enable local sound collection */
    ipc_sdk_run_var.media_info.media_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN] = TUYA_CODEC_AUDIO_PCM;/* Encoding format */
    ipc_sdk_run_var.media_info.media_info.audio_sample [E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_SAMPLE_8K;/* Sampling Rate */
    ipc_sdk_run_var.media_info.media_info.audio_databits [E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_DATABITS_16;/* Bit width */
    ipc_sdk_run_var.media_info.media_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_CHANNEL_MONO;/* channel */
    ipc_sdk_run_var.media_info.media_info.audio_fps[E_IPC_STREAM_AUDIO_MAIN] = 25;/* Fragments per second */

    /*local storage (custome whether enable or not)*/
    ipc_sdk_run_var.local_storage_info.enable = 1;
    ipc_sdk_run_var.local_storage_info.max_event_num_per_day = 500;
    ipc_sdk_run_var.local_storage_info.skills = 0;//0 means all skills
    ipc_sdk_run_var.local_storage_info.sd_status_cb = tuya_ipc_sd_status_upload ;
	strcpy(ipc_sdk_run_var.local_storage_info.storage_path, IPC_APP_SD_BASE_PATH);

	/*cloud storage (custome whether enable or not)*/
    /*if no ase,it can equal NULL;*/
    extern OPERATE_RET AES_CBC_init(VOID);
    extern OPERATE_RET AES_CBC_encrypt(IN BYTE_T *pdata_in,  IN UINT_T data_len,
            INOUT BYTE_T *pdata_out,  OUT UINT_T *pdata_out_len,
            IN BYTE_T *pkey, IN BYTE_T *piv);
    extern OPERATE_RET AES_CBC_destory(VOID);
	ipc_sdk_run_var.cloud_storage_info.enable = 1;
	ipc_sdk_run_var.aes_hw_info.aes_fun.init = AES_CBC_init;
	ipc_sdk_run_var.aes_hw_info.aes_fun.encrypt =AES_CBC_encrypt;
	ipc_sdk_run_var.aes_hw_info.aes_fun.destory = AES_CBC_destory;


	/*p2p function (essential)*/
	ipc_sdk_run_var.p2p_info.max_p2p_client=5;
	ipc_sdk_run_var.p2p_info.live_mode = TRANS_DEFAULT_STANDARD;
	ipc_sdk_run_var.p2p_info.transfer_event_cb = __TUYA_APP_p2p_event_cb;
	ipc_sdk_run_var.p2p_info.rev_audio_cb = __TUYA_APP_rev_audio_cb;

	/*AI detect (custome whether enable or not)*/
	ipc_sdk_run_var.cloud_ai_detct_info.enable = 1;

	/*door bell (custome whether enable or not)*/
	ipc_sdk_run_var.video_msg_info.enable = 1;
	ipc_sdk_run_var.video_msg_info.type = MSG_BOTH;
	ipc_sdk_run_var.video_msg_info.msg_duration = 10;

	/*dp function(essential)*/
	ipc_sdk_run_var.dp_info.dp_query = IPC_APP_handle_dp_query_objs;
	ipc_sdk_run_var.dp_info.raw_dp_cmd_proc = IPC_APP_handle_raw_dp_cmd_objs;
	ipc_sdk_run_var.dp_info.common_dp_cmd_proc = IPC_APP_handle_dp_cmd_objs;

	/*upgrade function(essential)*/
	ipc_sdk_run_var.upgrade_info.enable = true;
	ipc_sdk_run_var.upgrade_info.upgrade_cb = IPC_APP_Upgrade_Inform_cb;

	ipc_sdk_run_var.iot_info.gw_reset_cb = IPC_APP_Reset_System_CB;
	ipc_sdk_run_var.iot_info.gw_restart_cb = IPC_APP_Restart_Process_CB;


	OPERATE_RET ret ;
    ret = tuya_ipc_sdk_start(&ipc_sdk_run_var);
    if(ret !=0 )
    {
    	printf("ipc sdk v5 start fail,please check run parameter，ret=%d\n",ret);

    }
	return ret;
}
#if 0
OPERATE_RET IPC_APP_SDK_START_V4(WIFI_INIT_MODE_E init_mode, CHAR_T *p_token)
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

    env.upgrade_cb_info.sub_device_upgrade_cb = IPC_APP_Upgrade_Inform_cb;

    env.gw_rst_cb = IPC_APP_Reset_System_CB;
    env.gw_restart_cb = IPC_APP_Restart_Process_CB;
    env.mem_save_mode = FALSE;
    int ret = tuya_ipc_init_sdk(&env);
    if(ret != 0)
    {
        printf("ipc sdk v4 init fail,please check run parameter，ret=%d\n",ret);
        return -1;
    }
    IPC_APP_Set_Media_Info();
    TUYA_APP_Init_Ring_Buffer();
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
    tuya_ipc_set_region(REGION_CN);
    p_token = NULL;
#endif
    ret = tuya_ipc_start_sdk(init_mode, p_token);
    if(ret != 0)
    {
        printf("ipc sdk v4 start fail,please check run parameter，ret=%d\n",ret);
        return -1;
    }
    return OPRT_OK;
}
#endif
VOID usage(CHAR_T *app_name)
{
    printf("%s -m mode -t token -r raw path -h\n", (CHAR_T *)basename(app_name));
    printf("\t m: 0-WIFI_INIT_AUTO 1-WIFI_INIT_AP 2-WIFI_INIT_DEBUG, refer to WIFI_INIT_MODE_E\n"
        "\t t: token get form qrcode info\n"
        "\t r: raw source file path\n"
        "\t c: choose sdk run method\n"
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
    INT_T sdk_start_mode = 0;//default v4

#if defined(WIFI_GW) && (WIFI_GW==0)
    mode = WIFI_INIT_NULL;
#endif

#ifdef TUYA_DEMO_DEBUG_DUMP
    signal(SIGSEGV,dump);
#endif

    signal(SIGINT, signal_handle);
    signal(SIGKILL, signal_handle);
    signal(SIGTERM, signal_handle);
    signal(SIGPIPE, SIG_IGN);

    strcpy(s_raw_path, "/tmp"); //Path where demo resources locates
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
    while((res = getopt(argc, argv, "?m:t:s:r:c:p:u:a:h")) != -1)
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
        case 'c':
            sdk_start_mode = atoi(optarg);
            break;
        case 'p':
            strcpy(s_ipc_pid,optarg);
            break;
        case 'u':
            strcpy(s_ipc_uuid,optarg);
            break;
        case 'a':
            strcpy(s_ipc_authkey,optarg);
            break;
        case 'h':
        default:
            usage(argv[0]);
            return -1;
        }
    }
#endif

    //start SDK default v4
    int ret = -1;
    ret = TUYA_IPC_SDK_START(mode,token);
    if(ret != 0)
    {
        return 0;
    }


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

    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)thread_md_proc;
    stappTask.pcName = "motion_detect";
    LOS_TaskCreate((UINT32 *)&taskid, &stappTask);
#else
    pthread_t h264_output_thread;
    pthread_create(&h264_output_thread, NULL, thread_live_video, NULL);
    pthread_detach(h264_output_thread);

    pthread_t pcm_output_thread;
    pthread_create(&pcm_output_thread, NULL, thread_live_audio, NULL);
    pthread_detach(pcm_output_thread);

    pthread_t motion_detect_thread;
    pthread_create(&motion_detect_thread, NULL, thread_md_proc, NULL);
    pthread_detach(motion_detect_thread);
#endif

    /* whether SDK is connected to MQTT */
    while(s_mqtt_status != 1)
    {
        usleep(100000);
    }
#ifdef __HuaweiLite__
    tuya_ipc_report_p2p_msg();
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
            snprintf(snapfile,64,"%s/resource/media/demo_snapshot.jpg",s_raw_path);
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
        else if (0 == strcmp(test_input, "start_low_power"))
        {
//            //this device info get from tuya ipc SDK
//        //    char devbuf[]="6c44bbd5972e2a992funl2";
//        //    char keybuf[]="4d7fc735ccac2cae";
//            char devbuf[]="6cce58037f56937765kwqg";
//            char keybuf[]="9c4fc747f148c052";
//           // char devbuf[]="6cb88c4fb06aaf7cbewszc";
//           // char keybuf[]="89efed934616cc39";
//            //int ip = 0xd4402e47;//yufa
//            int ip = 0xaf188c48;
//            int port =443;
            int ip=0;
            int port=0;
            int ret = tuya_ipc_low_power_server_get(&ip, &port);
            if(ret != 0)
            {
                printf("get low power ip  error %d\n",ret);
                continue;
            }
#define COMM_LEN 30
            char devid[COMM_LEN]={0};
            int id_len=COMM_LEN;
            ret = tuya_ipc_device_id_get(devid, &id_len);
            if(ret != 0)
            {
                printf("get devide error %d\n",ret);
                continue;
            }
            char local_key[COMM_LEN]={0};
            int key_len=COMM_LEN;
            ret = tuya_ipc_local_key_get(local_key, &key_len);
            if(ret != 0)
            {
                printf("get local key  error %d\n",ret);
                continue;
            }

            TUYA_APP_LOW_POWER_START(devid,local_key,ip,port);
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

