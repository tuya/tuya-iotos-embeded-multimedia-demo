/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *FileName: tuya_ipc_dp_utils.c
  *
  * File Description：
  * 1. API implementation of DP point
  *
  * This file code is the basic code, users don't care it
  * Please do not modify any contents of this file at will. 
  * Please contact the Product Manager if you need to modify it.
  *
**********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "tuya_ipc_api.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_dp_handler.h"
#include "tuya_iot_com_api.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_cloud_types.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_xvr_dev.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef VOID (*TUYA_MAIN_DEV_DP_HANDLER)(IN TY_OBJ_DP_S *p_obj_dp);
typedef struct
{
    BYTE_T dp_id;
    TUYA_MAIN_DEV_DP_HANDLER handler;
}TUYA_MAIN_DEV_DP_INFO_S;

typedef VOID (*TUYA_SUB_DEV_DP_HANDLER)(char * id,IN CONST TY_OBJ_DP_S *p_obj_dp);
typedef struct
{
    BYTE_T dp_id;
    TUYA_SUB_DEV_DP_HANDLER handler;
}TUYA_SUB_DEV_DP_INFO_S;

STATIC VOID respone_dp_value(BYTE_T dp_id, INT_T val);
STATIC VOID respone_dp_bool(BYTE_T dp_id, BOOL_T true_false);
STATIC VOID respone_dp_enum(BYTE_T dp_id, CHAR_T *p_val_enum);
STATIC VOID respone_dp_str(BYTE_T dp_id, CHAR_T *p_val_str);
STATIC VOID handle_DP_SD_STORAGE_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp);

//------------------------------------------
VOID IPC_APP_upload_all_status(VOID)
{
#ifdef TUYA_DP_SLEEP_MODE
    respone_dp_bool(TUYA_DP_SLEEP_MODE, IPC_APP_get_sleep_mode() );
#endif

#ifdef TUYA_DP_LIGHT
    respone_dp_bool(TUYA_DP_LIGHT, IPC_APP_get_light_onoff() );
#endif

#ifdef TUYA_DP_FLIP
    respone_dp_bool(TUYA_DP_FLIP, IPC_APP_get_flip_onoff() );
#endif

#ifdef TUYA_DP_WATERMARK
    respone_dp_bool(TUYA_DP_WATERMARK, IPC_APP_get_watermark_onoff() );
#endif

#ifdef TUYA_DP_WDR
    respone_dp_bool(TUYA_DP_WDR, IPC_APP_get_wdr_onoff() );
#endif

#ifdef TUYA_DP_NIGHT_MODE
    respone_dp_enum(TUYA_DP_NIGHT_MODE, IPC_APP_get_night_mode() );
#endif

#ifdef TUYA_DP_ALARM_FUNCTION
    respone_dp_bool(TUYA_DP_ALARM_FUNCTION, IPC_APP_get_alarm_function_onoff() );
#endif

#ifdef TUYA_DP_ALARM_SENSITIVITY
    respone_dp_enum(TUYA_DP_ALARM_SENSITIVITY, IPC_APP_get_alarm_sensitivity() );
#endif

#ifdef TUYA_DP_ALARM_ZONE_ENABLE
    respone_dp_bool(TUYA_DP_ALARM_ZONE_ENABLE, IPC_APP_get_alarm_zone_onoff() );
#endif

#ifdef TUYA_DP_ALARM_ZONE_DRAW
    respone_dp_str(TUYA_DP_ALARM_ZONE_DRAW, IPC_APP_get_alarm_zone_draw());
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
    respone_dp_value(TUYA_DP_SD_STATUS_ONLY_GET, IPC_APP_get_sd_status() );
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
    handle_DP_SD_STORAGE_ONLY_GET(NULL);
#endif

#ifdef TUYA_DP_SD_RECORD_ENABLE
    respone_dp_bool(TUYA_DP_SD_RECORD_ENABLE, IPC_APP_get_sd_record_onoff() );
#endif

#ifdef TUYA_DP_SD_RECORD_MODE
    CHAR_T sd_mode[4];
    snprintf(sd_mode,4,"%d",IPC_APP_get_sd_record_mode());
    respone_dp_enum(TUYA_DP_SD_RECORD_MODE, sd_mode);
#endif


#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
    respone_dp_value(TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, 0 );
#endif

#ifdef TUYA_DP_BLUB_SWITCH
    respone_dp_bool(TUYA_DP_BLUB_SWITCH, IPC_APP_get_blub_onoff() );
#endif

#ifdef TUYA_DP_POWERMODE
    IPC_APP_update_battery_status();
#endif
}

//------------------------------------------
VOID IPC_APP_upload_all_status_XVR(VOID)
{
    INT_T mode = 0x2;
    respone_dp_value(TUYA_DP_XVR_SUPPORT_STORAGE, mode);

    int stor_mode = 0x2;
    respone_dp_value(TUYA_DP_XVR_SET_CUR_STORAGE, stor_mode);  //默认使能sd卡
    char respond_add[128]={0};
    snprintf(respond_add,128,"1,2,5,12");
    tuya_xvr_sub_dp_report(0, 236,PROP_STR,respond_add,1);
}


