#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tuya_fac_cmd_demo.h"
#include "tuya_fac_test.h"

#define LOG_FILE "/mnt/sdcard/fac_logpath.txt"
void* g_phdl = NULL;
void signal_handle(int sig)
{
    printf("################## get signal: %d\n", sig);

    switch (sig) {
        case SIGINT:
        case SIGKILL:
        case SIGTERM:
            tuya_fac_test_loop_exit(g_phdl);
            break;

        default:
            break;
    }

    return;
}

const char* optstr = "m:t:q:f:l:";
void showusage()
{
    TYDEBUG("options : \t %s \n", optstr);
    TYDEBUG("\t -m mode select ,0--测试(default) 1--老化\n");
    TYDEBUG("\t -t run time ,a unit of hour\n");
    TYDEBUG("\t -q video quality , 0 -- main channel , 1 -- sub channel\n");
    TYDEBUG("\t -f video flip , 0 -- disable , 1 -- enable\n");
    TYDEBUG("\t -l log , 0 -- disable , 1 -- enable\n");
    exit(1);
}
void tuya_log_init()
{
    fflush(stdout);
    setvbuf(stdout, NULL, _IONBF, 0);
    freopen(LOG_FILE, "a+", stdout);
}

int main(int argc, char** argv)
{
    char bin_path[128] = { 0 };
    strcpy(bin_path, "target");

    if (argc > 1 && access(argv[1], F_OK) == 0) {
        printf("argv[1] is %s\n", argv[1]);
        memset(bin_path, 0, sizeof(bin_path));
        strncpy(bin_path, argv[1], 127);
    }

    int ch;
    tuya_FacCfg_t info;
    memset(&info, 0, sizeof(tuya_FacCfg_t));
    info.mode = 0;
    info.run_time = 1000;
    while ((ch = getopt(argc, argv, optstr)) != -1) {
        switch (ch) {
            case 'm':
                info.mode = atoi(optarg);
                break;
            case 't':
                info.run_time = atoi(optarg);
                break;
            case 'q':
                info.quality = atoi(optarg);
                break;
            case 'f':
                info.flip = atoi(optarg);
                break;
            case 'l':
                info.log = atoi(optarg);
                break;
            case '?':
                TYDEBUG("Unknown option: %c\n", (char)optopt);
                showusage();
                break;
        }
    }

    if (info.log != 0) {
        tuya_log_init();
    }

    TYDEBUG("Begin___, Run time is %d hours\n", info.run_time);

    signal(SIGINT, signal_handle);
    signal(SIGKILL, signal_handle);
    signal(SIGTERM, signal_handle);
    signal(SIGPIPE, SIG_IGN);
    //设置配置参数信息
    tuya_fac_set_cfg(info);

    tuya_fac_test_start(&g_phdl);

    if (info.mode == 0) /*测试模式*/
        tuya_fac_test_loop(g_phdl, bin_path);
    else if (info.mode == 1) /*老化模式*/
        tuya_fac_aging_loop(g_phdl, bin_path, info.run_time);
    else {
        TYDEBUG("Not support mode %d\n", info.mode);
        showusage();
    }

    tuya_fac_test_stop(&g_phdl);

    TYDEBUG("end___\n");


    return 0;
}
