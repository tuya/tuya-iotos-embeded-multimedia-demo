/*********************************************************************************
  *Copyright(C),2019, www.tuya.comm
  *FileName:    ty_hdd_file_oper.h
**********************************************************************************/

#ifndef __TY_HDD_FILE_OPER_H__
#define __TY_HDD_FILE_OPER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

VOID hd_free();

INT_T hd_open(IN CONST BYTE_T * pHdName, IN CONST UINT_T fileType, IN CONST UINT_T fileNo, IN CONST INT_T mode, PVOID_T attr);

INT_T hd_write(IN CONST PVOID_T hdHandle,IN CONST UINT64_T uOffset, IN CONST BYTE_T * pBuff, IN CONST UINT_T writeLen, PVOID_T attr);

INT_T hd_read(IN CONST PVOID_T hdHandle,IN CONST UINT64_T uOffset, IN BYTE_T * pBuff, IN CONST UINT_T readLen, PVOID_T attr);

INT_T hd_seek(IN CONST PVOID_T hdHandle,IN CONST UINT64_T uOffset, IN CONST INT_T whence, PVOID_T attr);

INT_T hd_flush(IN CONST PVOID_T hdHandle,PVOID_T attr);

INT_T hd_close(IN CONST PVOID_T hdHandle,PVOID_T attr);


#ifdef __cplusplus
}
#endif

#endif