#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
VOID IPC_APP_report_sd_format_status(INT_T status)
{
    respone_dp_value(TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, status);
}
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
VOID IPC_APP_report_sd_status_changed(INT_T status)
{
    respone_dp_value(TUYA_DP_SD_STATUS_ONLY_GET, status);
}
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
VOID IPC_APP_report_sd_storage()
{
    CHAR_T tmp_str[100] = {0};

    UINT_T total = 100;
    UINT_T used = 0;
    UINT_T empty = 100;
    IPC_APP_get_sd_storage(&total, &used, &empty);

    //"total capacity|Current usage|remaining capacity"
    snprintf(tmp_str, 100, "%u|%u|%u", total, used, empty);
    respone_dp_str(TUYA_DP_SD_STORAGE_ONLY_GET, tmp_str);
}
#endif


#ifdef TUYA_DP_POWERMODE
VOID IPC_APP_update_battery_status(VOID)
{
    CHAR_T *power_mode = IPC_APP_get_power_mode();
    INT_T percent = IPC_APP_get_battery_percent();

    printf("current power mode:%s\r\n", power_mode);
    respone_dp_enum(TUYA_DP_POWERMODE, power_mode);
    printf("current battery percent:%d\r\n", percent);
    respone_dp_value(TUYA_DP_ELECTRICITY, percent);
}
#endif

//------------------------------------------
STATIC VOID respone_dp_value(BYTE_T dp_id, INT_T val)
{
    tuya_ipc_dp_report(NULL, dp_id,PROP_VALUE,&val,1);
}

STATIC VOID respone_dp_bool(BYTE_T dp_id, BOOL_T true_false)
{
    tuya_ipc_dp_report(NULL, dp_id,PROP_BOOL,&true_false,1);
}

STATIC VOID respone_dp_enum(BYTE_T dp_id, CHAR_T *p_val_enum)
{
    tuya_ipc_dp_report(NULL, dp_id,PROP_ENUM,p_val_enum,1);
}

STATIC VOID respone_dp_str(BYTE_T dp_id, CHAR_T *p_val_str)
{
    tuya_ipc_dp_report(NULL, dp_id,PROP_STR,p_val_str,1);
}

//------------------------------------------
STATIC BOOL_T check_dp_bool_invalid(IN TY_OBJ_DP_S *p_obj_dp)
{
    if(p_obj_dp == NULL)
    {
        printf("error! input is null \r\n");
        return -1;
    }

    if(p_obj_dp->type != PROP_BOOL)
    {
        printf("error! input is not bool %d \r\n", p_obj_dp->type);
        return -2;
    }

    if(p_obj_dp->value.dp_bool == 0)
    {
        return FALSE;
    }
    else if(p_obj_dp->value.dp_bool == 1)
    {
        return TRUE;
    }else
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->value.dp_bool);
        return -2;
    }
}

//------------------------------------------

#ifdef TUYA_DP_SLEEP_MODE
STATIC VOID handle_DP_SLEEP_MODE(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T sleep_mode = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_sleep_mode(sleep_mode);
    sleep_mode = IPC_APP_get_sleep_mode();

    respone_dp_bool(TUYA_DP_SLEEP_MODE, sleep_mode);
}
#endif

#ifdef TUYA_DP_LIGHT
STATIC VOID handle_DP_LIGHT(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T light_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_light_onoff(light_on_off);
    light_on_off = IPC_APP_get_light_onoff();

    respone_dp_bool(TUYA_DP_LIGHT, light_on_off);
}
#endif

#ifdef TUYA_DP_FLIP
STATIC VOID handle_DP_FLIP(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T flip_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_flip_onoff(flip_on_off);
    flip_on_off = IPC_APP_get_flip_onoff();

    respone_dp_bool(TUYA_DP_FLIP, flip_on_off);
}
#endif

#ifdef TUYA_DP_WATERMARK
STATIC VOID handle_DP_WATERMARK(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T watermark_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_watermark_onoff(watermark_on_off);
    watermark_on_off = IPC_APP_get_watermark_onoff();

    respone_dp_bool(TUYA_DP_WATERMARK, watermark_on_off);
}
#endif

#ifdef TUYA_DP_WDR
STATIC VOID handle_DP_WDR(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T wdr_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_wdr_onoff(wdr_on_off);
    wdr_on_off = IPC_APP_get_wdr_onoff();

    respone_dp_bool(TUYA_DP_WDR, wdr_on_off);
}
#endif

#ifdef TUYA_DP_NIGHT_MODE
STATIC VOID handle_DP_NIGHT_MODE(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }
    CHAR_T tmp_str[2] = {0};
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_night_mode(tmp_str);
    CHAR_T *p_night_mode = IPC_APP_get_night_mode();

    respone_dp_enum(TUYA_DP_NIGHT_MODE, p_night_mode);
}
#endif


#ifdef TUYA_DP_ALARM_FUNCTION
STATIC VOID handle_DP_ALARM_FUNCTION(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T alarm_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_alarm_function_onoff(alarm_on_off);
    alarm_on_off = IPC_APP_get_alarm_function_onoff();

    respone_dp_bool(TUYA_DP_ALARM_FUNCTION, alarm_on_off);
}
#endif

