export TOOL_ROOT_DIR := ../../
export PRJ_ROOT_DIR := ..
export MOD_ROOT_DIR := .
export MOD_ASM_DIRS :=
export MOD_SRC_DIRS := $(MOD_ROOT_DIR) $(MOD_ROOT_DIR)/cfg $(MOD_ROOT_DIR)/ex_inc $(PRJ_ROOT_DIR)/sample_common/ptp
#$(PRJ_ROOT_DIR)/vci8_common   $(PRJ_ROOT_DIR)/sample_common/ptp   $(PRJ_ROOT_DIR)/sample_common/flexray
export MOD_TARGET := $(notdir  $(CURDIR))
export CFLAGS :=  -DHW_VCI_6
export ASFLAGS := -DBOOTLOADER
export LD_SCRIPT_FILE := ./ld/boot_flash.ld
include $(PRJ_ROOT_DIR)/Makefile.mk