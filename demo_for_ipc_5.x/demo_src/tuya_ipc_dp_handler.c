/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm

  *FileName: tuya_ipc_dp_handler.c
  *
  * File Description：
  * 1. DP Point Setting and Acquisition Function API
  *
  * Developer work：
  * 1. Local configuration acquisition and update.
  * 2. Set local IPC attributes, such as picture flip, time watermarking, etc.
  *    If the function is not supported, leave the function blank.
  *
**********************************************************************************/
#include "tuya_iot_config.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_dp_handler.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_os_adapter.h"
#include "tuya_ipc_stream_storage_demo.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

/* Setting and Getting Integer Local Configuration.
The reference code here is written directly to tmp, requiring developers to replace the path.*/
STATIC VOID __tuya_app_write_INT(CHAR_T *key, INT_T value)
{
    //TODO

    CHAR_T tmp_cmd[128] = {0};
    snprintf(tmp_cmd, 128, "mkdir -p /tmp/tuya.cfgs/;echo %d > /tmp/tuya.cfgs/%s", value, key);
    printf("write int exc: %s \r\n", tmp_cmd);
    system(tmp_cmd);
}

STATIC INT_T __tuya_app_read_INT(CHAR_T *key)
{
    //TODO

    CHAR_T tmp_file[64] = {0};
    snprintf(tmp_file, 64, "cat /tmp/tuya.cfgs/%s", key);
    printf("read int exc: %s \r\n", tmp_file);
    FILE *p_file = popen(tmp_file, "r");
    if(p_file == NULL)
    {
        printf("open fail.\r\n");
        return 0;
    }
    memset(tmp_file, 0, 64);
    fgets(tmp_file, 64, p_file);
    INT_T value = atoi(tmp_file);
    printf("readstr:%s andint:%d \r\n", tmp_file, value);

    pclose(p_file);

    return value;
}

/* Setting and Getting String Local Configuration.
The reference code here is written directly to tmp, requiring developers to replace the path.*/
STATIC VOID __tuya_app_write_STR(CHAR_T *key, CHAR_T *value)
{
    //TODO

    CHAR_T tmp_cmd[256] = {0};
    snprintf(tmp_cmd, 256, "echo %s > /tmp/tuya.cfgs/%s", value, key);
    printf("write STR exc: %s \r\n", tmp_cmd);
    system(tmp_cmd);
}

STATIC INT_T __tuya_app_read_STR(CHAR_T *key, CHAR_T *value, INT_T value_size)
{
    //TODO

    memset(value, 0, value_size);

    CHAR_T tmp_file[64] = {0};
    snprintf(tmp_file, 64, "cat /tmp/tuya.cfgs/%s", key);
    printf("read str exc: %s \r\n", tmp_file);
    FILE *p_file = popen(tmp_file, "r");
    if(p_file == NULL)
    {
        printf("open fail.\r\n");
        return 0;
    }

    fgets(value, 256, p_file);
    value[strlen(value)] = 0;
    printf("readstr:%s \r\n", value);
    pclose(p_file);
    return 0;
}


#ifdef TUYA_DP_SLEEP_MODE
VOID IPC_APP_set_sleep_mode(BOOL_T sleep_mode)
{
    printf("set sleep_mode:%d \r\n", sleep_mode);
    //TODO
    /* sleep mode,BOOL type,true means sleep,false means wake */

    __tuya_app_write_INT("tuya_sleep_mode", sleep_mode);
}

