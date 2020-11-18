/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm

  *FileName:    tuya_ipc_cloud_storage_demo
**********************************************************************************/
#include <stdio.h>
#include "tuya_ipc_cloud_storage.h"

/*
---------------------------------------------------------------------------------
The cloud storage acquires the status of the storage order and upload of data has been impleted in SDK. 
Developers do not need to implement logic.

Whether the user purchases a contiguous storage or event storage mode, the developer needs to implement 
the following two interface calls.


tuya_ipc_cloud_storage_event_start: Called when an event is triggered, such as when a move is detected
tuya_ipc_cloud_storage_event_stop：Called at the end of the event, for example, when it is seen that the screen has not changed for more than 4 seconds;

call mode refer to tuya_ipc_motion_detect_demo.c
---------------------------------------------------------------------------------
*/

extern OPERATE_RET OpensslAES_CBC128_encrypt(IN CONST BYTE_T *pdata_in,  IN CONST UINT_T data_len,
                                OUT BYTE_T *pdata_out,  OUT UINT_T *pdata_out_len,
                                IN CONST BYTE_T *pkey, IN BYTE_T *piv);

/* Cloud storage must encrypt data to meet security requirements.
At present, TUYA adopts AES and CBC mode.This interface is encrypted by software, 
which can not meet the real-time requirement on some devices.
Suggestions for Replacing Hardware Encryption，the filling method needs to be PKCS5/PKCS7。
*/

OPERATE_RET AES_CBC_init(VOID)
{
    //Initialization operations required for encryption
    //If not, return directly
    return OPRT_OK;
}

/* This temporary interface uses CPU to encrypt，failure to meet delay requirements in IPC SOC */
extern OPERATE_RET OpensslAES_CBC128_encrypt(IN CONST BYTE_T *pdata_in,  IN CONST UINT_T data_len,
                                OUT BYTE_T *pdata_out,  OUT UINT_T *pdata_out_len,
                                IN CONST BYTE_T *pkey, IN BYTE_T *piv);

/* 
pdata_in： Encrypted source data, cannot be changed, otherwise it will affect other functions
data_len： Encrypted source data size
pdata_out：The address where the data is stored after encryption, has applied for memory, no need to free
pdata_out_len： Encrypted data size
pkey： Encryption key，16 bytes， internal management in the SDK, binding to accounts/devices
piv： Encryption vector，16 bytes
*/
OPERATE_RET AES_CBC_encrypt(IN BYTE_T *pdata_in,  IN UINT_T data_len,
                                 INOUT BYTE_T *pdata_out,  OUT UINT_T *pdata_out_len,
                                 IN BYTE_T *pkey, IN BYTE_T *piv)
{
    //Note that you cannot change the raw data in pdata_in
    //If the encrypted interface changes the original data, please copy pdata_in to other buffers first.
    return OpensslAES_CBC128_encrypt(pdata_in,data_len,pdata_out,pdata_out_len,pkey,piv);
}

OPERATE_RET AES_CBC_destory(VOID)
{
    //Need to add the anti-initialization operation required at the end of the program.
    //If not, return directly
    return OPRT_OK;
}

extern IPC_MEDIA_INFO_S s_media_info;
OPERATE_RET TUYA_APP_Enable_CloudStorage(VOID)
{
    AES_HW_CBC_FUNC aes_func;
    aes_func.init = AES_CBC_init;
    aes_func.encrypt = AES_CBC_encrypt;
    aes_func.destory = AES_CBC_destory;
    tuya_ipc_cloud_storage_init(&s_media_info, &aes_func);
    return OPRT_OK;
}


