#include <stdio.h>

#include "tuya_ring_buffer.h"
#include "tuya_xvr_dev.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_cloud_storage.h"
#include "tuya_xvr_cloud_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEMO_NVR_ONVIF_ENABLE
#include "tuya_ipc_onvif_client.h"

typedef struct {
    BOOL_T valid;
    CHAR_T ip[MAX_ONVIF_IP_LEN+1];
    INT_T port;
}IPC_APP_ONVIF_PROBE_INFO_S;

typedef struct {
    BOOL_T enable;
    ONVIF_CLIENT_STATUS_E status;
}IPC_APP_ONVIF_LOGIN_S;

typedef struct {
    IPC_APP_ONVIF_LOGIN_S login;
    ONVIF_PROFILE_S profiles[E_IPC_STREAM_VIDEO_MAX];
}IPC_APP_ONVIF_CLIENT_S;

#define MAX_PROBE_SIZE   DEMO_NVR_SUB_DEV_NUM
#define ONVIF_PROBE_TIMEOUT    60

BOOL_T ring_buffer_inited = FALSE;
STATIC IPC_APP_ONVIF_PROBE_INFO_S s_probe_infos[MAX_PROBE_SIZE];
STATIC IPC_APP_ONVIF_CLIENT_S s_onvif_clients[DEMO_NVR_SUB_DEV_NUM];
STATIC Ring_Buffer_User_Handle_S s_onvif_v_handles[DEMO_NVR_SUB_DEV_NUM][E_IPC_STREAM_VIDEO_MAX];
STATIC Ring_Buffer_User_Handle_S s_onvif_a_handles[DEMO_NVR_SUB_DEV_NUM][E_IPC_STREAM_VIDEO_MAX];

STATIC INT_T __IPC_APP_onvif_client_probe_cb(IN CHAR_T *uuid, IN CHAR_T *ip, IN INT_T port, IN CHAR_T *url)
{
    INT_T i = 0;
    for (i = 0; i < MAX_PROBE_SIZE; i++)
    {
        if (s_probe_infos[i].valid)
        {
            if (0 == strcmp(ip, s_probe_infos[i].ip) && port == s_probe_infos[i].port)
            {
                break;
            }
        }
        else
        {
            s_probe_infos[i].valid = TRUE;
            s_probe_infos[i].port = port;
            strncpy(s_probe_infos[i].ip, ip, MAX_ONVIF_IP_LEN);
            break;
        }
    }

    PR_DEBUG("onvif probe device list:");
    for (i = 0; i < MAX_PROBE_SIZE; i++)
    {
        if (s_probe_infos[i].valid)
        {
            PR_DEBUG("device index %d, ip = %s, port = %d", i, s_probe_infos[i].ip, s_probe_infos[i].port);
        }
    }

    return 0;
}

STATIC INT_T __IPC_APP_onvif_client_status_change_cb(IN INT_T device, IN ONVIF_CLIENT_STATUS_E status)
{
    if (device < 0 || device >= DEMO_NVR_SUB_DEV_NUM)
    {
        PR_ERR("invalid device %d", device);
        return -1;
    }
    PR_DEBUG("device: %d, status: %d", device, status);

    s_onvif_clients[device].login.status = status;

    return 0;
}

STATIC INT_T __IPC_APP_onvif_client_profiles_cb(IN INT_T device, IN ONVIF_PROFILE_S profiles[E_IPC_STREAM_VIDEO_MAX])
{
    if (device < 0 || device >= DEMO_NVR_SUB_DEV_NUM)
    {
        PR_ERR("invalid device %d", device);
        return -1;
    }

    INT_T i = 0;
    for (i = 0; i < E_IPC_STREAM_VIDEO_MAX; i++)
    {
        if (profiles[i].enable)
        {
            memcpy(&s_onvif_clients[device].profiles[i], &profiles[i], sizeof(ONVIF_PROFILE_S));
        }
    }

    return 0;
}

