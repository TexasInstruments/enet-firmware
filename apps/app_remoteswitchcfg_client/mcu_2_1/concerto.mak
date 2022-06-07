########################################################################

include $(PRELUDE)

ifeq ($(BUILD_CPU_MCU2_1),yes)
ifneq (,$(filter $(TARGET_CPU),R5F R5Ft))

CPU_ID=mcu2_1

TARGET      := app_remoteswitchcfg_client
TARGETTYPE  := exe

CSOURCES    := main.c
ifeq ($(TARGET_OS),FREERTOS)
  CSOURCES    += ../../ipc_cfg/ipc_trace.c
  ifeq ($(TARGET_PLATFORM),J721E)
    CSOURCES    += ../../common/r5f_mpu_j721e_default.c
  else ifeq ($(TARGET_PLATFORM),J7200)
    CSOURCES    += ../../common/r5f_mpu_j7200_default.c
  else ifeq ($(TARGET_PLATFORM),J784S4)
    CSOURCES    += ../../common/r5f_mpu_j784s4_default.c
  endif
endif

SOC_DIR     := $(call lowercase,$(TARGET_PLATFORM))

LINKER_CMD_FILES =  $(SDIR)/$(SOC_DIR)/linker_mem_map.cmd
ifeq ($(TARGET_OS),FREERTOS)
  LINKER_CMD_FILES += $(SDIR)/linker_freertos.cmd
endif

STATIC_LIBS += lib_remoteswitchcfg_client
STATIC_LIBS += lib_remote_device_client

# TODO: Client app should be agnostic of port specifics
ifeq ($(TARGET_PLATFORM),J7200)
  DEFS += ENABLE_QSGMII_PORTS
endif

# MAC-only ports are not supported in QNX virtual MAC driver
ifneq ($(BUILD_QNX_A72), yes)
  DEFS += ENABLE_MAC_ONLY_PORTS
endif

ifeq ($(TARGET_OS),FREERTOS)
  DEFS += MAKEFILE_BUILD FREERTOS
endif

ifeq ($(TARGET_OS),FREERTOS)
  ENET_APPUTILS_LIB = enet_example_utils_freertos
endif

ifeq ($(TARGET_OS),FREERTOS)
  ifeq ($(ETHFW_INTERCORE_ETH_SUPPORT),yes)
    DEFS += ETHAPP_ENABLE_INTERCORE_ETH
  endif
endif

include $(ETHFW_PATH)/apps/concerto_inc.mak

endif
endif

include $(FINALE)