#ifdef TUYA_DP_ALARM_SENSITIVITY
STATIC VOID handle_DP_ALARM_SENSITIVITY(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    CHAR_T tmp_str[2] = {0};
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_alarm_sensitivity(tmp_str);
    CHAR_T *p_sensitivity = IPC_APP_get_alarm_sensitivity();

    respone_dp_enum(TUYA_DP_ALARM_SENSITIVITY, p_sensitivity);
}
#endif

#ifdef TUYA_DP_ALARM_ZONE_ENABLE
STATIC VOID handle_DP_ALARM_ZONE_ENABLE(IN TY_OBJ_DP_S *p_dp_json)
{
    if(p_dp_json == NULL )
    {
        printf("Error!! type invalid %p \r\n", p_dp_json);
        return;
    }
    BOOL_T alarm_zone_enable = check_dp_bool_invalid(p_dp_json);
    IPC_APP_set_alarm_zone_onoff(alarm_zone_enable);
    respone_dp_bool(TUYA_DP_ALARM_ZONE_ENABLE, IPC_APP_get_alarm_zone_onoff());
}
#endif

#ifdef TUYA_DP_ALARM_ZONE_DRAW
STATIC VOID handle_DP_ALARM_ZONE_DRAW(IN TY_OBJ_DP_S *p_dp_json)
{
    if(p_dp_json == NULL )
    {
        printf("Error!! type invalid\r\n");
        return;
    }
    IPC_APP_set_alarm_zone_draw((cJSON *)(p_dp_json->value.dp_str));
    respone_dp_str(TUYA_DP_ALARM_ZONE_DRAW, IPC_APP_get_alarm_zone_draw());
}
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
STATIC VOID handle_DP_SD_STATUS_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T sd_status = IPC_APP_get_sd_status();

    respone_dp_value(TUYA_DP_SD_STATUS_ONLY_GET, sd_status);
}
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
STATIC VOID handle_DP_SD_STORAGE_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp)
{
    CHAR_T tmp_str[100] = {0};

    UINT_T total = 100;
    UINT_T used = 0;
    UINT_T empty = 100;
    IPC_APP_get_sd_storage(&total, &used, &empty);

    //"total capacity|Current usage|remaining capacity"
    snprintf(tmp_str, 100, "%u|%u|%u", total, used, empty);
    respone_dp_str(TUYA_DP_SD_STORAGE_ONLY_GET, tmp_str);
}
#endif

#ifdef TUYA_DP_SD_RECORD_ENABLE
STATIC VOID handle_DP_SD_RECORD_ENABLE(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T sd_record_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_sd_record_onoff(sd_record_on_off);
    sd_record_on_off = IPC_APP_get_sd_record_onoff();

    respone_dp_bool(TUYA_DP_SD_RECORD_ENABLE, sd_record_on_off);
}
#endif

#ifdef TUYA_DP_SD_RECORD_MODE
STATIC VOID handle_DP_SD_RECORD_MODE(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    IPC_APP_set_sd_record_mode(p_obj_dp->value.dp_enum);
    UINT_T mode = IPC_APP_get_sd_record_mode();
    CHAR_T sMode[2];
    snprintf(sMode,2,"%d",mode);
    respone_dp_enum(TUYA_DP_SD_RECORD_MODE,sMode);
}
#endif

#ifdef TUYA_DP_SD_UMOUNT
STATIC VOID handle_DP_SD_UMOUNT(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T umount_result = IPC_APP_unmount_sd_card();
    respone_dp_bool(TUYA_DP_SD_UMOUNT, umount_result);
}
#endif

#ifdef TUYA_DP_SD_FORMAT
STATIC VOID handle_DP_SD_FORMAT(IN TY_OBJ_DP_S *p_obj_dp)
{
#ifdef _USER_DO_NOT_OPEN_
    IPC_APP_format_sd_card();
#endif
    respone_dp_bool(TUYA_DP_SD_FORMAT, TRUE);
}
#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
STATIC VOID handle_DP_SD_FORMAT_STATUS_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T progress =0;
#ifdef _USER_DO_NOT_OPEN_
    progress = IPC_APP_get_sd_format_status();
#endif
    respone_dp_value(TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, progress);
}
#endif

#ifdef TUYA_DP_PTZ_CONTROL
STATIC VOID handle_DP_PTZ_CONTROL(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    //dp 119 format: {"range":["1","2","3","4","5","6","7","0"],"type":"enum"}
    UINT_T dp_directions[8] = {1,2,3,4,5,6,7,0};
    UINT_T direction = dp_directions[p_obj_dp->value.dp_enum];
    CHAR_T tmp_str[2] = {0};
    snprintf(tmp_str,2,"%d",direction);    
    IPC_APP_ptz_start_move(tmp_str);
    respone_dp_enum(TUYA_DP_PTZ_CONTROL,tmp_str);
}
#endif

#ifdef TUYA_DP_PTZ_STOP
STATIC VOID handle_DP_PTZ_STOP(IN TY_OBJ_DP_S *p_obj_dp)
{
    IPC_APP_ptz_stop_move();
    respone_dp_bool(TUYA_DP_PTZ_STOP, TRUE);
}
#endif

#ifdef TUYA_DP_PTZ_CHECK
STATIC VOID handle_DP_PTZ_CHECK(IN TY_OBJ_DP_S *p_obj_dp)
{
    IPC_APP_ptz_check();
    respone_dp_bool(TUYA_DP_PTZ_CHECK, TRUE);
}
#endif

