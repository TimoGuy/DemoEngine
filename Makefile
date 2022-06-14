UNAME_S = $(shell uname -s)

CC  = clang
CXX = clang++

# Libraries
PATH_LIB          = Libraries
PATH_FMOD_CORE    = $(PATH_LIB)/fmod_core
PATH_FMOD_STUDIO  = $(PATH_LIB)/fmod_studio
PATH_NVIDIA_PHYSX = $(PATH_LIB)/NvidiaPhysx
PATH_FREETYPE     = $(PATH_LIB)/freetype

# Library Include Paths
INCFLAGS  = -I$(PATH_FMOD_CORE)/inc
INCFLAGS += -I$(PATH_FMOD_STUDIO)/inc
INCFLAGS += -I$(PATH_NVIDIA_PHYSX)/include
INCFLAGS += -I$(PATH_FREETYPE)/include
INCFLAGS += -I$(PATH_LIB)/Include

LIBPATHS  = -L$(PATH_FMOD_CORE)/lib/x64
LIBPATHS += -L$(PATH_FMOD_STUDIO)/lib/x64
LIBPATHS += -L$(PATH_NVIDIA_PHYSX)/lib_checked		# /lib_release if release build; /lib_debug if debug build
LIBPATHS += -L$(PATH_LIB)/Lib

CCFLAGS  = -O1 -g -Wall -Wextra -Wpedantic
# CCFLAGS += -Wno-c99-extensions -Wno-unused-parameter -Wno-vla-extension
# CCFLAGS += -Wno-c++11-extensions -Wno-gnu-statement-expression
# CCFLAGS += -Wno-gnu-zero-variadic-macro-arguments -Wno-nested-anon-types
# CCFLAGS += -Wno-gnu-anonymous-struct

LDFLAGS  = -m64
# LDFLAGS += -lstdc++
LDFLAGS += -lglfw3_mt
LDFLAGS += -lopengl32
LDFLAGS += -lassimp-vc142-mt
LDFLAGS += -lPhysXPvdSDK_static_64
LDFLAGS += -lPhysXCharacterKinematic_static_64
LDFLAGS += -lPhysXCooking_64
LDFLAGS += -lPhysX_64
LDFLAGS += -lPhysXCommon_64
LDFLAGS += -lPhysXExtensions_static_64
LDFLAGS += -lPhysXFoundation_64
LDFLAGS += -lfmod_vc
LDFLAGS += -lfmodstudio_vc
LDFLAGS += -lfreetype		# This is 'freetyped' in DEBUG

MACROS  = -DNDEBUG
MACROS += -D_DEVELOP
MACROS += -D_CONSOLE
MACROS += -DGLFW_INCLUDE_NONE



### TARGET SPECIFICS ###
TARGET = checked_win_x64
########################

BIN_DIR = Bin
OUT     = $(BIN_DIR)/solanine_$(TARGET).exe
OUT_PDB = $(BIN_DIR)/solanine_$(TARGET).pdb

SRC_DIR      = TestGUI/src
SRC_CPP      = $(shell find $(SRC_DIR) -name "*.cpp")
SRC_C        = $(shell find $(SRC_DIR) -name "*.c")
SRC_COMBINED = $(SRC_CPP) $(SRC_C)

OBJ_DIR      = Obj
OBJ          = $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(notdir $(SRC_COMBINED))))


.PHONY: depend clean $(SRC_CPP) $(SRC_C) link $(OBJ)
all: build run

print:
	@echo $(SRC_COMBINED)
	@echo ---------------------
	@echo $(OBJ)

build: $(SRC_CPP) $(SRC_C)
	@make link

$(SRC_CPP):
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(LIBPATHS) -std=c++17 $(CCFLAGS) $(INCFLAGS) $(MACROS) -c $@ -o $(OBJ_DIR)/$(notdir $@).o $(LDFLAGS) -v

$(SRC_C):
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(LIBPATHS) $(CCFLAGS) $(INCFLAGS) $(MACROS) -c $@ -o $(OBJ_DIR)/$(notdir $@).o $(LDFLAGS) -v

link: $(OBJ)
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(LIBPATHS) $(CCFLAGS) $(INCFLAGS) $(MACROS) $(OBJ) -o $(OUT) $(LDFLAGS) -v

clean:
	rm -f $(OBJ) $(OUT) $(OUT_PDB)
