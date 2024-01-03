export CORE         ?= 0
export DEBUG        ?= 0

export CROSS_TOOL_PATH := $(TOOL_ROOT_DIR)/tools/powerpc-eabivle-4_9/bin
export SYSROOT_PATH := $(TOOL_ROOT_DIR)/tools/powerpc-eabivle-4_9/sysroot/e200_ewl2
export CROSS_TOOL_PREFIX := $(CROSS_TOOL_PATH)/powerpc-eabivle-
export RM			:= "$(TOOL_ROOT_DIR)/tools/busybox" rm -f
export SED			:= "$(TOOL_ROOT_DIR)/tools/busybox" sed
export PTP			:= "$(TOOL_ROOT_DIR)/tools/ptp"
export ECHO			:= "$(TOOL_ROOT_DIR)/tools/busybox" echo
export MAKE			:= "$(TOOL_ROOT_DIR)/tools/make"
export MKDIR        := "$(TOOL_ROOT_DIR)/tools/busybox" mkdir -p
export CP           := "$(TOOL_ROOT_DIR)/tools/busybox" cp
export AWK			:= "$(TOOL_ROOT_DIR)/tools/busybox" awk
export CC			:= "$(CROSS_TOOL_PREFIX)gcc"
export CXX          := "$(CROSS_TOOL_PREFIX)g++"
export LD			:= "$(CROSS_TOOL_PREFIX)gcc"
export AS			:= "$(CROSS_TOOL_PREFIX)gcc"
export AR			:= "$(CROSS_TOOL_PREFIX)ar"
export OBJCPY		:= "$(CROSS_TOOL_PREFIX)objcopy"
export STRIP		:= "$(CROSS_TOOL_PREFIX)strip"
export OBJDUMP		:= "$(CROSS_TOOL_PREFIX)objdump"
export SIZE			:= "$(CROSS_TOOL_PREFIX)size"
export MD5SUM       := "$(TOOL_ROOT_DIR)/tools/busybox" md5sum
export SVNREV       := "$(TOOL_ROOT_DIR)/tools/SubWCRev.exe"
export VCI_ENC      := "$(TOOL_ROOT_DIR)/tools/vci8_enc.exe"
ifeq ($(DEBUG), 0)
export OBJ_DIR      := $(MOD_ROOT_DIR)/obj/Release/core$(CORE)
export OUT_DIR      := $(MOD_ROOT_DIR)/out/Release
else
export OBJ_DIR      := $(MOD_ROOT_DIR)/obj/Debug/core$(CORE)
export OUT_DIR      := $(MOD_ROOT_DIR)/out/Debug
endif


export ASFLAGS      += -DTURN_ON_CPU$(CORE) -x assembler-with-cpp -DSEMIHOSTING -DDISABLE_SWT0 -DSTART_FROM_FLASH -DUSING_OS_FREERTOS -DCPU_MPC5748G -mbig -mvle -mregnames  --sysroot="$(SYSROOT_PATH)"
export CFLAGS       += -DTURN_ON_CPU$(CORE) -Wall -Wextra -Wstrict-prototypes -pedantic -funsigned-char -funsigned-bitfields  -fshort-enums -ffunction-sections -fdata-sections -fno-jump-tables -std=gnu99 -DCPU_MPC5748G -DDISABLE_SWT0 -mbig -mvle -mhard-float -Wall -mregnames -fno-common -msdata=eabi -mlra -DSTART_FROM_FLASH -DUSING_OS_FREERTOS  --sysroot="$(SYSROOT_PATH)"
export LDFLAGS      += -Xlinker --gc-sections -mhard-float -L$(OUT_DIR) -specs=ewl_c9x_noio.specs --sysroot="$(SYSROOT_PATH)"
#export LD_SCRIPT_FILE :=
export INC_DIRS += $(PRJ_ROOT_DIR)
#export LIBS         := $(TARGET_LIB)
export LIB_OBJS = 
#export EXES = 
export VPATH += $(OBJ_DIR) $(OUT_DIR)
export OBJS = 
#export DEPS = 


ifeq ($(CORE), 0)
ASFLAGS += -mcpu=e200z4
CFLAGS += -mcpu=e200z4
LDFLAGS += -mcpu=e200z4
else
ifeq ($(CORE), 1)
ASFLAGS += -mcpu=e200z4
CFLAGS += -mcpu=e200z4
LDFLAGS += -mcpu=e200z4
else
ifeq ($(CORE), 2)
ASFLAGS += -mcpu=e200z2
CFLAGS += -mcpu=e200z2
LDFLAGS += -mcpu=e200z2
else
$(error Invalid variable CORE definition.)
endif
endif
endif

ifeq ($(DEBUG), 0)
ASFLAGS += -O2 -fvisibility=hidden -s -DRELEASE=1
CFLAGS += -O2 -fvisibility=hidden -s -DRELEASE=1
else
ASFLAGS += -O0 -g3 -DDEBUG=1
CFLAGS += -O0 -g3 -DDEBUG=1
endif

include $(PRJ_ROOT_DIR)/FreeRTOS-Plus-TCP/Makefile.mk
include $(PRJ_ROOT_DIR)/FreeRTOS/Makefile.mk
include $(PRJ_ROOT_DIR)/drivers/Makefile.mk
include $(PRJ_ROOT_DIR)/devices/Makefile.mk

