[SDK files]
- demo_include: head files
- demo_src: source files
- sdk: TUYA XVR SDK files
- sdk/libs: Tuya XVR SDK libs
- sdk/include: Tuya XVR SDK head files


[how to build a quick demo]
1. edit <Makefile>，set COMPILE_PREX ?= to absolute path of your toolchain gcc.
2. execute "rm sdk/include/lwip sdk/include/compact -rf"
3. execute "make APP_NAME=demo" to build an application to run on board, locating at "output/tuya_xvr_demo".
ps:if include head files contain lwip and compact，you can delete it for complie correct
