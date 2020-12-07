/*********************************************************************************
  *Copyright(C),2015-2020, www.tuya.comm
  *FileName:    tuya_ipc_wired_demo
**********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tuya_hal_wifi.h"
#include "tuya_hal_wired.h"
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "tuya_cloud_error_code.h"
#include "tuya_cloud_types.h"
#include "tuya_ipc_ap_demo.h"
#include "tuya_ipc_wired_demo.h"

#define WLAN_DEV "wlan0"

void usr_set_net_ap()
{
    usr_set_net_dev(WLAN_DEV);
}

OPERATE_RET user_wifi_get_mac(INOUT NW_MAC_S* mac)
{
    if (NULL == mac) {
        return OPRT_INVALID_PARM;
    }

    FILE* pp = popen("ifconfig " WLAN_DEV, "r");
    if (pp == NULL) {
        return OPRT_COM_ERROR;
    }

    char tmp[256];
    memset(tmp, 0, sizeof(tmp));
    while (fgets(tmp, sizeof(tmp), pp) != NULL) {
        char* pMACStart = strstr(tmp, "ether ");
        if (pMACStart != NULL) {
            int x1, x2, x3, x4, x5, x6;
            sscanf(pMACStart + strlen("ether "), "%x:%x:%x:%x:%x:%x", &x1, &x2, &x3, &x4, &x5, &x6);
            mac->mac[0] = x1 & 0xFF;
            mac->mac[1] = x2 & 0xFF;
            mac->mac[2] = x3 & 0xFF;
            mac->mac[3] = x4 & 0xFF;
            mac->mac[4] = x5 & 0xFF;
            mac->mac[5] = x6 & 0xFF;

            break;
        }
    }
    pclose(pp);

    printf("WIFI Get MAC %02X-%02X-%02X-%02X-%02X-%02X\r\n",
        mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);

    return OPRT_OK;
}

int user_wifi_get_work_mode(int* mode)
{
    if (NULL == mode) {
        return OPRT_INVALID_PARM;
    }

    FILE* pp = popen("iwconfig wlan0", "r");
    if (pp == NULL) {
        printf("WIFI Get Mode Fail. Force Set Station \r\n");
        *mode = WWM_STATION;
        return OPRT_OK;
    }

    char scan_mode[10] = { 0 };
    char tmp[256] = { 0 };
    while (fgets(tmp, sizeof(tmp), pp) != NULL) {
        char* pModeStart = strstr(tmp, "Mode:");
        if (pModeStart != NULL) {
            // int x1,x2,x3,x4,x5,x6;
            sscanf(pModeStart + strlen("Mode:"), "%9s ", scan_mode);
            break;
        }
    }
    pclose(pp);

    *mode = WWM_STATION;
    if (strncasecmp(scan_mode, "Managed", strlen("Managed")) == 0) {
        *mode = WWM_STATION;
    }

    if (strncasecmp(scan_mode, "Master", strlen("Master")) == 0) {
        *mode = WWM_SOFTAP;
    }

    if (strncasecmp(scan_mode, "Monitor", strlen("Monitor")) == 0) {
        *mode = WWM_SNIFFER;
        printf("WIFI Get Mode [%s] %d\r\n", scan_mode, *mode);
    }
    printf("WIFI Get Mode [%s] %d\r\n", scan_mode, *mode);

    return OPRT_OK;
}
