/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_media_demo
**********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statfs.h>  
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_api.h"
#include "tuya_ring_buffer.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_ipc_media.h"
STATIC IPC_MEDIA_INFO_S s_media_info = {0};
extern CHAR_T s_raw_path[128];

/* Set audio and video properties */
VOID IPC_APP_Set_Media_Info(IPC_MEDIA_INFO_S *media)
{
    memcpy(&s_media_info, media, sizeof(IPC_MEDIA_INFO_S));

    PR_DEBUG("channel_enable:%d %d %d", s_media_info.channel_enable[0], s_media_info.channel_enable[1], s_media_info.channel_enable[2]);

    PR_DEBUG("fps:%u", s_media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("gop:%u", s_media_info.video_gop[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("bitrate:%u kbps", s_media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_main_width:%u", s_media_info.video_width[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_main_height:%u", s_media_info.video_height[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_freq:%u", s_media_info.video_freq[E_IPC_STREAM_VIDEO_MAIN]);
    PR_DEBUG("video_codec:%d", s_media_info.video_codec[E_IPC_STREAM_VIDEO_MAIN]);

    PR_DEBUG("audio_codec:%d", s_media_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("audio_sample:%d", s_media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("audio_databits:%d", s_media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN]);
    PR_DEBUG("audio_channel:%d", s_media_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN]);
}

IPC_MEDIA_INFO_S *IPC_APP_Get_Media_Info()
{
    return &s_media_info;
}
/*
 * The sample code simulates audio and video by reading and writing files in rawfiles.tar.gz
 */
#define AUDIO_FRAME_SIZE 640
#define AUDIO_FPS 25
#define VIDEO_BUF_SIZE	(1024 * 400) //Maximum frame

/* This is for demo only. Should be replace with real PCM/AAC/G711 encoder output */
void *thread_live_audio(void *arg)
{
    char fullpath[128] = {0};
    sprintf(fullpath, "%s/resource/media/demo_audio.raw", s_raw_path);

	Ring_Buffer_User_Handle_S a_handle = tuya_ipc_ring_buffer_open(0, 0, E_IPC_STREAM_AUDIO_MAIN, E_RBUF_WRITE);
	if(a_handle == NULL)
	{
		pthread_exit(0);
	}

    FILE *aFp = fopen(fullpath, "rb");
    if(aFp == NULL)
    {
        printf("can't read live audio file %s\n",fullpath);
        pthread_exit(0);
    }
    char audioBuf[AUDIO_FRAME_SIZE];
    MEDIA_FRAME_S pcm_frame = {0};
    pcm_frame.type = E_AUDIO_FRAME;

    while(1)
    {
        int size = fread(audioBuf, 1, AUDIO_FRAME_SIZE, aFp);
        if(size < AUDIO_FRAME_SIZE)
        {
            rewind(aFp);
            continue;
        }
        int frameRate = AUDIO_FPS;
        int sleepTick = 1000000/frameRate;
        static UINT64_T pts = 0;
        pts += sleepTick;
        pcm_frame.size = size;
        pcm_frame.p_buf = audioBuf;
        pcm_frame.pts = pts;
//        TUYA_APP_Put_Frame(E_IPC_STREAM_AUDIO_MAIN,&pcm_frame);
        TUYA_APP_Put_Frame(a_handle,&pcm_frame);


        usleep(sleepTick);
    }
    tuya_ipc_ring_buffer_close(a_handle);   
    pthread_exit(0);
}

/* This is for demo only. Should be replace with real H264 encoder output */
int read_one_frame_from_demo_video_file(unsigned char *pVideoBuf,unsigned int offset,unsigned int BufSize,unsigned int *IskeyFrame,unsigned int 
*FramLen,unsigned int *Frame_start)
{
    int pos = 0;
    int bNeedCal = 0;
    unsigned char NalType=0;
    int idx=0;
    if(BufSize<=5)
    {
        printf("bufSize is too small\n");
        return -1;
    }
    for(pos=0;pos <= BufSize-5;pos++)
    {
        if(pVideoBuf[pos]==0x00
            &&pVideoBuf[pos+1]==0x00
            &&pVideoBuf[pos+2]==0x00
            &&pVideoBuf[pos+3]==0x01)
        {
            NalType = pVideoBuf[pos + 4] & 0x1f;
            if(NalType == 0x7)
            {
                if(bNeedCal==1)
                {
                    *FramLen=pos-idx;
                    return 0;
                }
                
                *IskeyFrame = 1;
                *Frame_start = offset+pos;
                bNeedCal=1;
                idx=pos;
            }
            else if(NalType ==0x1)                
            {
               if(bNeedCal)
               {
                  *FramLen=pos-idx;
                  return 0;
               }
               *Frame_start=offset+pos;
               *IskeyFrame = 0;
               idx = pos;
               bNeedCal=1;
            }
        }
    }
    
    return 0;
}

void *thread_live_video(void *arg)
{
    char raw_fullpath[128] = {0};
    char info_fullpath[128] = {0};
    unsigned int FrameLen=0,Frame_start=0;
    unsigned int offset=0;
    unsigned int IsKeyFrame=0;
    unsigned char *pVideoBuf = NULL;
    sprintf(raw_fullpath, "%s/resource/media/demo_video.264", s_raw_path);

    PR_DEBUG("start live video using %s",raw_fullpath);

	Ring_Buffer_User_Handle_S v_handle = tuya_ipc_ring_buffer_open(0, 0, 0, E_RBUF_WRITE);
	if(v_handle == NULL)
	{
		pthread_exit(0);
	}

    FILE *streamBin_fp = fopen(raw_fullpath, "rb");
    if((streamBin_fp == NULL))
    {
        printf("can't read live video file %s\n",raw_fullpath);
        pthread_exit(0);
    }
    fseek(streamBin_fp, 0, SEEK_END);
    UINT_T file_size = ftell(streamBin_fp);
    fseek(streamBin_fp, 0, SEEK_SET);
    pVideoBuf = malloc(file_size);
    fread(pVideoBuf, 1, file_size, streamBin_fp);

    MEDIA_FRAME_S h264_frame = {0};
    while(1)
    {
        offset=Frame_start+FrameLen;
        if(offset>=file_size)
        {
            offset=0;
        }
        read_one_frame_from_demo_video_file(pVideoBuf+offset,offset,file_size-offset,&IsKeyFrame,&FrameLen,&Frame_start);
        //Note: For I frame of H264, SPS/PPS/SEI/IDR should be combined within one frame, and the NALU separator should NOT be deleted.
        if(IsKeyFrame==1)
        {
            h264_frame.type = E_VIDEO_I_FRAME;
            h264_frame.size = FrameLen;
        }
        else
        {
            h264_frame.type = E_VIDEO_PB_FRAME;
            h264_frame.size = FrameLen;
        }

        int frameRate = 30;
        int sleepTick = 1000000/frameRate;
        static UINT64_T pts = 0;
        pts += sleepTick;

        h264_frame.p_buf = pVideoBuf+Frame_start;
        h264_frame.pts = pts;

        /* Send HD video data to the SDK */
        TUYA_APP_Put_Frame(v_handle, &h264_frame);

        usleep(sleepTick);
    }
    tuya_ipc_ring_buffer_close(v_handle);
    pthread_exit(0);
}

//According to different chip platforms, users need to implement the interface of capture.
int IPC_APP_get_snapshot(char *snap_addr, int *snap_size)
{
    //we use file to simulate
    char snapfile[128];
    *snap_size = 0;
    extern char s_raw_path[];
    printf("get one motion snapshot\n");
    snprintf(snapfile,64,"%s/resource/media/demo_snapshot.jpg",s_raw_path);
    FILE*fp = fopen(snapfile,"r+");
    if(NULL == fp)
    {
        printf("fail to open snap.jpg\n");
        return -1;
    }
    fseek(fp,0,SEEK_END);
    *snap_size = ftell(fp);
    if(*snap_size < 100*1024)
    {
        fseek(fp,0,SEEK_SET);
        fread(snap_addr,*snap_size,1,fp);
    }
    fclose(fp);
    return 0;
}

VOID IPC_APP_Init_Media_Task()
{
#ifdef __HuaweiLite__
    TSK_INIT_PARAM_S stappTask;
    int taskid = -1;
    memset(&stappTask, 0, sizeof(TSK_INIT_PARAM_S));
    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)thread_live_video;
    stappTask.uwStackSize  = 0x80000;
    stappTask.pcName = "live_video";
    stappTask.usTaskPrio = 10;
    stappTask.uwResved   = LOS_TASK_STATUS_DETACHED;
    LOS_TaskCreate((UINT32 *)&taskid, &stappTask);

    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)thread_live_audio;
    stappTask.pcName = "live_video";
    LOS_TaskCreate((UINT32 *)&taskid, &stappTask);
#else
    pthread_t h264_output_thread;
    pthread_create(&h264_output_thread, NULL, thread_live_video, NULL);
    pthread_detach(h264_output_thread);

    pthread_t pcm_output_thread;
    pthread_create(&pcm_output_thread, NULL, thread_live_audio, NULL);
    pthread_detach(pcm_output_thread);
#endif    

    return;
}

/*
---------------------------------------------------------------------------------
code related RingBuffer
---------------------------------------------------------------------------------
*/
OPERATE_RET TUYA_APP_Init_Ring_Buffer(VOID)
{
	OPERATE_RET ret = OPRT_OK;

    STATIC BOOL_T s_ring_buffer_inited = FALSE;
    if(s_ring_buffer_inited == TRUE)
    {
        PR_DEBUG("The Ring Buffer Is Already Inited");
        return OPRT_OK;
    }

    IPC_STREAM_E ringbuffer_stream_type;
   // CHANNEL_E channel;
    Ring_Buffer_Init_Param_S param={0};
    for( ringbuffer_stream_type = E_IPC_STREAM_VIDEO_MAIN; ringbuffer_stream_type < E_IPC_STREAM_MAX; ringbuffer_stream_type++ )
    {
        PR_DEBUG("init ring buffer Channel:%d Enable:%d", ringbuffer_stream_type, s_media_info.channel_enable[ringbuffer_stream_type]);
        if(s_media_info.channel_enable[ringbuffer_stream_type] == TRUE)
        {
            if(ringbuffer_stream_type >= E_IPC_STREAM_AUDIO_MAIN)
            {
                param.bitrate = s_media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN]*s_media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN]/1024;
                param.fps = s_media_info.audio_fps[E_IPC_STREAM_AUDIO_MAIN];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                PR_DEBUG("audio_sample %d, audio_databits %d, audio_fps %d",s_media_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN],s_media_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN],s_media_info.audio_fps[E_IPC_STREAM_AUDIO_MAIN]);
                ret = tuya_ipc_ring_buffer_init(0,0,ringbuffer_stream_type,&param);
            }
            else
            {
                param.bitrate = s_media_info.video_bitrate[ringbuffer_stream_type];
                param.fps = s_media_info.video_fps[ringbuffer_stream_type];
                param.max_buffer_seconds = 0;
                param.requestKeyFrameCB = NULL;
                PR_DEBUG("video_bitrate %d, video_fps %d",s_media_info.video_bitrate[ringbuffer_stream_type],s_media_info.video_fps[ringbuffer_stream_type]);
                ret = tuya_ipc_ring_buffer_init(0,0,ringbuffer_stream_type, &param);
            }
            if(ret != 0)
            {
                PR_ERR("init ring buffer fails. %d %d", ringbuffer_stream_type, ret);
                return OPRT_MALLOC_FAILED;
            }
            PR_DEBUG("init ring buffer success. channel:%d", ringbuffer_stream_type);
        }
    }

    s_ring_buffer_inited = TRUE;

    return OPRT_OK;
}

OPERATE_RET TUYA_APP_Put_Frame(Ring_Buffer_User_Handle_S handle, IN CONST MEDIA_FRAME_S *p_frame)
{
    PR_TRACE("Put Frame. type:%d size:%u pts:%llu ts:%llu",
             p_frame->type, p_frame->size, p_frame->pts, p_frame->timestamp);

    OPERATE_RET ret = tuya_ipc_ring_buffer_append_data(handle,p_frame->p_buf, p_frame->size,p_frame->type,p_frame->pts);

    if(ret != OPRT_OK)
    {
        PR_ERR("Put Frame Fail.%d  type:%d size:%u pts:%llu ts:%llu",ret,
                  p_frame->type, p_frame->size, p_frame->pts, p_frame->timestamp);
    }
    return ret;
}



