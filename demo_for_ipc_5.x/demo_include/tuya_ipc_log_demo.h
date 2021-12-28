/*
 * tuya_ipc_log_demo.h
 *Copyright(C),2017-2022, TUYA company www.tuya.comm
 *
 *FILE description:
  *
 *  Created on: 2021年12月23日
 *      Author: dante
 */

#ifndef __TUYA_LOG_DEMO_H_
#define __TUYA_LOG_DEMO_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PR_ERR(fmt, ...)    printf("Err:"fmt"\r\n", ##__VA_ARGS__)
#define PR_DEBUG(fmt, ...)  printf("Dbg:"fmt"\r\n", ##__VA_ARGS__)
#define PR_TRACE


#ifdef __cplusplus
}
#endif
#endif