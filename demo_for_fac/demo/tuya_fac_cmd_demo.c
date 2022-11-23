#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cJSON.h"
#include "tuya_fac_cmd.h"
#include "tuya_fac_cmd_demo.h"
#include "tuya_fac_media_demo.h"
#include "tuya_fac_test.h"

tuya_FacCfg_t g_info;

static char filepath[128];
static int debug = 0;
char tcp_addr[32] = { 0 };

extern int com_fd;    // socket

#define FIRMWARE "tuya_fac_firmware"

#define LIGHT_WHITE_FLASH "light_white_flash"
#define LIGHT_WHITE_OFF   "light_white_off"

typedef enum
{
    TUYATEST_ERROR_PARAM_FAIL = 500000,
    TUYATEST_ERROR_DATA_LEN_FAIL = 500004,
    TUYATEST_ERROR_CRC_FAIL = 500007,
    TUYATEST_ERROR_JSON_FAIL = 500010,
    TUYATEST_ERROR_DATA_NULL = 500011,
} TUYATEST_ERROR_NUM;

static void _close_all_fds(void)
{
    int i;
    for (i = 0; i < sysconf(_SC_OPEN_MAX); i++) {
        if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO)
            close(i);
    }
}

static int _system(char *command)
{
    int pid = 0;
    int status = 0;
    char *argv[4];
    extern char **environ;    //环境变量，外部引用

    if (NULL == command)
        return -1;

    pid = vfork(); /* vfork() also works */
    if (pid < 0) {
        TYDEBUG("[line:%d] cmd %s error %s\n", __LINE__, command, strerror(errno));
        return -1;
    }
    if (0 == pid) {       /* child process */
        _close_all_fds(); /* 用来关闭所有继承的文件描述符*/
        argv[0] = "sh";
        argv[1] = "-c";
        argv[2] = command;
        argv[3] = NULL;

        if (-1 == execve("/bin/sh", argv, environ)) /* execve() also an implementation of exec() */
            TYDEBUG("[line:%d] cmd %s error %s\n", __LINE__, command, strerror(errno));
        _exit(127);
    }
    // else
    /* wait for child process to start */
    do {
        if (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR && errno != ECHILD) {
                TYDEBUG("[line:%d] cmd %s , error %d:%s\n", __LINE__, command, errno, strerror(errno));
                return -1;
            } else {
                return status;
            }
        }
    } while (1);

    return 0;
}

#define TMP_RESULT "/tmp/.result"
/*
 *替代system()函数
 */
int tuya_exec_cmd(char *cmd, char *out, int size)
{
    int ret = -1;

    if (debug)
        TYDEBUG("exec cmd : %s ,size = %d\n", cmd, size);

    if (size != 0) {
        char tmp[128];
        memset(tmp, 0, sizeof(tmp));
        snprintf(tmp, sizeof(tmp), "%s > %s", cmd, TMP_RESULT);
        ret = _system(tmp);
        if (ret == 0) {
            FILE *fp = NULL;
            fp = fopen(TMP_RESULT, "rb");
            if (fp) {
                fread(out, 1, size, fp);
                fclose(fp);
            }
        }
        remove(TMP_RESULT);
        return ret;
    }
    ret = _system(cmd);
    return ret;
}

/*去除字符串中的回车符*/
void remove_enter(char *str, int size)
{
    int i;
    for (i = 0; i < size; i++)
        if (str[i] == '\n')
            str[i] = '\0';
}

