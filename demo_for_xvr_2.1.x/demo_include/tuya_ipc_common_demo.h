/*
 *
 *Copyright(C),2017-2023, TUYA company www.tuya.com
 *
 *FILE description:
 *用于配置NVR DEMO 配置基本信息：产品pid,UUID,AUTHKEY，这些是设备运行的必备信息。
 *
 *  Created on: 2021年12月16日
 *      Author: kuiba
 */

#ifndef __TUYA_COMMON_DEMO_H_
#define __TUYA_COMMON_DEMO_H_
#include <stdio.h>
#include "tuya_cloud_types.h"
#include "tuya_ipc_api.h"
#include "uni_log.h"
#ifdef __cplusplus
extern "C" {
#endif

//TUYA XVR SDK
#define DEMO_USE_AS_NVR 1
#define DEMO_NVR_SUB_DEV_NUM  4 // according to UUID, must smaller than SUB_DEV_MAX_NUM(16)。根据授权的UUID是几路填写数值

//DEMO_USE_AS_NVR宏用于控制XVR SDK 是NVR模式还是基站模式。
#if defined(DEMO_USE_AS_NVR) && (DEMO_USE_AS_NVR == 1)
#define IPC_APP_PID "tuyapid" //"potalyu2wh72hmjq"//ci sdk可用 //"cvi5r86fgxpyaqc2"// ci不可用 客户 //
#define IPC_APP_UUID  "tuyauuid" //"zy0096494185ec6f4ef1" //UUID is the unique identification of each device
#define IPC_APP_AUTHKEY  "tuyaauthkey" //"kgJ9PnQnvEUr7jw0dc09XqSjZxPhealb"  //AUTHKEY is the authentication codes corresponding to UUID, one machine one code, paired with UUID.
#define IPC_APP_VERSION "1.0.0" //Firmware version information displayed on TUYA APP

#define IPC_APP_SUB_DEV_PID "tuyanvrsubdevpid"
#define IPC_APP_SUB_DEV_VERSION "1.0.0" //Firmware version information displayed on TUYA sub dev

#else
/*base station config*/
#define IPC_APP_PID "tuyapid" //PID is Product ID of TUYA device。
#define IPC_APP_UUID "tuyauuid"//"tuyaeafd1d78aec8f2f6" //UUID is the unique identification of each device
#define IPC_APP_AUTHKEY "tuyaauthkey"//"jPTmBQzISUc0xZhLYY3ICiiE1yXBArhV" //AUTHKEY is the authentication codes corresponding to UUID, one machine one code, paired with UUID.
#define IPC_APP_VERSION "1.0.0" //Firmware version information displayed on TUYA APP
#endif

//用于ONVIF功能开发控制宏
#ifdef BETA_DEBUG_ONVIF
#define DEMO_NVR_ONVIF_ENABLE 1
#define DEMO_ONVIF_DEVICE_USERNAME  "admin"
#define DEMO_ONVIF_DEVICE_PASSWD    "Admin123"
#endif

//用于动态更新功能开启
#define BETA_DYNAMIC_UPDATE_RING_BUFFER 1
// Port name
#define NET_DEV "ens38"
/*
 * __USER_DO_NOT_OPEN__宏括住的范围只是用于demo测试一些本地存储回放功能，对用户而言是不可用的。用户不可以打开此宏，否则会引起各种混乱错误
 * 但里面的回放逻辑可以稍带参考,也可以按照用户自己的逻辑实现。
 */
#define __USER_DO_NOT_OPEN__ 0

typedef struct
{
    CHAR_T storage_path[IPC_STORAGE_PATH_LEN + 1];/**Path to save sdk cfg ,need to read and write, doesn't loss when poweroff */
    CHAR_T upgrade_file_path[IPC_STORAGE_PATH_LEN + 1];/*Path to save upgrade file when OTA upgrading*/
    CHAR_T sd_base_path[IPC_STORAGE_PATH_LEN + 1];/**SD Card Mount Directory */
    CHAR_T product_key[IPC_PRODUCT_KEY_LEN + 1]; /**< product key */
    CHAR_T uuid[IPC_UUID_LEN + 1]; /*UUID is the unique identification of each device */
    CHAR_T auth_key[IPC_AUTH_KEY_LEN + 1]; /*AUTHKEY is the authentication codes corresponding to UUID, one machine one code, paired with UUID.*/
    CHAR_T p2p_id[IPC_P2P_ID_LEN + 1]; /*p2p_id is no need to provide*/
    CHAR_T dev_sw_version[IPC_SW_VER_LEN + 1]; /*version of the software */
    UINT_T max_p2p_user;/*max num of P2P supports*/
}IPC_MGR_INFO_S;


void exec_cmd(char *pCmd);

#define TYDEBUG(fmt, ...)              printf("%s:%d ty:"fmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define TYINFO(fmt, ...)               printf("%s:%d ty:"fmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define TYNOTICE(fmt, ...)             printf("%s:%d ty:"fmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define TYWARN(fmt, ...)               printf("%s:%d ty:"fmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define TYERROR(fmt, ...)              printf("%s:%d ty:"fmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define TYCRIT(fmt, ...)               printf("%s:%d ty:"fmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define TYALERT(fmt, ...)              printf("%s:%d ty:"fmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define TYEMERG(fmt, ...)              printf("%s:%d ty:"fmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif

