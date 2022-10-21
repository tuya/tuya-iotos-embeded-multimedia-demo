#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "tuya_os_adapter.h"
#include "tuya_cloud_base_defs.h"
#include "tuya_iot_com_api.h"
#include "tuya_iot_base_api.h"
#include "tuya_gw_subdev_api.h"
#include "tuya_xvr_dev.h"
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_skill.h"
#include "tuya_ipc_media_demo.h"
#include "tuya_xvr_cloud_storage.h"
#include "tuya_ipc_p2p.h"
#ifdef __cplusplus
extern "C" {
#endif
extern  CHAR_T s_nvr_sub_pid[64];
/* recv add sub device cmd 
   permit 0：禁止网关添加子设备 
*/
static int g_dev_cnt = 0;
static int g_one_dev_flag = 0;
static int g_two_dev_flag = 0;
void test_set_g_dev_cnt(int num)
{
  g_dev_cnt = num;
  printf("devi1  flag %d,devid2 flag %d",g_one_dev_flag,g_two_dev_flag);
}
static char *sub_dev_uuid = "dd07bd5ac6f13ede";//uuid
static char *sub_product_id= "mmnfbn6jwfen338s";//pid
static char *sub_dev_uuid2 = "2a2bcec989e55f78";//uuid
static char *sub_product_id2= "mmnfbn6jwfen338s";//pid
void test_set_g_dev_flag(char * devid)
{
    if(devid == NULL) {
        return;
    }
    if(strcmp(sub_dev_uuid ,devid) == 0) {
        g_one_dev_flag = 1;
    } else if(strcmp(sub_dev_uuid2 ,devid) == 0) {
        g_two_dev_flag = 1;
    }
    printf("set flag is %s,g_one_dev_flag %d g_one_dev_flag %d\n",devid,g_one_dev_flag,g_two_dev_flag);
}
BOOL_T ty_gw_subdev_add_dev(IN CONST GW_PERMIT_DEV_TP_T tp,
                            IN CONST BOOL_T permit, IN CONST UINT_T timeout)
{
    PR_DEBUG("tp: %d, permit: %d, timeout: %d\n", tp, permit, timeout);
#if defined(DEMO_USE_AS_NVR) && (DEMO_USE_AS_NVR==1)
    //need user todo nothing
#else
/*
     这个是基站模式，添加两个子设备的一个样例。用户需要提供对应子设备和uuid来绑定对应的子设备。
*/
    if (permit == 0){
        PR_ERR("permit 0, do nothing\n");
        return 0;
    }



    static char *sub_version= "11.0.11";

    TUYA_XVR_BIND_CTX_T bind_ctx={0};
    bind_ctx.bind_dev_cnt=1;
    bind_ctx.tp=10;
    bind_ctx.uddd=(0x2 << 24);
    if (g_one_dev_flag == 0) {
        memcpy(&bind_ctx.uuid, sub_dev_uuid, strlen(sub_dev_uuid));
        memcpy(&bind_ctx.product_id, sub_product_id, strlen(sub_product_id));
        memcpy(&bind_ctx.version, sub_version, strlen(sub_version));
        g_one_dev_flag = 1;
    } else if (g_two_dev_flag == 0) {
        memcpy(&bind_ctx.uuid, sub_dev_uuid2, strlen(sub_dev_uuid2));
        memcpy(&bind_ctx.product_id, sub_product_id2, strlen(sub_product_id2));
        memcpy(&bind_ctx.version, sub_version, strlen(sub_version));
        g_two_dev_flag = 1;

    } else {
        PR_ERR("total cnt  2 bind device error,%d\n",g_dev_cnt);
        return -1;
    }

    OPERATE_RET ret = tuya_xvr_dev_binds(&bind_ctx);
    if (ret < 0) {
        PR_ERR("bind device error\n");
    }
    if(1 == g_two_dev_flag) {
        tuya_xvr_dev_hearbeat_time_set(sub_dev_uuid,60);
    } else {
        tuya_xvr_dev_hearbeat_time_set(sub_dev_uuid2,60);
    }
    g_dev_cnt++;
#endif
    return TRUE;
}
VOID ty_gw_subdev_del_dev(IN CONST CHAR_T *pdevId, IN CONST GW_DELDEV_TYPE type)
{
    if(pdevId == NULL) {
        return;
    }
    PR_DEBUG("dev_id: %s\n", pdevId);
#if defined(DEMO_USE_AS_NVR) && (DEMO_USE_AS_NVR==1)
    //need user todo nothing
#else
    if(strcmp(sub_dev_uuid ,pdevId) == 0) {
        g_one_dev_flag = 0;
        PR_DEBUG("dev_id remove %s,reset ok\n", pdevId);
    } else if(strcmp(sub_dev_uuid2 ,pdevId) == 0) {
        g_two_dev_flag = 0;
        PR_DEBUG("dev_id remove %s,reset ok\n", pdevId);
    }
    //step:删除对应子设备信息.比如云存，比如对应的子设备的ring buffer。用户根据需求处理
    tuya_xvr_cloud_storage_stop((CHAR_T*)pdevId);
    //last step:客户不需要再调用tuya_xvr_dev_unbind_cxt_set函数，只需确保自身的业务停止即可。
	TUYA_APP_fake_video_stop(pdevId);
    INT_T device_index = -1;
    tuya_xvr_dev_chan_get_by_devId(pdevId, &device_index);
#endif
    return;
}
OPERATE_RET ty_gw_subdev_grp_inform_dev(IN CONST GRP_ACTION_E action,IN CONST CHAR_T *dev_id,IN CONST CHAR_T *grp_id)
{
    PR_DEBUG("ty_gw_subdev_grp_inform_dev dev_id: %s cb\n", dev_id);
    return 0;
}


OPERATE_RET ty_gw_subdev_scene_inform_dev(IN CONST SCE_ACTION_E action,IN CONST CHAR_T *dev_id,IN CONST CHAR_T *grp_id,IN CONST CHAR_T *sce_id)
{
    PR_DEBUG("ty_gw_subdev_scene_inform_dev dev_id: %s cb\n", dev_id);
    return 0;
}

VOID ty_gw_subdev_inform_dev(IN CONST CHAR_T *dev_id, IN CONST OPERATE_RET op_ret)
{
    PR_DEBUG("dev_id: %s ret[%d]\n", dev_id, op_ret); 
    //step 客户不需要再调用提供的tuya_xvr_bind_p2p_auth_info_update 和tuya_xvr_dev_bind_result_cb接口。
#if defined(DEMO_USE_AS_NVR) && (DEMO_USE_AS_NVR==1)
        //need user todo nothing
#else
    if (OPRT_OK == op_ret){
//        //step:必须步骤。设置绑定信息
//        if (OPRT_OK != tuya_xvr_dev_bind_ctx_set((char *)dev_id)){
//            PR_DEBUG("devId[%s] sub dev bind failed\n",dev_id);
//            return;
//        }
        //step:用户可以这里开启对应的设备的ring buffer 和云存等功能
        PR_DEBUG("start dev[%s] sdk business\n",dev_id);
        extern int ty_sub_dev_sdk_start(char * devId);
        ty_sub_dev_sdk_start((char *)dev_id);
    }
#endif
    return ;
}

VOID ty_gw_dev_reset_ifm(IN CONST CHAR_T *pdevId, IN DEV_RESET_TYPE_E type)
{
    if(pdevId == NULL) {
        return;
    }

    PR_DEBUG("dev_id: %s, type: %d\n", pdevId, type);

    if(type == DEV_REMOTE_RESET_FACTORY) {

    } else if (type == DEV_RESET_DATA_FACTORY) {

    }

    return;
}

int ty_sub_dev_sdk_start(char * devId)
{
    PR_DEBUG("sub dev [%s] bind\n",devId);
    test_set_g_dev_flag(devId);
    //ringbuffer init
    PR_DEBUG("sub dev [%s] ring buffer init\n",devId);
    INT_T device_index = -1;
    tuya_xvr_dev_chan_get_by_devId(devId, &device_index);
    TUYA_APP_Init_Ring_Buffer(device_index,(char *)devId);
    //local stor start here
    //PR_DEBUG("sub dev [%s] stor start\n",devId);
    //test fake send av info to ringbuffer, user can do it 
    TUYA_APP_fake_video_start(devId);
    tuya_xvr_dev_hearbeat_time_set(devId, 60);
    tuya_xvr_cloud_storage_start(devId);
    return 0;
}
#if  !(defined(DEMO_USE_AS_NVR) && (DEMO_USE_AS_NVR == 1))
INT_T TUYA_STATION_Dev_Fun_Start()
{
    INT_T charArr[16];
    INT_T cnt = -1;
    tuya_xvr_dev_chans_get(&cnt,  charArr);
    if(cnt == -1) {
        return -1;
    }
    INT_T  i = 0;
    for(i=0;i< cnt;i++) {
        CHAR_T dev_id[64] = {0};
        tuya_xvr_dev_devId_get_by_chan(charArr[i], dev_id, sizeof(dev_id));
        ty_sub_dev_sdk_start(dev_id);
    }
}
#endif

int TUYA_APP_Add_XVR_Dev()
{
#if defined(DEMO_USE_AS_NVR) && (DEMO_USE_AS_NVR == 1)
    //step：获取设备是否有绑定，有绑定 则bind_cnt不为0，则不需要重复绑定
    extern INT_T tuya_xvr_dev_binded_status_get();
    if(TRUE == tuya_xvr_dev_binded_status_get()) {
        return 0;
     }
    //step:当前主要以NVR模式绑定子设备。确保绑定后，不再重复绑定。
    // 若nvr还没添加子设备，则添加下
     TUYA_XVR_BIND_CTX_T bind_ctx={0};
     bind_ctx.bind_dev_cnt=DEMO_NVR_SUB_DEV_NUM;
     bind_ctx.tp=0;//参数表示子设备类型，通常可以10，或者0.建议些0，这边表示IPC主设备的类型，这样在升级时，可以复用对应IPC固件
     bind_ctx.uddd=(0x2 << 24);//以demo固定写死为准
     memcpy(bind_ctx.product_id,s_nvr_sub_pid,strlen(s_nvr_sub_pid));
     memcpy(bind_ctx.version,IPC_APP_SUB_DEV_VERSION,strlen(IPC_APP_SUB_DEV_VERSION));
     tuya_xvr_dev_binds(&bind_ctx);
#else
#endif
        return 0;
}

/*
 *用户根据需求，根据返回出来的设备id来决定是否显示子设备在线
 *当回调的子设备，3次都未调用tuya_xvr_dev_hb_fresh,会显示离线
 *回调函数的大概8到10秒 回调一次
 *如果想强制离线。比如本身没有真实设备的情况，不想显示对应的设备在线，可调用tuya_iot_dev_online_update强制刷成离线
 * */
void ty_dev_heartbeat_send_cb(const char *dev_id)
{
    //if dev no hb, IOT will callback 3times
    //if check devid online
    int is_online = 1;
    if (is_online == 1) {
        tuya_xvr_dev_hb_fresh(dev_id);
    } else {
        // is not noline  nothing todo ,and devid will display offline on app
    }
    return;
}

void TUYA_APP_Enable_SUB_DEV_HB()
{

    if (OPRT_OK != tuya_xvr_dev_hb_init(ty_dev_heartbeat_send_cb,120)){
        PR_ERR("init hb failed");
    }

    int i = 0;
    for (i = 0; i < DEMO_NVR_SUB_DEV_NUM; i++){
        CHAR_T devId[64];
        memset(devId,0x00, sizeof(devId));
        if (OPRT_OK == tuya_xvr_dev_devId_get_by_chan(i, devId,sizeof(devId))){
            //设置子设备与云端交互的心跳回调时间。不用太长，也不能太短。建议60到120之间。太短影响性能。太长可能不及时。
            tuya_xvr_dev_hearbeat_time_set(devId, 60);
        }
    }
    return ;
}
#ifdef __cplusplus
}
#endif
