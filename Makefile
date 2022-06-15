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

CCFLAGS  = -std=c++20 -O1 -g -w
# CCFLAGS  = -O1 -g -Wall -Wextra -Wpedantic
# CCFLAGS += -Wno-c99-extensions -Wno-unused-parameter -Wno-vla-extension
# CCFLAGS += -Wno-c++11-extensions -Wno-gnu-statement-expression
# CCFLAGS += -Wno-gnu-zero-variadic-macro-arguments -Wno-nested-anon-types
# CCFLAGS += -Wno-gnu-anonymous-struct

# Included Libs
LDFLAGS  = -m64		# NOTE: Idk if this really does anything?
LDFLAGS += -lstdc++
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

# Inherited libs
LDFLAGS += -lkernel32
LDFLAGS += -luser32
LDFLAGS += -lgdi32
LDFLAGS += -lwinspool
LDFLAGS += -lcomdlg32
LDFLAGS += -ladvapi32
LDFLAGS += -lshell32
LDFLAGS += -lole32
LDFLAGS += -loleaut32
LDFLAGS += -luuid
LDFLAGS += -lodbc32
LDFLAGS += -lodbccp32


# Predefined Macros
MACROS  = -DNDEBUG
MACROS += -DGLFW_INCLUDE_NONE
MACROS += -D_DEVELOP
MACROS += -D_CONSOLE
MACROS += -D_CONSOLE
MACROS += -D_UNICODE
MACROS += -DUNICODE



### TARGET SPECIFICS ###
TARGET = checked_win_x64
########################

BIN_DIR = bin
OUT     = $(BIN_DIR)/solanine_$(TARGET).exe
OUT_ILK = $(BIN_DIR)/solanine_$(TARGET).ilk
OUT_PDB = $(BIN_DIR)/solanine_$(TARGET).pdb

SRC_DIR      = TestGUI/src
SRC          = $(shell find $(SRC_DIR) -name "*.cpp") $(shell find $(SRC_DIR) -name "*.c")

OBJ_DIR      = obj
OBJ          = $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(SRC)))
DEP          = $(addsuffix .d,$(basename $(OBJ)))


.PHONY: all
all: build
	@make run

# NOTE: this was just for printing out various text manipulations to make sure I was setting up the files correctly.
# ALSO: I essentially wanted to replicate the obj/ folder to have the same file structure as the src/ folder  -Timo
# print:
# 	@echo $(OBJ)
# 	@echo -----------------------------------------------------
# 	@echo $(basename $(OBJ))
# 	@echo -----------------------------------------------------
# 	@echo $(patsubst $(OBJ_DIR)/%,%,$(basename $(OBJ)))

.PHONY: build
build: $(OBJ)
	@echo 
	@echo 
	@echo '===================================='
	@echo '==     Linking assembly files'
	@echo '===================================='
	@make link

.PHONY: run
run:
	@(cd TestGUI && ../$(OUT))

# NOTE: if you add $(OBJ) to .PHONY, it doesn't use the -include dependencies so the build gets rebuilt all the way from the beginning. ouch!
$(OBJ):
	@mkdir -p $(dir $@)
	@echo '==     Compiling '$(patsubst $(OBJ_DIR)/%,%,$(basename $@))
	@$(CXX) $(LIBPATHS) -MMD -MP $(CCFLAGS) $(INCFLAGS) $(MACROS) -c $(patsubst $(OBJ_DIR)/%,%,$(basename $@)) -o $@ $(LDFLAGS)
-include $(addsuffix .d,$(basename $@))

link: $(OBJ)
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(LIBPATHS) $(CCFLAGS) $(INCFLAGS) $(MACROS) $(OBJ) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(DEP) $(OUT) $(OUT_ILK) $(OUT_PDB)
