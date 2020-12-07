/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *
  * File Description：
  * 1. DP point setting and get function definition, please refer to the code comment in the .c file for details.
**********************************************************************************/

#ifndef _TUYA_IPC_DP_HANDLER_H
#define _TUYA_IPC_DP_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_ptz.h"

#ifdef TUYA_DP_SLEEP_MODE
VOID IPC_APP_set_sleep_mode(BOOL_T sleep_mode);
BOOL_T IPC_APP_get_sleep_mode(VOID);
#endif

#ifdef TUYA_DP_LIGHT
VOID IPC_APP_set_light_onoff(BOOL_T light_on_off);
BOOL_T IPC_APP_get_light_onoff(VOID);
#endif

#ifdef TUYA_DP_FLIP
VOID IPC_APP_set_flip_onoff(BOOL_T flip_on_off);
BOOL_T IPC_APP_get_flip_onoff(VOID);
#endif

#ifdef TUYA_DP_WATERMARK
VOID IPC_APP_set_watermark_onoff(BOOL_T watermark_on_off);
BOOL_T IPC_APP_get_watermark_onoff(VOID);
#endif

#ifdef TUYA_DP_WDR
VOID IPC_APP_set_wdr_onoff(BOOL_T wdr_on_off);
BOOL_T IPC_APP_get_wdr_onoff(VOID);
#endif

#ifdef TUYA_DP_NIGHT_MODE
VOID IPC_APP_set_night_mode(CHAR_T *p_night_mode);
CHAR_T *IPC_APP_get_night_mode(VOID);
#endif


#ifdef TUYA_DP_ALARM_FUNCTION
VOID IPC_APP_set_alarm_function_onoff(BOOL_T alarm_on_off);
BOOL_T IPC_APP_get_alarm_function_onoff(VOID);
#endif

#ifdef TUYA_DP_ALARM_SENSITIVITY
VOID IPC_APP_set_alarm_sensitivity(CHAR_T *p_sensitivity);
CHAR_T *IPC_APP_get_alarm_sensitivity(VOID);
#endif

#ifdef TUYA_DP_ALARM_ZONE_DRAW
VOID IPC_APP_set_alarm_zone_draw(cJSON * p_alarm_zone);
char * IPC_APP_get_alarm_zone_draw(VOID_T);
#endif

#ifdef TUYA_DP_ALARM_ZONE_ENABLE
VOID IPC_APP_set_alarm_zone_onoff(BOOL_T alarm_zone_on_off);
BOOL_T IPC_APP_get_alarm_zone_onoff(VOID_T);
#endif


//#ifdef TUYA_DP_ALARM_INTERVAL
//VOID IPC_APP_set_alarm_interval(CHAR_T *p_interval);
//CHAR_T *IPC_APP_get_alarm_interval(VOID);
//#endif

#ifdef TUYA_DP_SD_STATUS_ONLY_GET
INT_T IPC_APP_get_sd_status(VOID);
#endif

#ifdef TUYA_DP_SD_STORAGE_ONLY_GET
VOID IPC_APP_get_sd_storage(UINT_T *p_total, UINT_T *p_used, UINT_T *p_empty);
#endif

#ifdef TUYA_DP_SD_RECORD_ENABLE
VOID IPC_APP_set_sd_record_onoff(BOOL_T sd_record_on_off);
BOOL_T IPC_APP_get_sd_record_onoff(VOID);
#endif

#ifdef TUYA_DP_SD_RECORD_MODE
VOID IPC_APP_set_sd_record_mode(UINT_T sd_record_mode);
UINT_T IPC_APP_get_sd_record_mode(VOID);
#endif

#ifdef TUYA_DP_SD_UMOUNT
BOOL_T IPC_APP_unmount_sd_card(VOID);
#endif

#ifdef TUYA_DP_SD_FORMAT
VOID IPC_APP_format_sd_card(VOID);
#endif

#ifdef TUYA_DP_SD_FORMAT_STATUS_ONLY_GET
INT_T IPC_APP_get_sd_format_status(VOID);
#endif

#ifdef TUYA_DP_PTZ_CONTROL
VOID IPC_APP_ptz_start_move(CHAR_T *p_direction);
#endif

#ifdef TUYA_DP_PTZ_STOP
VOID IPC_APP_ptz_stop_move(VOID);
#endif

#ifdef TUYA_DP_PTZ_CHECK
void IPC_APP_ptz_check(VOID);
#endif

#ifdef TUYA_DP_TRACK_ENABLE
void IPC_APP_track_enable(BOOL_T track_enable);

BOOL_T IPC_APP_get_track_enable(void);

#endif

#ifdef TUYA_DP_LINK_MOVE_ACTION
VOID IPC_APP_set_link_move(INT_T bind_seq);
#endif

#ifdef TUYA_DP_LINK_MOVE_SET
VOID IPC_APP_set_link_pos(INT_T bind_seq);
#endif


#ifdef TUYA_DP_HUM_FILTER
void IPC_APP_human_filter(BOOL_T filter_enable);
#endif

#ifdef TUYA_DP_PATROL_MODE
void IPC_APP_set_patrol_mode(BOOL_T patrol_mode);
char IPC_APP_get_patrol_mode(void);

#endif

#ifdef TUYA_DP_PATROL_SWITCH
void IPC_APP_set_patrol_switch(BOOL_T patrol_switch);

BOOL_T IPC_APP_get_patrol_switch(void);

void IPC_APP_ptz_preset_reset(S_PRESET_CFG *preset_cfg);

#endif

#ifdef TUYA_DP_PATROL_TMODE
void IPC_APP_set_patrol_tmode(BOOL_T patrol_tmode);

char IPC_APP_get_patrol_tmode(void);
#endif

#ifdef TUYA_DP_PATROL_TIME
void IPC_APP_set_patrol_time(cJSON * p_patrol_time);
#endif

#ifdef TUYA_DP_PRESET_SET
void IPC_APP_set_preset(cJSON * p_preset_param);

#endif

#ifdef TUYA_DP_PATROL_STATE
void IPC_APP_patrol_state(int *patrol_state);
#endif


#ifdef TUYA_DP_BLUB_SWITCH
VOID IPC_APP_set_blub_onoff(BOOL_T blub_on_off);
BOOL_T IPC_APP_get_blub_onoff(VOID);
#endif

#ifdef TUYA_DP_ELECTRICITY
INT_T IPC_APP_get_battery_percent(VOID);
#endif

#ifdef TUYA_DP_POWERMODE
CHAR_T *IPC_APP_get_power_mode(VOID);
#endif

#ifdef __cplusplus
}
#endif

#endif  /*_TUYA_IPC_DP_HANDLER_H*/
