/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_upgrade_demo
**********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statfs.h>  
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_api.h"
#include "tuya_iot_com_api.h"
#ifdef __cplusplus
extern "C" {
#endif
/* OTA */
//Callback after downloading OTA files
VOID __IPC_APP_upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{

    PR_DEBUG("Upgrade Finish");
    PR_DEBUG("download_result:%d fw_url:%s", download_result, fw->fw_url);

    if(download_result == 0)
    {
        /* The developer needs to implement the operation of OTA upgrade, 
        when the OTA file has been downloaded successfully to the specified path. [ p_mgr_info->upgrade_file_path ]*/
    }
    //TODO
    //reboot system
}

//To collect OTA files in fragments and write them to local files
OPERATE_RET __IPC_APP_get_file_data_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len,IN CONST UINT_T offset,
                             IN CONST BYTE_T *data,IN CONST UINT_T len,OUT UINT_T *remain_len, IN PVOID_T pri_data)
{
    PR_DEBUG("Rev File Data");
    PR_DEBUG("total_len:%d  fw_url:%s", total_len, fw->fw_url);
    PR_DEBUG("Offset:%d Len:%d", offset, len);

    *remain_len = 0;

    return OPRT_OK;
}

VOID ty_gw_dev_upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{
    PCHAR_T pdevId = (PCHAR_T)pri_data;
    
    PR_DEBUG("upgrade finish, devid = [%s], progress: %d, fw_url: %s tp:%d\r\n", 
            pdevId, download_result, fw->fw_url, fw->tp);


    switch (download_result){
    case 0:
    {
        //ty_dev_upgrade_proc();
        break;
    }
    case OPRT_HTTP_OS_ERROR:
    case OPRT_HTTP_PR_REQ_ERROR:
    case OPRT_HTTP_SD_REQ_ERROR:
    case OPRT_HTTP_GET_RESP_ERROR:
    case OPRT_HTTPS_NO_SUPPORT_RANGE:
        PR_DEBUG("upgrade https error reboot system\n");
        break;

    default:
        //收到升级失败指令，清空升级状态
        PR_DEBUG("upgrade progress error\n");
        break;
    }

    if (pdevId != NULL) {
        free(pdevId);
    }

    return;
}

OPERATE_RET ty_gw_dev_get_file_data_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len, IN CONST UINT_T offset,
                                      IN CONST BYTE_T *data, IN CONST UINT_T len, OUT UINT_T *remain_len, IN PVOID_T pri_data)
{
//    printf("total_len:%d  fw_url: %s sw_ver:%s, Offset:%d Len:%d\n",
//            total_len, fw->fw_url, fw->sw_ver, offset, len);
    
    if (NULL == data || len <= 0 || pri_data == NULL) {
        PR_DEBUG("data error\n");
        return OPRT_COM_ERROR;
    }

    PCHAR_T pdevId = (PCHAR_T)pri_data;

    //proc ota data
    *remain_len = 0;

    return OPRT_OK;
}

INT_T ty_gw_dev_ug_inform(IN CONST CHAR_T *pdevId, IN CONST FW_UG_S* pfw)
{
    if(pdevId == NULL || pfw == NULL) {
        return -1;
    }

    PR_DEBUG("dev_id: %s\n", pdevId);
    char* pId = malloc(strlen(pdevId) + 1);
    if (NULL == pId){
        PR_DEBUG("malloc failed\n");
        return -1;
    }
    memset(pId, 0x0, strlen(pdevId) + 1);
    memcpy(pId, pdevId, strlen(pdevId));
    tuya_iot_upgrade_dev(pdevId, pfw, ty_gw_dev_get_file_data_cb,
                        ty_gw_dev_upgrade_notify_cb, pId); // cb free pId

    return OPRT_OK;
}

INT_T IPC_APP_Upgrade_Inform_cb(IN CONST FW_UG_S *fw)
{
    PR_DEBUG("Rev Upgrade Info");
    PR_DEBUG("fw->fw_url:%s", fw->fw_url);
    PR_DEBUG("fw->sw_ver:%s", fw->sw_ver);
    PR_DEBUG("fw->file_size:%u", fw->file_size);

    OPERATE_RET op_ret = tuya_iot_upgrade_gw(fw, __IPC_APP_get_file_data_cb, __IPC_APP_upgrade_notify_cb, NULL);
    if(OPRT_OK != op_ret) {
        PR_ERR("tuya_iot_upgrade_gw err: %d\n", op_ret);
    }

    return OPRT_OK;
}
#ifdef __cplusplus
}
#endif