#ifdef TUYA_DP_TRACK_ENABLE
STATIC VOID handle_DP_TRACK_ENABLE(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T track_enable = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_track_enable(track_enable);

    respone_dp_bool(TUYA_DP_TRACK_ENABLE, track_enable);
}

#endif

#ifdef TUYA_DP_LINK_MOVE_ACTION
STATIC VOID handle_DP_LINK_MOVE(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    CHAR_T tmp_str[2] = {0};
    int bind_move = 0;
    bind_move = p_obj_dp->value.dp_enum;
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_link_move(bind_move);
    respone_dp_enum(TUYA_DP_LINK_MOVE_ACTION, tmp_str);
}
#endif

#ifdef TUYA_DP_LINK_MOVE_SET
STATIC VOID handle_DP_LINK_MOVE_SET(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }

    CHAR_T tmp_str[2] = {0};
    int bind_move = 0;
    bind_move = p_obj_dp->value.dp_enum;
    tmp_str[0] = '0' + p_obj_dp->value.dp_enum;

    IPC_APP_set_link_pos(bind_move);
    respone_dp_enum(TUYA_DP_LINK_MOVE_SET, tmp_str);
}
#endif
#ifdef TUYA_DP_HUM_FILTER
STATIC VOID handle_DP_HUM_FILTER(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T hum_filter = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_human_filter(hum_filter);
 
    respone_dp_bool(TUYA_DP_HUM_FILTER, hum_filter);
}
#endif

#ifdef TUYA_DP_PATROL_MODE
STATIC VOID handle_DP_patrol_mode(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }
    IPC_APP_set_patrol_mode(p_obj_dp->value.dp_enum);
    CHAR_T sMode[2];
    snprintf(sMode,2,"%d",p_obj_dp->value.dp_enum);

    respone_dp_enum(TUYA_DP_PATROL_MODE,sMode);
}


#endif

#ifdef TUYA_DP_PATROL_SWITCH
STATIC VOID handle_DP_patrol_switch(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T patrol_mode = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_patrol_switch(patrol_mode);
 
    respone_dp_bool(TUYA_DP_PATROL_SWITCH, patrol_mode);
}
#endif

#ifdef TUYA_DP_PATROL_TMODE
STATIC VOID handle_DP_patrol_tmode(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_ENUM) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }
    IPC_APP_set_patrol_tmode(p_obj_dp->value.dp_enum);
    CHAR_T sMode[2];
    snprintf(sMode,2,"%d",p_obj_dp->value.dp_enum);
    respone_dp_enum(TUYA_DP_PATROL_TMODE,sMode);
}


#endif

#ifdef TUYA_DP_PATROL_TIME
STATIC VOID handle_DP_patrol_time(IN TY_OBJ_DP_S *p_dp_json)
{
    printf("---%s---\n",p_dp_json->value.dp_str);
    IPC_APP_set_patrol_time((cJSON *)(p_dp_json->value.dp_str));
    return ;
}
#endif
#ifdef TUYA_DP_PATROL_STATE
STATIC VOID handle_DP_patrol_state(IN TY_OBJ_DP_S *p_dp_json)
{
    int patrol_state = 0;
    //printf("---get_patrol_state\n");
    IPC_APP_patrol_state(&patrol_state);
    printf("---get_patrol_state:%d\n",patrol_state);

    CHAR_T sd_mode[4];
    snprintf(sd_mode,4,"%d",patrol_state);
    respone_dp_enum(TUYA_DP_PATROL_STATE, sd_mode);
    return ;
}
#endif

#ifdef TUYA_DP_PRESET_SET
STATIC VOID handle_DP_SET_PRESET(char *devid ,IN CONST TY_OBJ_DP_S *p_dp_json)
{
    printf("recve add preset \n");
    IPC_APP_set_preset(devid,(cJSON *)(p_dp_json->value.dp_str));
    return;
}
#endif


#ifdef TUYA_DP_DOOR_BELL
STATIC VOID handle_DP_DOOR_BELL(IN TY_OBJ_DP_S *p_obj_dp)
{
    printf("error! door bell can only trigged by IPC side.\r\n");
    respone_dp_str(TUYA_DP_DOOR_BELL, "-1");
}
#endif

#ifdef TUYA_DP_BLUB_SWITCH
STATIC VOID handle_DP_BLUB_SWITCH(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T blub_on_off = check_dp_bool_invalid(p_obj_dp);

    IPC_APP_set_blub_onoff(blub_on_off);
    blub_on_off = IPC_APP_get_blub_onoff();

    respone_dp_bool(TUYA_DP_BLUB_SWITCH, blub_on_off);
}
#endif

#ifdef TUYA_DP_ELECTRICITY
STATIC VOID handle_DP_ELECTRICITY(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T percent = IPC_APP_get_battery_percent();
    printf("current battery percent:%d\r\n", percent);
    respone_dp_value(TUYA_DP_ELECTRICITY, percent);
}
#endif

#ifdef TUYA_DP_POWERMODE
STATIC VOID handle_DP_POWERMODE(IN TY_OBJ_DP_S *p_obj_dp)
{
    CHAR_T *power_mode = IPC_APP_get_power_mode();
    printf("current power mode:%s\r\n", power_mode);
    respone_dp_enum(TUYA_DP_POWERMODE, power_mode);
}
#endif

