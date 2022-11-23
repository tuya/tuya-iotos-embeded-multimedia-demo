#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tuya_fac_cmd_demo.h"
#include "tuya_fac_media.h"
#include "tuya_fac_media_demo.h"
#include "tuya_fac_protocol.h"
#include "tuya_fac_test.h"

#ifdef RTSP_ENABLE
#include "rtsp.h"
#include "rtsp_server.h"
#endif

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
extern int com_fd;

#define AUDIO_SAMPLE_RATE 8000
#define MAXLINE           1024
#define PORT_NUM          8095

uint64_t ty_time_utc_ms()
{
    struct timeval now;

    gettimeofday(&now, NULL);

    return ((uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_usec / 1000);
}

typedef struct _wave_pcm_hdr
{
    char riff[4];    // = "RIFF"
    int size_8;      // = FileSize - 8
    char wave[4];    // = "WAVE"
    char fmt[4];     // = "fmt "
    int fmt_size;    // = "fmt_size": 16

    short int format_tag;         // = PCM : 1
    short int channels;           // = channels: 1
    int samples_per_sec;          // = samples: 8000 | 16000
    int avg_bytes_per_sec;        // = byte: samples_per_sec * channels * bits_per_sample / 8
    short int block_align;        // = aliagn: Channels * bits_per_sample / 8
    short int bits_per_sample;    // = sample_bi: 8 | 16

    char data[4];     // = "data";
    int data_size;    // = "data_size" : FileSize - 44
} wave_pcm_hdr;

wave_pcm_hdr default_wav_hdr = { { 'R', 'I', 'F', 'F' }, 0, { 'W', 'A', 'V', 'E' }, { 'f', 'm', 't', ' ' }, 16, 1, 1, 16000, 32000, 2, 16, { 'd', 'a', 't', 'a' }, 0 };

static int file_transport(char *file, char *ipaddr)
{
    struct sockaddr_in serv_addr;
    char buf[MAXLINE];
    int sock_id;
    int read_len = 0;
    int send_len = -1;
    FILE *fp;
    int i_ret;

    if ((fp = fopen(file, "rb")) == NULL) {
        TYDEBUG("Open file failed\n");
        return -1;
    }

    /*<---------------------socket----------------------->*/
    if ((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        TYDEBUG("Create socket failed\n");
        goto release;
    }
    /*<---------------------connect-------------------------------->*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_NUM);
    inet_pton(AF_INET, ipaddr, &serv_addr.sin_addr);

    i_ret = connect(sock_id, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    if (-1 == i_ret) {
        TYDEBUG("Connect server:%s socket failed\n", ipaddr);
        goto release;
    }
    TYDEBUG("Connect success #\n");
    /*<---------------------------client send part-------------------->*/
    bzero(buf, MAXLINE);

    while (1) {
        read_len = fread(buf, sizeof(char), MAXLINE, fp);
        if (read_len <= 0) {
            TYDEBUG("read len is %d\n", read_len);
            break;
        }

        send_len = send(sock_id, buf, read_len, 0);
        if (send_len < 0) {
            TYDEBUG("Send file failed\n");
            break;
        }

        bzero(buf, MAXLINE);
    }

release:
    if (fp)
        fclose(fp);
    if (sock_id)
        close(sock_id);

    if (send_len < 0) {
        return -1;
    }

    TYDEBUG("Send Finish\n");

    return 0;
}

FILE *aud_save_fp = NULL, *aud_save_pcm_fp = 0;
int data_size = 0;

void *tuya_fac_media_loop()
{
    //    Ring_Buffer_Node_S *node = NULL;
    //	unsigned int vid_cnt = 0, aud_cnt=0;
    //    while(1) {
    //        node = tuya_ipc_ring_buffer_get_video_frame(E_CHANNEL_VIDEO_SUB, E_USER_STREAM_STORAGE, false);
    //        if( node == NULL) {
    //            usleep(10*1000);
    //			vid_cnt++;
    //			if(vid_cnt > 20*60*1) // 4 min
    //			{
    //				TYDEBUG("video stream error ###\n");
    //				TYDEBUG("video stream error ###\n");
    //				TYDEBUG("video stream error ###\n");
    //				exit(0);
    //			}
    //        }else
    //        	vid_cnt = 0;
    //
    //        node = tuya_ipc_ring_buffer_get_audio_frame(E_CHANNEL_AUDIO, E_USER_STREAM_STORAGE, false);
    //        if( node == NULL) {
    //            usleep(10*1000);
    //			aud_cnt++;
    //			if(aud_cnt > 20*60*1) // 4 min
    //			{
    //				TYDEBUG("audio stream error ###\n");
    //				TYDEBUG("audio stream error ###\n");
    //				TYDEBUG("audio stream error ###\n");
    //				exit(0);
    //			}
    //        }else
    //        {
    //        	aud_cnt = 0;
    //			pthread_mutex_lock(&mutex);
    //			if(aud_save_fp){
    //				fwrite(node->rawData, 1, node->size, aud_save_fp);
    //				fwrite(node->rawData, 1, node->size, aud_save_pcm_fp);
    //				data_size += node->size;
    //			}
    //			pthread_mutex_unlock(&mutex);
    //        }
    //		usleep(40);
    //    }
    return NULL;
}

void *tuya_fac_media_record(void *arg)
{
    FILE *fp = NULL, *fp_pcm = NULL;
    char file[128] = { 0 };
    char file_pcm[128] = { 0 };
    wave_pcm_hdr wav_hdr = default_wav_hdr;
    char *ipaddr = (char *)arg;
    char resp_buf[128] = { 0 };
    char path[128] = { 0 };
    if (getcwd(path, sizeof(path)) == NULL) {
        TYDEBUG("record path error, current path is not writable\n");
        snprintf(resp_buf, sizeof(resp_buf), "{\"ret\":\"false\"}");
        goto Exit;
    }
    snprintf(file, sizeof(file), "%s/default.wav", path);
    snprintf(file_pcm, sizeof(file_pcm), "%s/default.pcm", path);

    TYDEBUG("record save file %s\n", file);

    fp = fopen(file, "wb");
    if (fp == NULL) {
        TYDEBUG("[%s:%d] open file %s err: %s\n", __FUNCTION__, __LINE__, file, strerror(errno));
        fclose(fp);
        snprintf(resp_buf, sizeof(resp_buf), "{\"ret\":\"false\"}");
        goto Exit;
    }

    fp_pcm = fopen(file_pcm, "wb");
    if (fp_pcm == NULL) {
        TYDEBUG("[%s:%d] open file %s err: %s\n", __FUNCTION__, __LINE__, file_pcm, strerror(errno));
        fclose(fp_pcm);
        snprintf(resp_buf, sizeof(resp_buf), "{\"ret\":\"false\"}");
        goto Exit;
    }

    wav_hdr.bits_per_sample = 16;
    wav_hdr.samples_per_sec = AUDIO_SAMPLE_RATE;
    wav_hdr.block_align = 16 / 8;
    wav_hdr.avg_bytes_per_sec = wav_hdr.samples_per_sec * wav_hdr.bits_per_sample / 8;

    fwrite(&wav_hdr, sizeof(wav_hdr), 1, fp);

    pthread_mutex_lock(&mutex);
    data_size = 0;
    aud_save_fp = fp;
    aud_save_pcm_fp = fp_pcm;
    pthread_mutex_unlock(&mutex);
    long sttm = ty_time_utc_ms();

    int run = 1;
    while (run) {
        pthread_mutex_lock(&mutex);
        if (!aud_save_fp)
            run = 0;
        pthread_mutex_unlock(&mutex);

        long curtm = ty_time_utc_ms();
        if (curtm - sttm >= 3 * 1000) {
            pthread_mutex_lock(&mutex);
            aud_save_fp = NULL;
            aud_save_pcm_fp = NULL;
            run = 0;
            pthread_mutex_unlock(&mutex);
        }
    }

    wav_hdr.data_size = data_size;
    wav_hdr.size_8 += wav_hdr.data_size + (sizeof(wav_hdr) - 8);

    pthread_mutex_lock(&mutex);
    /* wave file */
    fseek(fp, 4, 0);
    fwrite(&wav_hdr.size_8, sizeof(wav_hdr.size_8), 1, fp);
    fseek(fp, 40, 0);
    fwrite(&wav_hdr.data_size, sizeof(wav_hdr.data_size), 1, fp);
    fclose(fp);
    fp = NULL;
    fclose(fp_pcm);
    fp_pcm = NULL;
    data_size = 0;
    aud_save_fp = NULL;
    aud_save_pcm_fp = NULL;
    pthread_mutex_unlock(&mutex);

    file_transport(file, ipaddr);
    snprintf(resp_buf, sizeof(resp_buf), "{\"ret\":\"true\"}");

Exit:

    tuya_put_frame(TUYATEST_MIC_TEST, resp_buf, strlen(resp_buf));
    tuya_exec_cmd("rm -rf /tmp/default.*", NULL, 0);
    TYDEBUG("record exit #@#\n");
    return (void *)0;
}

#ifdef RTSP_ENABLE
// typedef struct {
//     int active;
//     CHANNEL_E vchn;
//     CHANNEL_E achn;
// } rtsp_user_info_s;

#define MAX_RTSP_USER 5

// static rtsp_user_info_s users[MAX_RTSP_USER] = {0};

int rtsp_user_start()
{
    //    for(int i = 0; i < MAX_RTSP_USER; i++){
    //        if (0 == users[i].active){
    //            users[i].active = 1;
    //            users[i].vchn = E_CHANNEL_MAX;
    //            users[i].achn = E_CHANNEL_MAX;
    //            return i;
    //        }
    //    }
    //    return -1;

    return 0;
}

int rtsp_user_stop(int user_id)
{
    //    users[user_id].active = 0;
    //    if (users[user_id].vchn != E_CHANNEL_MAX){
    //        tuya_ipc_ring_buffer_clean_user_state(users[user_id].vchn, E_USER_RTSP + user_id);
    //    }
    //    if (users[user_id].achn != E_CHANNEL_MAX){
    //        tuya_ipc_ring_buffer_clean_user_state(users[user_id].achn, E_USER_RTSP + user_id);
    //    }
    return 0;
}

// static int rtsp_get_frame(int user_id, RTSP_MEDIA_TYPE_E type, char** buf, int *plen, uint64_t *pts)
//{
//     rtsp_user_info_s* puser= &users[user_id];
//     Ring_Buffer_Node_S* pnode = NULL;
//     static unsigned char g711_buffer[MAX_RTSP_USER][1600] = {0};

//    if (type == RTSP_MEDIA_TYPE_VIDEO){
//        pnode = tuya_ipc_ring_buffer_get_video_frame(puser->vchn, E_USER_RTSP + user_id, 0);
//    }
//    else {
//        pnode = tuya_ipc_ring_buffer_get_audio_frame(puser->achn, E_USER_RTSP + user_id, 0);
//        if (NULL != pnode){

//            printf("pack audio len(%d)\n", pnode->size);

//            static FILE* fp;
//            if (NULL == fp){
//            fp = fopen("/tmp/audio.dump", "w");
//            }
//            if (fp){
//            fwrite(pnode->rawData, pnode->size, 1, fp);
//            }
//
//        }
//    }

//    if (NULL == pnode){
//        return -1;
//    }

//    if (type == RTSP_MEDIA_TYPE_AUDIO){
//        tuya_g711_encode(TUYA_G711_MU_LAW, (unsigned short *)pnode->rawData,pnode->size, g711_buffer[user_id], (unsigned int*)plen);
//        *buf = (char*)g711_buffer[user_id];
//    }
//    else{
//        *buf = (char *)pnode->rawData;
//        *plen = pnode->size;
//    }

//    *pts = pnode->timestamp;

//    return 0;
//}

int rtsp_get_frame_main(int user_id, RTSP_MEDIA_TYPE_E type, char **buf, int *plen, uint64_t *pts)
{
    //    users[user_id].vchn = E_CHANNEL_VIDEO_MAIN;
    //    users[user_id].achn = E_CHANNEL_AUDIO;
    //    return rtsp_get_frame(user_id, type, buf, plen, pts);
    return 0;
}

int rtsp_get_frame_sub(int user_id, RTSP_MEDIA_TYPE_E type, char **buf, int *plen, uint64_t *pts)
{
    //    users[user_id].vchn = E_CHANNEL_VIDEO_SUB;
    //    users[user_id].achn = E_CHANNEL_AUDIO;
    //    return rtsp_get_frame(user_id, type, buf, plen, pts);

    return 0;
}


static RTSP_CODEC_E get_codec(RTSP_MEDIA_TYPE_E type)
{
    switch (type) {
        case RTSP_MEDIA_TYPE_VIDEO:
            return RTSP_CODEC_H264;
        case RTSP_MEDIA_TYPE_AUDIO:
            return RTSP_CODEC_G711A;
        default:
            return 0;
    }
}

static int get_sample_rate(RTSP_MEDIA_TYPE_E type)
{
    switch (type) {
        case RTSP_MEDIA_TYPE_VIDEO:
            return 90000;
        case RTSP_MEDIA_TYPE_AUDIO:
            return 8000;
        default:
            return 8000;
    }
}
#endif

int tuya_fac_media_start_rtsp()
{
    /* 启动RTSP Server，供上位机取流 */
#ifdef RTSP_ENABLE
    rtsp_stream_src_t src[2] = { 0 };
    src[0].get_codec = get_codec;
    src[0].get_frame = rtsp_get_frame_main;
    src[0].get_sample_rate = get_sample_rate;
    src[0].start = rtsp_user_start;
    src[0].stop = rtsp_user_stop;

    src[1].get_codec = get_codec;
    src[1].get_frame = rtsp_get_frame_sub;
    src[1].get_sample_rate = get_sample_rate;
    src[1].start = rtsp_user_start;
    src[1].stop = rtsp_user_stop;

    rtsp_server_start(NULL, NULL);
    rtsp_server_register_stream_src(src);
    rtsp_server_register_stream_src(src + 1);
#endif
    return 0;
}

int tuya_fac_media_stop_rtsp()
{
#ifdef RTSP_ENABLE
    rtsp_server_stop();
#endif
    return 0;
}
