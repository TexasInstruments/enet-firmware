ifeq ($(OS),Windows_NT)
    HOST_OS=Windows_NT
	ifeq ($(shell uname -o),Cygwin)
        HOST_OS=CYGWIN
	endif
else
    OS=$(shell uname -s)
    ifeq ($(OS),Linux)
        HOST_OS=LINUX
        HOST_NUM_CORES := $(shell cat /proc/cpuinfo | grep processor | wc -l)
    endif
endif


ifeq ($(HOST_OS),CYGWIN)
PSDK_PATH ?= $(shell cygpath -m ${abspath ..})
else
PSDK_PATH ?= $(abspath ..)
endif

ifeq ($(HOST_OS),CYGWIN)
ETHFW_PATH ?= $(shell cygpath -m ${abspath .})
else
ETHFW_PATH ?= $(abspath .)
endif



#CCS Path needed for CCS project build
CCS_PATH ?= D:/ccs_v8_3/ccsv8
TIARMCGT_ROOT_AM65XX ?= $(PSDK_PATH)/ti-cgt-arm_20.2.0.LTS
TIARMCGT_ROOT_J721E ?= $(PSDK_PATH)/ti-cgt-arm_20.2.0.LTS
TIARMCGT_ROOT_J7200 ?= $(PSDK_PATH)/ti-cgt-arm_20.2.0.LTS
ifneq (,$(filter $(HOST_OS),Windows_NT CYGWIN))
GCC_SYSBIOS_ARM_ROOT ?= $(PSDK_PATH)/gcc-linaro-9.2-2019.12-mingw-w64-i686_aarch64-elf
else
GCC_SYSBIOS_ARM_ROOT ?= $(PSDK_PATH)/gcc-linaro-9.2-2019.12-x86_64_aarch64-elf
endif

BIOS_PATH_AM65XX ?= $(PSDK_PATH)/bios_6_83_00_18
BIOS_PATH_J721E ?= $(PSDK_PATH)/bios_6_83_00_18
BIOS_PATH_J7200 ?= $(PSDK_PATH)/bios_6_83_00_18
XDCTOOLS_PATH ?= $(PSDK_PATH)/xdctools_3_61_03_29_core
NDK_PATH ?= $(PSDK_PATH)/ndk_3_80_00_19
NS_PATH  ?= $(PSDK_PATH)/ns_2_80_00_17
CTOOLSLIB_PATH ?= $(PSDK_PATH)/ctoolslib
REMOTE_DEVICE_PATH ?= $(PSDK_PATH)/remote_device

PDK_PATH ?= $(PSDK_PATH)/pdk

BUILD_OS ?= Linux

ifeq ($(BUILD_OS),Linux)
GCC_LINUX_ROOT ?= /usr/
endif