#ifdef TUYA_DP_LOWELECTRIC
STATIC VOID handle_DP_LOWELECTRIC(IN TY_OBJ_DP_S *p_obj_dp)
{
    if( (p_obj_dp == NULL) || (p_obj_dp->type != PROP_VALUE) )
    {
        printf("Error!! type invalid %d \r\n", p_obj_dp->type);
        return;
    }
    respone_dp_value(TUYA_DP_LOWELECTRIC, p_obj_dp->value.dp_value);
}
#endif


#ifdef TUYA_DP_SLEEP_MODE
STATIC OPERATE_RET APP_DP_SLEEP_MODE_PROC(IN CONST BOOL_T dp_value,OUT  BOOL_T * dp_report_value )
{
    IPC_APP_set_sleep_mode(dp_value);
    *dp_report_value = IPC_APP_get_sleep_mode();
    return 0;
}
#endif
#ifdef TUYA_DP_LIGHT
STATIC OPERATE_RET APP_DP_LIGHT_PROC(IN CONST BOOL_T light_on_off,OUT  BOOL_T * dp_report_value)
{

    IPC_APP_set_light_onoff(light_on_off);
    *dp_report_value = IPC_APP_get_light_onoff();
     return  0;
}
#endif
//for local storage

#define SD_ENABLE_VALUE     (1)
#define HDD_ENABLE_VALUE     (2)

#ifdef TUYA_DP_LED
STATIC VOID handle_DP_LED(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T enable = p_obj_dp->value.dp_bool;
    TYDEBUG("set led mode [%d]\n",enable);
    respone_dp_bool(TUYA_DP_LED, enable);
}
#endif
#ifdef TUYA_DP_SUPPORT_STORAGE
STATIC VOID handle_DP_SUPPORT_STORAGE(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T mode = 0x3;

    int sd = 1;  //0 未插入  1 插入
    int usb = 0;  //0 未插入  1 插入

    mode = sd|(usb << 1);
    TYDEBUG("cur mode [%d]\n",mode);
    respone_dp_value(TUYA_DP_SUPPORT_STORAGE, mode);
}
#endif
#ifdef TUYA_DP_STOR_VOLUME
STATIC VOID handle_DP_STOR_VOLUME(IN TY_OBJ_DP_S *p_obj_dp)
{
    //INT_T sd_status = IPC_APP_get_sd_status();
    TYDEBUG("SD VALUE [%d]\n", p_obj_dp->value.dp_value);
    CHAR_T tmp_str[100] = {0};
    UINT64_T totol = 0;
    UINT64_T free = 0;

    int stor_mode = SD_ENABLE_VALUE; //HDD_ENABLE_VALUE
#if defined(__USER_DO_NOT_OPEN__) && (__USER_DO_NOT_OPEN__ == 1)
    ty_hdd_get_volume(0, &totol, &free);
#endif

    snprintf(tmp_str, 100, "%llu|%llu|%llu", totol>>10, (totol - free)>>10, free>>10);
    respone_dp_str(TUYA_DP_STOR_VOLUME, tmp_str);
}
#endif
#ifdef TUYA_DP_STOR_STATUS
STATIC VOID handle_DP_STOR_STATUS(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T mode = 1;
    TYDEBUG("cur mode [%d]\n",mode);
    respone_dp_value( TUYA_DP_STOR_STATUS, mode);
}
#endif
#ifdef TUYA_DP_STOR_UNMOUNT
STATIC VOID handle_DP_STOR_UNMOUNT(IN TY_OBJ_DP_S *p_obj_dp)
{
    //BOOL_T umount_result = IPC_APP_unmount_sd_card();
    TYDEBUG("SD UNMOUT\n");
    respone_dp_bool(TUYA_DP_STOR_UNMOUNT, TRUE);
}
#endif

extern  int stor_format_stat;

#ifdef TUYA_DP_STOR_FORMAT_STATUS
void ty_dp_format_percent(int percent)
{
    static int cnt = 0;
    printf("percent[%d]\n",percent);
    if (cnt++%10 == 0){
        stor_format_stat = percent*9/10 + 10;
        respone_dp_value(TUYA_DP_STOR_FORMAT_STATUS, stor_format_stat);
    }
    cnt++;
}
#endif

#ifdef TUYA_DP_STOR_FORMAT
STATIC VOID handle_DP_STOR_FORMAT(IN TY_OBJ_DP_S *p_obj_dp)
{
    //IPC_APP_format_sd_card();
    TYDEBUG("STOR FORMAT\n");
    respone_dp_bool(TUYA_DP_STOR_FORMAT, TRUE);
    extern void tuya_stor_format_set();
    tuya_stor_format_set();
}
#endif

