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
PSDK_PATH = $(abspath ..)
endif

#CCS Path needed for CCS project build
CCS_PATH ?= D:/ccs_v8_3/ccsv8
TIARMCGT_ROOT ?= $(PSDK_PATH)/ti-cgt-arm_16.9.9.LTS_linux
CGT7X_ROOT ?= $(PSDK_PATH)/ti-cgt-c7000_1.0.0A18263
CGT6X_ROOT ?= $(PSDK_PATH)/ti-cgt-c6000_8.2.4
ifneq (,$(filter $(HOST_OS),Windows_NT CYGWIN))
GCC_SYSBIOS_ARM_ROOT ?= $(PSDK_PATH)/gcc-linaro-7.2.1-2017.11-i686-mingw32_aarch64-elf
GCC_LINUX_ARM_ROOT ?= $(PSDK_PATH)/gcc-linaro-7.2.1-2017.11-i686-mingw32_aarch64-linux-gnu
else
GCC_SYSBIOS_ARM_ROOT ?= $(PSDK_PATH)/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-elf
GCC_LINUX_ARM_ROOT ?= $(PSDK_PATH)/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu
endif
LINUX_CMEM_PATH ?= $(PSDK_PATH)/ludev
LINUX_KERNEL_PATH ?= $(PSDK_PATH)/../ks3-linux-integrated/linux/
LINUX_FS_PATH ?= $(PSDK_PATH)/../ks3-linux-integrated/buildroot/output/target/

BIOS_PATH_AM65XX ?= $(PSDK_PATH)/bios_6_75_02_00
BIOS_PATH_J721E ?= $(PSDK_PATH)/bios_6_76_00_01_eng
XDCTOOLS_PATH ?= $(PSDK_PATH)/xdctools_linux/xdctools_3_51_01_18_core
NDK_PATH ?= $(PSDK_PATH)/ndk_3_40_01_01
NS_PATH  ?= $(PSDK_PATH)/ns_2_40_01_02

ETHSWITCHFW_PATH ?= $(PSDK_PATH)/ethswitchfw

PDK_PATH ?= $(PSDK_PATH)/pdk

BUILD_OS ?= Linux


ifeq ($(BUILD_OS),Linux)
GCC_LINUX_ROOT ?= /usr/
endif