STATIC INT_T __IPC_APP_onvif_rtsp_stream_cb(IN INT_T device, IN IPC_STREAM_E stream, IN ONVIF_RTSP_FRAME_S *frame)
{
    if (!ring_buffer_inited)
    {
        return 0;
    }

    if (device < 0 || device >= DEMO_NVR_SUB_DEV_NUM || stream < 0 || stream >= E_IPC_STREAM_VIDEO_MAX)
    {
        PR_ERR("invalid device %d or stream %d", device, stream);
        return -1;
    }

    ONVIF_PROFILE_S *profile = &s_onvif_clients[device].profiles[stream];
    BOOL_T audio_enable = profile->audio_config.audio_sample > 0 ? TRUE : FALSE;

    if (s_onvif_v_handles[device][stream] == NULL)
    {
        s_onvif_v_handles[device][stream] = tuya_ipc_ring_buffer_open(device, 0, stream, E_RBUF_WRITE);
        if (s_onvif_v_handles[device][stream] == NULL)
        {
            PR_ERR("tuya_ipc_ring_buffer_open video fails, device %d, stream %d", device, stream);
            Ring_Buffer_Init_Param_S param = {0};
            param.bitrate = profile->video_config.video_bitrate;
            param.fps = profile->video_config.video_fps;
            param.max_buffer_seconds = 0;
            param.requestKeyFrameCB = NULL;
            tuya_ipc_ring_buffer_init(device, 0, stream, &param);
            return -1;
        }
    }
    if (audio_enable == TRUE && s_onvif_a_handles[device][stream] == NULL)
    {
        s_onvif_a_handles[device][stream] = tuya_ipc_ring_buffer_open(device, 0, stream+E_IPC_STREAM_AUDIO_MAIN, E_RBUF_WRITE);
        if (s_onvif_a_handles[device][stream] == NULL)
        {
            PR_ERR("tuya_ipc_ring_buffer_open audio fails, device %d, stream %d", device, stream);
            Ring_Buffer_Init_Param_S param = {0};
            param.bitrate = profile->audio_config.audio_sample * TUYA_AUDIO_DATABITS_16 / 1024;
            param.fps = 25;
            param.max_buffer_seconds = 0;
            param.requestKeyFrameCB = NULL;
            tuya_ipc_ring_buffer_init(device, 0, stream+E_IPC_STREAM_AUDIO_MAIN, &param);
            return -1;
        }
    }

    if (frame->type == E_AUDIO_FRAME)
    {
        if (audio_enable == TRUE)
        {
            tuya_ipc_ring_buffer_append_data(s_onvif_a_handles[device][stream], frame->buf, frame->size, frame->type, frame->timestamp * 1000);
        }
    }
    else
    {
        tuya_ipc_ring_buffer_append_data(s_onvif_v_handles[device][stream], frame->buf, frame->size, frame->type, frame->timestamp * 1000);
    }

    return 0;
}

STATIC INT_T __IPC_APP_onvif_client_event_cb(ONVIF_EVENT_S *event)
{
    int ret = 0;
    if (event == NULL)
    {
        return -1;
    }

    static int sub_dev_event_id[SUB_DEV_MAX_NUM] = {0};
    int channel = event->event.cell_motion_detection.channel;
    printf("tuya xvr start a event, channel=[%d], state=[%d]\n", channel, event->event.cell_motion_detection.state);
    CHAR_T devId[64] = {0};
    tuya_xvr_dev_devId_get_by_chan(channel, devId, sizeof(devId));
    printf("sub dev chan[1] id[%s] trggger move event\n", devId);
    /* Push the detection message and the current snapshot image to the APP . 
    Snapshot image acquisition needs to be implemented by the developer */
    if (event->event.cell_motion_detection.state == true)
    {
        sub_dev_event_id[channel] = tuya_xvr_cloud_storage_event_add(devId, EVENT_TYPE_MOTION_DETECT, 20);// event start
        ret = tuya_xvr_notify_with_event((CHAR_T*)devId, event->snapshot_buf, event->snapshot_size, NOTIFICATION_CONTENT_JPEG, NOTIFICATION_NAME_MOTION, TRUE);
        if (OPRT_OK != ret) {
            printf("tuya_ipc_dev_notify_with_event err %d\n", ret);
            return -1;
        }
    }
    else
    {
        ret = tuya_xvr_cloud_storage_event_delete(devId, sub_dev_event_id[channel]); // event stop
    }
}