#ifdef TUYA_DP_STOR_FORMAT_STATUS
STATIC VOID handle_DP_STOR_FORMAT_STATUS(IN TY_OBJ_DP_S *p_obj_dp)
{
    respone_dp_value(TUYA_DP_STOR_FORMAT_STATUS, stor_format_stat);
}
#endif
#ifdef TUYA_DP_SET_CUR_STORAGE
STATIC VOID handle_DP_SET_CUR_STORAGE(IN TY_OBJ_DP_S *p_obj_dp)
{
    //INT_T progress = IPC_APP_get_sd_format_status();
    int mode = p_obj_dp->value.dp_value;
    TYDEBUG("HDD SET CUR STORAGE [%d], but we just set as 1\n", mode);
    //mode =1;
    int lst_mode = SD_ENABLE_VALUE;
    //ty_gw_cfg_db_read(BASIC_IPC_STOR_MODE, &lst_mode);
    if (lst_mode != mode){
#if defined(__USER_DO_NOT_OPEN__) && (__USER_DO_NOT_OPEN__ == 1)
        ty_hdd_uninit();
        ty_hdd_init(mode >> 1);
#endif
     //   ty_gw_cfg_db_write(BASIC_IPC_STOR_MODE, &mode);
    }
    respone_dp_value(TUYA_DP_SET_CUR_STORAGE, mode);
}
#endif
#ifdef TUYA_DP_SIREN_SWITCH
STATIC VOID  handle_DP_SIREN_SWITCH(IN TY_OBJ_DP_S *p_obj_dp)
{
    BOOL_T mode = p_obj_dp->value.dp_bool;
    TYDEBUG("get siren mode [%d]\n",mode);


    respone_dp_bool(TUYA_DP_SIREN_SWITCH, mode);
}

#endif

STATIC VOID handle_DP_RESERVED(IN TY_OBJ_DP_S *p_obj_dp)
{
    printf("error! not implememt yet.\r\n");
}


