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

#define PR_ERR(fmt, ...)    printf("Err:"fmt"\r\n", ##__VA_ARGS__)
#define PR_DEBUG(fmt, ...)  printf("Dbg:"fmt"\r\n", ##__VA_ARGS__)
//#define PR_TRACE(fmt, ...)  printf("Trace:"fmt"\r\n", ##__VA_ARGS__)
#define PR_TRACE(fmt, ...)


VOID TUYA_APP_Enable_Speaker_CB(BOOL_T enabled);

VOID TUYA_APP_Rev_Audio_CB(IN CONST MEDIA_FRAME_S *p_audio_frame,
                           TUYA_AUDIO_SAMPLE_E audio_sample,
                           TUYA_AUDIO_DATABITS_E audio_databits,
                           TUYA_AUDIO_CHANNEL_E audio_channel);

VOID *thread_live_video(VOID *arg);
VOID *thread_live_audio(VOID *arg);


/* Set audio and video properties */
VOID IPC_APP_Set_Media_Info(VOID);
/* TUYA_APP_Init_Ring_Buffer */
OPERATE_RET TUYA_APP_Init_Ring_Buffer(VOID);
/* TUYA_APP_Init_Stream_Storage */
OPERATE_RET TUYA_APP_Init_Stream_Storage(IN CONST CHAR_T *p_sd_base_path);
/* Send the raw data of audio and video  to the SDK */
OPERATE_RET TUYA_APP_Put_Frame(IN CONST CHANNEL_E channel, IN CONST MEDIA_FRAME_S *p_frame);
/* Get the raw data of audio and video */
OPERATE_RET TUYA_APP_Get_Frame(IN CONST CHANNEL_E channel, IN CONST USER_INDEX_E user_index, IN CONST BOOL_T isRetry, IN CONST BOOL_T ifBlock, INOUT MEDIA_FRAME_S *p_frame);
/* TUYA_APP_Enable_P2PTransfer */
OPERATE_RET TUYA_APP_Enable_P2PTransfer(IN UINT_T max_users);
/* TUYA_APP_Enable_EchoShow_Chromecast */
OPERATE_RET TUYA_APP_Enable_EchoShow_Chromecast(VOID);

/* In the event mode recording, notify the SDK to record when an event occurs.*/
OPERATE_RET TUYA_APP_Trigger_Event_Storage(VOID);

/* TUYA_APP_Enable_CloudStorage */
OPERATE_RET TUYA_APP_Enable_CloudStorage(VOID);

#ifdef __cplusplus
}
#endif

#endif  /* _TUYA_IPC_MEDIA_UTILS_H */
