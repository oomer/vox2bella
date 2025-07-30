BELLA_SDK_NAME	= bella_engine_sdk
EXECUTABLE_NAME	= vox2bella
PLATFORM        = $(shell uname)
BUILD_TYPE      ?= release# Default to release build if not specified

# Common paths
BELLA_SDK_PATH    = ../bella_engine_sdk

OBJ_DIR           = obj/$(PLATFORM)/$(BUILD_TYPE)
BIN_DIR           = bin/$(PLATFORM)/$(BUILD_TYPE)
OUTPUT_FILE       = $(BIN_DIR)/$(EXECUTABLE_NAME)


ifeq ($(PLATFORM), Darwin)
    # macOS configuration
    SDK_LIB_EXT = dylib
	MACOS_SDK_PATH       = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
    # Compiler settings
    CC                   = clang
    CXX                  = clang++

    # Architecture flags - support both x86_64 and arm64
    ARCH_FLAGS           = -arch x86_64 -arch arm64 -mmacosx-version-min=11.0 -isysroot $(MACOS_SDK_PATH)

    # Library directory for weak linking
    LIBDIR               = /usr/local/lib

    # Check if Vulkan library exists
    VULKAN_LIB          = $(LIBDIR)/libvulkan.dylib
    VULKAN_FLAGS        = $(if $(wildcard $(VULKAN_LIB)),-weak_library $(VULKAN_LIB),)

    # Linking flags - Use multiple rpath entries to look in executable directory
    LINKER_FLAGS         = $(ARCH_FLAGS) -framework Cocoa -framework IOKit -fvisibility=hidden -O5 \
                          -rpath @executable_path \
                          -rpath . \
                          $(VULKAN_FLAGS)

                          #-rpath @loader_path \
                          #-Xlinker -rpath -Xlinker @executable_path
else
    # Linux configuration
    SDK_LIB_EXT          = so

    # Compiler settings
    CC                   = gcc
    CXX                  = g++

    # Architecture flags
    ARCH_FLAGS           = -m64 -D_FILE_OFFSET_BITS=64

    # Library directory for weak linking
    LIBDIR               = /usr/lib/x86_64-linux-gnu

    # Linking flags - Linux weak linking equivalent
    LINKER_FLAGS         = $(ARCH_FLAGS) -fvisibility=hidden -O3 -Wl,-rpath,'$$ORIGIN' -Wl,-rpath,'$$ORIGIN/lib' \
                          -Wl,--no-as-needed -L$(LIBDIR) -lvulkan

endif

# Common include and library paths
INCLUDE_PATHS      = -I$(BELLA_SDK_PATH)/src -I$(LIBEFSW_PATH)/include -I$(LIBEFSW_PATH)/src 
SDK_LIB_PATH       = $(BELLA_SDK_PATH)/lib
SDK_LIB_FILE       = lib$(BELLA_SDK_NAME).$(SDK_LIB_EXT)
# Library flags
LIB_PATHS          = -L$(SDK_LIB_PATH) 
LIBRARIES          = -l$(BELLA_SDK_NAME) -lm -ldl 

# Build type specific flags
ifeq ($(BUILD_TYPE), debug)
    CPP_DEFINES = -D_DEBUG -DDL_USE_SHARED
    COMMON_FLAGS = $(ARCH_FLAGS) -fvisibility=hidden -g -O0 $(INCLUDE_PATHS)
else
    CPP_DEFINES = -DNDEBUG=1 -DDL_USE_SHARED
    COMMON_FLAGS = $(ARCH_FLAGS) -fvisibility=hidden -O3 $(INCLUDE_PATHS)
endif

# Language-specific flags
C_FLAGS            = $(COMMON_FLAGS) -std=c17
CXX_FLAGS          = $(COMMON_FLAGS) -std=c++17 -Wno-deprecated-declarations

# Objects
OBJECTS            = $(EXECUTABLE_NAME).o 
OBJECT_FILES       = $(patsubst %,$(OBJ_DIR)/%,$(OBJECTS))

# Build rules
$(OBJ_DIR)/$(EXECUTABLE_NAME).o: $(EXECUTABLE_NAME).cpp
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXX_FLAGS) $(CPP_DEFINES)

$(OUTPUT_FILE): $(OBJECT_FILES)
	@mkdir -p $(@D)
	$(CXX) -o $@ $(OBJECT_FILES) $(LINKER_FLAGS) $(LIB_PATHS) $(LIBRARIES)
	@echo "Copying libraries to $(BIN_DIR)..."
	@cp $(SDK_LIB_PATH)/$(SDK_LIB_FILE) $(BIN_DIR)/$(SDK_LIB_FILE)	
	@cp -P $(SDK_LIB_PATH)/libdl_usd_ms.$(SDK_LIB_EXT) $(BIN_DIR)/	# copy libdl_usd_ms library file from bellangine_sdk to bin directory
	@echo "Build complete: $(OUTPUT_FILE)"

# Add default target
all: $(OUTPUT_FILE)

.PHONY: clean cleanall all
clean:
	rm -f $(OBJ_DIR)/$(EXECUTABLE_NAME).o
	rm -f $(OUTPUT_FILE)
	rm -f $(BIN_DIR)/$(SDK_LIB_FILE)
	rm -f $(BIN_DIR)/*.dylib
	rmdir $(OBJ_DIR) 2>/dev/null || true
	rmdir $(BIN_DIR) 2>/dev/null || true

cleanall:
	rm -f obj/*/release/*.o
	rm -f obj/*/debug/*.o
	rm -f bin/*/release/$(EXECUTABLE_NAME)
	rm -f bin/*/debug/$(EXECUTABLE_NAME)
	rm -f bin/*/release/$(SDK_LIB_FILE)
	rm -f bin/*/debug/$(SDK_LIB_FILE)
	rm -f bin/*/release/*.dylib
	rm -f bin/*/debug/*.dylib
	rmdir obj/*/release 2>/dev/null || true
	rmdir obj/*/debug 2>/dev/null || true
	rmdir bin/*/release 2>/dev/null || true
	rmdir bin/*/debug 2>/dev/null || true
	rmdir obj/* 2>/dev/null || true
	rmdir bin/* 2>/dev/null || true
	rmdir obj 2>/dev/null || true
	rmdir bin 2>/dev/null || true