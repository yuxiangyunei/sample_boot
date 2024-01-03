# c makefile template
SRC_DIRS	:= .
INC_DIRS	:= ../inc
LIB_DIRS	:= ../bin
OBJ_DIR		:= ../obj
DEP_DIR		:= ../dep
BIN_DIR		:= ../bin
VPATH		:= $(SRC_DIRS) $(INC_DIRS) $(LIB_DIRS) $(OBJ_DIR) $(BIN_DIR) $(DEP_DIR)

LIBS		:= vciprog

ifeq ($(shell uname), Linux)
TARGET		:= vci8_prog
else
TARGET	  	:= vci8_prog.exe
endif

CSRCS		:= $(notdir $(foreach v,$(SRC_DIRS),$(wildcard $(v)/*.c)))
CXXSRCS		:= $(notdir $(foreach v,$(SRC_DIRS),$(wildcard $(v)/*.cpp)))

DEPS		:= $(patsubst %.c, %.d, $(CSRCS)) $(patsubst %.cpp, %.d, $(CXXSRCS))
OBJS		:= $(patsubst %.c, %.o, $(CSRCS)) $(patsubst %.cpp, %.o, $(CXXSRCS))

CFLAGS		:= -Wall -O2 -static $(addprefix -I, $(INC_DIRS)) $(addprefix -I, $(SRC_DIRS))
CXXFLAGS	:= -Wall -O2 -static $(addprefix -I, $(INC_DIRS)) $(addprefix -I, $(SRC_DIRS))
LDFLAGS		:= -static $(addprefix -L, $(LIB_DIRS)) $(addprefix -l, $(LIBS))

RM			:= rm -f
CC			:= $(CROSS_PREFIX)gcc
CXX			:= $(CROSS_PREFIX)g++
LD			:= $(CROSS_PREFIX)g++
SED			:= sed
ECHO		:= echo
MKDIR		:= mkdir -p

.PHONY: all clean veryclean mkdirs
all: $(TARGET)

include $(addprefix $(DEP_DIR)/, $(DEPS)) 

$(DEP_DIR)/%.d: %.c
	@$(ECHO) "Build dep file: $@"; \
	$(CC) -MM $(CFLAGS) $< | $(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

$(DEP_DIR)/%.d: %.cpp
	@$(ECHO) "Build dep file: $@"; \
	$(CXX) -MM $(CXXFLAGS) $< | $(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

%.o: %.c
	@$(ECHO) "Build obj file: $@"; \
	$(CC) $(CFLAGS) -c -o $(OBJ_DIR)/$@ $<

%.o: %.cpp
	@$(ECHO) "Build obj file: $@"; \
	$(CXX) $(CXXFLAGS) -c -o $(OBJ_DIR)/$@ $<

$(TARGET): $(OBJS)
	@$(ECHO) "Build target file: $@"; \
	$(LD) -o $(BIN_DIR)/$@ $(addprefix $(OBJ_DIR)/, $(OBJS)) $(LDFLAGS);
	
clean:
	@$(RM) $(addprefix $(OBJ_DIR)/, $(OBJS)); \
	$(ECHO) "Clean OK!"

veryclean:
	@$(RM) $(addprefix $(DEP_DIR)/, $(DEPS)) $(addprefix $(OBJ_DIR)/, $(OBJS)) $(BIN_DIR)/$(TARGET); \
	$(ECHO) "Very Clean OK!"

mkdirs:
	@$(MKDIR) $(SRC_DIRS) $(INC_DIRS) $(LIB_DIRS) $(OBJ_DIR) $(DEP_DIR) $(BIN_DIR); \
	$(ECHO) "Make directories OK!"