OPERATE_RET TUYA_APP_Enable_Onvif_Client(VOID)
{
    TUYA_IPC_ONVIF_CLIENT_PARAM_S param = {0};
    strncpy(param.netif, NET_DEV, MAX_ONVIF_NET_IF_LEN);
    param.max_device_num = DEMO_NVR_SUB_DEV_NUM;
    param.probe_cb = __IPC_APP_onvif_client_probe_cb;
    param.status_change_cb = __IPC_APP_onvif_client_status_change_cb;
    param.profiles_cb = __IPC_APP_onvif_client_profiles_cb;
    param.rtsp_stream_cb = __IPC_APP_onvif_rtsp_stream_cb;
    param.event_notify_cb = __IPC_APP_onvif_client_event_cb;

    return tuya_ipc_onvif_client_init(&param);
}

OPERATE_RET TUYA_APP_Onvif_Probe_Start(VOID)
{
    memset(s_probe_infos, 0, sizeof(s_probe_infos));
    return tuya_ipc_onvif_client_probe_start(ONVIF_PROBE_TIMEOUT);
}

OPERATE_RET TUYA_APP_Onvif_Probe_Stop(VOID)
{
    return tuya_ipc_onvif_client_probe_stop();
}

OPERATE_RET TUYA_APP_Onvif_Add_Device(INT_T device, INT_T probe_index)
{
    if (device < 0 || device >= E_IPC_STREAM_VIDEO_MAX)
    {
        PR_ERR("invalid device %d", device);
        return OPRT_INVALID_PARM;
    }
    if (probe_index < 0 || probe_index >= MAX_PROBE_SIZE)
    {
        PR_ERR("invalid probe_index %d", probe_index);
        return OPRT_INVALID_PARM;
    }

    if (!s_probe_infos[probe_index].valid)
    {
        PR_ERR("device not found in probe list");
        return OPRT_INVALID_PARM;
    }
    if (s_onvif_clients[device].login.enable)
    {
        PR_ERR("device already add in ch %d", device);
        return OPRT_INVALID_PARM;
    }

    ONVIF_ADD_DEVICE_PARAM_S param = {0};
    param.device = device;
    param.port = s_probe_infos[probe_index].port;
    strncpy(param.ip, s_probe_infos[probe_index].ip, MAX_ONVIF_IP_LEN);
    strncpy(param.username, DEMO_ONVIF_DEVICE_USERNAME, MAX_ONVIF_USERNAME_LEN);
    strncpy(param.passwd, DEMO_ONVIF_DEVICE_PASSWD, MAX_ONVIF_PASSWORD_LEN);
    OPERATE_RET ret = tuya_ipc_onvif_client_add_device(&param);
    if (ret != OPRT_OK)
    {
        PR_ERR("device add failed, ret: %d", ret);
        return ret;
    }

    s_onvif_clients[device].login.enable = TRUE;
    return OPRT_OK;
}

OPERATE_RET TUYA_APP_Onvif_Delete_Device(INT_T device)
{
    if (device < 0 || device >= E_IPC_STREAM_VIDEO_MAX)
    {
        PR_ERR("invalid device %d", device);
        return OPRT_INVALID_PARM;
    }
    OPERATE_RET ret = tuya_ipc_onvif_client_delete_device(device);

    memset(&s_onvif_clients[device], 0, sizeof(IPC_APP_ONVIF_CLIENT_S));
    return ret;
}