/*忽略大小写比较*/
int cmd_strcasecmp(const char *s1, const char *s2)
{
    if (!s1) {
        return (s1 == s2) ? 0 : 1;
    }
    if (!s2) {
        return 1;
    }
    for (; tolower(*s1) == tolower(*s2); ++s1, ++s2)
        if (*s1 == 0)
            return 0;
    return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

//测试SD卡读写功能
int tuya_test_sd_write()
{
    //用户自行实现，以下是样例
    /*******************************************************************************
    char filename[100];
    char test_buffer[] = "hello world!";
    char test_read[20] = {0};
    snprintf(filename, sizeof(filename), "/mnt/sdcard/sd_test.txt");
    FILE *test_fp = fopen(filename, "w+");
    TYDEBUG("sizeof(dataPtr) = %d\n",sizeof(test_buffer));
    if(test_fp)
    {
        int ret_w = fwrite(test_buffer,1,sizeof(test_buffer),test_fp);// 返回值为13
        TYDEBUG("tuya_sd_write = %d\n",ret_w);
        fseek(test_fp,0,SEEK_SET);
        int ret_r = fread(test_read,1,sizeof(test_buffer),test_fp);// 返回值为13
        TYDEBUG("tuya_sd_read = %d\n",ret_r);

        fclose (test_fp);

        if(ret_r == sizeof(test_buffer))
            return 0;
        else
            return -1;

    }
    else
        return -1;
    ********************************************************************************/

    return 0;
}

//测试设备可读写分区功能
int tuya_test_rw_partition()
{
    //用户自行实现，以下是样例
    /*******************************************************************************
    char filename[100];
    char test_buffer[] = "hello world!";
    char test_read[20] = {0};
    snprintf(filename, sizeof(filename), "/etc/tuya/mtd_zone_test.txt");
    FILE *test_fp = fopen(filename, "w+");
    TYDEBUG("sizeof(dataPtr) = %d\n",sizeof(test_buffer));
    if(test_fp)
    {
        int ret_w = fwrite(test_buffer,1,sizeof(test_buffer),test_fp);// 返回值为13
        TYDEBUG("tuya_mtd_zone_write = %d\n",ret_w);
        fseek(test_fp,0,SEEK_SET);
        int ret_r = fread(test_read,1,sizeof(test_buffer),test_fp);// 返回值为13
        TYDEBUG("tuya_mtd_zone_read = %d\n",ret_r);

        fclose (test_fp);

        if(ret_r == sizeof(test_buffer))
            return 0;
        else
            return -1;

    }
    else
        return -1;
    ********************************************************************************/

    return 0;
}

/* 进入/退出产测
 * input :  data   上报内容
 * return : 0--Success , -1 --Fail
 */
int tuya_test_mode(char *str)
{
    //用户实现进入/退出产测的操作，以下是样例
    if (str != NULL) {
        cJSON *root = cJSON_Parse(str);
        if (!root) {
            TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
            return -1;
        }

        cJSON *obj = cJSON_GetObjectItem(root, "action");
        if (!obj) {
            TYDEBUG("Can't Find 'action' in string :\n%s\n", str);
            cJSON_Delete(root);
            return -1;
        }
        TYDEBUG("tuya_test_mode: %s \n", obj->valuestring);
        cJSON_Delete(root);
    }
    return 0;
}

/* 读取当前固件版本号
 * input :  data   上报内容
 * return : 0--Success , -1 --Fail
 */
int tuya_get_version(char *data)
{
    //用户实现设备固件版本号的获取，以下是样例
    char *version = "1.0.0";
    char FIRMNAME[32] = "IPC_001";
    sprintf(data, "{\"ret\":true,\"firmName\":\"%s\",\"firmVer\":\"%s\"}", FIRMNAME, version);
    TYDEBUG("tuya_get_version: %s \n", data);

    return 0;
}

/* 读取从固件版本号
 * input :  data   上报内容
 * return : 0--Success , -1 --Fail
 */
int tuya_get_slave_version(char *data)
{
    //用户实现设备固件版本号的获取，以下是样例
    char *version = "2.0.0";
    char FIRMNAME[32] = "IPC_002";
    sprintf(data, "{\"ret\":true,\"firmName\":\"%s\",\"firmVer\":\"%s\"}", FIRMNAME, version);
    TYDEBUG("tuya_get_slave_version: %s \n", data);

    return 0;
}

/* 画面翻转*/
int tuya_video_flip()
{
    //用户实现画面翻转
    TYDEBUG("tuya_video_flip start\n");

    return 0;
}

/* 指示灯开关 */
int tuya_test_led(char *str)
{
    //用户实现指示灯开关功能
    TYDEBUG("tuya_test_led start\n");

    return 0;
}

/* 按键测试 */
int tuya_test_button()
{
    //用户实现按键触发事件
    TYDEBUG("tuya_test_button start\n");
    char buf[64] = { 0 };
    snprintf(buf, sizeof(buf), "{\"ret\":true,\"keyEvent\":\"key0\"}");
    TYDEBUG("key0_down info: %s\n", buf);
    tuya_put_frame(TUYATEST_BUTTON_TEST, buf, strlen(buf));
    return 0;
}

int tuya_get_iperf_result(char *str, double *avr_v, double *max_v, double *min_v)
{
    double min = 0xffffffff, max = 0, avr = 0, sum = 0;
    int count = 0;
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON *err = cJSON_GetObjectItem(root, "error");
    if (err) {
        TYDEBUG("Find 'error' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }

    cJSON *interv = cJSON_GetObjectItem(root, "intervals");
    if (!interv) {
        TYDEBUG("can't Find 'intervals' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }
    int i;
    for (i = 0; i < cJSON_GetArraySize(interv); i++) {
        cJSON *sub = cJSON_GetArrayItem(interv, i);
        if (sub != NULL) {
            cJSON *sub1 = cJSON_GetObjectItem(sub, "sum");
            if (sub1) {
                cJSON *bits = cJSON_GetObjectItem(sub1, "bits_per_second");

                cJSON *sec = cJSON_GetObjectItem(sub1, "seconds");
                if (sec && bits) {
                    double bitrate = (bits->valuedouble / 1000);

                    if (debug)
                        printf("interval:%d : %f kbit/s\n", i, bitrate);
                    count++;
                    if (max < bitrate)
                        max = bitrate;
                    if (min > bitrate)
                        min = bitrate;
                    sum += bitrate;
                }
            }
        }
    }

    if (count)
        avr = sum / count;
    TYDEBUG("aver of [0-%d] : ", count - 1);
    if ((avr / (1000)) > 5)
        TYDEBUG("%.2lfMbit/s , ", avr / (1000));
    else
        TYDEBUG("%.2lfkbit/s , ", avr);

    if ((min / (1000)) > 5)
        TYDEBUG("min is %.2lfMbit/s , ", min / (1000));
    else
        TYDEBUG("min is %.2lfkbit/s , ", min);

    if ((max / (1000)) > 5)
        TYDEBUG("max is %.2lfMbit/s \n", max / (1000));
    else
        TYDEBUG("max is %.2lfkbit/s \n", max / (1000));

    *avr_v = avr;
    *max_v = max;
    *min_v = min;

    cJSON_Delete(root);
    return 0;
}

/*iperf测试*/
typedef struct tycam_test_iperf_handle_
{
    int start;
    pthread_t atid;
    char server_ip[32];
    int time;
} tycam_test_iperf_handle_s;

static tycam_test_iperf_handle_s iperf_pahdl = { 0 };

void *tycam_iperf_test_proc(void *arg)
{
    pthread_detach(pthread_self());

    char cmd[128] = { 0 };
    char *data = NULL;
    char iperf_tmp_file[128] = { 0 };
    char *iperf_file_path = "/tmp/fac";
    double avr = 0, max = 0, min = 0;
    char buf[256] = { 0 };
    int ret = -1;
    FILE *fp = NULL;

    /*执行结果以json 格式输出到文件中*/
    snprintf(iperf_tmp_file, sizeof(iperf_tmp_file), "%s/iperf_out.txt", iperf_file_path);
    remove(iperf_tmp_file);
    snprintf(cmd, sizeof(cmd), "%s/iperf3 -c %s -w 64k -t %d -i 1 -J >  %s", filepath, iperf_pahdl.server_ip, iperf_pahdl.time, iperf_tmp_file);
    /*涂鸦上位机启动iperf server端速度较慢，建议等待几秒*/
    sleep(1);
    ret = tuya_exec_cmd(cmd, NULL, 0);
    if (ret == -1) {
        TYDEBUG("cmd ret = %d\n", ret);
        goto err;
    }

    /*读取并解析json文件*/
    fp = fopen(iperf_tmp_file, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (debug)
            TYDEBUG("iperf file size %d\n", size);

        data = (char *)malloc(size * sizeof(char));
        if (data == NULL) {
            TYDEBUG("malloc err: %s\n", strerror(errno));
            ret = -1;
            goto err;
        }
        fread(data, 1, size, fp);
        fclose(fp);
        fp = NULL;

        if (debug)
            TYDEBUG("read from %s:%s\n", iperf_tmp_file, data);

        ret = tuya_get_iperf_result(data, &avr, &max, &min);
        if (ret != 0) {
            TYDEBUG("tuya_get_iperf_result err\n");
            ret = -1;
            goto err;
        }
        TYDEBUG("iperf done ret = %d!@\n", ret);
    } else {
        ret = -1;
        TYDEBUG("open file %s err:%s\n", iperf_tmp_file, strerror(errno));
        goto err;
    }

err:
    if (fp)
        fclose(fp);
    if (data)
        free(data);
    memset(&iperf_pahdl, 0, sizeof(tycam_test_iperf_handle_s));

    if (ret != -1) {
        snprintf(buf, sizeof(buf), "{\"ret\":true,\"bandwidth\":\"%.2lfMB/sec\",\"max\":\"%.2lfMB/sec\",\"min\":\"%.2lfMB/sec\"}", avr / 1000, max / 1000, min / 1000);
    } else {
        snprintf(buf, sizeof(buf), "{\"ret\":false}");
    }
    TYDEBUG("resp_buf:%s\n", buf);
    tuya_put_frame(TUYATEST_IPERF_TEST, buf, strlen(buf));

    tuya_exec_cmd("rm -rf /tmp/fac/iperf_out.txt", NULL, 0);
    iperf_pahdl.start = 0;

    return NULL;
}

/*
 * 吞吐量测试
 */
int tuya_test_iperf(char *addr, char *str)
{
    if (iperf_pahdl.start == 1) {
        TYDEBUG("iperf already running\n");
        return -1;
    }
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error to Parse str to json: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON *thresh = cJSON_GetObjectItem(root, "thresh");
    if (!thresh) {
        TYDEBUG("can't Find 'thresh' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }

    cJSON *server = cJSON_GetObjectItem(root, "server");
    if (!server) {
        TYDEBUG("can't Find 'server' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }
    strcpy(iperf_pahdl.server_ip, server->valuestring);

    if (strlen(iperf_pahdl.server_ip) == 0) {
        TYDEBUG("server ip is NULL\n");
        cJSON_Delete(root);
        return -1;
    }
    TYDEBUG("server ip:%s\n", iperf_pahdl.server_ip);

    cJSON *time = cJSON_GetObjectItem(root, "time");
    if (!time) {
        TYDEBUG("can't Find 'time' in string :\n%s\n", str);
        iperf_pahdl.time = 10;
    } else {
        iperf_pahdl.time = time->valueint;
    }
    TYDEBUG("time:%d\n", iperf_pahdl.time);
    cJSON_Delete(root);

    iperf_pahdl.start = 1;
    pthread_t tid;
    if (pthread_create(&tid, NULL, tycam_iperf_test_proc, tcp_addr) < 0) {
        TYDEBUG("pthread create fail! %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

/* 测试 WIFI强度 */
int tuya_test_wifi_strength(char *data)
{
    //用户实现wifi信号强度的获取，单位是Dbm，以下是样例
    int ret = 0;
    char *buffer = "-30";
    strcat(data, buffer);
    return ret;
}


/*rtsp 出图测试*/
int tuya_test_video(char *data)
{
    //用户实现出流或者启动RTSP
    TYDEBUG("tuya_test_video start\n");
    tuya_fac_media_start_rtsp();

    char buf[128] = { 0 };
    struct sockaddr_in ser_addr;
    socklen_t ser_len;
    ser_len = sizeof(ser_addr);
    getsockname(com_fd, (struct sockaddr *)&ser_addr, &ser_len);
    inet_ntop(AF_INET, &ser_addr.sin_addr, buf, sizeof(buf));
    TYDEBUG("host %s:%d\n", buf, ntohs(ser_addr.sin_port));

    sprintf(data, "{\"ret\":true,\"rtspUrl\":\"rtsp://%s:8554/stream0\"}", buf);
    TYDEBUG("tuya_test_video: %s \n", data);

    return 0;
}

/*rtsp 暗箱测试*/
int tuya_test_black_video(char *data)
{
    //用户实现出流或者启动RTSP
    TYDEBUG("tuya_test_black_video start\n");
    // tuya_fac_media_start_rtsp();
    char buf[128] = { 0 };
    struct sockaddr_in ser_addr;
    socklen_t ser_len;
    ser_len = sizeof(ser_addr);
    getsockname(com_fd, (struct sockaddr *)&ser_addr, &ser_len);
    inet_ntop(AF_INET, &ser_addr.sin_addr, buf, sizeof(buf));
    TYDEBUG("host %s:%d\n", buf, ntohs(ser_addr.sin_port));

    sprintf(data, "{\"ret\":true,\"rtspUrl\":\"rtsp://%s:8554/stream0\"}", buf);
    TYDEBUG("tuya_test_black_video: %s \n", data);

    return 0;
}

/*喇叭测试 cjson data is: {"sound":false} */
int tuya_test_speaker(char *str)
{

    cJSON *cJ_info;
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJ_info = cJSON_GetObjectItem(root, "sound");
    TYDEBUG("tuya_test_speaker comm: %d \n", cJ_info->valueint);
    cJSON_Delete(root);

    return 0;
}

/*麦克风测试*/
int tuya_test_mic(char *str, char *ipaddr)
{
    memset(tcp_addr, 0, sizeof(tcp_addr));
    strcpy(tcp_addr, ipaddr);
    pthread_t tid;
    if (pthread_create(&tid, NULL, tuya_fac_media_record, tcp_addr) < 0) {
        TYDEBUG("pthread create ty_fac_transport_file err:%s\n", strerror(errno));
        return -1;
    }

    return 0;
}

/*IRCUT 切换测试*/
int tuya_test_ircut(char *str)
{
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error to Parse str to json: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON *obj = cJSON_GetObjectItem(root, "ircutSwitch");
    if (!obj) {
        TYDEBUG("can't Find 'ircutSwitch' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }
    // time = obj->valueint;
    cJSON_Delete(root);

    //用户实现IRCUT切换,    obj->valuestring是上位机下传的次数，也可自行定义切换次数
    TYDEBUG("tuya_test_ircut start\n");
    //建议另起线程

    return 0;
}

/*红外灯测试*/
int tuya_test_irled(char *str)
{

    //用户实现红外灯开关
    TYDEBUG("tuya_test_ircut start\n");

    return 0;
}

/*读取PID*/
int tuya_read_pid(char *pid, int size)
{
    //用户实现数据读取
    int ret = 0;
    TYDEBUG("tuya_read_pid start\n");

    return ret;
}

/*读取UUID*/
int tuya_read_uuid(char *uuid, int size)
{
    //用户实现数据读取
    int ret = 0;
    TYDEBUG("tuya_read_uuid start\n");

    return ret;
}
/*读取KEY*/
int tuya_read_authkey(char *authkey, int size)
{
    //用户实现数据读取
    int ret = 0;
    TYDEBUG("tuya_read_authkey start\n");

    return ret;
}
/*写入PID*/
int tuya_write_pid(char *pid, int size)
{
    //用户实现数据写入
    int ret = 0;
    TYDEBUG("tuya_write_pid start\n");

    return ret;
}

/*写入UUID*/
int tuya_write_uuid(char *uuid, int size)
{
    //用户实现数据写入
    int ret = 0;
    TYDEBUG("tuya_write_uuid start\n");

    return ret;
}
/*写入KEY*/
int tuya_write_authkey(char *authkey, int size)
{
    //用户实现数据写入
    int ret = 0;
    TYDEBUG("tuya_write_authkey start\n");

    return ret;
}

/*提取Json字符串中的cfg信息*/
int parse_cfg_info(char *str, char *pid, char *uuid, char *authkey)
{
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON *obj = cJSON_GetObjectItem(root, "uuid");
    if (!obj) {
        TYDEBUG("Can't Find 'uuid' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }

    strcpy(uuid, obj->valuestring);

    cJSON *obj1 = cJSON_GetObjectItem(root, "auzkey");
    if (!obj1) {
        TYDEBUG("Can't Find 'authkey' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }

    strcpy(authkey, obj1->valuestring);

    cJSON *obj2 = cJSON_GetObjectItem(root, "pid");
    if (!obj2) {
        TYDEBUG("Can't Find 'pid' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }

    strcpy(pid, obj2->valuestring);

    cJSON_Delete(root);
    return 0;
}

/*写入配置信息*/
int tuya_write_cfg(char *data)
{
    char pid[128], uuid[512], authkey[512];
    bzero(pid, sizeof(pid));
    bzero(uuid, sizeof(uuid));
    bzero(authkey, sizeof(authkey));

    TYDEBUG("data is %s\n", data);
    if (parse_cfg_info(data, pid, uuid, authkey) != 0)
        return -1;
    TYDEBUG("pid : %s ,uuid : %s , authkey : %s\n", pid, uuid, authkey);

    if (tuya_write_pid(pid, strlen(pid)) != 0) {
        return -1;
    }
    if (tuya_write_uuid(uuid, strlen(uuid)) != 0) {
        return -1;
    }
    if (tuya_write_authkey(authkey, strlen(authkey)) != 0) {
        return -1;
    }

    return 0;
}

/*读取本机的配置信息*/
int tuya_read_cfg(char *data)
{
    //用户完成设备信息的读取,注意产测帧最大长度大小，防止越界
    char *pid = "123";
    char *uuid = "456";
    char *key = "789";
    sprintf(data, "{\"pid\":\"%s\",\"uuid\":\"%s\",\"auzkey\":\"%s\"}", pid, uuid, key);
    TYDEBUG("tuya_read_cfg: %s \n", data);

    return 0;
}

/*写入SN信息*/
int tuya_write_sn(char *str)
{
    char sn[128];
    bzero(sn, sizeof(sn));

    TYDEBUG("data is %s\n", str);
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON *obj = cJSON_GetObjectItem(root, "sn");
    if (!obj) {
        TYDEBUG("Can't Find 'sn' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }
    strcpy(sn, obj->valuestring);
    cJSON_Delete(root);
    TYDEBUG("sn : %s \n", sn);

    //用户完成设备SN号的写入

    return 0;
}

/*读取本机的SN信息*/
int tuya_read_sn(char *data)
{
    //用户完成设备SN号的读取
    char *sn = "123";
    sprintf(data, "{\"sn\":\"%s\"}", sn);
    TYDEBUG("tuya_read_sn: %s \n", data);

    return 0;
}

/* 写入BSN信息 */
int tuya_write_bsn(char *str)
{
    char bsn[128];
    bzero(bsn, sizeof(bsn));

    TYDEBUG("data is %s\n", str);
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON *obj = cJSON_GetObjectItem(root, "bsn");
    if (!obj) {
        TYDEBUG("Can't Find 'bsn' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }
    strcpy(bsn, obj->valuestring);
    cJSON_Delete(root);
    TYDEBUG("bsn : %s \n", bsn);

    //用户完成设备BSN号的写入

    return 0;
}

/* 读取本机的BSN信息 */
int tuya_read_bsn(char *data)
{
    //用户完成设备BSN号的读取
    char *bsn = "123";
    sprintf(data, "{\"bsn\":\"%s\"}", bsn);
    TYDEBUG("tuya_read_bsn: %s \n", data);

    return 0;
}

/* 读取本机MAC信息 */
int tuya_read_mac(char *data)
{
    //用户完成设备MAC号的读取
    char *mac = "123";
    sprintf(data, "{\"mac\":\"%s\"}", mac);
    TYDEBUG("tuya_read_mac: %s \n", data);

    return 0;
}

/* 写入本机MAC信息 */
int tuya_write_mac(char *str)
{
    char mac[32];
    bzero(mac, sizeof(mac));

    TYDEBUG("data is %s\n", str);
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON *obj = cJSON_GetObjectItem(root, "mac");
    if (!obj) {
        TYDEBUG("Can't Find 'uuid' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }
    strcpy(mac, obj->valuestring);
    cJSON_Delete(root);
    TYDEBUG("mac : %s \n", mac);

    char cmd[128] = { 0 };
    int ret = 0;
    char mac_tmp[32];
    int i, j;
    bzero(mac_tmp, sizeof(mac_tmp));
    for (i = 0, j = 0; i < strlen(mac); i++) {
        printf("i = %d, j = %d, %c\n", i, j, mac[i]);
        if (mac[i] != ':')
            mac_tmp[j++] = mac[i];
    }
    TYDEBUG("mac_tmp is --%s--\n", mac_tmp);
    char test[128] = { 0 };
    snprintf(test, sizeof(test), "%s/rtwpriv", filepath);
    if (access(test, F_OK) == 0)
        snprintf(cmd, sizeof(cmd), "%s/rtwpriv wlan0 efuse_set mac,%s", filepath, mac_tmp);
    else
        snprintf(cmd, sizeof(cmd), "rtwpriv wlan0 efuse_set mac,%s", mac_tmp);
    ret = tuya_exec_cmd(cmd, NULL, 0);
    if (ret != 0) {
        TYDEBUG("cmd ret = %d , %s\n", ret, strerror(errno));
        return ret;
    }

    return ret;
}

/*读取国家码*/
int tuya_read_cc(char *data)
{
    //用户自己实现国家码的读取，以下为样例
    char *buffer = "CC";
    strcat(data, "{\"ret\":true,\"country\":\"");
    strcat(data, buffer);
    strcat(data, "\"}");

    return 0;
}

/*写入国家码*/
int tuya_write_cc(char *str)
{
    char cc[64];
    int ret = 0;
    char s1[16] = "US";
    char s2[16] = "EU";
    char s3[16] = "JAPAN";
    bzero(cc, sizeof(cc));

    TYDEBUG("country code is %s\n", str);
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON *obj = cJSON_GetObjectItem(root, "country");
    if (!obj) {
        TYDEBUG("Can't Find 'cc' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }
    strcpy(cc, obj->valuestring);
    cJSON_Delete(root);
    TYDEBUG("country code: %s \n", cc);

    char test[128] = { 0 };
    if (cmd_strcasecmp(cc, s1) == 0) {
        snprintf(test, sizeof(test), "echo 0x00 > /proc/net/rtl8188fu/wlan0/chan_plan");
    } else if (cmd_strcasecmp(cc, s2) == 0) {
        snprintf(test, sizeof(test), "echo 0x4 > /proc/net/rtl8188fu/wlan0/chan_plan");
    } else if (cmd_strcasecmp(cc, s3) == 0) {
        snprintf(test, sizeof(test), "echo 0x9 > /proc/net/rtl8188fu/wlan0/chan_plan");
    } else {
        snprintf(test, sizeof(test), "echo 0x20 > /proc/net/rtl8188fu/wlan0/chan_plan");
    }
    ret = tuya_exec_cmd(test, NULL, 0);
    if (ret != 0) {
        TYDEBUG("cmd ret = %d , %s\n", ret, strerror(errno));
    }

    return ret;
}


/*电机测试*/
int tuya_test_motor()
{
    TYDEBUG("tuya_test_motor \n");

    return 0;
}
/*人体检测模块测试*/
int tuya_test_pir()
{
    //建议返回有效的人体检测模块个数(编号)
    TYDEBUG("tuya_test_pir \n");
    char buf[64] = { 0 };
    snprintf(buf, sizeof(buf), "{\"ret\": true,\"pirEvent\":[1,2]}");
    TYDEBUG("pir info: %s\n", buf);
    tuya_put_frame(TUYATEST_PIR_TEST, buf, strlen(buf));
    return 0;
}

/*光敏测试*/
int tuya_read_light_sensor()
{
    //反馈光敏是否生效
    TYDEBUG("tuya_read_light_sensor \n");

    return 0;
}

int tuya_get_battery_value(char *data)
{
    int val = 0;
    int ret = 0;
    TYDEBUG("get battery val = %d\n", val);
    if (ret < 0) {
        TYDEBUG("get battery falied, %d\n", ret);
        return -1;
    }

    sprintf(data, "{\"ret\":true,\"val\":%d}", val);

    return 0;
}

int tuya_read_battery_info(char *data)
{
    //用户完成设备SN号的读取
    char buffer[256] = { 0 };
    int len = sizeof(buffer);
    char cmd[128] = { 0 };

    snprintf(cmd, sizeof(cmd), "nvram get FACTORY");
    tuya_exec_cmd(cmd, buffer, len);
    remove_enter(buffer, sizeof(buffer));
    strcat(data, "{\"ret\":true,\"factory\":\"");
    strcat(data, buffer);

    bzero(cmd, sizeof(cmd));
    bzero(buffer, sizeof(buffer));
    snprintf(cmd, sizeof(cmd), "nvram get TYPE");
    tuya_exec_cmd(cmd, buffer, len);
    remove_enter(buffer, sizeof(buffer));
    strcat(data, "\",\"type\":\"");
    strcat(data, buffer);

    bzero(cmd, sizeof(cmd));
    bzero(buffer, sizeof(buffer));
    snprintf(cmd, sizeof(cmd), "nvram get CAPACITY");
    tuya_exec_cmd(cmd, buffer, len);
    remove_enter(buffer, sizeof(buffer));
    strcat(data, "\",\"capacity\":");
    strcat(data, buffer);

    bzero(cmd, sizeof(cmd));
    bzero(buffer, sizeof(buffer));
    snprintf(cmd, sizeof(cmd), "nvram get INFO");
    tuya_exec_cmd(cmd, buffer, len);
    remove_enter(buffer, sizeof(buffer));
    strcat(data, ",\"info\":\"");
    strcat(data, buffer);
    strcat(data, "\"}");

    return 0;
}

int tuya_write_battery_info(char *str)
{
    char buf[256] = { 0 };
    int value = 0;

    TYDEBUG("data is %s\n", str);
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }
    /*******************************************************/
    cJSON *obj_factory = cJSON_GetObjectItem(root, "factory");
    if (!obj_factory) {
        TYDEBUG("Can't Find 'factory' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }

    bzero(buf, sizeof(buf));
    strcpy(buf, obj_factory->valuestring);
    remove_enter(buf, sizeof(buf));
    TYDEBUG("buf : %s \n", buf);

    /*******************************************************/
    cJSON *obj_type = cJSON_GetObjectItem(root, "type");
    if (!obj_type) {
        TYDEBUG("Can't Find 'type' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }

    bzero(buf, sizeof(buf));
    strcpy(buf, obj_type->valuestring);
    remove_enter(buf, sizeof(buf));
    TYDEBUG("buf : %s \n", buf);

    /*******************************************************/
    cJSON *obj_capacity = cJSON_GetObjectItem(root, "capacity");
    if (!obj_capacity) {
        TYDEBUG("Can't Find 'capacity' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }
    value = obj_capacity->valueint;
    TYDEBUG("value:%d \n", value);

    /*******************************************************/
    cJSON *obj_info = cJSON_GetObjectItem(root, "info");
    if (!obj_info) {
        TYDEBUG("Can't Find 'info' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }

    bzero(buf, sizeof(buf));
    strcpy(buf, obj_info->valuestring);
    remove_enter(buf, sizeof(buf));
    TYDEBUG("buf : %s \n", buf);

    cJSON_Delete(root);


    //用户完成数据写入

    return 0;
}

int tuya_get_wifi_calibration_flag(char *str)
{
    //用户判断wifi校准是否成功
    return 0;
}

int tuya_set_wifi_cfg(char *str)
{
    char ssid[32 + 1] = { 0 };
    char pwd[64 + 1] = { 0 };

    TYDEBUG("data is %s\n", str);
    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }
    /*******************************************************/
    cJSON *obj_ssid = cJSON_GetObjectItem(root, "ssid");
    if (!obj_ssid) {
        TYDEBUG("Can't Find 'ssid' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }

    strcpy(ssid, obj_ssid->valuestring);
    remove_enter(ssid, sizeof(ssid));
    TYDEBUG("ssid : %s \n", ssid);

    /*******************************************************/
    cJSON *obj_pwd = cJSON_GetObjectItem(root, "pwd");
    if (!obj_pwd) {
        TYDEBUG("Can't Find 'pwd' in string :\n%s\n", str);
        cJSON_Delete(root);
        return -1;
    }

    strcpy(pwd, obj_pwd->valuestring);
    remove_enter(pwd, sizeof(pwd));
    TYDEBUG("pwd : %s \n", pwd);

    cJSON_Delete(root);

    //用户完成联网操作

    return 0;
}

int tuya_set_light_white_flash(void)
{
    //用户设置白灯闪
    return 0;
}

int tuya_set_light_white_close(void)
{
    //用户设置白灯关闭
    return 0;
}

/*布尔类型通用测试
1.解析{“testItem”:"xxx","paras":[{"key":"xx","value":"xx"},{"key":"xx","value":"xx"}]}
paras字段为可选字段，若不需要参数，则不用下发paras字段信息
2.返回{“testItem”:"xxx",“ret“:true }/{“ret“:false,"errCode":xx," errMsg":"xxx"}
*/
int tuya_test_bool_general(char *str, char *ret_buf, int buf_size)
{
    int ret = -1;

    memset(ret_buf, 0, buf_size);

    cJSON *root = cJSON_Parse(str);
    if (!root) {
        TYDEBUG("Error before: [%s]\n", cJSON_GetErrorPtr());
        ret = TUYATEST_ERROR_JSON_FAIL;
        goto ERR_RET;
    }

    cJSON *obj_testItem = cJSON_GetObjectItem(root, "testItem");
    if (!obj_testItem) {
        TYDEBUG("Can't Find 'ssid' in string :\n%s\n", str);
        cJSON_Delete(root);
        ret = TUYATEST_ERROR_PARAM_FAIL;
        goto ERR_RET;
    }

    if (strcmp(obj_testItem->valuestring, LIGHT_WHITE_FLASH) == 0) {
        tuya_set_light_white_flash();
    } else if (strcmp(obj_testItem->valuestring, LIGHT_WHITE_OFF) == 0) {
        tuya_set_light_white_close();
    } else {
        ret = TUYATEST_ERROR_PARAM_FAIL;
        TYDEBUG("dont support [%s]\n", obj_testItem->valuestring);
        goto ERR_RET;
    }

    snprintf(ret_buf, buf_size, "{\"testItem\":\"%s\",\"ret\":true}", obj_testItem->valuestring);
    TYDEBUG("obj_testItem->valuestring %s\n", obj_testItem->valuestring);

    cJSON_Delete(root);
    return 0;
ERR_RET:
    snprintf(ret_buf, buf_size, "{\"ret\":false,\"errCode\":%d}", ret);
    return -1;
}

/*
    函数名：
        int tuya_fac_set_cfg(tuya_FacCfg_t info)
    函数说明：
    入参：
    返回值：
        无
*/
int tuya_fac_set_cfg(tuya_FacCfg_t info)
{
    memcpy(&g_info, &info, sizeof(tuya_FacCfg_t));
    g_info.run_time = info.run_time * 60 * 60;
    gettimeofday(&(g_info.start_tv), NULL);
    return 0;
}

/*
    函数名：
        int tuya_fac_aging_loop(void* phdl, char *path, int run_time)
    函数说明：
        老化测试
    入参：
        控制句柄，路径
    返回值：
 */
int tuya_fac_aging_loop(void *phdl, char *path, int run_time)
{
    while (1) {
        //用户实现老化需要的功能测试

        struct timeval tv;
        gettimeofday(&tv, NULL);
        if ((tv.tv_sec - g_info.start_tv.tv_sec) > run_time) {
            TYDEBUG("------------------------------\n");
            TYDEBUG("---Time' up , Program exit----\n");
            TYDEBUG("------------------------------\n");
            //用户实现老化结束后的状态
        }
    }

    return 0;
}

void tuya_fac_video_start()
{
    /* 启动设备视频出流服务 */
}

void tuya_fac_audio_start()
{
    /* 启动设备音频出流服务 */
}

void tuya_test_path(char *path, int verbose)
{
    TYDEBUG("file path is %s\n", path);
    memset(filepath, 0, sizeof(filepath));
    strcpy(filepath, path);
    debug = verbose;
}

int tuya_fac_test_start(void **pphdl)
{
    TYDEBUG("start \n");
    tuya_fac_inf_s *pdevhdl = (tuya_fac_inf_s *)malloc(sizeof(tuya_fac_inf_s));
    if (pdevhdl == NULL) {
        return -1;
    }
    *pphdl = pdevhdl;
    memset(pdevhdl, 0, sizeof(tuya_fac_inf_s));
    return 0;
}

int tuya_fac_test_stop(void **pphdl)
{
    TYDEBUG("stop \n");
    tuya_fac_inf_s *pdevhdl = (tuya_fac_inf_s *)*pphdl;
    pdevhdl->start = 0;
    if (pdevhdl == NULL) {
        return -1;
    }
    free(pdevhdl);
    return 0;
}
