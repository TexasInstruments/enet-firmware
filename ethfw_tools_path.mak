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
TIARMCGT_ROOT_AM65XX ?= $(PSDK_PATH)/ti-cgt-arm_18.1.5.LTS
TIARMCGT_ROOT_J721E ?= $(PSDK_PATH)/ti-cgt-arm_18.12.1.LTS
ifneq (,$(filter $(HOST_OS),Windows_NT CYGWIN))
GCC_SYSBIOS_ARM_ROOT ?= $(PSDK_PATH)/gcc-linaro-7.2.1-2017.11-i686-mingw32_aarch64-elf
else
GCC_SYSBIOS_ARM_ROOT ?= $(PSDK_PATH)/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-elf
endif

BIOS_PATH_AM65XX ?= $(PSDK_PATH)/bios_6_75_02_00
BIOS_PATH_J721E ?= $(PSDK_PATH)/bios_6_76_01_08_eng
XDCTOOLS_PATH ?= $(PSDK_PATH)/xdctools_3_55_01_14_core_eng
NDK_PATH ?= $(PSDK_PATH)/ndk_3_61_01_01
NS_PATH  ?= $(PSDK_PATH)/ns_2_60_01_06
CTOOLSLIB_PATH ?= $(PSDK_PATH)/ctoolslib

PDK_PATH ?= $(PSDK_PATH)/pdk

BUILD_OS ?= Linux

ifeq ($(BUILD_OS),Linux)
GCC_LINUX_ROOT ?= /usr/
endif
