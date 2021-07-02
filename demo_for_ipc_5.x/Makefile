COMPILE_PREX = ##full path of your gcc location
AR = $(COMPILE_PREX)ar
CC = $(COMPILE_PREX)gcc
NM = $(COMPILE_PREX)nm
CPP = $(COMPILE_PREX)g++
OBJCOPY = $(COMPILE_PREX)objcopy
OBJDUMP = $(COMPILE_PREX)objdump

ROOT_DIR = $(abspath .)
SRC_DIR = demo_src
SDK_DIR = sdk

LINKFLAGS = \
	-L$(ROOT_DIR)/$(SDK_DIR)/lib -ltuya_iot -lpthread -lrt -ldl -lm -Wl,--gc-sections
	
CCFLAGS = \
	-g 

DEFINES = 

CFLAGS = $(CCFLAGS) $(DEFINES) $(SDK_INCS) $(USER_INCS)


OUTPUT_DIR = $(ROOT_DIR)/output/
OUTPUT_DIR_OBJS = $(OUTPUT_DIR)/objs

SDK_INC_BASE_DIR = $(ROOT_DIR)/$(SDK_DIR)/include
SDK_INCS =  $(addprefix -I ,  $(shell find $(SDK_INC_BASE_DIR) -type d) )

USER_SRC_BASE_DIR 	=  $(ROOT_DIR)/demo_src
USER_INC_BASE_DIR 	= $(ROOT_DIR)/demo_include

USER_SRC_DIRS = $(shell find $(USER_SRC_BASE_DIR) -type d)
USER_SRCS += $(foreach dir, $(USER_SRC_DIRS), $(wildcard $(dir)/*.c)) 
USER_INCS = $(addprefix -I ,  $(shell find $(USER_INC_BASE_DIR) -type d) )
USER_OBJS = $(addsuffix .o, $(basename  $(USER_SRCS) ) )
USER_OBJS_OUT =  $(subst $(ROOT_DIR),$(OUTPUT_DIR_OBJS), $(USER_OBJS))

build_app: $(USER_OBJS)
	@mkdir -p $(OUTPUT_DIR)
	$(CC)  $(USER_OBJS_OUT) $(CFLAGS)  $(LINKFLAGS)  -o $(OUTPUT_DIR)/tuya_ipc_$(APP_NAME) 
	@echo "Build APP Finish"

%.o: %.c
	@mkdir -p $(dir $(subst $(ROOT_DIR),$(OUTPUT_DIR_OBJS), $@)); 
	$(CC) $(CFLAGS) -o  $(subst $(ROOT_DIR),$(OUTPUT_DIR_OBJS), $@)   -c $<
	
%.o: %.cpp
	@mkdir -p $(dir $(subst $(ROOT_DIR),$(OUTPUT_DIR_OBJS), $@)); 
	$(CC) $(CFLAGS) -o $(subst $(ROOT_DIR),$(OUTPUT_DIR_OBJS), $@)   -c $<

%.o: %.s
	@mkdir -p $(dir $(subst $(ROOT_DIR),$(OUTPUT_DIR_OBJS), $@)); 
	$(CC) $(CFLAGS) -o $(subst $(ROOT_DIR),$(OUTPUT_DIR_OBJS), $@)  -c $<

%.o: %.S
	@mkdir -p $(dir $(subst $(ROOT_DIR),$(OUTPUT_DIR_OBJS), $@)); 
	$(CC) $(CFLAGS) -D__ASSEMBLER__ -o $(subst $(ROOT_DIR),$(OUTPUT_DIR_OBJS), $@)  -c $<
	
	
.PHONY:clean SHOWARGS
clean:
	rm -rf $(OUTPUT_DIR)

