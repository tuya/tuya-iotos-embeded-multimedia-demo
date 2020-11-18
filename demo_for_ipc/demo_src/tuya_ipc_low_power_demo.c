/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_p2p_demo
**********************************************************************************/
#include <stdlib.h>
#include "tuya_ipc_common_demo.h"
#include "tuya_ipc_media.h"
#include "tuya_ipc_dp_utils.h"
#include "tuya_ipc_stream_storage.h"

/*
---------------------------------------------------------------------------------
Low power access reference code
en:TRUE is sleep      FALSE is wake
---------------------------------------------------------------------------------
*/
STATIC CHAR_T s_wakeup_data[32] = {0};
STATIC UINT_T s_wakeup_len = 32;
STATIC INT_T s_wakeup_fd = -1;
STATIC pthread_t s_wake_send_pthread = -1;
STATIC BOOL_T s_wake_task_stat = FALSE;


void __wakeup_task(void * argv)
{
    PR_DEBUG("into task");
    CHAR_T buffer[32];
    while(TRUE == s_wake_task_stat){
        //Continuously receive wakeup data
        recv(s_wakeup_fd, buffer, s_wakeup_len, 0);
        //Users ensure that wakeup data is integrally received
        if (0 == strncmp(buffer, s_wakeup_data, s_wakeup_len)){
            //After receiving the wakeup data, it wakes up and processes
            /*********************/
            /*********************/
            /*********************/
            /*********************/
            return ;
        }
    }

    return ;
}

OPERATE_RET TUYA_APP_LOW_POWER_ENABLE()
{
    PR_DEBUG("low power en");
    BOOL_T  doorStat = FALSE;
    OPERATE_RET ret = 0;

    //Report sleep status to tuya
    ret = tuya_ipc_dp_report(NULL, TUYA_DP_DOOR_STATUS,PROP_BOOL,&doorStat,1);
    if (OPRT_OK != ret){
        //This dp point is very important. The user should repeat call the report interface until the report is successful when it is fail.
        //If it is failure continues, the user needs to check the network connection.
        PR_ERR("dp report failed");
        return ret;
    }
    ret = tuya_ipc_book_wakeup_topic();
    if (OPRT_OK != ret){
        PR_ERR("tuya_ipc_book_wakeup_topic failed");
        return ret;            
    }
    ret = tuya_ipc_get_wakeup_data(s_wakeup_data, &s_wakeup_len);
    if (OPRT_OK != ret){
        PR_ERR("tuya_ipc_get_wakeup_data failed");
        return ret;   
    }

    int i = 0;

    for(i = 0; i < s_wakeup_len; i++) {
        printf("%x ", s_wakeup_data[i]);
    }

    printf("\n");

    //Get fd for server to wakeup
    s_wakeup_fd =  tuya_ipc_get_mqtt_socket_fd();
    if (-1 == s_wakeup_fd){
        PR_ERR("tuya_ipc_get_mqtt_socket_fd failed");
        return ret; 
    }

    //Create a sock receive thread and receive the wakeup package
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1024 * 1024);
    s_wake_task_stat = TRUE;
    ret = pthread_create(&s_wake_send_pthread, &attr, __wakeup_task, NULL);
    if (OPRT_OK != ret){
        PR_ERR("task create failed");
        s_wake_task_stat = FALSE;
        return ret;
    }
    pthread_attr_destroy(&attr);

    return ret;
}

OPERATE_RET TUYA_APP_LOW_POWER_DISABLE()
{
    BOOL_T  doorStat = TRUE;
    OPERATE_RET ret = 0;

    //Close the thread that receives the wakeup data
    if (-1 != s_wake_send_pthread){
        s_wake_task_stat = FALSE;
        pthread_join(s_wake_send_pthread, NULL);
    } 
    ret = tuya_ipc_dp_report(NULL, TUYA_DP_DOOR_STATUS,PROP_BOOL,&doorStat,1);
    return ret;
}

