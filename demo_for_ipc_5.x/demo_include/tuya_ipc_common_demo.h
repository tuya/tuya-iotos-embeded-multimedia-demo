#ifndef __TUYA_COMMON_DEMO_H_
#define __TUYA_COMMON_DEMO_H_
#include <stdio.h>
#include "tuya_cloud_types.h"
#include "tuya_ipc_api.h"


#define PR_ERR(fmt, ...)    printf("Err:"fmt"\r\n", ##__VA_ARGS__)
#define PR_DEBUG(fmt, ...)  printf("Dbg:"fmt"\r\n", ##__VA_ARGS__)
#define PR_TRACE

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

#endif
