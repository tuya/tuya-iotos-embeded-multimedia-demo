#ifndef _TUYA_FAC_PROTOCOL_H__
#define _TUYA_FAC_PROTOCOL_H__

// tuya产测帧长度 根据具体需求自定义
#define TUYA_FRAME_MAX_LEN 256
#define INFO_MAX           32

typedef enum
{
    LED_CMD_MIN = -1,
    LED_CMD_OFF = 0,
    LED_CMD_ON = 1,
    LED_CMD_CHANGE = 2,
    LED_CMD_MAX

} LED_CMD_E;


typedef enum
{
    TUYATEST_MODE = 0x00,
    TUYATEST_R_MASTER_FIRMWARE = 0x01,
    TUYATEST_R_SLAVE_FIRMWARE = 0x02,
    TUYATEST_W_CFGINFO = 0x03,
    TUYATEST_R_CFGINFO = 0x04,
    TUYATEST_W_MASTER_MAC = 0x05,
    TUYATEST_R_MASTER_MAC = 0x06,
    TUYATEST_W_SLAVE_MAC = 0x07,
    TUYATEST_R_SLAVE_MAC = 0x08,
    TUYATEST_SELF_TEST = 0x09,
    TUYATEST_BUTTON_TEST = 0x0a,
    TUYATEST_LED_TEST = 0x0b,
    TUYATEST_W_BSN = 0x0c,
    TUYATEST_R_BSN = 0x0d,
    TUYATEST_W_SN = 0x0e,
    TUYATEST_R_SN = 0x0f,
    TUYATEST_MASTER_CAP = 0x10,
    TUYATEST_SLAVE_CAP = 0x11,
    TUYATEST_IPERF_TEST = 0x12,
    TUYATEST_VIDEO_TEST = 0x13,
    TUYATEST_IRCUT_TEST = 0x14,
    TUYATEST_SPEAKER_TEST = 0x15,
    TUYATEST_MIC_TEST = 0x16,
    TUYATEST_IRLED_TEST = 0x17,
    TUYATEST_BLACK_TEST = 0X1A,
    TUYATEST_W_COUNTRY_CODE = 0X1C, /*country_code*/
    TUYATEST_R_COUNTRY_CODE = 0X1D,
    TUYATEST_MOTOR_TEST = 0X1B, /*motor test*/
    TUYATEST_PIR_TEST = 0X25,   /*pir test*/
    TUYATEST_SD_TEST = 0X27,
    TUYATEST_DIGITAL_SENSOR = 0x28,
    TUYATEST_WIFI_FLAG = 0X2A,
    TUYATEST_GET_BATTERY_VALUE = 0X2C,
    TUYATEST_W_BATTERY_INFO = 0X38,
    TUYATEST_R_BATTERY_INFO = 0X39,
    TUYATEST_SET_WIFI_CFG = 0X3A,    // set ssid  passwd
    TUYATEST_BOOL_GENERAL = 0X47,
} TUYATEST_CMD;

/**************************************************test frame*****************************************************/
//产测帧结构
typedef struct tuya_test_frame
{
    unsigned char head[2];
    unsigned char version[1];
    unsigned char command[1];
    unsigned char data_len[2];
    unsigned char *data;
    unsigned char check_sum[1];
    unsigned int data_len_uint;     //产测帧数据部分长度
    unsigned char *frame_buffer;    //指向的buffer含有帧的所有数据，最后要释放掉
} TuyaTestFrame;

typedef TuyaTestFrame *(*TUYA_GET_FRAME)(void);
typedef void (*TUYA_PUT_FRAME)(unsigned char cmd, unsigned char *data);
typedef void (*TUYA_FREE_FRAME)(TuyaTestFrame *frame);

typedef struct tuya_fac_inf_
{
    int start;
    void *pstream_hdl;
} tuya_fac_inf_s;

#endif
