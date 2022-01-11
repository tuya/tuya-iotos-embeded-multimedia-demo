/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_dp_utils.h
  *
  *Do not modify any of the contents of this file at will.
  Please contact the Product Manager if you need to modify it! !
  * File Description：
  * 1. DP point definition
  * 2. DP point API definition
**********************************************************************************/

#ifndef _TUYA_IPC_DP_UTILS_H
#define _TUYA_IPC_DP_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include "tuya_ipc_api.h"
#include "tuya_cloud_com_defs.h"

#include "cJSON.h"

/* Basic function settings page of APP*/
#define TUYA_DP_SLEEP_MODE                 105         /* Sleep, BOOL type, true means sleep, false means wake*/
#define TUYA_DP_LIGHT                      101         /* Status indicator, BOOL type, true means open, false means closed */
#define TUYA_DP_FLIP                       103         /* Flip state, BOOL type, true means reverse, false means normal */
#define TUYA_DP_WATERMARK                  104         /* Watermark, BOOL type, true menas open, false means closed*/
#define TUYA_DP_WDR                        107         /* Wide dynamic range mode, BOOL type, true means open, false means closed*/
#define TUYA_DP_NIGHT_MODE                 108         /* Infrared night vision, ENUM type, 0-auto 1-turn 2-open*/
/* APP移动侦测报警页面 */
#define TUYA_DP_ALARM_FUNCTION             134         /* Motion detection alarm switch, BOOL type, true means open, false mens closed */
#define TUYA_DP_ALARM_SENSITIVITY          106         /* Motion detection alarm sensitivity, ENUM type, 0 is low sensitivity, 1 is medium sensitivity, 2 is high sensitivity*/
/* APP储存卡设置页面 */
#define TUYA_DP_SD_STATUS_ONLY_GET         110         /*SD card status, VALUE type, 1-normal, 2-anomaly, 3-insufficient space, 4-formatting, 5-no SD card */
#define TUYA_DP_SD_STORAGE_ONLY_GET        109         /* SD card capacity, STR type, "Total capacity|Current usage|Remaining capacity", unit: kb */
#define TUYA_DP_SD_RECORD_ENABLE           150         /* SD card recording switch, BOOL type, true means open, false means closed */
#define TUYA_DP_SD_RECORD_MODE             151         /* SD card recording mode, ENUM type, 0-event recording 1-continuous recording*/
#define TUYA_DP_SD_UMOUNT                  112         /* Exit SD card, BOOL type, true means it has been exited, false means it has not exited */
#define TUYA_DP_SD_FORMAT                  111         /* Format sd card, BOOL type */
#define TUYA_DP_SD_FORMAT_STATUS_ONLY_GET  117         /* Format sd card status, VALUE type, -2000: SD card is being formatted, -2001: SD card formatting is abnormal, 
                                                          -2002: No SD card, -2003: SD card error. Positive number is formatting progress */
/* Camera control page of APP */
#define TUYA_DP_PTZ_CONTROL                119         /* PTZ rotation control, ENUM type, 0-up, 1-right upper, 2-right, 3-right down, 4-down, 5--lower left, 6-left, 7-left upper
                                                        but 0-upper right, 1-right, 2-lower right, 3-down, 4-left down, 5-left, 6-up left, 7-up in 4.0 SDK*/
#define TUYA_DP_PTZ_STOP                   116         /* PTZ rotation stops, BOOL type*/
#define TUYA_DP_PTZ_CHECK                  132       /*PTZ self-check switch, 0 means off, 1 means on, default value is off*/
#define TUYA_DP_TRACK_ENABLE               161        /*Move tracking switch, BOOL type, true means on, false means closed*/
#define TUYA_DP_HUM_FILTER                 170        /*Human filtering，BOOL type，,true means open，false means closed*/
#define TUYA_DP_PATROL_SWITCH              174       /*patrol switch，0 means closed  ，1 means open*/
#define TUYA_DP_PATROL_MODE                175       /*patrol path mode，0：automatically patrol  ，1：patrol according to presets*/
#define TUYA_DP_PATROL_TMODE               176       /*patrol time mode，0：patrol whole day，1：patrol timing */
#define TUYA_DP_PATROL_TIME                177        /*patrol time setting*/
#define TUYA_DP_PRESET_SET                 178         /*Preset point operation，string type，type:1 add，type:2 delete.Different types are different from data strings*/
#define TUYA_DP_PATROL_STATE               179         /*Patrol status query command，0:automatically patrol 1：patrol according to presets 2：not patrolling*/

/*Motion detection area setting page*/
#define TUYA_DP_ALARM_ZONE_ENABLE          168        /* Detection area switch, BOOL type, true means on, false means closed*/
#define TUYA_DP_ALARM_ZONE_DRAW            169        /* Detection area setting, STR type，example:{"num":1, "region0":{"x":30,"y":40,"xlen":50,"ylen":60}}  */

