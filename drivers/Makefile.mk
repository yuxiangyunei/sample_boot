LIBDRV_ROOT_DIR := $(PRJ_ROOT_DIR)/drivers
LIBDRV_SRC_DIRS := \
$(LIBDRV_ROOT_DIR)/src/adc_sar \
$(LIBDRV_ROOT_DIR)/src/bctu \
$(LIBDRV_ROOT_DIR)/src/clock/MPC57xx \
$(LIBDRV_ROOT_DIR)/src/cmp \
$(LIBDRV_ROOT_DIR)/src/crc \
$(LIBDRV_ROOT_DIR)/src/dspi \
$(LIBDRV_ROOT_DIR)/src/edma \
$(LIBDRV_ROOT_DIR)/src/emios \
$(LIBDRV_ROOT_DIR)/src/enet \
$(LIBDRV_ROOT_DIR)/src/fccu \
$(LIBDRV_ROOT_DIR)/src/flash_c55 \
$(LIBDRV_ROOT_DIR)/src/flexcan \
$(LIBDRV_ROOT_DIR)/src/hsm \
$(LIBDRV_ROOT_DIR)/src/i2c \
$(LIBDRV_ROOT_DIR)/src/interrupt \
$(LIBDRV_ROOT_DIR)/src/linflexd \
$(LIBDRV_ROOT_DIR)/src/pass \
$(LIBDRV_ROOT_DIR)/src/phy \
$(LIBDRV_ROOT_DIR)/src/pins/siul2 \
$(LIBDRV_ROOT_DIR)/src/pit \
$(LIBDRV_ROOT_DIR)/src/power \
$(LIBDRV_ROOT_DIR)/src/rtc_api \
$(LIBDRV_ROOT_DIR)/src/sai \
$(LIBDRV_ROOT_DIR)/src/sema42 \
$(LIBDRV_ROOT_DIR)/src/smpu \
$(LIBDRV_ROOT_DIR)/src/stm \
$(LIBDRV_ROOT_DIR)/src/swi2c \
$(LIBDRV_ROOT_DIR)/src/swt \
$(LIBDRV_ROOT_DIR)/src/tdm \
$(LIBDRV_ROOT_DIR)/src/usdhc \
$(LIBDRV_ROOT_DIR)/src/wkpu \
$(LIBDRV_ROOT_DIR)/src/eee

#$(LIBDRV_ROOT_DIR)/src/zipwire
#$(LIBDRV_ROOT_DIR)/src/srx
#$(LIBDRV_ROOT_DIR)/src/sdadc
#$(LIBDRV_ROOT_DIR)/src/mpu_e200
#$(LIBDRV_ROOT_DIR)/src/mpu
#$(LIBDRV_ROOT_DIR)/src/mcan
#$(LIBDRV_ROOT_DIR)/src/igf
#$(LIBDRV_ROOT_DIR)/src/eim
#$(LIBDRV_ROOT_DIR)/src/ctu
#$(LIBDRV_ROOT_DIR)/src/cse
#$(LIBDRV_ROOT_DIR)/src/eqadc
#$(LIBDRV_ROOT_DIR)/src/erm
#$(LIBDRV_ROOT_DIR)/src/esci
#$(LIBDRV_ROOT_DIR)/src/etimer
#$(LIBDRV_ROOT_DIR)/src/flexpwm

LIBDRV_SRCS := $(foreach v, $(LIBDRV_SRC_DIRS),$(wildcard $(v)/*.c))
DEPS += $(patsubst %.c, %.d, $(notdir $(LIBDRV_SRCS)))
LIBDRV_OBJS := $(patsubst %.c, %.o, $(notdir $(LIBDRV_SRCS)))

LIB_CSRCS += $(LIBDRV_SRCS)

#global variables 
INC_DIRS += $(LIBDRV_ROOT_DIR)/inc $(LIBDRV_ROOT_DIR)/inc/phy $(LIBDRV_SRC_DIRS)
VPATH += $(LIBDRV_SRC_DIRS)
OBJS += $(LIBDRV_OBJS)
LIB_OBJS += $(LIBDRV_OBJS)
