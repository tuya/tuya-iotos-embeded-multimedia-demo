#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "tuya_fac_cmd.h"
#include "tuya_fac_media.h"
#include "tuya_fac_protocol.h"
#include "tuya_fac_test.h"

#ifndef NULL
#define NULL 0
#endif
#define BACKLOG 1

static bool start_flag = 0;
static bool verbose = true;
static char header[3] = { 0x55, 0xaa, 0x00 };
static char client_addr[32];
int com_fd = -1;
unsigned char cur_cmd = -1;

/* Readme:
 * 命令字是以frame的形式来传递的，frame格式如下
 * 格式: 帧头(2bytes) + 版本(1byte) + 命令字(1byte) + 数据长度(2bytes)+数据内容(x bytes) + checksum(1bytes)
 * 帧头 ： 固定为0x55aa
 * 版本：  升级扩展用
 * 命令字： 具体帧类型
 * 数据长度：如果是json格式的字符串则不包括字符串的结束符
 * 数据： x bytes
 * checksum: 从帧头累加到数据字段的最后一个字节再对256求余
 */

/******************************************Json处理*******************************************/
/*
    函数名：
        static unsigned char tuya_get_input(unsigned char *buffer)
    函数说明：
        从串口获取数据，若收到tuya产测帧，将其存储在入参buffer中，并返回1
    入参：
        buffer:串口获取的产测帧将存储在其中
    返回值：
        TuyaTestFrame*：成功获取一个封装好的产测帧
        NULL：失败

*/
static TuyaTestFrame *tuya_get_valid_frame(unsigned char *buffer, int size)
{
    unsigned int datalen = 0, datasum = 0;
    unsigned char checksum;
    int i = 0, offset = 0;
    ;

    // 1,确保传输的前三位是协议头部
    while (1) {
        /*最短7 bytes*/
        if (size - i < 7) {
            // TYDEBUG("data not enough:%d_%d\n",size,i);
            return NULL;
        }
        if (memcmp(buffer + i, header, 3) == 0)
            break;
        i++;
    }
    offset = i;
    if (verbose)
        TYDEBUG("header in buffer offset is %d\n", offset);
    i += 4;
    /*2 ,读取命令字和数据长度*/
    datalen = (buffer[i] << 8) + (buffer[i + 1] << 0);    // 16bit datalen
    i += 2;

    TYDEBUG("tuya_get_input datalen %d \n", datalen);

    /*根据数据长度读取数据内容*/
    if (datalen > TUYA_FRAME_MAX_LEN || offset + 7 + datalen < size)    //长度不足
    {
        TYDEBUG("datalen [%d] is not valid , size [%d]\n", datalen, size);
        return NULL;
    }

    /*读取checksum*/
    checksum = buffer[i + datalen];

    /*累加校验和,检验checksum        2+1+1+2=6*/
    for (i = 0; i < datalen + 6; i++) {
        if (verbose)
            printf("add 0x%x\n", buffer[i]);
        datasum += (unsigned char)buffer[i];
    }
    //    datasum %= 256;
    if ((datasum & 0xff) != checksum) {
        printf("datasum [ 0x%x ]  !=  checksum [ 0x%x ]\n", datasum, checksum);
        return NULL;
    }

    //将buffer转化成TuyaTestFrame
    TuyaTestFrame *frame = (TuyaTestFrame *)malloc(sizeof(TuyaTestFrame));
    if (frame == NULL) {
        TYDEBUG("malloc err:%s\n", strerror(errno));
        return NULL;
    }
    memset(frame, 0, sizeof(TuyaTestFrame));
    memcpy(frame, buffer + offset, 6);
    frame->check_sum[0] = checksum;
    frame->data = (unsigned char *)malloc((datalen + 1) * sizeof(unsigned char));
    memset(frame->data, 0, (datalen + 1));
    memcpy(frame->data, buffer + offset + 6, datalen);
    //    if(datalen)
    {
        printf("%s , data hex:\n", frame->data);
        for (i = 0; i < datalen; i++)
            printf("0x%x \n", frame->data[i]);
        printf("/n");
    }
    return frame;
}

