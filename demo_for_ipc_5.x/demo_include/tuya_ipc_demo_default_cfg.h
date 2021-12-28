/*
 * tuya_ipc_demo_default_cfg.h
 *Copyright(C),2017-2022, TUYA company www.tuya.comm
 *
 *FILE description: 
  *
 *  Created on: 2021年12月23日
 *      Author: dante
 */

#ifndef __TUYA_IPC_DEMO_DEFAULT_CFG_H__
#define __TUYA_IPC_DEMO_DEFAULT_CFG_H__


#ifdef __cplusplus
extern "C" {
#endif

#define IPC_APP_STORAGE_PATH    "/tmp/"   //Path to save tuya sdk DB files, should be readable, writeable and storable
#define IPC_APP_UPGRADE_FILE    "/tmp/upgrade.file" //File with path to download file during OTA
#define IPC_APP_SD_BASE_PATH    "/tmp/"      //SD card mount directory
#define IPC_APP_VERSION         "1.2.3"     //Firmware version displayed on TUYA APP

#define IPC_APP_PID             "tuya_pid"      //Product ID of TUYA device, this is for demo only.
#define IPC_APP_UUID            "tuya_uuid"     //Unique identification of each device//Contact tuya PM/BD for developing devices or BUY more
#define IPC_APP_AUTHKEY         "tuya_authkey"  //Authentication codes corresponding to UUID, one machine one code, paired with UUID.


#ifdef __cplusplus
}
#endif
#endif /* __TUYA_IPC_DEMO_DEFAULT_CFG_H__ */

