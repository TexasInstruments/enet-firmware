#
# Utility makefile to build PDK libaries and related components
#
# Edit this file to suit your specific build needs
#

MAKE_EXTRA_OPTIONS ?= -j $(HOST_NUM_CORES)

ifeq ($(PROFILE), $(filter $(PROFILE),release all))
PDK_BUILD_PROFILE_LIST_ALL+=release
endif
ifeq ($(PROFILE), $(filter $(PROFILE),debug all))
PDK_BUILD_PROFILE_LIST_ALL+=debug
endif

ifeq ($(BUILD_CPU_MPU1),yes)
PDK_CORE_LIST_ALL+=mpu1_0
endif
ifeq ($(BUILD_CPU_MCU1_0),yes)
PDK_CORE_LIST_ALL+=mcu1_0
endif
ifeq ($(BUILD_CPU_MCU1_1),yes)
PDK_CORE_LIST_ALL+=mcu1_1
endif
ifeq ($(BUILD_CPU_MCU2_0),yes)
PDK_CORE_LIST_ALL+=mcu2_0
endif
ifeq ($(BUILD_CPU_MCU2_1),yes)
PDK_CORE_LIST_ALL+=mcu2_1
endif
ifeq ($(BUILD_CPU_MCU3_0),yes)
PDK_CORE_LIST_ALL+=mcu3_0
endif
ifeq ($(BUILD_CPU_MCU3_1),yes)
PDK_CORE_LIST_ALL+=mcu3_1
endif
ifeq ($(BUILD_CPU_C6x_1),yes)
PDK_CORE_LIST_ALL+=c66xdsp_1
endif
ifeq ($(BUILD_CPU_C6x_2),yes)
PDK_CORE_LIST_ALL+=c66xdsp_2
endif
ifeq ($(BUILD_CPU_C7x_1),yes)
PDK_CORE_LIST_ALL+=c7x
endif

pdk_build:
	make -C $(PDK_PATH)/packages/ti/build BOARD=j721e_sim custom_target BUILD_PROFILE_LIST_ALL="$(PDK_BUILD_PROFILE_LIST_ALL)" CORE_LIST_ALL="$(PDK_CORE_LIST_ALL)" BUILD_TARGET_LIST_ALL="$(PDK_BUILD_TARGET_LIST_ALL)" -s $(MAKE_EXTRA_OPTIONS)
	
pdk:
ifeq ($(BUILD_TARGET_MODE),yes)
	make pdk_build PDK_BUILD_TARGET_LIST_ALL="pdk_libs"
endif

pdk_clean:
	make pdk_build PDK_BUILD_TARGET_LIST_ALL="pdk_libs_clean"

.PHONY: pdk pdk_build pdk_clean
