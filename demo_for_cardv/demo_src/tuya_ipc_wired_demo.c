/*********************************************************************************
  *Copyright(C),2015-2020, www.tuya.comm
  *FileName:    tuya_ipc_wired_demo
**********************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "tuya_hal_wired.h"
#include "tuya_cloud_error_code.h"
#include "tuya_cloud_types.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define NET_DEV "ens33"
static char g_net_dev[32]= NET_DEV;


void usr_set_net_dev(char *dev)
{
    memset(g_net_dev, 0, sizeof(g_net_dev));
    memcpy(g_net_dev, dev, MIN(sizeof(g_net_dev),strlen(dev)));
}

void usr_set_net_wire()
{
    usr_set_net_dev(NET_DEV);
}

/***********************************************************
*************************micro define***********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/

VOID gw_upload_rst_log_set(IN CONST bool upload)
{
    return;
}

int tuya_hal_wired_get_nw_stat(GW_BASE_NW_STAT_T *stat)
{
	return OPRT_OK;
}


// Obtain the ip address of the port
OPERATE_RET tuya_hal_wired_get_ip(OUT NW_IP_S *ip)
{
    int sock;
    char ipaddr[50];

    struct   sockaddr_in *sin;
    struct   ifreq ifr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
         printf("socket create failse...GetLocalIp!\n");
         return OPRT_COM_ERROR;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, g_net_dev, sizeof(ifr.ifr_name) - 1);

    if( ioctl( sock, SIOCGIFADDR, &ifr) < 0 ) {
         printf("ioctl error\n");
         close(sock);
         return OPRT_COM_ERROR;
    }

    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    strcpy(ip->ip,inet_ntoa(sin->sin_addr));
    close(sock);
printf("get net %s ip %s\n",g_net_dev, ip->ip);
    return OPRT_OK;
}

// Get the connection status of the port
// NOTICE: high frequently called
bool tuya_hal_wired_station_conn(VOID)
{
    int sock;
    struct   sockaddr_in *sin;
    struct   ifreq ifr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
         printf("socket create failse...GetLocalIp!\n");
         goto err;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, g_net_dev, sizeof(ifr.ifr_name) - 1);

    if(ioctl(sock,SIOCGIFFLAGS,&ifr) < 0) {
        printf("ioctl error\n");
        close(sock);
       goto err;
    }
    close(sock);

    if(0 == (ifr.ifr_flags & IFF_UP)) { 
        goto err;
    }  

    return TRUE;

err:
    printf("station_conn error\n");

return FALSE;
    
}

/* If the gateway is in wifi+wired mode, the user needs to implement the wifi connection function.
When the network is deployed, the app will pass the ssid and passwd of the specified route.*/
// SDK will be called automatically
OPERATE_RET tuya_hal_wired_wifi_set_station_connect(IN CONST CHAR_T *ssid,IN CONST CHAR_T *passwd)
{
    return OPRT_COM_ERROR;
}

/* If the gateway is in wifi+wired mode, it returns true. At this time, 
the app will call “hwl_bnw_set_station_connect” to pass the ssid and passwd of the specified route.*/
bool tuya_hal_wired_wifi_need_cfg(VOID)
{
    return FALSE;
}

/* If the gateway is in wifi+wired mode, the interface returns the actual connection signal 
value of wifi and router, which needs to be implemented by the user.*/
OPERATE_RET tuya_hal_wired_wifi_station_get_conn_ap_rssi(OUT SCHAR_T *rssi)
{
    *rssi = 99;

    return OPRT_OK;
}

// get MAC of device
OPERATE_RET tuya_hal_wired_get_mac(OUT NW_MAC_S *mac)
{
    int sock;
    struct   sockaddr_in *sin;
    struct   ifreq ifr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
         printf("socket create failse...GetLocalIp!\n");
         return OPRT_COM_ERROR;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, g_net_dev, sizeof(ifr.ifr_name) - 1);

    if(ioctl(sock,SIOCGIFHWADDR,&ifr) < 0) {
        printf("ioctl error errno\n");
        close(sock);
        return FALSE;
    }
    memcpy(mac->mac,ifr.ifr_hwaddr.sa_data,sizeof(mac->mac));

    printf("WIFI Get MAC %02X-%02X-%02X-%02X-%02X-%02X\r\n",
             mac->mac[0],mac->mac[1],mac->mac[2],mac->mac[3],mac->mac[4],mac->mac[5]);
    close(sock);

    return OPRT_OK;
}

// No need to implement
OPERATE_RET tuya_hal_wired_set_mac(IN CONST NW_MAC_S *mac)
{
    if(NULL == mac) {
        return OPRT_INVALID_PARM;
    }
    printf("WIFI Set MAC\r\n");

    return OPRT_OK;
}

int tuya_hal_wired_if_connect_internet(bool *status)
{
	return OPRT_OK;
}


