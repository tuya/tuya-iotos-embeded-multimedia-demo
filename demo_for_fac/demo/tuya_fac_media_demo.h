#ifndef __TYCAM_FAC_MEDIA_H__
#define __TYCAM_FAC_MEDIA_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RTSP_ENABLE
#include "rtsp.h"
#endif
int tuya_fac_media_init();

int tuya_fac_media_start_rtsp();

int tuya_fac_media_stop_rtsp();

void *tuya_fac_media_record(void *arg);

#ifdef RTSP_ENABLE
int rtsp_user_start();
int rtsp_user_stop(int user_id);
int rtsp_get_frame_main(int user_id, RTSP_MEDIA_TYPE_E type, char **buf, int *plen, uint64_t *pts);
int rtsp_get_frame_sub(int user_id, RTSP_MEDIA_TYPE_E type, char **buf, int *plen, uint64_t *pts);
#endif

#ifdef __cplusplus
}
#endif

#endif
