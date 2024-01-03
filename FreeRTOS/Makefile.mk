LIBRTOS_ROOT_DIR := $(PRJ_ROOT_DIR)/FreeRTOS
LIBRTOS_SRC_DIRS := \
$(LIBRTOS_ROOT_DIR)/Source \
$(LIBRTOS_ROOT_DIR)/Source/portable/GCC/PowerPC \
$(LIBRTOS_ROOT_DIR)/osif
#$(LIBRTOS_ROOT_DIR)/Source/portable/Common
#$(LIBRTOS_ROOT_DIR)/portable/MemMang

LIBRTOS_SRCS := $(foreach v,$(LIBRTOS_SRC_DIRS),$(wildcard $(v)/*.c)) $(LIBRTOS_ROOT_DIR)/Source/portable/MemMang/heap_4.c
LIBRTOS_ASMS := $(foreach v,$(LIBRTOS_SRC_DIRS),$(wildcard $(v)/*.s))
DEPS += $(patsubst %.c, %.d, $(notdir $(LIBRTOS_SRCS)))
LIBRTOS_OBJS := $(patsubst %.c, %.o, $(notdir $(LIBRTOS_SRCS))) $(patsubst %.s, %.o, $(notdir $(LIBRTOS_ASMS)))

#global variables
INC_DIRS += $(PRJ_ROOT_DIR)/FreeRTOS 
INC_DIRS += $(PRJ_ROOT_DIR)/FreeRTOS/Source/include
INC_DIRS += $(PRJ_ROOT_DIR)/FreeRTOS/Source/portable/GCC/PowerPC
INC_DIRS += $(PRJ_ROOT_DIR)/FreeRTOS/osif
VPATH += $(LIBRTOS_SRC_DIRS) $(LIBRTOS_ROOT_DIR)/Source/portable/MemMang
OBJS += $(LIBRTOS_OBJS)
LIB_OBJS += $(LIBRTOS_OBJS)
