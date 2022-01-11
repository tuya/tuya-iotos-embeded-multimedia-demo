#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <execinfo.h>
#include <signal.h>
#include "tuya_cloud_base_defs.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_cloud_storage.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_system_control_demo.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_ipc_motion_detect_demo.h"
#include "tuya_ipc_doorbell_demo.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_p2p_demo.h"
#include "tuya_ipc_upgrade_demo.h"
#include "tuya_ipc_low_power_demo.h"
#include "tuya_ipc_video_proc.h"
#include "tuya_ipc_demo_default_cfg.h"
#if defined(ENABLE_IPC_GW_BOTH) && (ENABLE_IPC_GW_BOTH==1)
#include "scene_linkage.h"
#endif


STATIC CHAR_T s_token[30] = {0};

CHAR_T s_raw_path[128] = {0};
CHAR_T s_ipc_pid[64] = IPC_APP_PID;//Product ID of TUYA device, this is for demo only.
CHAR_T s_ipc_uuid[64] = IPC_APP_UUID;//Unique identification of each device//Contact tuya PM/BD for developing devices or BUY more
CHAR_T s_ipc_authkey[64] = IPC_APP_AUTHKEY;//Authentication codes corresponding to UUID, one machine one code, paired with UUID.

STATIC TUYA_IPC_SDK_RUN_VAR_S g_sdk_run_info = {0};

// #define TUYA_DEMO_DEBUG_DUMP
#ifdef TUYA_DEMO_DEBUG_DUMP
#define EXE_FILE_PATH "cmake-build/out/demo-ipc"  //if customer use backtrace, set bin path here.
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