BOOL_T IPC_APP_get_sleep_mode(VOID)
{
    BOOL_T sleep_mode = __tuya_app_read_INT("tuya_sleep_mode");
    printf("curr sleep_mode:%d \r\n", sleep_mode);
    return sleep_mode;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_LIGHT
VOID IPC_APP_set_light_onoff(BOOL_T light_on_off)
{
    printf("set light_on_off:%d \r\n", light_on_off);
    //TODO
    /* Status indicator,BOOL type,true means on,false means off */

    __tuya_app_write_INT("tuya_light_onoff", light_on_off);
}

BOOL_T IPC_APP_get_light_onoff(VOID)
{
    BOOL_T light_on_off = __tuya_app_read_INT("tuya_light_onoff");
    printf("curr light_on_off:%d \r\n", light_on_off);
    return light_on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_FLIP
VOID IPC_APP_set_flip_onoff(BOOL_T flip_on_off)
{
    printf("set flip_on_off:%d \r\n", flip_on_off);
    //TODO
    /* flip state,BOOL type,true means inverse,false means normal */

    __tuya_app_write_INT("tuya_flip_onoff", flip_on_off);
}

BOOL_T IPC_APP_get_flip_onoff(VOID)
{
    BOOL_T flip_on_off = __tuya_app_read_INT("tuya_flip_onoff");
    printf("curr flip_on_off:%d \r\n", flip_on_off);
    return flip_on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_WATERMARK
VOID IPC_APP_set_watermark_onoff(BOOL_T watermark_on_off)
{
    printf("set watermark_on_off:%d \r\n", watermark_on_off);
    //TODO
    /* Video watermarking (OSD),BOOL type,true means adding watermark on,false means not */

    __tuya_app_write_INT("tuya_watermark_onoff", watermark_on_off);
}

BOOL_T IPC_APP_get_watermark_onoff(VOID)
{
    BOOL_T watermark_on_off = __tuya_app_read_INT("tuya_watermark_onoff");
    printf("curr watermark_on_off:%d \r\n", watermark_on_off);
    return watermark_on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_WDR
VOID IPC_APP_set_wdr_onoff(BOOL_T wdr_on_off)
{
    printf("set wdr_on_off:%d \r\n", wdr_on_off);
    //TODO
    /* Wide Dynamic Range Model,BOOL type,true means on,false means off */

    __tuya_app_write_INT("tuya_wdr_onoff", wdr_on_off);

}

BOOL_T IPC_APP_get_wdr_onoff(VOID)
{
    BOOL_T wdr_on_off = __tuya_app_read_INT("tuya_wdr_onoff");
    printf("curr watermark_on_off:%d \r\n", wdr_on_off);
    return wdr_on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_NIGHT_MODE
STATIC CHAR_T s_night_mode[4] = {0};//for demo
VOID IPC_APP_set_night_mode(CHAR_T *p_night_mode)
{//0-automatic 1-off 2-on
    printf("set night_mode:%s \r\n", p_night_mode);
    //TODO
    /* Infrared night vision function,ENUM type*/

    __tuya_app_write_STR("tuya_night_mode", p_night_mode);
}

CHAR_T *IPC_APP_get_night_mode(VOID)
{
    __tuya_app_read_STR("tuya_night_mode", s_night_mode, 4);
    printf("curr watermark_on_off:%s \r\n", s_night_mode );
    return  s_night_mode;
}
#endif


//------------------------------------------

#ifdef TUYA_DP_ALARM_FUNCTION
VOID IPC_APP_set_alarm_function_onoff(BOOL_T alarm_on_off)
{
    printf("set alarm_on_off:%d \r\n", alarm_on_off);
    /* motion detection alarm switch,BOOL type,true means on,false means off.
     * This feature has been implemented, and developers can make local configuration settings and properties.*/

    __tuya_app_write_INT("tuya_alarm_on_off", alarm_on_off);
}

BOOL_T IPC_APP_get_alarm_function_onoff(VOID)
{
    BOOL_T alarm_on_off = __tuya_app_read_INT("tuya_alarm_on_off");
    printf("curr alarm_on_off:%d \r\n", alarm_on_off);
    return alarm_on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_ALARM_SENSITIVITY
STATIC CHAR_T s_alarm_sensitivity[4] = {0};//for demo
VOID IPC_APP_set_alarm_sensitivity(CHAR_T *p_sensitivity)
{
    printf("set alarm_sensitivity:%s \r\n", p_sensitivity);
    //TODO
    /* Motion detection alarm sensitivity,ENUM type,0 means low sensitivity,1 means medium sensitivity,2 means high sensitivity*/

    __tuya_app_write_STR("tuya_alarm_sen", p_sensitivity);
}

CHAR_T *IPC_APP_get_alarm_sensitivity(VOID)
{
    __tuya_app_read_STR("tuya_alarm_sen", s_alarm_sensitivity, 4);
    printf("curr alarm_sensitivity:%s \r\n", s_alarm_sensitivity);
    return s_alarm_sensitivity;
}
#endif

#ifdef TUYA_DP_ALARM_ZONE_ENABLE
VOID IPC_APP_set_alarm_zone_onoff(BOOL_T alarm_zone_on_off)
{
    /* Motion detection area setting switch,BOOL type,true means on,false is off*/
    printf("set alarm_zone_onoff:%d \r\n", alarm_zone_on_off);
  
    /* This feature has been implemented, and developers can make local configuration settings and properties.*/

    __tuya_app_write_INT("alarm_zone_on_off", alarm_zone_on_off);

}

BOOL_T IPC_APP_get_alarm_zone_onoff(VOID)
{
    BOOL_T alarm_zone_on_off = __tuya_app_read_INT("alarm_zone_on_off");
    printf("curr alarm_on_off:%d \r\n", alarm_zone_on_off);
    return alarm_zone_on_off;
}
#endif


#ifdef TUYA_DP_ALARM_ZONE_DRAW

#define MAX_ALARM_ZONE_NUM      (6)     //Supports the maximum number of detection areas
//Detection area structure
typedef struct{
    char pointX;    //Starting point x  [0-100]
    char pointY;    //Starting point Y  [0-100]
    char width;     //width    [0-100]
    char height;    //height    [0-100]
}ALARM_ZONE_T;

typedef struct{
    int iZoneNum;   //Number of detection areas
    ALARM_ZONE_T alarmZone[MAX_ALARM_ZONE_NUM];
}ALARM_ZONE_INFO_T;


VOID IPC_APP_set_alarm_zone_draw(cJSON * p_alarm_zone)
{
    if (NULL == p_alarm_zone){
        return ;
    }
    #if 0
    /*demo code*/
    /*Motion detection area setting switch*/
    printf("%s %d set alarm_zone_set:%s \r\n",__FUNCTION__,__LINE__, (char *)p_alarm_zone);
    ALARM_ZONE_INFO_T strAlarmZoneInfo;
    INT_T i = 0;
    cJSON * pJson = cJSON_Parse((CHAR_T *)p_alarm_zone);

    if (NULL == pJson){
        printf("%s %d step error\n",__FUNCTION__,__LINE__);
        //free(pResult);
        return;
    }
    cJSON * tmp = cJSON_GetObjectItem(pJson, "num");
    if (NULL == tmp){
        printf("%s %d step error\n",__FUNCTION__,__LINE__);
        cJSON_Delete(pJson);
        //free(pResult);
        return ;
    }
    memset(&strAlarmZoneInfo, 0x00, sizeof(ALARM_ZONE_INFO_T));
    strAlarmZoneInfo.iZoneNum = tmp->valueint;
    printf("%s %d step num[%d]\n",__FUNCTION__,__LINE__,strAlarmZoneInfo.iZoneNum);
    if (strAlarmZoneInfo.iZoneNum > MAX_ALARM_ZONE_NUM){
        printf("#####error zone num too big[%d]\n",strAlarmZoneInfo.iZoneNum);
        cJSON_Delete(pJson);
        //free(pResult);
        return ;
    }
    for (i = 0; i < strAlarmZoneInfo.iZoneNum; i++){
        char region[12] = {0};
        cJSON * cJSONRegion = NULL;
        snprintf(region, 12, "region%d",i);
        cJSONRegion = cJSON_GetObjectItem(pJson, region);
        if (NULL == cJSONRegion){
            printf("#####[%s][%d]error\n",__FUNCTION__,__LINE__);
            cJSON_Delete(pJson);
            //free(pResult);
            return;
        }
        strAlarmZoneInfo.alarmZone[i].pointX = cJSON_GetObjectItem(cJSONRegion, "x")->valueint;
        strAlarmZoneInfo.alarmZone[i].pointY = cJSON_GetObjectItem(cJSONRegion, "y")->valueint;
        strAlarmZoneInfo.alarmZone[i].width = cJSON_GetObjectItem(cJSONRegion,  "xlen")->valueint;
        strAlarmZoneInfo.alarmZone[i].height = cJSON_GetObjectItem(cJSONRegion, "ylen")->valueint;
        printf("#####[%s][%d][%d,%d,%d,%d]\n",__FUNCTION__,__LINE__,strAlarmZoneInfo.alarmZone[i].pointX,\
            strAlarmZoneInfo.alarmZone[i].pointY,strAlarmZoneInfo.alarmZone[i].width,strAlarmZoneInfo.alarmZone[i].height);
    }
    cJSON_Delete(pJson);
    //free(pResult);
    #endif
    return ;
}
static char s_alarm_zone[256] = {0};
char * IPC_APP_get_alarm_zone_draw(VOID)
{
    /*demo code*/
    int i;
    ALARM_ZONE_INFO_T strAlarmZoneInfo;

    memset(&strAlarmZoneInfo, 0x00, sizeof(ALARM_ZONE_INFO_T));
    //tycam_kv_db_read(BASIC_IPC_ALARM_ZONE_SET,&strAlarmZoneInfo);
    /*get param of alarmzoneInfo yourself*/
    memset(s_alarm_zone, 0x00, 256);
    if (strAlarmZoneInfo.iZoneNum > MAX_ALARM_ZONE_NUM){
        printf("[%s] [%d ]get iZoneNum[%d] error",__FUNCTION__,__LINE__,strAlarmZoneInfo.iZoneNum);
        return s_alarm_zone;
    }
    for (i = 0; i < strAlarmZoneInfo.iZoneNum; i++){
        char region[64] = {0};
        //{"169":"{\"num\":1,\"region0\":{\"x\":0,\"y\":0,\"xlen\":50,\"ylen\":50}}"}
        if (0 == i){
            snprintf(s_alarm_zone, 256,"{\"num\":%d",strAlarmZoneInfo.iZoneNum);
        }
        snprintf(region, 64, ",\"region%d\":{\"x\":%d,\"y\":%d,\"xlen\":%d,\"ylen\":%d}",i,strAlarmZoneInfo.alarmZone[i].pointX,\
            strAlarmZoneInfo.alarmZone[i].pointY,strAlarmZoneInfo.alarmZone[i].width,strAlarmZoneInfo.alarmZone[i].height);
        strcat(s_alarm_zone, region);
        if(i == (strAlarmZoneInfo.iZoneNum - 1)){
            strcat(s_alarm_zone, "}");
        }
    }
    printf("[%s][%d] alarm zone[%s]\n",__FUNCTION__,__LINE__,s_alarm_zone);
    return s_alarm_zone;
}
#endif

//------------------------------------------

//#ifdef TUYA_DP_ALARM_INTERVAL
//STATIC CHAR_T s_alarm_interval[4] = {0};//for demo
//VOID IPC_APP_set_alarm_interval(CHAR_T *p_interval)
//{
//    printf("set alarm_interval:%s \r\n", p_interval);
//    //TODO
//    /* Motion detection alarm interval,unit is minutes,ENUM type,"1","5","10","30","60" */

//    __tuya_app_write_STR("tuya_alarm_interval", p_interval);
//}

//CHAR_T *IPC_APP_get_alarm_interval(VOID)
//{
//    /* Motion detection alarm interval,unit is minutes,ENUM type,"1","5","10","30","60" */
//    __tuya_app_read_STR("tuya_alarm_interval", s_alarm_interval, 4);
//    printf("curr alarm_intervaly:%s \r\n", s_alarm_interval);
//    return s_alarm_interval;
//}
//#endif

//------------------------------------------

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
INT_T IPC_APP_get_sd_status(VOID)
{
    INT_T sd_status = 1;
    /* SD card status, VALUE type, 1-normal, 2-anomaly, 3-insufficient space, 4-formatting, 5-no SD card */
    /* Developer needs to return local SD card status */
    //TODO
    sd_status = 1;

    printf("curr sd_status:%d \r\n", sd_status);
    return sd_status;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
VOID IPC_APP_get_sd_storage(UINT_T *p_total, UINT_T *p_used, UINT_T *p_empty)
{//unit is kb
    //TODO
    /* Developer needs to return local SD card status */
    *p_total = 128 * 1000 * 1000;
    *p_used = 32 * 1000 * 1000;
    *p_empty = *p_total - *p_used;

    printf("curr sd total:%u used:%u empty:%u \r\n", *p_total, *p_used, *p_empty);
}
#endif

//------------------------------------------

#ifdef TUYA_DP_SD_RECORD_ENABLE
VOID IPC_APP_set_sd_record_onoff(BOOL_T sd_record_on_off)
{
    printf("set sd_record_on_off:%d \r\n", sd_record_on_off);
    /* SD card recording function swith, BOOL type, true means on, false means off.
     * This function has been implemented, and developers can make local configuration settings and properties.*/

//    if(sd_record_on_off == TRUE)
//    {
//         IPC_APP_set_sd_record_mode( IPC_APP_get_sd_record_mode()  );
//    }else
//    {
//        tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_NONE);
//    }

    __tuya_app_write_INT("tuya_sd_record_on_off", sd_record_on_off);
}

BOOL_T IPC_APP_get_sd_record_onoff(VOID)
{
    BOOL_T sd_record_on_off  = __tuya_app_read_INT("tuya_sd_record_on_off");
    printf("curr sd_record_on_off:%d \r\n", sd_record_on_off);
    return sd_record_on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_SD_RECORD_MODE
VOID IPC_APP_set_sd_record_mode(UINT_T sd_record_mode)
{
    printf("set sd_record_mode:%d \r\n", sd_record_mode);
    if(0 == sd_record_mode)
    {
         tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_EVENT);
    }
    else if(1 == sd_record_mode)
    {
         tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_ALL);
    }
    else
    {
        printf("Error, should not happen\r\n");
        tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_ALL);
    }
    __tuya_app_write_INT("tuya_sd_record_mode", sd_record_mode);
}

UINT_T IPC_APP_get_sd_record_mode(VOID)
{
    BOOL_T sd_record_mode = __tuya_app_read_INT("tuya_sd_record_mode");
    printf("curr sd_record_mode:%d \r\n", sd_record_mode);
    return sd_record_mode;
}

#endif

//------------------------------------------

#ifdef TUYA_DP_SD_UMOUNT
BOOL_T IPC_APP_unmount_sd_card(VOID)
{
    BOOL_T umount_ok = TRUE;

    //TODO
    /* unmount sdcard */

    printf("unmount result:%d \r\n", umount_ok);
    return umount_ok;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_SD_FORMAT
/* -2000: SD card is being formatted, -2001: SD card formatting is abnormal, -2002: No SD card, 
   -2003: SD card error. Positive number is formatting progress */
STATIC INT_T s_sd_format_progress = 0;
void *thread_sd_format(void *arg)
{
    /* First notify to app, progress 0% */
    s_sd_format_progress = 0;
    IPC_APP_report_sd_format_status(s_sd_format_progress);
    sleep(1);

    /* Stop local SD card recording and playback, progress 10%*/
    s_sd_format_progress = 10;
    IPC_APP_report_sd_format_status(s_sd_format_progress);
    tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_NONE);
    tuya_ipc_ss_pb_stop_all();
    sleep(1);

    /* Delete the media files in the SD card, the progress is 30% */
    s_sd_format_progress = 30;
    IPC_APP_report_sd_format_status(s_sd_format_progress);
    //tuya_ipc_ss_delete_all_files();
    sleep(1);

    /* Perform SD card formatting operation */
    tuya_ipc_sd_format();

    s_sd_format_progress = 80;
    IPC_APP_report_sd_format_status(s_sd_format_progress);
    //TODO
    tuya_ipc_ss_set_write_mode(SS_WRITE_MODE_ALL);
//    IPC_APP_set_sd_record_onoff( IPC_APP_get_sd_record_onoff());

    sleep(1);
    IPC_APP_report_sd_storage();
    /* progress 100% */
    s_sd_format_progress = 100;
    IPC_APP_report_sd_format_status(s_sd_format_progress);

    pthread_exit(0);
}

VOID IPC_APP_format_sd_card(VOID)
{
    printf("start to format sd_card \r\n");
    /* SD card formatting.
     * The SDK has already completed the writing of some of the code, 
     and the developer only needs to implement the formatting operation. */

    pthread_t sd_format_thread;
    pthread_create(&sd_format_thread, NULL, thread_sd_format, NULL);
    pthread_detach(sd_format_thread);
}
#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
INT_T IPC_APP_get_sd_format_status(VOID)
{
    return s_sd_format_progress;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_PTZ_CONTROL
VOID IPC_APP_ptz_start_move(CHAR_T *p_direction)
{
    printf("ptz start move:%s \r\n", p_direction);
    //0-up, 1-upper right, 2-right, 3-lower right, 4-down, 5-down left, 6-left, 7-top left

}
#endif

#ifdef TUYA_DP_PTZ_STOP
VOID IPC_APP_ptz_stop_move(VOID)
{
    printf("ptz stop move \r\n");
    //TODO
    /* PTZ rotation stops */

}
#endif

#ifdef TUYA_DP_PTZ_CHECK
void IPC_APP_ptz_check(VOID)
{
    printf("ptz check \r\n");
}
#endif

#ifdef TUYA_DP_TRACK_ENABLE
void IPC_APP_track_enable(BOOL_T track_enable)
{
    printf("track_enable %d\r\n",track_enable);
}

BOOL_T IPC_APP_get_track_enable(void)
{
    char track_enable = 0;
    //the value you get yourself
    return (BOOL_T)track_enable;
}

#endif

#ifdef TUYA_DP_HUM_FILTER
void IPC_APP_human_filter(BOOL_T filter_enable)
{
    printf("filter_enable %d\r\n",filter_enable);
    return;
}
#endif

#ifdef TUYA_DP_PATROL_MODE
void IPC_APP_set_patrol_mode(BOOL_T patrol_mode)
{
    printf("patrol_mode %d\r\n",patrol_mode);
    return;
}

char IPC_APP_get_patrol_mode(void)
{
    char patrol_mode = 0;
    //the value you get yourself
    return patrol_mode;
}

#endif

#ifdef TUYA_DP_PATROL_SWITCH
void IPC_APP_set_patrol_switch(BOOL_T patrol_switch)
{
    printf("patrol_switch %d\r\n",patrol_switch);
    return;
}

BOOL_T IPC_APP_get_patrol_switch(void)
{
    char patrol_switch = 0;
    //the value you get yourself
    return (BOOL_T)patrol_switch;
}

void IPC_APP_ptz_preset_reset(S_PRESET_CFG *preset_cfg)
{
    /*Synchronize data from server*/
    return;
}

#endif

#ifdef TUYA_DP_PATROL_TMODE
void IPC_APP_set_patrol_tmode(BOOL_T patrol_tmode)
{
    printf("patrol_tmode %d\r\n",patrol_tmode);
    return;
}

char IPC_APP_get_patrol_tmode(void)
{
    char patrol_tmode = 0;
    //the value you get yourself
    return patrol_tmode;
}

#endif

#ifdef TUYA_DP_PATROL_TIME
void IPC_APP_set_patrol_time(cJSON * p_patrol_time)
{
    //set your patrol_time
    /*

    cJSON * pJson = cJSON_Parse((CHAR_T *)p_patrol_time);
    if (NULL == pJson){
        TYWARN("----error---\n");

        return -1;
    }
    cJSON* t_start = cJSON_GetObjectItem(pJson, "t_start");
    cJSON* t_end = cJSON_GetObjectItem(pJson, "t_end");
    if ((NULL == t_start) || (NULL == t_end)){
        TYWARN("----t_start---\n");
        cJSON_Delete(pJson);
        return -1;
    }
    TYDEBUG("stare%s--end:%s\n", t_start->valuestring,t_end->valuestring);

    */
    return;
}

#endif

#ifdef TUYA_DP_PRESET_SET
void IPC_APP_set_preset(cJSON * p_preset_param)
{
    //preset add ,preset del, preset go
#if 0
    cJSON * pJson = cJSON_Parse((CHAR_T *)p_preset_param);
    if (NULL == pJson){
        TYERROR("null preset set input");
        return -1;
    }
    cJSON* type = cJSON_GetObjectItem(pJson, "type");
    cJSON* data = cJSON_GetObjectItem(pJson, "data");
    if ((NULL == type) || (NULL == data)){
        TYERROR("invalid preset set input");
        return -1;
    }
   

    TYDEBUG("preset set type: %d",type->valueint);
    //1:add preset point 2:delete preset point 3:call preset point
    if(type->valueint == 1)
    {
        char pic_buffer[PRESET_PIC_MAX_SIZE] = {0};  
        int pic_size = sizeof(pic_buffer);  /*Image to be shown*/
        S_PRESET_POSITION preset_pos;
        char respond_add[128] = {0}; 
        /*mpId is 1,2,3,4,5,6，The server will generate a set of its own preset point number information based on the mpid.*/
        preset_pos.mpId = 1;   
        preset_pos.ptz.pan = 100; /*horizontal position*/
        preset_pos.ptz.tilt = 60;/*vertical position*/
        cJSON* name = cJSON_GetObjectItem(data, "name");
        int name_len = 0;
        int error_num = 0;
        
        if(name == NULL)
        {
            TYERROR("name is null\n");
            return -1;
        }
        name_len = strlen(name->valuestring);
        name_len = (name_len > 30)?30:name_len;
        memcpy(&preset_pos.name,name->valuestring,(name_len));
        preset_pos.name[name_len] = '\0';
        error_num = tuya_ipc_preset_add(&preset_pos);

        snprintf(respond_add,128,"{\"type\":%d,\"data\":{\"error\":%d}}",type->valueint,error_num);

        tuya_ipc_dp_report(NULL, TUYA_DP_PRESET_SET,PROP_STR,respond_add,1);

        //tuya_ipc_preset_add_pic(pic_buffer,pic_size); /*if you need show pic ,you should set this api*/
    }
    else if(type->valueint == 2)
    {
        cJSON* num = cJSON_GetObjectItem(data, "num"); //can delete one or more
        cJSON* sets = cJSON_GetObjectItem(data, "sets");
        char respond_del[128] = {0}; 
        cJSON* del_data;
        int del_num = num->valueint;
        for(i = 0; i < del_num; i++)
        {
            del_data = cJSON_GetArrayItem(sets,i);
            cJSON* devId = cJSON_GetObjectItem(del_data, "devId");  /*devid is the preset point number registered in the server*/
            cJSON* mpId = cJSON_GetObjectItem(del_data, "mpId");  /*mpid is the preset point number managed on the device*/
            if((NULL == devId) || (NULL == mpId))
            {
                printf("devid or mpid is error\n");
                return -1;
            }
            del_preset.seq = atoi(mpId->valuestring);
            
            printf("%d---%s\n",del_preset.seq,devId->valuestring);

            error_num = (int)time(NULL);

            tuya_ipc_preset_del(devId->valuestring);
            
            snprintf(respond_add,128,"{\"type\":%d,\"data\":{\"error\":%d}}",type->valueint,error_num);
        }
    }
    else if(type->valueint == 3)
    {
        cJSON* mpId = cJSON_GetObjectItem(data, "mpId");

        preset_seq = atoi(mpId->valuestring);
        //get your seq pos and go there
    }
#endif
    return;
}

#endif

#ifdef TUYA_DP_PATROL_STATE
void IPC_APP_patrol_state(int *patrol_state)
{
    //printf("patrol_state %d\r\n",atoi(patrol_state));
    //return your patrol_state
    return;
}

#endif


#ifdef TUYA_DP_LINK_MOVE_SET
VOID IPC_APP_set_link_pos(INT_T bind_seq)
{
    /*set the link pos*/
    printf("IPC_APP_set_bind_pos:%d \r\n", bind_seq);
    /*demo
    step1: get the current position
    step2: save the position to flash
    */
    return;
}
#endif


#ifdef TUYA_DP_LINK_MOVE_ACTION
VOID IPC_APP_set_link_move(INT_T bind_seq)
{
    /*move to the link pos*/
    printf("IPC_APP_set_bind_move:%d \r\n", bind_seq);
    /*demo
     step1: get the position base seq
     step2: go to the target position
    */
    return;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_BLUB_SWITCH
VOID IPC_APP_set_blub_onoff(BOOL_T blub_on_off)
{
    printf("set blub_on_off:%d \r\n", blub_on_off);
    //TODO
    /* light control switch, BOOL type,true means on,false menas off */

    __tuya_app_write_INT("tuya_blub_on_off", blub_on_off);
}

BOOL_T IPC_APP_get_blub_onoff(VOID)
{
    BOOL_T blub_on_off = __tuya_app_read_INT("tuya_blub_on_off");
    printf("curr blub_on_off:%d \r\n", blub_on_off);
    return blub_on_off;
}
#endif

//------------------------------------------

#ifdef TUYA_DP_ELECTRICITY
INT_T IPC_APP_get_battery_percent(VOID)
{
    //TODO
    /* battery power percentage VALUE type,[0-100] */

    return 80;
}
#endif

#ifdef TUYA_DP_POWERMODE
CHAR_T *IPC_APP_get_power_mode(VOID)
{
    //TODO
    /* Power supply mode, ENUM type, 
    "0" is the battery power supply state, "1" is the plug-in power supply state (or battery charging state) */

    return "1";
}
#endif

#if defined(WIFI_GW) && (WIFI_GW==1)
#ifdef TUYA_DP_AP_MODE
extern OPERATE_RET tuya_adapter_wifi_get_work_mode(OUT WF_WK_MD_E *mode);
extern OPERATE_RET tuya_adapter_wifi_get_mac(IN CONST WF_IF_E wf,INOUT NW_MAC_S *mac);
extern OPERATE_RET tuya_adapter_wifi_ap_start(IN CONST WF_AP_CFG_IF_S *cfg);
extern OPERATE_RET tuya_adapter_wifi_ap_stop(VOID);

STATIC CHAR_T response[128] = {0};
#define WIFI_SSID_LEN 32    // tuya sdk definition WIFI SSID MAX LEN
#define WIFI_PASSWD_LEN 64  // tuya sdk definition WIFI PASSWD MAX LEN
CHAR_T * IPC_APP_get_ap_mode(VOID)
{
    CHAR_T ap_ssid[WIFI_SSID_LEN + 1] = {0};
    CHAR_T ap_pw[WIFI_PASSWD_LEN + 1] = {0};
    WF_WK_MD_E cur_mode = WWM_STATION;
    NW_MAC_S mac = {{0}};
    tuya_adapter_wifi_get_work_mode(&cur_mode);
    INT_T is_ap = 0;

    OPERATE_RET op_ret = tuya_adapter_wifi_get_mac(WF_AP,&mac);
    if(OPRT_OK != op_ret) {
        strncpy(ap_ssid, "TUYA_IPC_AP", WIFI_SSID_LEN);
    }
    else
    {
        snprintf(ap_ssid, WIFI_SSID_LEN, "TUYA_IPC-%02X%02X",mac.mac[4],mac.mac[5]);
    }
    //{ is_ap: 1, ap_ssid: "xxxx"}

    if(cur_mode == WWM_SOFTAP)
    {
        is_ap = 1;
    }
    __tuya_app_read_STR("tuya_ap_passwd", ap_pw, WIFI_PASSWD_LEN);

    snprintf(response, 128, "{\"is_ap\":%d,\"ap_ssid\":\"%s\",\"password\":\"%s\"}",is_ap,ap_ssid,ap_pw);
    return response;
}
#endif
#ifdef TUYA_DP_AP_SWITCH
VOID* __ap_change_thread(void *arg)
{
    WF_WK_MD_E cur_mode = WWM_STATION;
    tuya_adapter_wifi_get_work_mode(&cur_mode);
    WF_AP_CFG_IF_S ap_cfg;
    INT_T ap_onoff = 0;
    sleep(5);

    ap_onoff = __tuya_app_read_INT("tuya_ap_on_off");
    memset(&ap_cfg, 0, SIZEOF(WF_AP_CFG_IF_S));
    printf("%s : ap_onoff:%d\n", __func__, ap_onoff);
    if(ap_onoff)
    {
        //ap, sta, monitor, do same
        __tuya_app_read_STR("tuya_ap_ssid", ap_cfg.ssid, WIFI_SSID_LEN);
        __tuya_app_read_STR("tuya_ap_passwd", ap_cfg.passwd, WIFI_PASSWD_LEN);

        ap_cfg.s_len = strlen((CHAR_T *)(ap_cfg.ssid));
        ap_cfg.ssid_hidden = 0;
        ap_cfg.p_len = strlen((CHAR_T *)(ap_cfg.passwd));
        if(ap_cfg.p_len == 0)
        {
            ap_cfg.md = WAAM_OPEN;
        }
        else
        {
            ap_cfg.md = WAAM_WPA2_PSK;
        }
        tuya_adapter_wifi_ap_start(&ap_cfg);
    }
    else
    {
        if(cur_mode == WWM_SOFTAP)
        {

            tuya_adapter_wifi_ap_stop();
           // tuya_ipc_reconnect_wifi();
        }
        else
        {
            //not ap mode,do nothing
        }

    }
    return NULL;
}

VOID change_ap_process()
{
    pthread_t ap_change_thread;
    int ret = pthread_create(&ap_change_thread, NULL, __ap_change_thread, NULL);
    if(ret != 0)
    {
       printf("ap_change_thread ,create fail! ret:%d\n",ret);
    }

    pthread_detach(ap_change_thread);
}

INT_T IPC_APP_set_ap_mode(IN cJSON *p_ap_info)
{
    if (NULL == p_ap_info){
        return 0;
    }

    INT_T ap_onoff = -1;

    printf("%s %d handle_DP_AP_SWITCH:%s \r\n",__FUNCTION__,__LINE__, (char *)p_ap_info);

    cJSON * pJson = cJSON_Parse((CHAR_T *)p_ap_info);

    if (NULL == pJson){
        printf("%s %d step error\n",__FUNCTION__,__LINE__);
        return ap_onoff;
    }
    //{ ap_enable : 1, ap_ssid : xxxx, ap_pwd : xxx }
    cJSON * tmp = cJSON_GetObjectItem(pJson, "ap_enable");
    if (NULL == tmp){
        printf("%s %d step error\n",__FUNCTION__,__LINE__);
        cJSON_Delete(pJson);
        return ap_onoff;
    }
    ap_onoff = tmp->valueint;
    __tuya_app_write_INT("tuya_ap_on_off", ap_onoff);

    if(ap_onoff == 1)
    {
        cJSON *tmpSsid = cJSON_GetObjectItem(pJson, "ap_ssid");
        if (NULL == tmpSsid){
            printf("%s %d step error\n",__FUNCTION__,__LINE__);
            cJSON_Delete(pJson);
            return ap_onoff;
        }
        printf("###[%d] get ssid:%s\n", ap_onoff, tmpSsid->valuestring);

        cJSON *tmpPwd = cJSON_GetObjectItem(pJson, "ap_pwd");
        if (NULL == tmpPwd){
            printf("%s %d step error\n",__FUNCTION__,__LINE__);
            cJSON_Delete(pJson);
            return ap_onoff;
        }
        printf("###get pwd:%s\n", tmpPwd->valuestring);

        __tuya_app_write_STR("tuya_ap_ssid", tmpSsid->valuestring);
        __tuya_app_write_STR("tuya_ap_passwd", tmpPwd->valuestring);
       // tuya_ipc_save_ap_info(tmpSsid->valuestring,tmpPwd->valuestring);
    }

    cJSON_Delete(pJson);

    return ap_onoff;

}
#endif
#endif
