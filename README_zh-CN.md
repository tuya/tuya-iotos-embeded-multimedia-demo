[English](./README.md) | 简体中文

# Tuya嵌入式多媒体SDK


## 介绍
本仓库里包含了多种嵌入式应用层代码，基于 **[涂鸦多媒体SDK](https://github.com/tuya/tuya-iotos-embeded-sdk-multimedia)** 开发应用于不同产品形态的可执行程序，包括IPC、NVR、DVR、可视门铃、低功耗门铃、摄像头灯等。

## 快速上手

* 准备开发环境

建议使用Ubuntu 16.04.4 LTS 或更高的兼容版本作为开发环境<br>
从应用市场下载安装涂鸦智能APP。

* 编译IPC demo

下载 [SDK for Ubuntu x64](https://github.com/tuya/tuya-iotos-embeded-sdk-multimedia/), 解压为目录 demo_for_ipc/sdk/. <br>
```
# cd demo_for_ipc
# make APP_NAME=name
```

* 运行虚拟设备

Copy demo演示音视频文件夹
```
# cp demo_resource demo_for_ipc/output/resource -r
```
在涂鸦智能APP上添加设备，选择"二维码方式"，并在二维码中解析出TOKEN字段([什么是TOKEN?](https://github.com/tuya/tuya-iotos-embeded-multimedia-demo/wiki))<br>

使用自己的PID/UUID/AUTHKEY运行demo.([什么是PID/UUID/AUTHKEY?](https://github.com/tuya/tuya-iotos-embeded-multimedia-demo/wiki))<br>
```
# cd output
# ./tuya_ipc_demo -m 2 -p [PID] -u [UUID] -a [AUTHKEY] -r "./" -t "[TOKEN]"
```
在涂鸦智能APP上选择下一步，等待设备添加成功，并可以预览演示视频。

## 技术支持

开发者中心：https://developer.tuya.com/en/ <br>
帮助中心: https://support.tuya.com/en/help <br>
技术支持工单中心:    https://iot.tuya.com/council/

## License
[MIT License](./LICENSE)