STATIC TUYA_MAIN_DEV_DP_INFO_S s_main_dev_dp_table[] =
{
#ifdef TUYA_DP_SLEEP_MODE
    {TUYA_DP_SLEEP_MODE,            handle_DP_SLEEP_MODE},
#endif
#ifdef TUYA_DP_LIGHT
    {TUYA_DP_LIGHT,                 handle_DP_LIGHT},
#endif
#ifdef TUYA_DP_FLIP
    {TUYA_DP_FLIP,                  handle_DP_FLIP},
#endif
#ifdef TUYA_DP_WATERMARK
    {TUYA_DP_WATERMARK,             handle_DP_WATERMARK},
#endif
#ifdef TUYA_DP_WDR
    {TUYA_DP_WDR,                   handle_DP_WDR},
#endif
#ifdef TUYA_DP_NIGHT_MODE
    {TUYA_DP_NIGHT_MODE,            handle_DP_NIGHT_MODE},
#endif
#ifdef TUYA_DP_ALARM_FUNCTION
    {TUYA_DP_ALARM_FUNCTION,        handle_DP_ALARM_FUNCTION},
#endif
#ifdef TUYA_DP_ALARM_SENSITIVITY
    {TUYA_DP_ALARM_SENSITIVITY,     handle_DP_ALARM_SENSITIVITY},
#endif
//#ifdef TUYA_DP_ALARM_INTERVAL
//    {TUYA_DP_ALARM_INTERVAL,        handle_DP_ALARM_INTERVAL},
//#endif
#ifdef TUYA_DP_ALARM_ZONE_ENABLE
    {TUYA_DP_ALARM_ZONE_ENABLE,     handle_DP_ALARM_ZONE_ENABLE},
#endif

#ifdef TUYA_DP_ALARM_ZONE_DRAW
    {TUYA_DP_ALARM_ZONE_DRAW,     handle_DP_ALARM_ZONE_DRAW},
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
    {TUYA_DP_SD_STATUS_ONLY_GET,    handle_DP_SD_STATUS_ONLY_GET},
#endif
#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
    {TUYA_DP_SD_STORAGE_ONLY_GET,   handle_DP_SD_STORAGE_ONLY_GET},
#endif
#ifdef TUYA_DP_SD_RECORD_ENABLE
    {TUYA_DP_SD_RECORD_ENABLE,      handle_DP_SD_RECORD_ENABLE},
#endif
#ifdef TUYA_DP_SD_RECORD_MODE
    {TUYA_DP_SD_RECORD_MODE,        handle_DP_SD_RECORD_MODE},
#endif
#ifdef TUYA_DP_SD_UMOUNT
    {TUYA_DP_SD_UMOUNT,             handle_DP_SD_UMOUNT},
#endif
#ifdef TUYA_DP_SD_FORMAT
    {TUYA_DP_SD_FORMAT,             handle_DP_SD_FORMAT},
#endif
#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
    {TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, handle_DP_SD_FORMAT_STATUS_ONLY_GET},
#endif
#ifdef TUYA_DP_PTZ_CONTROL
    {TUYA_DP_PTZ_CONTROL,           handle_DP_PTZ_CONTROL},
#endif
#ifdef TUYA_DP_PTZ_STOP
    {TUYA_DP_PTZ_STOP,              handle_DP_PTZ_STOP},
#endif
#ifdef TUYA_DP_PTZ_CHECK
    {TUYA_DP_PTZ_CHECK,              handle_DP_PTZ_CHECK},
#endif
#ifdef TUYA_DP_TRACK_ENABLE
    {TUYA_DP_TRACK_ENABLE,           handle_DP_TRACK_ENABLE},
#endif
#ifdef TUYA_DP_HUM_FILTER
    {TUYA_DP_HUM_FILTER,             handle_DP_HUM_FILTER},
#endif
#ifdef TUYA_DP_PATROL_MODE
    {TUYA_DP_PATROL_MODE,            handle_DP_patrol_mode},
#endif
#ifdef TUYA_DP_PATROL_SWITCH
    {TUYA_DP_PATROL_SWITCH,          handle_DP_patrol_switch},
#endif
#ifdef TUYA_DP_PATROL_TMODE
    {TUYA_DP_PATROL_TMODE,           handle_DP_patrol_tmode},
#endif
#ifdef TUYA_DP_PATROL_TIME
    {TUYA_DP_PATROL_TIME,           handle_DP_patrol_time},
#endif

#ifdef TUYA_DP_PATROL_STATE
    {TUYA_DP_PATROL_STATE,           handle_DP_patrol_state},
#endif

//#ifdef TUYA_DP_PRESET_SET
//    {TUYA_DP_PRESET_SET,              handle_DP_SET_PRESET},
//#endif

#ifdef TUYA_DP_LINK_MOVE_ACTION
    {TUYA_DP_LINK_MOVE_ACTION,    handle_DP_LINK_MOVE},
#endif
#ifdef TUYA_DP_LINK_MOVE_SET
    {TUYA_DP_LINK_MOVE_SET,    handle_DP_LINK_MOVE_SET},
#endif

#ifdef TUYA_DP_DOOR_BELL
    {TUYA_DP_DOOR_BELL,             handle_DP_DOOR_BELL},
#endif
#ifdef TUYA_DP_BLUB_SWITCH
    {TUYA_DP_BLUB_SWITCH,           handle_DP_BLUB_SWITCH},
#endif
#ifdef TUYA_DP_SOUND_DETECT
    {TUYA_DP_SOUND_DETECT,          handle_DP_RESERVED},
#endif
#ifdef TUYA_DP_SOUND_SENSITIVITY
    {TUYA_DP_SOUND_SENSITIVITY,     handle_DP_RESERVED},
#endif
#ifdef TUYA_DP_SOUND_ALARM
    {TUYA_DP_SOUND_ALARM,           handle_DP_RESERVED},
#endif
#ifdef TUYA_DP_TEMPERATURE
    {TUYA_DP_TEMPERATURE,           handle_DP_RESERVED},
#endif
#ifdef TUYA_DP_HUMIDITY
    {TUYA_DP_HUMIDITY,              handle_DP_RESERVED},
#endif
#ifdef TUYA_DP_ELECTRICITY
    {TUYA_DP_ELECTRICITY,           handle_DP_ELECTRICITY},
#endif
#ifdef TUYA_DP_POWERMODE
    {TUYA_DP_POWERMODE,             handle_DP_POWERMODE},
#endif
#ifdef TUYA_DP_LOWELECTRIC
    {TUYA_DP_LOWELECTRIC,           handle_DP_LOWELECTRIC},
#endif
#ifdef TUYA_DP_LED
    {TUYA_DP_LED,    handle_DP_LED},
#endif
#ifdef TUYA_DP_SUPPORT_STORAGE
    {TUYA_DP_SUPPORT_STORAGE,    handle_DP_SUPPORT_STORAGE},
#endif
#ifdef TUYA_DP_STOR_VOLUME
    {TUYA_DP_STOR_VOLUME,    handle_DP_STOR_VOLUME},
#endif
#ifdef TUYA_DP_STOR_STATUS
    {TUYA_DP_STOR_STATUS,    handle_DP_STOR_STATUS},
#endif
#ifdef TUYA_DP_STOR_FORMAT
    {TUYA_DP_STOR_FORMAT,    handle_DP_STOR_FORMAT},
#endif
#ifdef TUYA_DP_STOR_UNMOUNT
    {TUYA_DP_STOR_UNMOUNT,    handle_DP_STOR_UNMOUNT},
#endif
#ifdef TUYA_DP_STOR_FORMAT_STATUS
    {TUYA_DP_STOR_FORMAT_STATUS,    handle_DP_STOR_FORMAT_STATUS},
#endif
#ifdef TUYA_DP_SET_CUR_STORAGE
    {TUYA_DP_SET_CUR_STORAGE,    handle_DP_SET_CUR_STORAGE},
#endif
#ifdef TUYA_DP_SIREN_SWITCH
    {TUYA_DP_SIREN_SWITCH,    handle_DP_SIREN_SWITCH},
#endif
};

