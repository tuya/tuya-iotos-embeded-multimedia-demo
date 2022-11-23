#ifndef __TUYA_FAC_TEST_H__
#define __TUYA_FAC_TEST_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

#ifndef TYDEBUG
#define TYDEBUG(fmt, ...) printf("Dbg:[%s:%d] " fmt "\r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

typedef struct
{
    int mode;
    int run_time;
    int quality;
    int flip;
    int log;
    int motor_time;
    int need_ota;
    char target_main_fw_ver[16];
    char target_sub_fw_ver[16];
    struct timeval start_tv;
} tuya_FacCfg_t;

void tuya_test_path(char* path, int verbose);

/*
函数名:
    int tuya_fac_test_start(void** pphdl )
函数说明:
    程序启动
入参:
    控制句柄,控制loop启停
返回值:
前置条件:
    需要开发者保证局域网连接成功且完成音视频初始化
*/
int tuya_fac_test_start(void** pphdl);

/*
函数名:
    int tuya_fac_test_stop(void** pphdl )
函数说明:
    程序停止
入参:
    控制句柄,控制loop启停
返回值:
*/
int tuya_fac_test_stop(void** pphdl);

/*
函数名:
    int tuya_fac_test_loop(void* phdl , char *path)
函数说明:
    封装了上位机TCP/IP通讯协议
入参:
    void* phdl 控制句柄,控制loop启停
    char *path 文件传输入口，调试使用
返回值:
*/
int tuya_fac_test_loop(void* phdl, char* path);

int tuya_fac_test_loop_exit(void* phdl);

/*
函数名:
    int tuya_fac_query_fd(int* fd);
函数说明:
    查询当前套接字的文件描述符
出参:
    int* fd
返回值:
*/
int tuya_fac_query_fd(int* fd);

/*
函数名:
    int tuya_fac_query_cmd(unsigned char* cmd)
函数说明:
    查询当前执行中的命令字
出参:
    int* cmd  命令字
返回值:
*/
int tuya_fac_query_cmd(int* cmd);

/*
函数名:
    int tuya_put_frame(int cmd, char *data , int size)
函数说明:
    数据上报接口
入参:
    int cmd  16进制命令字
    char* data 数据内容
    int size 数据大小
返回值:
*/
int tuya_put_frame(int cmd, char* data, int size);

#ifdef __cplusplus
}
#endif

#endif