#else
void signal_handle(int sig)
{
    char cThreadName[32] = {0};
    prctl(PR_GET_NAME, (unsigned long)cThreadName);
    printf("[%s, %d] get signal(%d) thread(%ld) name(%s)\n", __FUNCTION__, __LINE__, sig, syscall(__NR_gettid), cThreadName);

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

#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
VOID IPC_APP_qrcode_shorturl_cb(CHAR_T* shorturl)
{
    if(shorturl)
        PR_DEBUG("Get Shorturl: [%s]", shorturl);

    return;
}
#endif

STATIC VOID * tuya_ipc_sdk_mqtt_online_proc(PVOID_T arg)
{
    PR_DEBUG("tuya_ipc_sdk_mqtt_online_proc thread start success\n");
    while(IPC_APP_Get_MqttStatus() == FALSE)
    {
        sleep(1);
    }
    PR_DEBUG("tuya_ipc_sdk_mqtt_online_proc is start run\n");
    int ret;
    //同步服务器时间
    TIME_T time_utc;
    INT_T time_zone;
    do
    {
        //需要SDK同步到时间后才能开启下面的业务
        ret = tuya_ipc_get_service_time_force(&time_utc, &time_zone);

    } while(ret != OPRT_OK);

    if(FALSE == g_sdk_run_info.quick_start_info.enable)
    {
        TUYA_APP_Enable_P2PTransfer(&(g_sdk_run_info.p2p_info));
    }

    if(g_sdk_run_info.local_storage_info.enable)
    {
        ret = TUYA_APP_Init_Stream_Storage(&(g_sdk_run_info.local_storage_info));
        PR_DEBUG("local storage init result is %d\n",ret);
    }

    if(g_sdk_run_info.cloud_ai_detct_info.enable)
    {
        ret = TUYA_APP_Enable_AI_Detect();
        PR_DEBUG("ai detect result is %d\n",ret);
    }

    if(g_sdk_run_info.video_msg_info.enable)
    {
        ret =  TUYA_APP_Enable_Video_Msg(&(g_sdk_run_info.video_msg_info));
        PR_DEBUG("door bell init result is %d\n",ret);
    }

    if(g_sdk_run_info.cloud_storage_info.enable)
    {
        ret = TUYA_APP_Enable_CloudStorage(&(g_sdk_run_info.cloud_storage_info));
        PR_DEBUG("cloud storage init result is %d\n",ret);
    }

    IPC_APP_upload_all_status();

    tuya_ipc_upload_skills();
    PR_DEBUG("tuya_ipc_sdk_mqtt_online_proc is end run\n");

    return NULL;
}

STATIC VOID * tuya_ipc_sdk_low_power_p2p_init_proc(VOID * args)
{
    PR_DEBUG("start low power p2p\n");
    //todo process fail
    TUYA_APP_Enable_P2PTransfer(&(g_sdk_run_info.p2p_info));
    return NULL;
}

OPERATE_RET tuya_ipc_app_start(IN CONST TUYA_IPC_SDK_RUN_VAR_S * pRunInfo)
{
	if(NULL == pRunInfo)
	{
		PR_ERR("start sdk para is NULL\n");
		return OPRT_INVALID_PARM;
	}

    OPERATE_RET ret = 0;
    STATIC BOOL_T s_ipc_sdk_started = FALSE;
    if(TRUE == s_ipc_sdk_started )
    {
        PR_DEBUG("IPC SDK has started\n");
        return ret;
    }

	memcpy(&g_sdk_run_info, pRunInfo, SIZEOF(TUYA_IPC_SDK_RUN_VAR_S));

	/* 将码流信息保存到s_media_info，用于P2P的一些回调中匹配。客户可以根据自己的逻辑来实现。此处仅作参考 */
	IPC_APP_Set_Media_Info(&(g_sdk_run_info.media_info.media_info));

	//低功耗 优先开启P2P
    if(g_sdk_run_info.quick_start_info.enable)
    {
        pthread_t low_power_p2p_thread_handler;
        int op_ret = pthread_create(&low_power_p2p_thread_handler,NULL, tuya_ipc_sdk_low_power_p2p_init_proc, NULL);
        if(op_ret < 0)
        {
            PR_ERR("create p2p start thread is error\n");
            return -1;
        }
    }

    //setup1:创建等待mqtt上线进程，mqtt上线后，再开启与网络相关的业务
    pthread_t mqtt_status_change_handle;
    int op_ret = pthread_create(&mqtt_status_change_handle,NULL, tuya_ipc_sdk_mqtt_online_proc, NULL);
    if(op_ret < 0)
    {
        PR_ERR("create tuya_ipc_sdk_mqtt_online_proc  thread is error\n");
        return -1;
    }

	//setup2:init sdk
    TUYA_IPC_ENV_VAR_S env;
    memset(&env, 0, sizeof(TUYA_IPC_ENV_VAR_S));
    strcpy(env.storage_path, pRunInfo->iot_info.cfg_storage_path);
    strcpy(env.product_key,pRunInfo->iot_info.product_key);
    strcpy(env.uuid, pRunInfo->iot_info.uuid);
    strcpy(env.auth_key, pRunInfo->iot_info.auth_key);
    strcpy(env.dev_sw_version, pRunInfo->iot_info.dev_sw_version);
    strcpy(env.dev_serial_num, "tuya_ipc");
    //TODO:raw
    env.dev_raw_dp_cb = pRunInfo->dp_info.raw_dp_cmd_proc;
    env.dev_obj_dp_cb = pRunInfo->dp_info.common_dp_cmd_proc;
    env.dev_dp_query_cb = pRunInfo->dp_info.dp_query;
    env.status_changed_cb = pRunInfo->net_info.net_status_change_cb;
    env.upgrade_cb_info.upgrade_cb = pRunInfo->upgrade_info.upgrade_cb;
    env.gw_rst_cb = pRunInfo->iot_info.gw_reset_cb;
    env.gw_restart_cb = pRunInfo->iot_info.gw_restart_cb;
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
    env.qrcode_active_cb = pRunInfo->qrcode_active_cb;
#endif
    env.dev_type = pRunInfo->iot_info.dev_type;
    ret = tuya_ipc_init_sdk(&env);
	if(OPRT_OK != ret)
	{
		PR_ERR("init sdk is error\n");
		return ret;
	}

	//设置日志等级
	tuya_ipc_set_log_attr(pRunInfo->debug_info.log_level,NULL);
	
	//ring buffer 创建。
	ret = TUYA_APP_Init_Ring_Buffer();
	if(OPRT_OK != ret)
	{
		PR_ERR("create ring buffer is error\n");
		return ret;
	}
	
	ret = tuya_ipc_start_sdk(pRunInfo->net_info.connect_mode,pRunInfo->debug_info.qrcode_token);
	if(OPRT_OK != ret)
	{
		PR_ERR("start sdk is error\n");
		return ret;
	}

	s_ipc_sdk_started = true;
	PR_DEBUG("tuya ipc sdk start is complete\n");
	return ret;
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
	
	//normal device
	ipc_sdk_run_var.iot_info.dev_type= NORMAL_POWER_DEV;
	//if needed, change to low power device
	//ipc_sdk_run_var.iot_info.dev_type= LOW_POWER_DEV;

	/*connect mode (essential)*/
	ipc_sdk_run_var.net_info.connect_mode = connect_mode;
	ipc_sdk_run_var.net_info.net_status_change_cb = IPC_APP_Net_Status_cb;
	if(p_token)
	{
	    strcpy(ipc_sdk_run_var.debug_info.qrcode_token,p_token);
	}
	/* 0-5, the bigger, the more log */
	ipc_sdk_run_var.debug_info.log_level = 4;
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

    /*local storage (customer whether enable or not)*/
    ipc_sdk_run_var.local_storage_info.enable = 1;
    ipc_sdk_run_var.local_storage_info.max_event_num_per_day = 500;
    ipc_sdk_run_var.local_storage_info.skills = 0;//0 means all skills
    ipc_sdk_run_var.local_storage_info.sd_status_cb = tuya_ipc_sd_status_upload ;
	strcpy(ipc_sdk_run_var.local_storage_info.storage_path, IPC_APP_SD_BASE_PATH);

	/*cloud storage (custome whether enable or not)*/
    /*if no AES, ipc_sdk_run_var.aes_hw_info.aes_fun.* can equal NULL;*/
	ipc_sdk_run_var.cloud_storage_info.enable = TRUE;
    ipc_sdk_run_var.cloud_storage_info.en_audio_record = TRUE;
    ipc_sdk_run_var.cloud_storage_info.pre_record_time = -1; //set -1 to ignore it. default 2 seconds. 

	/*p2p function (essential)*/
    ipc_sdk_run_var.p2p_info.enable = TRUE;
    ipc_sdk_run_var.p2p_info.is_lowpower = FALSE;
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

    /*QR-active function(essential)*/
#if defined(QRCODE_ACTIVE_MODE) && (QRCODE_ACTIVE_MODE==1)
    ipc_sdk_run_var.qrcode_active_cb = IPC_APP_qrcode_shorturl_cb;
#endif

	OPERATE_RET ret ;
    ret = tuya_ipc_app_start(&ipc_sdk_run_var);
    if(ret !=0 )
    {
    	PR_DEBUG("ipc sdk start fail,please check run parameter，ret=%d\n",ret);
    }
	return ret;
}

VOID IPC_APP_simulation()
{
    /* Manual simulation of related functions */
    char test_input[64] = {0};
    while(1)
    {
        scanf("%s",test_input);
        /* Simulation of the start of motion detection events */
        if(0 == strcmp(test_input,"start"))
        {
            IPC_APP_set_motion_status(1);
        }
        /* Simulation of the end of event */
        else if(0 == strcmp(test_input,"stop"))
        {
            IPC_APP_set_motion_status(0);
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
            int snapshot_size = 150*1024;
            char *snapshot_buf = (char *)malloc(snapshot_size);
            int ret = IPC_APP_get_snapshot(snapshot_buf, &snapshot_size);
            if(ret != 0)
            {
                printf("Get snap fail!\n");
                continue;
            }
            /* Push the detection message and the current snapshot image to the APP.
            Snapshot image acquisition needs to be implemented by the developer */
            tuya_ipc_door_bell_press(DOORBELL_NORMAL, snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG);
            free(snapshot_buf);
        }
        /* Simulation of low power ipc */
        else if (0 == strcmp(test_input, "start_low_power"))
        {
            tuya_ipc_low_power_sample();
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
        else if( 0 == strcmp(test_input, "dp_test"))
        {
            //dp_simulation_unload();
            //dp_simulation_load();
        }

        usleep(100*1000);
    }

    return;
}

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
    while((res = getopt(argc, argv, "?m:t:s:r:p:u:a:h")) != -1)
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

    OPERATE_RET ret = OPRT_OK;
    ret = TUYA_IPC_SDK_START(mode,token);
    if(ret != OPRT_OK)
    {
        return ret;
    }

    IPC_APP_Init_Media_Task();
    TUYA_APP_Enable_Motion_Detect();

    /* whether SDK is connected to MQTT */
    while(IPC_APP_Get_MqttStatus() != 1)
    {
        usleep(100000);
    }
#ifdef __HuaweiLite__
    tuya_ipc_report_p2p_msg();
#endif

    IPC_APP_simulation();

    return 0;
}

