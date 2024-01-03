#LIBDEV_TARGET := libdev_core$(CORE).a
LIBDEV_ROOT_DIR := $(PRJ_ROOT_DIR)/devices
LIBDEV_SRC_DIRS := \
$(LIBDEV_ROOT_DIR) \
$(LIBDEV_ROOT_DIR)/MPC5748G/startup \
$(LIBDEV_ROOT_DIR)/MPC5748G/startup/gcc

LIBDEV_SRCS := $(foreach v,$(LIBDEV_SRC_DIRS),$(wildcard $(v)/*.c))
LIBDEV_ASMS := \
$(LIBDEV_ROOT_DIR)/MPC5748G/startup/gcc/core$(CORE)_startup.S\
$(LIBDEV_ROOT_DIR)/MPC5748G/startup/gcc/core1_startup.S\
$(LIBDEV_ROOT_DIR)/MPC5748G/startup/gcc/core2_startup.S\
$(LIBDEV_ROOT_DIR)/MPC5748G/startup/gcc/startup_MPC5748G.S \
$(LIBDEV_ROOT_DIR)/MPC5748G/startup/gcc/interrupt_vectors.S


LD_SCRIPT_FILE ?= "$(LIBDEV_ROOT_DIR)/MPC5748G/linker/gcc/MPC5748G_flash.ld" 

#LDFLAGS += -ldev_core$(CORE)
#endif
LDFLAGS += -T $(LD_SCRIPT_FILE)

DEPS += $(patsubst %.c, %.d, $(notdir $(LIBDEV_SRCS))) $(patsubst %.S, %.d, $(notdir $(LIBDEV_ASMS)))
LIBDEV_OBJS := $(patsubst %.c, %.o, $(notdir $(LIBDEV_SRCS))) $(patsubst %.S, %.o, $(notdir $(LIBDEV_ASMS)))

#global variables 
INC_DIRS += $(PRJ_ROOT_DIR)/devices 
INC_DIRS += $(PRJ_ROOT_DIR)/devices/common 
INC_DIRS += $(PRJ_ROOT_DIR)/devices/MPC5748G/include 
INC_DIRS += $(PRJ_ROOT_DIR)/devices/MPC5748G/startup 
#DIST_LIBS += $(LIBDEV_TARGET)
#DIST_INCS += $(wildcard $(PRJ_ROOT_DIR)/devices/*.h)
#DIST_INCS += $(wildcard $(PRJ_ROOT_DIR)/devices/common/*.h)
#DIST_INCS += $(wildcard $(PRJ_ROOT_DIR)/devices/MPC5748G/include/*.h)
#DIST_INCS += $(wildcard $(PRJ_ROOT_DIR)/devices/MPC5748G/startup/*.h)
#LIBS += $(LIBDEV_TARGET)
VPATH += $(LIBDEV_SRC_DIRS)
OBJS += $(LIBDEV_OBJS)
LIB_OBJS += $(LIBDEV_OBJS)
#MOD_OBJS += $(LIBDEV_OBJS)

#$(LIBDEV_TARGET): $(LIBDEV_OBJS)
#	@$(ECHO) "Build lib file: $@"
#	@$(AR) rcs $(OUT_DIR)/$@ $(addprefix $(OBJ_DIR)/,$(LIBDEV_OBJS))

