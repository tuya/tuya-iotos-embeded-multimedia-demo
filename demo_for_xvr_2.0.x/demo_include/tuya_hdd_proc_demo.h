/*********************************************************************************
  *Copyright(C),2019, www.tuya.comm
  *FileName:    ty_hdd_proc.h
**********************************************************************************/

#ifndef __TY_HDD_PROC_H__
#define __TY_HDD_PROC_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STOR_TYPE_OF_SD   (0)
#define STOR_TYPE_OF_HDD  (1)

int ty_hdd_init(int type);
int ty_hdd_uninit();
int ty_hdd_start(char * devId);
int ty_hdd_stop(char * devId);
int ty_hdd_set_stat(char * devId, int stat,int type);

int ty_hdd_get_volume(int type, UINT64_T * totol, UINT64_T * free);

int ty_hdd_format(int type);

int ty_hdd_set_dev_name(int type, char * dev_name);

#ifdef __cplusplus
}
#endif

#endif

