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
#include "tuya_iot_config.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_dp_handler.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_iot_config.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

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
    IPC_APP_format_sd_card();
    respone_dp_bool(TUYA_DP_SD_FORMAT, TRUE);
}
#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
STATIC VOID handle_DP_SD_FORMAT_STATUS_ONLY_GET(IN TY_OBJ_DP_S *p_obj_dp)
{
    INT_T progress = IPC_APP_get_sd_format_status();
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
STATIC VOID handle_DP_SET_PRESET(IN TY_OBJ_DP_S *p_dp_json)
{
  
    IPC_APP_set_preset((cJSON *)(p_dp_json->value.dp_str));
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

STATIC VOID handle_DP_RESERVED(IN TY_OBJ_DP_S *p_obj_dp)
{
    printf("error! not implememt yet.\r\n");
}



#if defined(WIFI_GW) && (WIFI_GW==1)
#ifdef TUYA_DP_AP_MODE
STATIC VOID handle_DP_AP_MODE(IN TY_OBJ_DP_S *p_dp_json)
{
    if(p_dp_json == NULL )
    {
        printf("Error!! type invalid\r\n");
        return;
    }
    respone_dp_str(TUYA_DP_AP_MODE, IPC_APP_get_ap_mode());


}
#endif
#ifdef TUYA_DP_AP_SWITCH
STATIC VOID handle_DP_AP_SWITCH(IN TY_OBJ_DP_S *p_dp_json)
{
    CHAR_T resp[32] = {0};
    INT_T ap_enable = IPC_APP_set_ap_mode((cJSON *)p_dp_json->value.dp_str);
    if(ap_enable < 0)
    {
        snprintf(resp, 32, "{\\\"ap_enable\\\":0,\\\"errcode\\\":0}");
    }
    else
    {
        snprintf(resp, 32, "{\\\"ap_enable\\\":%d,\\\"errcode\\\":0}",ap_enable);
    }
    respone_dp_str(TUYA_DP_AP_SWITCH, resp);
    if(ap_enable >= 0)
    {
        change_ap_process();
    }
}
#endif
#endif

#ifdef handle_RAW_DP_DEMO_STYLE
STATIC VOID handle_RAW_DP_DEMO_STYLE(TY_RECV_RAW_DP_S * raw_ctx)
{
    printf("rec raw dp is %s\n",raw_ctx->data);
    // developer do it according to needing
    return;
}
#endif

typedef VOID (*TUYA_DP_HANDLER)(IN TY_OBJ_DP_S *p_obj_dp);
typedef struct
{
    BYTE_T dp_id;
    TUYA_DP_HANDLER handler;
}TUYA_DP_INFO_S;

STATIC TUYA_DP_INFO_S s_dp_table[] =
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

#ifdef TUYA_DP_PRESET_SET
    {TUYA_DP_PRESET_SET,              handle_DP_SET_PRESET},
#endif

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

#if defined(WIFI_GW) && (WIFI_GW==1)

#ifdef TUYA_DP_AP_MODE
    {TUYA_DP_AP_MODE,               handle_DP_AP_MODE},
#endif
#ifdef TUYA_DP_AP_SWITCH
    {TUYA_DP_AP_SWITCH,             handle_DP_AP_SWITCH},
#endif

#endif
};

typedef VOID (*TUYA_RAW_DP_HANDLER)(IN TY_RECV_RAW_DP_S *p_obj_dp);
typedef struct
{
    BYTE_T dp_id;
    TUYA_RAW_DP_HANDLER handler;
}TUYA_RAW_DP_INFO_S;

STATIC TUYA_RAW_DP_INFO_S s_raw_dp_table[] =
{
#ifdef TUYA_RAW_DP_DEMO_STYLE
    {TUYA_RAW_DP_DEMO_STYLE,            handle_RAW_DP_DEMO_STYLE},
#endif

};

STATIC SIMULATION_FILTER dp_simulation_filter = NULL;
VOID IPC_APP_set_dp_filter(SIMULATION_FILTER filter_cb)
{
    dp_simulation_filter = filter_cb;
    return;
}
VOID IPC_APP_handle_dp_cmd_objs(IN CONST TY_RECV_OBJ_DP_S *dp_rev)
{
    TY_OBJ_DP_S *dp_data = (TY_OBJ_DP_S *)(dp_rev->dps);
    UINT_T cnt = dp_rev->dps_cnt;
    INT_T table_idx = 0;
    INT_T table_count = ( sizeof(s_dp_table) / sizeof(s_dp_table[0]) );
    INT_T index = 0;
    for(index = 0; index < cnt; index++)
    {
        TY_OBJ_DP_S *p_dp_obj = dp_data + index;

        if(dp_simulation_filter)
        {
            if(dp_simulation_filter(p_dp_obj->dpid) == 0)
            {
                continue;
            }
        }

        for(table_idx = 0; table_idx < table_count; table_idx++)
        {
            if(s_dp_table[table_idx].dp_id == p_dp_obj->dpid)
            {
                s_dp_table[table_idx].handler(p_dp_obj);
                break;
            }
        }
    }
}
VOID IPC_APP_handle_raw_dp_cmd_objs(IN CONST TY_RECV_RAW_DP_S *dp_rev)
{

    INT_T table_idx = 0;
    INT_T table_count = ( sizeof(s_raw_dp_table) / sizeof(s_raw_dp_table[0]) );
    INT_T index = 0;

    for(table_idx = 0; table_idx < table_count; table_idx++)
    {
        if(s_raw_dp_table[table_idx].dp_id == dp_rev->dpid)
        {
            s_raw_dp_table[table_idx].handler(dp_rev);
            break;
        }
    }
}

VOID IPC_APP_handle_dp_query_objs(IN CONST TY_DP_QUERY_S *dp_query)
{
    INT_T table_idx = 0;
    INT_T table_count = ( sizeof(s_dp_table) / sizeof(s_dp_table[0]) );
    INT_T index = 0;
    for(index = 0; index < dp_query->cnt; index++)
    {
        if(dp_simulation_filter)
        {
            if(dp_simulation_filter(dp_query->dpid) == 0)
            {
                continue;
            }
        }
        for(table_idx = 0; table_idx < table_count; table_idx++)
        {
            if(s_dp_table[table_idx].dp_id == dp_query->dpid[index])
            {
                s_dp_table[table_idx].handler(NULL);
                break;
            }
        }
    }
}

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

OPERATE_RET APP_DP_BOOL_CMD_PROC(IN CONST INT_T dp_id, IN CONST BOOL_T dp_value,OUT  BOOL_T * dp_report_value )
{
	OPERATE_RET ret = 0;
	switch(dp_id)
	{
        case TUYA_DP_LIGHT:
        {
            APP_DP_LIGHT_PROC(dp_value,dp_report_value);
            break;
        }
		case TUYA_DP_SLEEP_MODE:
		{
			//用户处理 TUYA_DP_SLEEP_MODE
			ret = APP_DP_SLEEP_MODE_PROC(dp_value,dp_report_value);
			break;
		}
		case TUYA_DP_TRACK_ENABLE:
		{
			//用户处理TUYA_DP_TRACK_ENABLE
			*dp_report_value =  dp_value;
			break;
		}
		case TUYA_DP_ALARM_FUNCTION:
		{
			//用户处理移动侦测报警
			//保存配置
		//s	__tuya_app_write_INT("tuya_alarm_on_off", alarm_on_off);
			IPC_APP_set_alarm_function_onoff(dp_value);
			*dp_report_value = dp_value;

			break;
		}
		case TUYA_DP_SD_RECORD_ENABLE:
		{
			//用户处理存储功能设置
			IPC_APP_set_sd_record_onoff(dp_value);
			*dp_report_value = dp_value;
			break;
		}
		default:
		{
			ret = -1;
			break;
		}

	}
	return ret;
}
OPERATE_RET APP_DP_VALUE_CMD_PROC(IN CONST INT_T dp_id, IN CONST INT_T dp_value,OUT  INT_T * dp_report_value)
{
	OPERATE_RET ret = 0;
	switch (dp_id)
	{
		case TUYA_DP_SD_STATUS_ONLY_GET:
		{
			/* SD card status, VALUE type, 1-normal, 2-anomaly, 3-insufficient space, 4-formatting, 5-no SD card */
			/* Developer needs to return local SD card status */
			*dp_report_value = 1;
			break;
		}
		defalut:
		{
			return -1;
			break;
		}
	}
	return ret;
}
OPERATE_RET APP_DP_STR_CMD_PROC(IN CONST INT_T dp_id, IN CONST CHAR_T *dp_value,OUT  CHAR_T * dp_report_value)
{
	OPERATE_RET ret = 0;
	switch (dp_id)
	{
		case TUYA_DP_SD_STORAGE_ONLY_GET:
		{

		    /* Developer needs to return local SD card status */
		    UINT_T total = 128 * 1000 * 1000;
		    UINT_T used =32 * 1000 * 1000;
		    UINT_T empty = total - used;
		    //"total capacity|Current usage|remaining capacity"
		    //dp_report_value 最大长度256
		    snprintf(dp_report_value, 100, "%u|%u|%u", total, used, empty);

			break;
		}
		default:
		{
			ret = -1;
			break;
		}
	}
	return ret;
}

OPERATE_RET APP_DP_ENUM_CMD_PROC(IN CONST INT_T dp_id, IN CONST UINT_T dp_value,OUT  CHAR_T * dp_report_value)
{
	OPERATE_RET ret = 0;
	switch (dp_id)
	{
		case TUYA_DP_SD_RECORD_MODE:
		{
			IPC_APP_set_sd_record_mode(dp_value);
			UINT_T mode = IPC_APP_get_sd_record_mode();
			snprintf(dp_report_value,2,"%d",mode);
			break;
		}
		default:
		{
			ret = -1;
			break;
		}
	}
	return ret;
}
OPERATE_RET APP_DP_BITMAP_CMD_PROC(IN CONST INT_T dp_id, IN CONST UINT_T dp_value,OUT  UINT_T * dp_report_value)
{
	OPERATE_RET ret = 0;
	return ret;
}