/*
    函数名：
        static void tuya_put_out(unsigned char *buf,unsigned int len)
    函数说明：
        从串口输出长度为len的buf
    入参：
        buf: 要输出的字符串
        len：字符串的长度
    返回值：
        无
*/
static int tuya_put_out(int fd, char *buf, unsigned int len)
{
    if (buf != NULL && fd >= 0) {
        return write(fd, buf, len);
    }
    return -1;
}

void hex_dump(void *dat, int size)
{
    int i;
    unsigned char *data = (unsigned char *)dat;
    printf("Size is %d : \n", size);
    for (i = 0; i < size; i++)
        printf(" 0x%x\n", data[i]);
    printf("\n");
}
/*
    函数名：
        TuyaTestFrame * tuya_get_frame()
    函数说明：
        获取一个封装好的产测帧（TuyaTestFrame*），获取成功返回（TuyaTestFrame*），失败返回NULL
    入参：
        无
    返回值：
        TuyaTestFrame*：成功获取一个封装好的产测帧
        NULL：失败
*/
TuyaTestFrame *tuya_get_frame(int *client_fd)
{
    unsigned char tmp[TUYA_FRAME_MAX_LEN] = { 0x0 };
    TuyaTestFrame *frame;
    int ret;
    int fd = *client_fd;

    //读取数据
    bzero(tmp, sizeof(tmp));
    ret = read(fd, tmp, sizeof(tmp));
    if (ret < 0) {
        TYDEBUG("read error : %s\n", strerror(errno));
        return NULL;
    }
    if (ret == 0) {
        TYDEBUG("Close by Client\n");
        close(fd);
        *client_fd = -1;
        return NULL;
    }

    hex_dump(tmp, ret);

    /*检查数据是否合法*/
    frame = tuya_get_valid_frame(tmp, ret);
    if (!frame) {
        return NULL;
    }

    TYDEBUG("get one valid frame , cmd = 0x%x #@#\n", frame->command[0]);
    return frame;
}

/*
    函数名：
        void tuya_put_frame(int cmd,unsigned char *data, int size)
    函数说明：
        输出一个tuya产测帧
    入参：
        cmd： 产测命令
        data：产测数据
    返回值：
        无
*/

int tuya_put_frame(int cmd, char *data, int size)
{
    if (size > (TUYA_FRAME_MAX_LEN - sizeof(header))) {
        TYDEBUG("data size over frame max len\n");
        return -1;
    }
    char buffer[TUYA_FRAME_MAX_LEN] = { 0 };
    unsigned int datalen;
    unsigned char i = 0;

    datalen = size;
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer + i, header, sizeof(header));
    i += sizeof(header);

    buffer[i++] = cmd;
    buffer[i++] = (datalen >> 8) & 0xff;
    buffer[i++] = (datalen >> 0) & 0xff;

    int dataOffset = i;

    memcpy(buffer + i, data, datalen);

    int total = dataOffset + datalen + 1;

    unsigned int datasum = 0;
    for (i = 0; i < total - 1; i++)
        datasum += buffer[i];
    buffer[total - 1] = (datasum % 256) & 0xff;
    if (verbose) {
        printf("data sum is 0x%x\n", buffer[total - 1]);
    }

    return tuya_put_out(com_fd, buffer, total);
}

/*
    函数名：
        void tuya_free_frame(TuyaTestFrame *frame)
    函数说明：
        释放一个tuya产测帧（TuyaTestFrame *）
    入参：
        frame： tuya产测帧
    返回值：
        无
*/
void tuya_free_frame(TuyaTestFrame *frame)
{
    if (frame != NULL) {
        if (frame->frame_buffer != NULL)
            free(frame->frame_buffer);
        if (frame->data != NULL)
            free(frame->data);
        free(frame);
    }
    return;
}

