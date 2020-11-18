English | [简体中文](./README_zh-CN.md)

# tuya-iotos-embeded-multimedia-demo
demo codes for Tuya multimedia SDK applications

## Introduction
Application demo codes and resources are supplied to develop kinds of devices, based on **[Tuya multimedia sdk](https://github.com/tuya/tuya-iotos-embeded-sdk-multimedia)**, such as IP camera, NVR, DVR, doorbell, lowpower doorbell, floodlight etc. 

## Get Started

* Prepare developing environment

Ubuntu 16.04.4 LTS is suggested(not limited, Linux is required).<br>
Install Tuya Smart on your mobile phone and register.

* Build a quick demo of IPC

Download [SDK for Ubuntu x64](https://github.com/tuya/tuya-iotos-embeded-sdk-multimedia/), and decompression to demo_for_ipc\sdk\. <br>
```
# cd demo_for_ipc
# make APP_NAME=name
```

* Run a virtual device

Copy demo resources to demo_for_ipc\output
```
# cp demo_resource demo_for_ipc\output\resources -r
```
Use Tuya Smart APP to get a TOKEN for pairing.([What is TOKEN?](https://github.com/tuya/tuya-iotos-embeded-multimedia-demo/wiki))<br>


Run with new PID/UUID/AUTHKEY.([What is PID/UUID/AUTHKEY?](https://github.com/tuya/tuya-iotos-embeded-multimedia-demo/wiki))<br>
```
# cd output
# ./tuya_ipc_demo -m 2 -p [PID] -u [UUID] -a [AUTHKEY] -r [./] -t "[TOKEN]"
```
Check on Tuya Smart APP, a new IPC is added. Click and view a demo video.

## Support
Tuya Developer Center: https://developer.tuya.com/en/ <br>
Tuya Smart Help Center: https://support.tuya.com/en/help <br>
Technical Support Council: https://iot.tuya.com/council/ 
