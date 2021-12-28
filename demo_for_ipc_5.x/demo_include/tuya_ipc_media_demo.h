/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_media_demo.h
**********************************************************************************/


#ifndef _TUYA_IPC_MEDIA_H
#define _TUYA_IPC_MEDIA_H

#include "tuya_ring_buffer.h"
#include "tuya_cloud_types.h"


#ifdef __cplusplus
extern "C" {
#endif


VOID TUYA_APP_Enable_Speaker_CB(BOOL_T enabled);

VOID TUYA_APP_Rev_Audio_CB(IN CONST MEDIA_FRAME_S *p_audio_frame,
                           TUYA_AUDIO_SAMPLE_E audio_sample,
                           TUYA_AUDIO_DATABITS_E audio_databits,
                           TUYA_AUDIO_CHANNEL_E audio_channel);

int IPC_APP_get_snapshot(char *snap_addr, int *snap_size);

VOID IPC_APP_Init_Media_Task();

/* Set audio and video properties */
VOID IPC_APP_Set_Media_Info(IPC_MEDIA_INFO_S *media);
/* Get audio and video properties */
IPC_MEDIA_INFO_S *IPC_APP_Get_Media_Info();

/* TUYA_APP_Init_Ring_Buffer */
OPERATE_RET TUYA_APP_Init_Ring_Buffer(VOID);

/* Send the raw data of audio and video  to the SDK */
OPERATE_RET TUYA_APP_Put_Frame(Ring_Buffer_User_Handle_S handle,IN CONST MEDIA_FRAME_S *p_frame);
/* Get the raw data of audio and video */
//OPERATE_RET TUYA_APP_Get_Frame(IN CONST CHANNEL_E channel, IN CONST USER_INDEX_E user_index, IN CONST BOOL_T isRetry, IN CONST BOOL_T ifBlock, INOUT MEDIA_FRAME_S *p_frame);




#ifdef __cplusplus
}
#endif

#endif  /* _TUYA_IPC_MEDIA_UTILS_H */
