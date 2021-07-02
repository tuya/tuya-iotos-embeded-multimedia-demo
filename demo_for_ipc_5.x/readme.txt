[SDK files]
- demo_include: head files
- demo_src: source files
- sdk: TUYA IPC SDK files
- sdk/libs: Tuya IPC SDK libs
- sdk/include: Tuya IPC SDK head files


[how to build a quick demo]
1. edit <Makefile>，set COMPILE_PREX ?= to absolute path of your toolchain gcc.
2. execute "make APP_NAME=demo" to build an application to run on board, locating at "output/tuya_ipc_demo".