/* special DP point in IPC */
#define TUYA_DP_DOOR_BELL                  136         /* Doorbell call, STR type, "current timestamp"*/
#define TUYA_DP_BLUB_SWITCH                138         /* Light control switch, BOOL type, true menas open, false means closed*/
#define TUYA_DP_SOUND_DETECT               139         /* Decibel detection switch,BOOL type,true means open,false means closed */
#define TUYA_DP_SOUND_SENSITIVITY          140         /* Decibel detection sensitivity, ENUM type, 0 means low sensitivity, 1 means high sensitivity */
#define TUYA_DP_SOUND_ALARM                141         /* Decibel alarm channel, STR type, "current timestamp" */
#define TUYA_DP_TEMPERATURE                142         /* Temperature measurement, VALUE type,[0-50] */
#define TUYA_DP_HUMIDITY                   143         /* Humidity measurement, VALUE type,[0-100] */
#define TUYA_DP_ELECTRICITY                145         /* Battery power percentage, VALUE type[0-100] */
#define TUYA_DP_POWERMODE                  146         /* Power supply mode, ENUM type, "0" is the battery power supply state, "1" is the plug-in power supply state (or battery charging state) */
#define TUYA_DP_LOWELECTRIC                147         /* Low battery alarm threshold, VALUE type */
#define TUYA_DP_DOOR_STATUS                149         /* Doorbell status notification, BOOL type, true means open, false means closed  */

#define TUYA_DP_MOTION_DETECTION_ALARM     115         /* Motion detection message alarm */
#define TUYA_DP_DOOR_BELL_SNAP             154         /* Doorbell push tips from screenshot*/

/*ptz联动dp点*/
#define TUYA_DP_LINK_MOVE_ACTION          190         /* ptz联动dp点*/
#define TUYA_DP_LINK_MOVE_SET             199         /* ptz联动dp点*/
#define TUYA_DP_AP_MODE                   231         /* AP mode query, BOOL type, value 1 means ap mode, value 0 means not in ap mode*/
#define TUYA_DP_AP_SWITCH                 232         /*data from app:{ ap_enable : 1, ap_ssid : xxxx, ap_pwd : xxx } data to app: {ap_enable:0, errcode : 0 }*/

#define TUYA_RAW_DP_DEMO_SYTLE            102        /* a examples of raw dp*/

/* Report the latest status of all local DP points*/
VOID IPC_APP_upload_all_status(VOID);

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
/* SD card formatting progress report*/
VOID IPC_APP_report_sd_format_status(INT_T status);
#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
/*When the SD card changes (plug and unplug), call this API to notify to APP*/
VOID IPC_APP_report_sd_status_changed(INT_T status);
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
/* After formatting, call this API to report the storage capacity. */
VOID IPC_APP_report_sd_storage();
#endif


#ifdef TUYA_DP_POWERMODE
/* Call this API to notify the app when the battery status changes. */
VOID IPC_APP_update_battery_status(VOID);
#endif

#ifdef TUYA_DP_MOTION_DETECTION_ALARM
/* When a motion detection event occurs, the API is called to push an alarm picture to the APP. */
OPERATE_RET IPC_APP_Send_Motion_Alarm_From_Buffer(CHAR_T *data, UINT_T size, NOTIFICATION_CONTENT_TYPE_E type);
OPERATE_RET IPC_APP_Send_Motion_Alarm(CHAR_T *p_abs_file, NOTIFICATION_CONTENT_TYPE_E file_type);
#endif

#ifdef TUYA_DP_DOOR_BELL_SNAP
/* When the doorbell is pressed, it will captures a picture and calls the API to push the picture to the APP. */
OPERATE_RET IPC_APP_Send_DoorBell_Snap(CHAR_T *p_snap_file, NOTIFICATION_CONTENT_TYPE_E file_type);
#endif

/* basic API */
VOID IPC_APP_handle_dp_cmd_objs(IN CONST TY_RECV_OBJ_DP_S *dp_rev);
/* API */
VOID IPC_APP_handle_dp_query_objs(IN CONST TY_DP_QUERY_S *dp_query);
VOID IPC_APP_handle_raw_dp_cmd_objs(IN CONST TY_RECV_RAW_DP_S *dp_rev);

typedef OPERATE_RET (*SIMULATION_FILTER)(BYTE_T recv_dp_id);
VOID IPC_APP_set_dp_filter(SIMULATION_FILTER);

#if defined(TUYA_EXTEND_DEVELOP)
OPERATE_RET APP_DP_VALUE_CMD_PROC(IN CONST INT_T dp_id, IN CONST INT_T dp_value,OUT  INT_T * dp_report_value);
OPERATE_RET APP_DP_BOOL_CMD_PROC(IN CONST INT_T dp_id, IN CONST BOOL_T dp_value,OUT  BOOL_T * dp_report_value );
OPERATE_RET APP_DP_STR_CMD_PROC(IN CONST INT_T dp_id, IN CONST CHAR_T *dp_value,OUT  CHAR_T * dp_report_value);
OPERATE_RET APP_DP_ENUM_CMD_PROC(IN CONST INT_T dp_id, IN CONST UINT_T dp_value,OUT  CHAR_T * dp_report_value);
OPERATE_RET APP_DP_BITMAP_CMD_PROC(IN CONST INT_T dp_id, IN CONST UINT_T dp_value,OUT  UINT_T * dp_report_value);
#endif

#ifdef __cplusplus
}
#endif

#endif  /*_TUYA_IPC_DP_UTILS_H*/