MOD_TARGET_EXT := elf
MOD_SRCS += $(foreach v,$(MOD_SRC_DIRS),$(notdir $(wildcard $(v)/*.c)))
MOD_ASMS += $(foreach v,$(MOD_ASM_DIRS),$(notdir $(wildcard $(v)/*.S)))
#DEPS += $(patsubst %.c, %.d, $(notdir $(MOD_SRCS)))
#DEPS += $(patsubst %.S, %.d, $(notdir $(MOD_ASMS)))
MOD_OBJS += $(patsubst %.c, %.o, $(notdir $(MOD_SRCS))) $(patsubst %.S, %.o, $(notdir $(MOD_ASMS)))

#global variables
INC_DIRS += $(MOD_SRC_DIRS)
#EXES += $(MOD_TARGET)_core$(CORE).$(MOD_TARGET_EXT)
VPATH += $(MOD_SRC_DIRS)
OBJS += $(MOD_OBJS)
LDFLAGS += -Wl,-Map,"$(OUT_DIR)/$(MOD_TARGET)_core$(CORE).map"
DEPS = $(patsubst %.o, %.d, $(notdir $(OBJS)))


all: clean $(MOD_TARGET)_core$(CORE).$(MOD_TARGET_EXT)

#$(MOD_TARGET).$(MOD_TARGET_EXT): $(MOD_OBJS) $(LIBS)
#	@$(ECHO) "Build exe file: $@"
#	@$(LD) $(addprefix $(OBJ_DIR)/,$(MOD_OBJS)) $(LDFLAGS) $(addprefix -l:,$(LIBS)) -o $(OUT_DIR)/$@

$(MOD_TARGET)_core$(CORE).$(MOD_TARGET_EXT): prep_dir update_rev $(MOD_OBJS) $(LIB_OBJS)
	@$(ECHO) "Build exe file: $@"
	@$(LD) $(addprefix $(OBJ_DIR)/,$(MOD_OBJS)) $(addprefix $(OBJ_DIR)/,$(LIB_OBJS)) $(LDFLAGS) -o $(OUT_DIR)/$@
	@$(OBJCPY) -O ihex $(OUT_DIR)/$@ $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).hex
	@$(VCI_ENC) $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).hex $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).s19
	@$(RM) $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).hex
	@$(SIZE) $(OUT_DIR)/$@
	@$(MD5SUM) -b $(OUT_DIR)/$@ > $(OUT_DIR)/$@.md5
	@$(MD5SUM) -b $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).s19 > $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).s19.md5

prep_dir:
	@$(MKDIR) $(OBJ_DIR)
	@$(MKDIR) $(OUT_DIR)

#$(TARGET_LIB): $(LIB_OBJS)
#	@$(ECHO) "Build lib file: $@"
#	@$(AR) rcs $(OUT_DIR)/$@ $(addprefix $(OBJ_DIR)/,$(LIB_OBJS))

%.o: %.S
	@$(ECHO) "Assemble ASM file: $< for CORE $(CORE)"
	@$(AS) $(ASFLAGS) -c $(addprefix -I, $(INC_DIRS)) -o $(OBJ_DIR)/$@ $<
	
%.o: %.s
	@$(ECHO) "Assemble asm file: $<  for CORE $(CORE)"
	@$(AS) $(ASFLAGS) -c $(addprefix -I, $(INC_DIRS)) -o $(OBJ_DIR)/$@ $<

%.o: %.c
	@$(ECHO) "Compile c file: $< for CORE $(CORE)"
	@$(CC) $(CFLAGS) -c $(addprefix -I, $(INC_DIRS)) -o $(OBJ_DIR)/$@ $<

%.o: %.cpp
	@$(ECHO) "Compile c++ file: $< for CORE $(CORE)"
	@$(CXX) $(CXXFLAGS) -c -o $(OBJ_DIR)/$@ $<

%.d: %.c
	@$(ECHO) "Build dep file: $@
	@$(CC) -MM $(CFLAGS) $(addprefix -I, $(INC_DIRS)) $< | $(SED) -e "s,$*\.o[ :]*,$(basename $@).o $@: ,g" > $(OBJ_DIR)/$@

%.d: %.s
	@$(ECHO) "Build dep file: $@
	@$(AS) -MM $(ASFLAGS) $(addprefix -I, $(INC_DIRS)) $< | $(SED) -e "s,$*\.o[ :]*,$(basename $@).o $@: ,g" > $(OBJ_DIR)/$@

%.d: %.S
	@$(ECHO) "Build dep file: $@
	@$(AS) -MM $(ASFLAGS) $(addprefix -I, $(INC_DIRS)) $< | $(SED) -e "s,$*\.o[ :]*,$(basename $@).o $@: ,g" > $(OBJ_DIR)/$@



-include $(addprefix $(OBJ_DIR)/, $(DEPS))

clean:
#	@$(RM) $(addprefix $(OBJ_DIR)/, $(OBJS)) $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).$(MOD_TARGET_EXT) $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).$(MOD_TARGET_EXT).md5 $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).map $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).s19 $(MOD_TARGET)_core$(CORE).s19.md5 
#	$(PRJ_ROOT_DIR)/version.h 

clean_all:
	@$(RM) $(addprefix $(OBJ_DIR)/, $(OBJS)) $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).$(MOD_TARGET_EXT) $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).$(MOD_TARGET_EXT).md5 $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).map $(OUT_DIR)/$(MOD_TARGET)_core$(CORE).s19 $(MOD_TARGET)_core$(CORE).s19.md5 
#	$(PRJ_ROOT_DIR)/version.h 

update_rev:
	