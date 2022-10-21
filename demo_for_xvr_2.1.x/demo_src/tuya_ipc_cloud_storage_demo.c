/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm

  *FileName:    tuya_ipc_cloud_storage_demo
**********************************************************************************/
#include <stdio.h>
#include "tuya_ipc_cloud_storage.h"
#include "tuya_xvr_dev.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_xvr_cloud_storage.h"
#include "tuya_ipc_media_demo.h"
#ifdef DEMO_NVR_ONVIF_ENABLE
#include "tuya_ipc_onvif_client_demo.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
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

OPERATE_RET AES_CBC_init(IN CONST INT_T chn, VOID ** handle)
{
    //Initialization operations required for encryption
    //init hwl encrypt  *handle = hwl_encrypt_init();
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
OPERATE_RET AES_CBC_encrypt(IN CONST INT_T chn, VOID *handle,IN BYTE_T *pdata_in,  IN UINT_T data_len,
                                 INOUT BYTE_T *pdata_out,  OUT UINT_T *pdata_out_len,
                                 IN BYTE_T *pkey, IN BYTE_T *piv)
{
    //Note that you cannot change the raw data in pdata_in
    //If the encrypted interface changes the original data, please copy pdata_in to other buffers first.
    return OpensslAES_CBC128_encrypt(pdata_in,data_len,pdata_out,pdata_out_len,pkey,piv);
}

OPERATE_RET AES_CBC_destory(IN CONST INT_T chn, VOID *handle)
{
    //Need to add the anti-initialization operation required at the end of the program.
    //hwl_encrypt_destory(handle);
    //If not, return directly
    return OPRT_OK;
}

VOID __cloud_stor_get_media(IN CONST INT_T chn,IN OUT IPC_MEDIA_INFO_S *param)
{
    //user insert media info for chn
    //insure input as same as ringbuffer init
    //cloud storage has 2 seconds pre record,please insure GOP set is ok
    CHAR_T devId[64] = {0};
    tuya_xvr_dev_devId_get_by_chan(chn, devId, 64);
#ifdef DEMO_NVR_ONVIF_ENABLE
    TUYA_APP_Onvif_Get_Media_Info(devId, param);
#else
    IPC_APP_Get_Media_Info(chn,devId,param);
#endif
    return;
}
INT_T _sub_cloud_stor_event_Cb(XVR_ClOUD_STORAGE_EVENT_E type, VOID_T * args)
{
    printf("[%s-%d] recv type[%d]\n",__FUNCTION__,__LINE__,type);

    switch(type){
        case ORDER_OPEN:
        {
            XVR_CLOUD_STORAGE_ORDER_T * pOrder = (XVR_CLOUD_STORAGE_ORDER_T *)args;
            printf("[%s-%d] recv chn-id[%d-%s] order open\n",__FUNCTION__,__LINE__,pOrder->chn,pOrder->id);
            //user get order info
        }
        break;
        case ORDER_CLOSE:
        {
            XVR_CLOUD_STORAGE_ORDER_T * pOrder = (XVR_CLOUD_STORAGE_ORDER_T *)args;
            printf("[%s-%d] recv chn-id[%d-%s] order close\n",__FUNCTION__,__LINE__,pOrder->chn,pOrder->id);
        }
        break;
        default:
            printf("[%s-%d] not support [%d]\n",__FUNCTION__,__LINE__,type);
        break;
    }

    return 0;
}

//user need call add/delete api ,when event start/stop
//EVENT_ID tuya_xvr_cloud_storage_event_add(CHAR_T * devId,CHAR_T *snapshot_buffer, UINT_T snapshot_size, ClOUD_STORAGE_EVENT_TYPE_E type, UINT_T max_duration);
//OPERATE_RET tuya_xvr_cloud_storage_event_add(CHAR_T * devId,EVENT_ID event_id);
OPERATE_RET TUYA_APP_Enable_SubDev_CloudStorage(VOID)
{
#if defined(DEMO_USE_AS_NVR) && (DEMO_USE_AS_NVR == 1)
    //for NVR
    AES_HW_CBC_FUNC aes_func;
    memset(&aes_func, 0, sizeof(AES_HW_CBC_FUNC));

    //step:初始化XVR 云存
    TUYA_XVR_CLOUD_STORAGE_INIT_T xvr_cloud_storage_init={0};
    xvr_cloud_storage_init.aes_func=&aes_func;
    xvr_cloud_storage_init.media_cb=__cloud_stor_get_media;//此回调用于云存媒体参数的获取
    xvr_cloud_storage_init.stream_clarity=E_STREAM_VIDEO_MAIN_CLARITY;//用于设置云存是主码流还是附码流。默认主码流。子码流可以更少的内存消耗。

    tuya_xvr_cloud_storage_init(&xvr_cloud_storage_init);
    tuya_xvr_cloud_order_cb_regist(_sub_cloud_stor_event_Cb);

    int i = 0;
    for (i = 0; i < SUB_DEV_MAX_NUM ; i++){
      //step：启动对应子设备云存功能
        int  ret =  tuya_xvr_cloud_storage_start_by_chn(i);
      printf("start sub cloud storage ret = %d,sub=%d\n",ret,i);
    }

    return OPRT_OK;

#else
    //for Base station
    AES_HW_CBC_FUNC aes_func;
    memset(&aes_func, 0, sizeof(AES_HW_CBC_FUNC)); //no aes fun is also supported

    TUYA_XVR_CLOUD_STORAGE_INIT_T xvr_cloud_storage_init={0};
    xvr_cloud_storage_init.aes_func=&aes_func;
    xvr_cloud_storage_init.media_cb=__cloud_stor_get_media;

    tuya_xvr_cloud_storage_init(&xvr_cloud_storage_init);

    return OPRT_OK;
#endif

}
#ifdef __cplusplus
}
#endif
