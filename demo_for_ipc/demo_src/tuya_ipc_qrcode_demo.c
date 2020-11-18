/*********************************************************************************
  *Copyright(C),2015-2020, TUYA www.tuya.comm
  *FileName:    tuya_ipc_qrcode_demo
**********************************************************************************/
#include "tuya_ipc_api.h"
#include "tuya_ipc_qrcode_proc.h"

#if defined(WIFI_GW) && (WIFI_GW==1)

STATIC CHAR_T* __tuya_linux_get_snap_qrcode(VOID)
{
    //Developers need to parse QR code information from cameras
    //a typical string is {"s":"ssidxxxx","p":"password","t":"token frome tuya cloud"}
    //using 'Tuya_Ipc_QRCode_Enhance' when hard to parse the results

    // Developers get qrcode_yuv from user-defined stream channel
    unsigned char * qrcode_yuv = NULL;
    int in_w=640;
    int in_h=360;

    unsigned char *enhance_qrcode_yuv=NULL;
    int out_w=0;
    int out_h=0;

    //Support for debugging
    int binary_thres=128;
    int scale_flag=0;

    Tuya_Ipc_QRCode_Enhance(qrcode_yuv, in_w, in_h, &enhance_qrcode_yuv, &out_w, &out_h,binary_thres, scale_flag);

    //use enhance_qrcode_yuv to parse
    //....

    //free
    if (enhance_qrcode_yuv!=NULL)
    {
        free(enhance_qrcode_yuv);
        enhance_qrcode_yuv=NULL;
    }
    return "1234";
}

int s_enable_qrcode = 1;
void *thread_qrcode(void *arg)
{
    printf("Qrcode Thread start\r\n");
    while(s_enable_qrcode)
    {
        usleep(1000*1000);
        char *pStr = __tuya_linux_get_snap_qrcode();
        if(pStr)
        {
            printf("get string from qrcode %s\r\n",pStr);
            OPERATE_RET ret = tuya_ipc_direct_connect(pStr, TUYA_IPC_DIRECT_CONNECT_QRCODE);
            if(ret == OPRT_OK)
            {
                printf("register to tuya cloud via qrcode success\r\n");
                break;
            }
        }
    }

    printf("Qrcode Proc Finish\r\n");
    return (void *)0;
}
#endif

