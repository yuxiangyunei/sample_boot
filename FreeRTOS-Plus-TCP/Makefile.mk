LIBTCPIP_ROOT_DIR := $(PRJ_ROOT_DIR)/FreeRTOS-Plus-TCP
LIBTCPIP_SRC_DIRS := \
$(LIBTCPIP_ROOT_DIR)\
$(LIBTCPIP_ROOT_DIR)/portable/NetworkInterface/MPC5748G \
$(LIBTCPIP_ROOT_DIR)/portable/NetworkInterface/Common

LIBTCPIP_SRCS := $(foreach v,$(LIBTCPIP_SRC_DIRS),$(wildcard $(v)/*.c))
LIBTCPIP_SRCS += $(LIBTCPIP_ROOT_DIR)/portable/BufferManagement/BufferAllocation_1.c
LIBTCPIP_ASMS := $(foreach v,$(LIBTCPIP_SRC_DIRS),$(wildcard $(v)/*.s))
DEPS += $(patsubst %.c, %.d, $(notdir $(LIBTCPIP_SRCS)))
LIBTCPIP_OBJS := $(patsubst %.c, %.o, $(notdir $(LIBTCPIP_SRCS))) $(patsubst %.s, %.o, $(notdir $(LIBTCPIP_ASMS)))

#global variables
INC_DIRS += $(PRJ_ROOT_DIR)/FreeRTOS-Plus-TCP
INC_DIRS += $(PRJ_ROOT_DIR)/FreeRTOS-Plus-TCP/include
INC_DIRS += $(PRJ_ROOT_DIR)/FreeRTOS-Plus-TCP/portable/NetworkInterface/include
INC_DIRS += $(PRJ_ROOT_DIR)/FreeRTOS-Plus-TCP/portable/Compiler/GCC


VPATH += $(LIBTCPIP_SRC_DIRS) $(LIBTCPIP_ROOT_DIR)/portable/BufferManagement
OBJS += $(LIBTCPIP_OBJS)
LIB_OBJS += $(LIBTCPIP_OBJS)
