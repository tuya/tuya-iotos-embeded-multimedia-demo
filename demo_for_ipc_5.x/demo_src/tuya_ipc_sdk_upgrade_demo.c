/*
 * tuya_ipc_sdk_upgrade_demo.c
 *
 *  Created on: 2020年12月28日
 *      Author: kuiba
 */

#include<stdbool.h>
#include "tuya_ipc_api.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_upgrade_demo.h"

extern IPC_MGR_INFO_S s_mgr_info;
/* OTA */
//Callback after downloading OTA files
VOID __IPC_APP_upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{
    FILE *p_upgrade_fd = (FILE *)pri_data;
    fclose(p_upgrade_fd);

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

    //report UPGRADE process, NOT only download percent, consider flash-write time
    //APP will report overtime fail, if uprgade process is not updated within 60 seconds

    int download_percent = (offset * 100) / (total_len+1);
    int report_percent = download_percent/2; // as an example, download 100% = 50%  upgrade work finished
    tuya_ipc_upgrade_progress_report(report_percent);

    if(offset == total_len) // finished downloading
    {
        //start write OTA file to flash by parts
        /* only for example:
        FILE *p_upgrade_fd = (FILE *)pri_data;
        fwrite(data, 1, len, p_upgrade_fd);
        *remain_len = 0;
        */
        // finish 1st part
        report_percent+=10;
        tuya_ipc_upgrade_progress_report(report_percent);
        // finish 2nd part
        sleep(5);
        report_percent+=10;
        tuya_ipc_upgrade_progress_report(report_percent);
        // finish all parts, set to 90% for example
        report_percent = 90;
        tuya_ipc_upgrade_progress_report(report_percent);
    }

    //APP will report "uprage success" after reboot and new FW version is reported inside SDK automaticlly

    return OPRT_OK;
}


VOID IPC_APP_Upgrade_Inform_cb(IN CONST FW_UG_S *fw)
{
    PR_DEBUG("Rev Upgrade Info");
    PR_DEBUG("fw->fw_url:%s", fw->fw_url);
    //PR_DEBUG("fw->fw_md5:%s", fw->fw_md5);
    PR_DEBUG("fw->sw_ver:%s", fw->sw_ver);
    PR_DEBUG("fw->file_size:%u", fw->file_size);

    FILE *p_upgrade_fd = fopen(s_mgr_info.upgrade_file_path, "w+b");
    tuya_ipc_upgrade_sdk(fw, __IPC_APP_get_file_data_cb, __IPC_APP_upgrade_notify_cb, p_upgrade_fd);
}

#if 0
/**
 * @brief start upgrade notify
 *
 * @param[in] file_size upgraded file size
 *
 * @retval  =0      success
 * @retval  <0      other errror
 */

int IPC_APP_Upgrade_Start_CB(unsigned int  file_size)
{
    printf("Rev Upgrade Info,upgrade is %u\n",file_size);
    //OTA
    if(file_size == 0)
    {
    	return -1;
    }
    //TODO:developer process.eg:maloc space to save upgrade file;

	return 0;
}

/**
 * @brief ota data process
 *
 * @param[in] total_len: ota upgrade file total size
 * @param[in] offset: current data  offset in ota upgrade file
 * @param[in] data:  current ota data buffer address
 * @param[in] len: current ota buffer len;
 * @param[out] remain_len: data len that not process
 * @param[in] pri_data: reserved
 *
 * @retval  =0      sucess
 * @retval  <0      other error
 */

int IPC_APP_Upgrade_Data_Process_CB(const unsigned int total_len, const unsigned int offset,
                              const unsigned char* data, const unsigned int len, unsigned int* remain_len, void* pri_data)
{
	//TODO:developer save ota data to space which malloc in IPC_APP_Upgrade_Start_CB
    return 0;
}

/**
 * @brief upgrade data receiv over
 *
 * param[in]  reset: reset flags.
 * @retval  =0      success
 * @retval  <0      other error
 */
int IPC_APP_Upgrade_End_CB(bool reset)
{
    // verify
	 printf("end Upgrade Info,upgrade is %d\n",reset);
    //TODO:developer determine reset device

    return 0;
}
#endif
