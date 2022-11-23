#ifndef _TUYA_FACTORY_CMD_H_
#define _TUYA_FACTORY_CMD_H_
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tuya_fac_protocol.h"

int tuya_test_mode(char *str);

int tuya_get_version(char *data);

int tuya_get_slave_version(char *data);

int tuya_test_button();

int tuya_test_led(char *str);

int tuya_test_video(char *data);

int tuya_test_audio();

int tuya_test_ircut(char *str);

int tuya_test_speaker(char *str);

int tuya_test_mic(char *str, char *ipaddr);

int tuya_test_irled(char *str);

int tuya_test_black_video(char *data);

int tuya_read_pid(char *pid, int size);

int tuya_write_pid(char *pid, int size);

int tuya_read_uuid(char *uuid, int size);

int tuya_write_uuid(char *uuid, int size);

int tuya_read_authkey(char *authkey, int size);

int tuya_write_authkey(char *authkey, int size);

int tuya_write_cfg(char *data);

int tuya_read_cfg(char *data);

int tuya_write_sn(char *str);

int tuya_read_sn(char *data);

int tuya_write_bsn(char *str);

int tuya_read_bsn(char *data);

int tuya_read_mac(char *data);

int tuya_write_mac(char *str);

int tuya_write_cc(char *str);

int tuya_read_cc(char *data);

int tuya_test_wifi_strength(char *data);

int tuya_test_iperf(char *addr, char *str);

int tuya_get_iperf_result(char *str, double *avr_v, double *max_v, double *min_v);

int tuya_test_motor();

int tuya_test_pir();

int tuya_read_light_sensor();

int tuya_get_battery_value(char *data);

int tuya_write_battery_info(char *str);

int tuya_read_battery_info(char *data);

int tuya_get_wifi_calibration_flag(char *str);

int tuya_set_wifi_cfg(char *str);

int tuya_dn_switch_start();

int tuya_test_bool_general(char *str, char *ret_buf, int buf_size);

#endif /*_TUYA_FACTORY_CMD_H_*/
