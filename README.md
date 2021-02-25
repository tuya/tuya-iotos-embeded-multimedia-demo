English | [中文版](./README_zh-CN.md)

# tuya-iotos-embeded-multimedia-demo
This demo provides the code for Tuya multimedia SDK applications.

## Overview
Application demo code and resources are supplied to develop different types of devices based on the **[Tuya Embedded SDK for Multimedia Devices](https://github.com/tuya/tuya-iotos-embeded-sdk-multimedia)**. For example, the following devices are supported: IP cameras (IPCs), network video recorders (NVRs), digital video recorders (DVRs), video doorbells, low-power doorbells, and floodlights. 

## Get started

1. Prepare the development environment.

   Ubuntu 16.04.4 LTS is suggested (not limited, Linux is required).
Install Tuya Smart on your mobile phone and register.

2. Build a quick demo of IP cameras (IPCs).

   Download [SDK for Ubuntu x64](https://github.com/tuya/tuya-iotos-embeded-sdk-multimedia/), and decompress the SDK to demo_for_ipc\sdk\. <br>
```
# cd demo_for_ipc
# make APP_NAME=name
```

3. Run a virtual device.


Copy demo resources to demo_for_ipc/output
```
# cp demo_resource demo_for_ipc/output/resource -r
```

Use the Tuya Smart app to get a token and pair the device. ([What is TOKEN?](https://github.com/tuya/tuya-iotos-embeded-multimedia-demo/wiki))


Run the device with new PID/UUID/AUTHKEY. ([What is PID/UUID/AUTHKEY?](https://github.com/tuya/tuya-iotos-embeded-multimedia-demo/wiki))<br>
```
# cd output
# ./tuya_ipc_demo -m 2 -p [PID] -u [UUID] -a [AUTHKEY] -r "./" -t "[TOKEN]"
```
Open the Tuya Smart app to check whether an IPC is added. You can tap the IPC to view a demo video.

## Support
Tuya IoT Developer Platform: https://developer.tuya.com/en/ <br>
Tuya Smart Help Center: https://support.tuya.com/en/help <br>
Technical Support Console: https://service.console.tuya.com/
