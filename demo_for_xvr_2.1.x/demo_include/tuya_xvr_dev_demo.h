/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_sub_dev_demo.h
**********************************************************************************/


#ifndef _TUYA_IPC_SUB_DEV_DEMO_H
#define _TUYA_IPC_SUB_DEV_DEMO_H

#include "tuya_cloud_types.h"
#include "tuya_gw_com_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

BOOL_T ty_gw_subdev_add_dev(IN CONST GW_PERMIT_DEV_TP_T tp,
                            IN CONST BOOL_T permit, IN CONST UINT_T timeout);

VOID ty_gw_subdev_del_dev(IN CONST CHAR_T *pdevId, IN CONST GW_DELDEV_TYPE type);

OPERATE_RET ty_gw_subdev_grp_inform_dev(IN CONST GRP_ACTION_E action,IN CONST CHAR_T *dev_id,IN CONST CHAR_T *grp_id);

OPERATE_RET ty_gw_subdev_scene_inform_dev(IN CONST SCE_ACTION_E action,IN CONST CHAR_T *dev_id,IN CONST CHAR_T *grp_id,IN CONST CHAR_T *sce_id);

VOID ty_gw_subdev_inform_dev(IN CONST CHAR_T *dev_id, IN CONST OPERATE_RET op_ret);


VOID ty_gw_dev_reset_ifm(IN CONST CHAR_T *pdevId, IN DEV_RESET_TYPE_E type);

int ty_sub_dev_sdk_start(char * devId);

int ty_sub_dev_sdk_stop(char * devId);

int TUYA_APP_Enable_Sub_Dev();
int TUYA_APP_Add_Sub_Dev();
int TUYA_APP_Add_XVR_Dev();
void TUYA_APP_Enable_SUB_DEV_HB();
INT_T TUYA_STATION_Dev_Fun_Start();

#ifdef __cplusplus
}
#endif

#endif  /* _TUYA_IPC_SUB_DEV_DEMO_H */

