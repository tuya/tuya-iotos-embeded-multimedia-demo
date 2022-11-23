# POSIX标准环境的SDK工程说明
## 版本
| 版本时间  | 说明         |
| ------ | ----------- |
| 2022.11.18  | 首次提交暂不支持rtsp |

## 常用功能

### 快速使用

```shell
cd demo
make
cd output/demo
./tuya_fac_demo
```
#### 出现以下打印即可认定正常运行
```
Dbg:[main:92] Begin___, Run time is 1000 hours

Dbg:[tuya_fac_test_start:1303] start

Dbg:[tuya_fac_test_loop:631] --------------------------

Dbg:[tuya_fac_test_loop:632] @ Start Factory Test @

Dbg:[tuya_fac_test_loop:633] --------------------------

Dbg:[tuya_test_path:1295] file path is target

Dbg:[tuya_fac_test_loop:669] Wait for client ...
```
### 其它命令

- 常用 make 目标

| 名称   | 说明         |
| ------ | ----------- |
| clean  | 清除中间文件 |

## 功能适配

### 产测授权
```
1.打开demo\tuya_fac_cmd_demo.c文件
2.定位int tuya_write_cfg(char *data)函数
3.完成一下三个函数，将上位机传输的数据写入至设备即可。
int tuya_write_pid(char *pid, int size)
int tuya_write_uuid(char *uuid, int size)
int tuya_write_authkey(char *authkey, int size)
```