/*xvr sub dev dp process*/
STATIC TUYA_SUB_DEV_DP_INFO_S s_sub_dev_dp_table[] =
{
#if 0
#ifdef TUYA_DP_PATROL_MODE
    {TUYA_DP_PATROL_MODE,            handle_DP_patrol_mode},
#endif
#ifdef TUYA_DP_PATROL_SWITCH
    {TUYA_DP_PATROL_SWITCH,          handle_DP_patrol_switch},
#endif
#ifdef TUYA_DP_PATROL_TMODE
    {TUYA_DP_PATROL_TMODE,           handle_DP_patrol_tmode},
#endif
#ifdef TUYA_DP_PATROL_TIME
    {TUYA_DP_PATROL_TIME,           handle_DP_patrol_time},
#endif

#ifdef TUYA_DP_PATROL_STATE
    {TUYA_DP_PATROL_STATE,           handle_DP_patrol_state},
#endif
#endif
#ifdef TUYA_DP_PRESET_SET
    {TUYA_DP_PRESET_SET,              handle_DP_SET_PRESET},
#endif

};
//主设备dp处理入口函数
void tuya_xvr_main_dev_dp_process(CONST TY_RECV_OBJ_DP_S *dp)
{
    UINT_T index = 0;
    for(index = 0; index < dp->dps_cnt; index++) {
        CONST TY_OBJ_DP_S *p_dp_obj = dp->dps + index;
        printf("get gw dp cmd [%d]\n",p_dp_obj->dpid);
        INT_T table_count = ( sizeof(s_main_dev_dp_table) / sizeof(s_main_dev_dp_table[0]) );
        INT_T table_idx = 0;
        for(table_idx = 0; table_idx < table_count; table_idx++){
            if(s_main_dev_dp_table[table_idx].dp_id == p_dp_obj->dpid){
                s_main_dev_dp_table[table_idx].handler((TY_OBJ_DP_S *)p_dp_obj);
                break;
            }
        }
    }

    return;
}

void tuya_xvr_sub_dev_dp_process(CONST TY_RECV_OBJ_DP_S *dp)
{
    printf("SOC Rev DP Obj Cmd t1: %d t2: %d CNT: %u\n", dp->cmd_tp, dp->dtt_tp, dp->dps_cnt);

    UINT_T index = 0;
    TY_OBJ_DP_S *dp_data = (TY_OBJ_DP_S*) (dp->dps);
    INT_T table_idx = 0;
    INT_T table_count = (sizeof(s_sub_dev_dp_table) / sizeof(s_sub_dev_dp_table[0]));

    for (index = 0;index < dp->dps_cnt;index++) {
        CONST TY_OBJ_DP_S *p_dp_obj = dp->dps + index;
        printf("idx: %d dpid: %d type: %d ts: %u cid = %s\n", index, p_dp_obj->dpid, p_dp_obj->type, p_dp_obj->time_stamp, dp->cid);
        //step：处理对应dp
        for (table_idx = 0;table_idx < table_count;table_idx++) {
            if (s_sub_dev_dp_table[table_idx].dp_id == p_dp_obj->dpid) {
                s_sub_dev_dp_table[table_idx].handler(dp->cid, p_dp_obj);
                break;
            }
        }

    }
    return;
}

/*IPC_APP_handle_dp_cmd_objs ：用于接收来自APP端和云端的DP操作。
 *在xvr设备里，通过参数中的dp->cid区分主设备和子设备。当参数dp->cid为空，则表示是xvr 主设备的dp点回调。当cid != NULL，则表示是对应的子设备dp回到。
 *通常dp是和产品PID关联的，主设备和子设备都有对应的PID,也都配置了各字的DP点(主设备和子设备的dp值可以相同，但是可以用于不同展示不同的功能)。用户根据自身需求，实现对对应dp的处理。
*/
VOID IPC_APP_handle_dp_cmd_objs(IN CONST TY_RECV_OBJ_DP_S *dp)
{
    if (dp == NULL) {
        printf("soc not have cid\n");
        return;
    }

    if (NULL == dp->cid) {
        //主设备dp处理入口
        tuya_xvr_main_dev_dp_process(dp);
    } else {
        INT_T chn = 0;
        tuya_xvr_dev_chan_get_by_devId(dp->cid, &chn);
        //客户根据chn 来感知是哪个通道的dp信令，下面的处理接口可做下修改适配
        tuya_xvr_sub_dev_dp_process(dp);
    }

    return;
}
/*
 * 处理一种称为 raw 类型dp点，这类dp通常是自定义的一些内容。
 * 处理的原则同 非raw的规则。demo只是简单回复
 * */
VOID IPC_APP_handle_dp_cmd_raw(IN CONST TY_RECV_RAW_DP_S *dp)
{
    if (dp == NULL || dp->cid == NULL) {
        printf("soc not have cid\n");
    }

    printf("SOC Rev DP Raw Cmd t1: %d t2: %d dpid: %d len: %u\n", dp->cmd_tp, dp->dtt_tp, dp->dpid, dp->len);

    OPERATE_RET op_ret = dev_report_dp_raw_sync(dp->cid, dp->dpid, dp->data, dp->len, 0);
    if (OPRT_OK != op_ret) {
        printf("dev_report_dp_json_async op_ret: %d\n", op_ret);
    }

    return;
}
/*
 * dp点查询回调接口。
 *
 * */
VOID IPC_APP_handle_dp_query_objs(IN CONST TY_DP_QUERY_S *dp_query)
{
//    INT_T table_idx = 0;
//    INT_T table_count = (sizeof(s_main_dev_dp_table) / sizeof(s_main_dev_dp_table[0]));
//    INT_T index = 0;
//    for (index = 0;index < dp_query->cnt;index++) {
//        for (table_idx = 0;table_idx < table_count;table_idx++) {
//            if (s_main_dev_dp_table[table_idx].dp_id == dp_query->dpid[index]) {
//                s_main_dev_dp_table[table_idx].handler(NULL);
//                break;
//            }
//        }
//    }
}

#ifdef __cplusplus
}
#endif

