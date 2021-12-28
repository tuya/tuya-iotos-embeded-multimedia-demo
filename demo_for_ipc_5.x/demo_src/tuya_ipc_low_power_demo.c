/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_p2p_demo
**********************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include  "tuya_ipc_api.h"
#include "tuya_ipc_low_power_api.h"
#include "tuya_ipc_low_power_demo.h"
/*
---------------------------------------------------------------------------------
Low power access reference code
en:TRUE is sleep      FALSE is wake
---------------------------------------------------------------------------------
*/

#define PR_ERR(fmt, ...)    printf("Err:"fmt"\r\n", ##__VA_ARGS__)
#define PR_DEBUG(fmt, ...)  printf("Dbg:"fmt"\r\n", ##__VA_ARGS__)
#define PR_TRACE
#define MAXBUF 512

OPERATE_RET TUYA_APP_LOW_POWER_START(CHAR_T *devbuf,CHAR_T *keybuf,INT_T ip,INT_T port)
{
    INT_T ret=1;
    INT_T i=0;
    INT_T low_power_socket =-1;
    CHAR_T wakeData[36] = {0};
    INT_T wake_data_len = SIZEOF(wakeData);

    while(ret!=0)
    {
        ret = tuya_ipc_low_power_server_connect(ip, port, devbuf, strlen(devbuf), keybuf, strlen(keybuf));
    }

    PR_DEBUG("power_server_connect over.\n");

    while(low_power_socket == -1)
    {
       low_power_socket= tuya_ipc_low_power_socket_fd_get();
    }

    tuya_ipc_low_power_wakeup_data_get(wakeData, &wake_data_len);
    
    PR_DEBUG("wake up date is { ");
    for(i=0;i<wake_data_len;i++)
    {
        PR_DEBUG("0x%x ",wakeData[i]);
    }
    PR_DEBUG(" }\n");

    CHAR_T heart_beat[12] = {0};
    INT_T heart_beat_len = SIZEOF(heart_beat);
    tuya_ipc_low_power_heart_beat_get(heart_beat,&heart_beat_len);
    PR_DEBUG("heart beat data is { ");

    for(i=0;i<heart_beat_len;i++)
    {
        PR_DEBUG("0x%x ",heart_beat[i]);
    }
    PR_DEBUG(" }\n");

    fd_set rfds;
    struct timeval tv;
    INT_T retval, maxfd = -1;

    INT_T len=0;
    CHAR_T recBuf[MAXBUF]={0};
    INT_T heart_timeout=5;
    INT_T user_set_timeout=10;
    while(1)
    {
        FD_ZERO(&rfds);
        FD_SET(0,&rfds);
        maxfd=0;
        FD_SET(low_power_socket,&rfds);
        if (low_power_socket > maxfd)
        {
          maxfd = low_power_socket;
        }
        tv.tv_sec = user_set_timeout;//default 10 seconds;
        tv.tv_usec = 0;

        retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if (retval == -1)
        {
          PR_DEBUG("Will exit and the select is error! %s", strerror(errno));
          break;
        }
        else if (retval == 0)
        {
          PR_DEBUG("============send heart beat==============\n");
          len = send(low_power_socket, heart_beat, heart_beat_len, 0);
          if (len < 0)
          {
              PR_DEBUG("socket =%d %d\n",low_power_socket,errno);
              break;
          }
          else
          {
            PR_DEBUG("News: %d \t send, sent a total of %d bytes!\n",
                    heart_beat_len, len);
          }

          continue;
        }
        else
        {
            if (FD_ISSET(low_power_socket, &rfds))
            {
              bzero(recBuf, MAXBUF+1);
              PR_DEBUG("============recv data==============\n");
              len = recv(low_power_socket, recBuf, MAXBUF, 0);
              if (len > 0)
              {
                  PR_DEBUG("Successfully received the message: is {");
                  for(i=0;i<len;i++)
                      PR_DEBUG("0x%02x ",recBuf[i]);
                  PR_DEBUG("}\n");
                  if(strncmp(recBuf,wakeData,wake_data_len)==0)
                  {
                      //TODO  启动SDK
                      PR_DEBUG("recve data is wake up\n");
                  }

              }
              else
              {
                if (len < 0)
                    PR_DEBUG("Failed to receive the message! \
                          The error code is %d, error message is '%s'\n",
                          errno, strerror(errno));
                else
                    PR_DEBUG("Chat to terminate len=0x%x!\n",len);

                break;
              }
            }

        }
    }

    return 0;
}

VOID tuya_ipc_low_power_sample()
{
//            //this device info get from tuya ipc SDK
//        //    char devbuf[]="6c44bbd5972e2a992funl2";
//        //    char keybuf[]="4d7fc735ccac2cae";
//            char devbuf[]="6cce58037f56937765kwqg";
//            char keybuf[]="9c4fc747f148c052";
//           // char devbuf[]="6cb88c4fb06aaf7cbewszc";
//           // char keybuf[]="89efed934616cc39";
//            //int ip = 0xd4402e47;//yufa
//            int ip = 0xaf188c48;
//            int port =443;
  int ip=0;
  int port=0;
  int ret = tuya_ipc_low_power_server_get(&ip, &port);
  if(ret != 0)
  {
      PR_ERR("get low power ip  error %d\n",ret);
      return;
  }
  #define COMM_LEN 30
  char devid[COMM_LEN]={0};
  int id_len=COMM_LEN;
  ret = tuya_ipc_device_id_get(devid, &id_len);
  if(ret != 0)
  {
      PR_ERR("get devide error %d\n",ret);
      return;
  }
  char local_key[COMM_LEN]={0};
  int key_len=COMM_LEN;
  ret = tuya_ipc_local_key_get(local_key, &key_len);
  if(ret != 0)
  {
      PR_ERR("get local key  error %d\n",ret);
      return;
  }

  TUYA_APP_LOW_POWER_START(devid,local_key,ip,port);  
  return;
}