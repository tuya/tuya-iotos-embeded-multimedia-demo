/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    tuya_ipc_p2p_demo
**********************************************************************************/
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include  "tuya_ipc_api.h"
#include "tuya_ipc_low_power_api.h"
/*
---------------------------------------------------------------------------------
Low power access reference code
en:TRUE is sleep      FALSE is wake
---------------------------------------------------------------------------------
*/

#define MAXBUF 512

OPERATE_RET TUYA_APP_LOW_POWER_START(char * devbuf,char *keybuf,int ip,int port)
{

    //TODO
//    //Report sleep status to tuya
//    ret = tuya_ipc_dp_report(NULL, TUYA_DP_DOOR_STATUS,PROP_BOOL,&doorStat,1);
//    if (OPRT_OK != ret){
//        //This dp point is very important. The user should repeat call the report interface until the report is successful when it is fail.
//        //If it is failure continues, the user needs to check the network connection.
//        PR_ERR("dp report failed");
//        return ret;
//    }
    int ret=1;
    while(ret!=0)
    {
        ret =tuya_ipc_low_power_server_connect(ip,port,devbuf,strlen(devbuf),keybuf,strlen(keybuf));
    }

    printf("hello lowpower\n");

    int low_power_socket =-1;

    while(low_power_socket == -1)
    {
       low_power_socket= tuya_ipc_low_power_socket_fd_get();
    }

    char wakeData[36]={0};
    int wake_data_len=sizeof(wakeData);
    tuya_ipc_low_power_wakeup_data_get(wakeData,&wake_data_len);
    int i=0;
    printf("wake up date is { ");
    for(i=0;i<wake_data_len;i++)
    {
        printf("0x%x ",wakeData[i]);
    }
    printf(" }\n");

    char heart_beat[12]={0};
    int heart_beat_len=sizeof(heart_beat);
    tuya_ipc_low_power_heart_beat_get(heart_beat,&heart_beat_len);
    printf("heart beat data is { ");

    for(i=0;i<heart_beat_len;i++)
    {
        printf("0x%x ",heart_beat[i]);
    }
    printf(" }\n");


    fd_set rfds;
    struct timeval tv;
    int retval, maxfd = -1;

    int len=0;
    char recBuf[MAXBUF]={0};
    int heart_timeout=5;
    int user_set_timeout=10;
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
          printf("Will exit and the select is error! %s", strerror(errno));
          break;
        }
        else if (retval == 0)
        {
//            timeout++;
//
////            if(timeout*tv.tv_sec<=user_set_timeout)
////            {
////                printf("============not send heart beat============== %d,timeout*tv.tv_sec=%d timeout*tv.tv_sec=%d\n",timeout,timeout*tv.tv_sec,timeout*tv.tv_sec);
////                continue;
////            }
//          timeout=0;
          printf("============send heart beat==============\n");
          len = send(low_power_socket, heart_beat, heart_beat_len, 0);
          if (len < 0)
          {
              printf("socket =%d %d\n",low_power_socket,errno);
//            printf("Message '%s' failed to send ! \
//                  The error code is %d, error message '%s'\n",
//                  low_power_socket,heart_beat, errno, strerror(errno));
            break;
          }
          else
          {
            printf("News: %d \t send, sent a total of %d bytes!\n",
                    heart_beat_len, len);
          }

          continue;
        }
        else
        {
            if (FD_ISSET(low_power_socket, &rfds))
            {
              bzero(recBuf, MAXBUF+1);
              printf("============recv data==============\n");
              len = recv(low_power_socket, recBuf, MAXBUF, 0);
              if (len > 0)
              {
                  printf("Successfully received the message: is {");
                  for(i=0;i<len;i++)
                      printf("0x%02x ",recBuf[i]);
                  printf("}\n");
                  if(strncmp(recBuf,wakeData,wake_data_len)==0)
                  {
                      //TODO  启动SDK
                      printf("recve data is wake up\n");
                  }

              }
              else
              {
                if (len < 0)
                    printf("Failed to receive the message! \
                          The error code is %d, error message is '%s'\n",
                          errno, strerror(errno));
                else
                    printf("Chat to terminate len=0x%x!\n",len);

                break;
              }
            }

        }
    }

    return 0;

}