BOOL_T TUYA_APP_Onvif_Device_Online(VOID)
{
    INT_T i = 0;

    for (i = 0; i < E_IPC_STREAM_VIDEO_MAX; i++)
    {
        if (s_onvif_clients[i].login.enable)
        {
            break;
        }
    }
    if (i >= E_IPC_STREAM_VIDEO_MAX)
    {
        PR_ERR("no device is add");
        return FALSE;
    }
    
    for (i = 0; i < E_IPC_STREAM_VIDEO_MAX; i++)
    {
        if (s_onvif_clients[i].login.enable && (s_onvif_clients[i].login.status != ONVIF_CLIENT_STATUS_ONLINE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

VOID TUYA_APP_Onvif_Get_Media_Info(char *devId, IPC_MEDIA_INFO_S *pMedia)
{
    if (devId == NULL || pMedia == NULL)
    {
        PR_ERR("input is invalid");
        return;
    }

    INT_T device = 0;
    OPERATE_RET ret = tuya_xvr_dev_chan_get_by_devId(devId, (INT_T *)&device);
    if (OPRT_OK != ret)
    {
        PR_ERR("get chn id failed");
        return;
    }

    if (device >= DEMO_NVR_SUB_DEV_NUM)
    {
        PR_ERR("onvif not support device %d", device);
        return;
    }

    ONVIF_PROFILE_S *profiles = s_onvif_clients[device].profiles;
    INT_T i = 0;
    for (i = 0; i < E_IPC_STREAM_VIDEO_MAX; i++)
    {
        if (profiles[i].enable)
        {
            pMedia->channel_enable[i] = TRUE;
            pMedia->video_fps[i] = profiles[i].video_config.video_fps;
            pMedia->video_gop[i] = profiles[i].video_config.video_gop;
            pMedia->video_bitrate[i] = profiles[i].video_config.video_bitrate;
            pMedia->video_width[i] = profiles[i].video_config.video_width;
            pMedia->video_height[i] = profiles[i].video_config.video_height;
            pMedia->video_freq[i] = 90000; /* Clock frequency 时钟频率 */
            pMedia->video_codec[i] = profiles[i].video_config.video_codec;

            if (profiles[i].audio_config.audio_sample > 0)
            {
                pMedia->channel_enable[i+E_IPC_STREAM_AUDIO_MAIN] = TRUE;
                pMedia->audio_codec[i+E_IPC_STREAM_AUDIO_MAIN] = profiles[i].audio_config.audio_codec;
                pMedia->audio_sample[i+E_IPC_STREAM_AUDIO_MAIN] = profiles[i].audio_config.audio_sample;
                pMedia->audio_databits[i+E_IPC_STREAM_AUDIO_MAIN] = TUYA_AUDIO_DATABITS_16;
                pMedia->audio_channel[i+E_IPC_STREAM_AUDIO_MAIN] = TUYA_AUDIO_CHANNEL_MONO;
                pMedia->audio_fps[i+E_IPC_STREAM_AUDIO_MAIN] = 25;
            }
        }
    }
}

VOID TUYA_IPC_Onvif_Start_Stream(VOID)
{
    ring_buffer_inited = TRUE;
}

OPERATE_RET TUYA_IPC_Onvif_PTZ_Move_Start(INT_T device, FLOAT_T velocity_pan, FLOAT_T velocity_tilt, FLOAT_T velocity_zoom)
{
    return tuya_ipc_onvif_client_ptz_move_start(device, velocity_pan, velocity_tilt, velocity_zoom);
}

OPERATE_RET TUYA_IPC_Onvif_PTZ_Move_Stop(INT_T device)
{
    return tuya_ipc_onvif_client_ptz_move_stop(device);
}

OPERATE_RET TUYA_IPC_Onvif_Get_PTZ_Preset(INT_T device)
{
    ONVIF_PTZ_PRESETS_S presets = {0};
    OPERATE_RET op_ret = tuya_ipc_onvif_client_get_ptz_presets(device, &presets);
    if (op_ret != OPRT_OK)
    {
        return op_ret;
    }

    INT_T i = 0;
    PR_DEBUG("device: %d, presets_num: %d", device, presets.num);
    for (i = 0; i < presets.num; i++)
    {
        PR_DEBUG("preset[%d]=> token: %s, name: %s, pan: %f, tilt: %f, zoom: %f", i, presets.presets[i].token, presets.presets[i].name, 
            presets.presets[i].position_pan, presets.presets[i].position_tilt, presets.presets[i].position_zoom);
    }

    return OPRT_OK;
}

OPERATE_RET TUYA_IPC_Onvif_Add_PTZ_Preset(INT_T device, CHAR_T *preset_name)
{
    return tuya_ipc_onvif_client_set_ptz_preset(device, NULL, preset_name);
}

OPERATE_RET TUYA_IPC_Onvif_Set_PTZ_Preset(INT_T device, CHAR_T *preset_token, CHAR_T *preset_name)
{
    return tuya_ipc_onvif_client_set_ptz_preset(device, preset_token, preset_name);
}

OPERATE_RET TUYA_IPC_Onvif_Remove_PTZ_Preset(INT_T device, CHAR_T *preset_token)
{
    return tuya_ipc_onvif_client_remove_ptz_preset(device, preset_token);
}

OPERATE_RET TUYA_IPC_Onvif_Goto_PTZ_Preset(INT_T device, CHAR_T *preset_token)
{
    return tuya_ipc_onvif_client_goto_ptz_preset(device, preset_token);
}

#endif

#ifdef __cplusplus
}
#endif
