########################################################################

ifeq ($(BUILD_CPU_MCU2_1),yes)
ifneq (,$(filter $(TARGET_CPU),R5F R5Ft))

CPU_ID=mcu2_1

# This name becomes the suffix of final _MODULE name
_MODULE=normal

include $(PRELUDE)

TARGET      := app_remoteswitchcfg_client
TARGETTYPE  := exe
TARGET_OS_LC := $(call lowercase,$(TARGET_OS))
SOC_LC      := $(call lowercase,$(TARGET_PLATFORM))

CSOURCES    := main.c
ifeq ($(TARGET_OS),FREERTOS)
  CSOURCES += ../../ipc_cfg/ipc_trace.c
  CSOURCES += ../../common/r5f_mpu_$(SOC_LC)_default.c
else ifeq ($(TARGET_OS),SAFERTOS)
  CSOURCES += ../../ipc_cfg/ipc_trace.c
  CSOURCES += ../../common/r5f_mpu_$(SOC_LC)_safertos.c
endif

LINKER_CMD_FILES =  $(SDIR)/$(SOC_LC)/linker_mem_map.cmd
ifneq ($(filter $(TARGET_OS),FREERTOS SAFERTOS),)
  LINKER_CMD_FILES += $(SDIR)/linker_$(TARGET_OS_LC).cmd
endif

# Use separate linker cmd file for J7200 which has different
# entry point name than J721E/J784S4, this is due to
# J7200 being in an older SafeRTOS version than J721E/J784S4.
# This is temporary for SDK 8.6 and should be aligned once new
# SafeRTOS drop for J7200 is available and all SoCs are aligned.
ifeq ($(TARGET_OS),SAFERTOS)
  ifeq ($(TARGET_PLATFORM),J7200)
    LINKER_CMD_FILES =  $(SDIR)/$(SOC_LC)/linker_mem_map.cmd
    LINKER_CMD_FILES += $(SDIR)/linker_$(TARGET_OS_LC)_j7200.cmd
  endif
endif

STATIC_LIBS += lib_remoteswitchcfg_client
STATIC_LIBS += lib_remote_device_client

# TODO: Client app should be agnostic of port specifics
ifeq ($(TARGET_PLATFORM),J7200)
  DEFS += ENABLE_QSGMII_PORTS
endif

# Enable MAC-only ports. Comment out for switch only ports
DEFS += ENABLE_MAC_ONLY_PORTS

ifneq ($(filter $(TARGET_OS),FREERTOS SAFERTOS),)
  DEFS += MAKEFILE_BUILD
endif

ifneq ($(filter $(TARGET_OS),FREERTOS SAFERTOS),)
  ENET_APPUTILS_LIB = enet_example_utils_$(TARGET_OS_LC)
endif

ifneq ($(filter $(TARGET_OS),FREERTOS SAFERTOS),)
  ifeq ($(ETHFW_INTERCORE_ETH_SUPPORT),yes)
    DEFS += ETHAPP_ENABLE_INTERCORE_ETH
  endif
endif

include $(ETHFW_PATH)/apps/concerto_inc.mak

endif
endif

include $(FINALE)

########################################################################

ifeq ($(BUILD_CPU_MCU2_1),yes)
ifneq (,$(filter $(TARGET_CPU),R5F R5Ft))

CPU_ID=mcu2_1

# This name becomes the suffix of final _MODULE name
_MODULE=qnx

include $(PRELUDE)

TARGET      := app_remoteswitchcfg_client_qnx
TARGETTYPE  := exe
TARGET_OS_LC := $(call lowercase,$(TARGET_OS))
SOC_LC      := $(call lowercase,$(TARGET_PLATFORM))

DEFS+=A72_QNX_OS

CSOURCES    := main.c
ifeq ($(TARGET_OS),FREERTOS)
  CSOURCES += ../../ipc_cfg/ipc_trace.c
  CSOURCES += ../../common/r5f_mpu_$(SOC_LC)_default.c
else ifeq ($(TARGET_OS),SAFERTOS)
  CSOURCES += ../../ipc_cfg/ipc_trace.c
  CSOURCES += ../../common/r5f_mpu_$(SOC_LC)_safertos.c
endif

LINKER_CMD_FILES =  $(SDIR)/$(SOC_LC)/linker_mem_map.cmd
ifneq ($(filter $(TARGET_OS),FREERTOS SAFERTOS),)
  LINKER_CMD_FILES += $(SDIR)/linker_$(TARGET_OS_LC).cmd
endif

# Use separate linker cmd file for J7200 which has different
# entry point name than J721E/J784S4, this is due to
# J7200 being in an older SafeRTOS version than J721E/J784S4.
# This is temporary for SDK 8.6 and should be aligned once new
# SafeRTOS drop for J7200 is available and all SoCs are aligned.
ifeq ($(TARGET_OS),SAFERTOS)
  ifeq ($(TARGET_PLATFORM),J7200)
    LINKER_CMD_FILES =  $(SDIR)/$(SOC_LC)/linker_mem_map.cmd
    LINKER_CMD_FILES += $(SDIR)/linker_$(TARGET_OS_LC)_j7200.cmd
  endif
endif

STATIC_LIBS += lib_remoteswitchcfg_client
STATIC_LIBS += lib_remote_device_client

# TODO: Client app should be agnostic of port specifics
ifeq ($(TARGET_PLATFORM),J7200)
  DEFS += ENABLE_QSGMII_PORTS
endif

# Enable MAC-only ports. Comment out for switch only ports
DEFS += ENABLE_MAC_ONLY_PORTS

ifneq ($(filter $(TARGET_OS),FREERTOS SAFERTOS),)
  DEFS += MAKEFILE_BUILD
endif

ifneq ($(filter $(TARGET_OS),FREERTOS SAFERTOS),)
  ENET_APPUTILS_LIB = enet_example_utils_$(TARGET_OS_LC)
endif

# Intercore virtual Ethernet currently not supported in QNX

include $(ETHFW_PATH)/apps/concerto_inc.mak

endif
endif

include $(FINALE)