int tuya_fac_query_cmd(int *cmd)
{
    TYDEBUG("cmd = 0x%x\n", cur_cmd);
    *cmd = (int)cur_cmd;

    return 0;
}

int tuya_fac_query_fd(int *fd)
{
    TYDEBUG("fd = %d\n", com_fd);
    *fd = com_fd;
    return 0;
}

/*
    函数名：
        void tuya_dispatch_cmd(int fd,TuyaTestFrame *frame)
    函数说明：
        执行测试命令
    入参：
        fd:套接字
        frame： tuya产测帧
    返回值：
        无
*/
void tuya_dispatch_cmd(int fd, TuyaTestFrame *frame)
{
    char buf[TUYA_FRAME_MAX_LEN] = { 0 };
    unsigned char cmd = frame->command[0];
    int ret = 0;
    char data[128] = { 0 };

    cur_cmd = frame->command[0];

    start_flag = true;
    TYDEBUG("put frame cmd = %d ,start_flag=%d\n", cmd, start_flag);

    if (start_flag == true) {
        switch (cmd) {
            case TUYATEST_MODE: /*获取产测帧,进入产测模式*/
                TYDEBUG("Enter Tuya Test Mode\n");
                if (tuya_test_mode((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"comProtocol\":\"1.0.0\",\"deviceType\":\"IPcamera\",\"writeMac\":\"true\"}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;
            case TUYATEST_R_MASTER_FIRMWARE: /*获取主固件指纹*/
                TYDEBUG("get fw info\n");
                if (tuya_get_version(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;
            case TUYATEST_R_SLAVE_FIRMWARE:
                TYDEBUG("get slave fw info\n");
                if (tuya_get_slave_version(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;
            case TUYATEST_W_CFGINFO: /*写入PID , UUID , AUTHKEY*/
                TYDEBUG("write cfg info\n");
                if (tuya_write_cfg((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_R_CFGINFO: /*读取本机PID , UUID , AUTHKEY*/
                TYDEBUG("read cfg info\n");
                if (tuya_read_cfg(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_W_BSN: /*写入BSN*/
                TYDEBUG("write bsn info\n");
                if (tuya_write_bsn((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_R_BSN: /*读取BSN*/
                TYDEBUG("read bsn info\n");
                if (tuya_read_bsn(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_W_SN: /*写入SN*/
                TYDEBUG("write sn info\n");
                if (tuya_write_sn((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_R_SN: /*读取SN*/
                TYDEBUG("read sn info\n");
                if (tuya_read_sn(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_W_MASTER_MAC: /*写入本机MAC*/
                TYDEBUG("write mac\n");
                if (tuya_write_mac((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_R_MASTER_MAC: /*读取本机MAC*/
                TYDEBUG("read mac\n");
                if (tuya_read_mac(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_BUTTON_TEST:    //按键测试
                TYDEBUG("press button\n");
                if (tuya_test_button() != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_LED_TEST:    // LED检测
                TYDEBUG("blink led\n");
                if (tuya_test_led((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_IPERF_TEST:    // iperf wifi速率检测
                TYDEBUG("run iperf\n");
                if (tuya_test_iperf(client_addr, (char *)frame->data) != 0) {
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                    break;
                } else
                    return;

            case TUYATEST_VIDEO_TEST:    //启动RTSP Server预览测试
                TYDEBUG("start rtsp server\n");
                if (tuya_test_video(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_IRCUT_TEST:    // IRCUT测试
                TYDEBUG("switch ircut\n");
                if (tuya_test_ircut((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_SPEAKER_TEST:    //喇叭测试
                TYDEBUG("play sound\n");
                if (tuya_test_speaker((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");

                break;

            case TUYATEST_MIC_TEST:    // 麦克风测试
                TYDEBUG("mic test\n");
                tuya_test_mic((char *)frame->data, client_addr);
                return;

            case TUYATEST_IRLED_TEST:    // 红外灯测试
                TYDEBUG("ir led\n");
                if (tuya_test_irled((char *)frame->data) == 0)
                    sprintf(buf, "{\"ret\":true}");
                else
                    sprintf(buf, "{\"ret\":false}");
                break;

            case TUYATEST_BLACK_TEST:    //  暗箱测试
                TYDEBUG("black test\n");
                if (tuya_test_black_video(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_MASTER_CAP:    //  WIFI测试
                TYDEBUG("WIFI strength test\n");
                if (tuya_test_wifi_strength(data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true,\"rssi\":\"%s\"}", data);
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;


            case TUYATEST_W_COUNTRY_CODE: /*写入国家码*/
                TYDEBUG("country code write test\n");
                if (tuya_write_cc((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_R_COUNTRY_CODE: /*读取国家码*/
                TYDEBUG("read country code\n");
                if (tuya_read_cc(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_MOTOR_TEST: /*电机测试*/
                TYDEBUG("motor test\n");
                if (tuya_test_motor() == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_PIR_TEST: /*人体检测模块测试*/
                TYDEBUG("pir test\n");
                if (tuya_test_pir() != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false,\"pirEvent\":[]}");
                break;

            case TUYATEST_DIGITAL_SENSOR:
                TYDEBUG("read light sensor\n");
                if (tuya_read_light_sensor() == 0)
                    snprintf(buf, sizeof(buf), "{\"sensorType\":\"light\",\"sensorID\":0,\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"sensorType\":\"light\",\"sensorID\":0,\"ret\":false}");
                break;

            case TUYATEST_GET_BATTERY_VALUE:
                TYDEBUG("get battery value\n");
                if (tuya_get_battery_value(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_W_BATTERY_INFO:
                TYDEBUG("set battery info\n");
                if (tuya_write_battery_info((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_R_BATTERY_INFO:
                TYDEBUG("set battery info\n");
                if (tuya_read_battery_info(buf) != 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_SET_WIFI_CFG:
                TYDEBUG("get wifi cfg\n");
                if (tuya_set_wifi_cfg((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false}");
                break;

            case TUYATEST_WIFI_FLAG:
                TYDEBUG("wifi flag\n");
                if (tuya_get_wifi_calibration_flag((char *)frame->data) == 0)
                    snprintf(buf, sizeof(buf), "{\"ret\":true,\"flagType\":\"wifi_rf\"}");
                else
                    snprintf(buf, sizeof(buf), "{\"ret\":false,\"flagType\":\"wifi_rf\"}");
                break;
            case TUYATEST_BOOL_GENERAL:
                TYDEBUG("bool general test \n");
                tuya_test_bool_general((char *)frame->data, buf, sizeof(buf));
                break;
            default:
                TYDEBUG("unknown command 0x%x\n", cmd);

                return;
                break;
        }
    }
    if (verbose)
        TYDEBUG("Send ACK : %s\n", buf);
    /*CMD ACK*/
    if (strlen(buf) != 0) {
        ret = tuya_put_frame(cmd, buf, strlen(buf));
    }
    TYDEBUG("put frame size = %d\n", ret);

    return;
}

/*
    函数名：
        void tuya_create_tcp_socket(int port)
    函数说明：
        创建TCP通讯
    入参：
        port:端口号
    返回值：
        无
*/
int tuya_create_tcp_socket(int port)
{
    int sock;
    struct sockaddr_in server; /* server's address information */

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        /* handle exception */
        TYDEBUG("socket() error. Failed to initiate a socket :%s\n", strerror(errno));
        return -1;
    }

    /* set socket option */
    int on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
        /* handle exception */
        TYDEBUG("setsockopt() error. Failed to set a socket :%s\n", strerror(errno));
        close(sock);
        return -1;
    }

    struct timeval timeout = { 6, 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr)) < 0) {
        /* handle exception */
        TYDEBUG("Bind() error : %s\n", strerror(errno));
        close(sock);
        return -1;
    }

    if (listen(sock, BACKLOG) == -1) {
        TYDEBUG("listen() error : %s\n", strerror(errno));
        close(sock);
        return -1;
    }

    return sock;
}

void *tycam_fac_media_check(void *arg)
{
    tuya_fac_media_loop();
    return (void *)0;
}

/*
    函数名：
        int tuya_fac_test_loop(void* phdl, char *path)
    函数说明：
        基础测试
    入参：
        句柄、路径

    返回值：
 */
int tuya_fac_test_loop(void *phdl, char *path)
{
    int ret;
    int listenfd = -1;
    int port = 8090;
    tuya_fac_inf_s *pfachdl = (tuya_fac_inf_s *)phdl;

    TYDEBUG("--------------------------\n");
    TYDEBUG("@ Start Factory Test @\n");
    TYDEBUG("--------------------------\n");

    listenfd = tuya_create_tcp_socket(port);
    if (listenfd < 0) {
        TYDEBUG("create tcp socket err\n");
        return -1;
    }

    tuya_test_path(path, verbose);

    /*
        获取产测帧，如果接收到了产测帧，根据产测帧中的命令字段执行不同的操作
        举了两个例子
        1.如果命令字段为0x00，则将命令字段和要返回的数据传给ty_handler->tuya_put_frame，ty_handler->tuya_put_frame会将
          这些组合成产测帧，通过串口发送出去
        2.如果命令字段为0x01，原理类似1
    */
    fd_set fds;
    struct timeval timeout;    // select等待3秒，3秒轮询，要非阻塞就置0

    pthread_t tid;
    if (pthread_create(&tid, NULL, tuya_fac_media_loop, NULL) < 0) {
        perror("pthread create tuya_fac_video_test error\n");
        return -1;
    }
    pthread_detach(tid);

    pfachdl->start = 1;

    while (pfachdl->start) {
        if (com_fd <= 0) {
            struct sockaddr_in client; /* client's address information */
            socklen_t addrlen;
            addrlen = sizeof(client);

            memset(client_addr, 0, sizeof(client_addr));
            TYDEBUG("Wait for client ...\n");
            com_fd = accept(listenfd, (struct sockaddr *)&client, &addrlen);
            if (com_fd < 0) {
                if (errno != EAGAIN)
                    TYDEBUG("fail to create socket port=%d :%d %s\n", port, errno, strerror(errno));
                usleep(30 * 1000);
                continue;
            }

            start_flag = false;
            strcpy(client_addr, inet_ntoa(client.sin_addr));
            TYDEBUG("client %s %d is connected !\n", client_addr, com_fd);
            TYDEBUG("Wait to Recv Cmd\n");
        }

        FD_ZERO(&fds);           //每次循环都要清空集合，否则不能检测描述符变化
        FD_SET(com_fd, &fds);    //添加描述符
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        ret = select(com_fd + 1, &fds, NULL, NULL, &timeout);
        if (ret < 0) {
            TYDEBUG("select error : %s\n", strerror(errno));
            close(com_fd);
            com_fd = -1;
        } else if (ret == 0) {
            TYDEBUG("wait for next cmd \n");
            // timeout
        } else {
            //测试sock是否可读，即是否网络上有数据
            if (FD_ISSET(com_fd, &fds)) {
                TYDEBUG("socket fd %d is readable \n", com_fd);
                TuyaTestFrame *frame = tuya_get_frame(&com_fd);    //接受网络数据 ,并获取完整一帧
                if (frame) {
                    tuya_dispatch_cmd(com_fd, frame); /*执行相应命令字*/
                    tuya_free_frame(frame);
                }
            } else {
                TYDEBUG("FD_ISSET error\n");
            }
        }
    }

    close(com_fd);

    return 0;
}

int tuya_fac_test_loop_exit(void *phdl)
{
    tuya_fac_inf_s *pfachdl = (tuya_fac_inf_s *)phdl;

    if (pfachdl == NULL) {
        return -1;
    }

    pfachdl->start = 0;

    return 0;
}
