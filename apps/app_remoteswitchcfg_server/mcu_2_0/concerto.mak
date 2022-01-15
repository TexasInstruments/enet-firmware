########################################################################

ifeq ($(BUILD_CPU_MCU2_0),yes)
ifneq (,$(filter $(TARGET_CPU),R5F R5Ft))

CPU_ID=mcu2_0

# This name becomes the suffix of final _MODULE name
_MODULE=normal

include $(PRELUDE)

TARGET      := app_remoteswitchcfg_server
TARGETTYPE  := exe

ifeq ($(BUILD_QNX_A72), yes)
  DEFS+=A72_QNX_OS
endif

CSOURCES    := main.c
ifeq ($(TARGET_OS),SYSBIOS)
  CSOURCES    += $(foreach cfile,$(call all-c-files-in,$(SDIR)/webdata),webdata/$(cfile))
  ASSEMBLY    := utilsCopyVecs2ATmc.asm
else ifeq ($(TARGET_OS),FREERTOS)
  CSOURCES    += ../../ipc_cfg/ipc_trace.c
  ifeq ($(TARGET_PLATFORM),J721E)
    CSOURCES    += ../../common/r5f_mpu_j721e_default.c
  else ifeq ($(TARGET_PLATFORM),J7200)
    CSOURCES    += ../../common/r5f_mpu_j7200_default.c
  endif
endif

SOC_DIR     := $(call lowercase,$(TARGET_PLATFORM))

ifeq ($(TARGET_OS),SYSBIOS)
  XDC_BLD_FILE = $(SDIR)/../../bios_cfg/config_$(call lowercase,$(TARGET_CPU)).bld
  XDC_CFG_FILE = $(SDIR)/mcu2_0.cfg
  XDC_INCLUDE_PACKAGES_PATH  = $(SDIR)/../../bios_cfg/
  XDC_INCLUDE_PACKAGES_PATH += $(SDIR)/../../bios_cfg/$(SOC_DIR)/
  XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})
endif

LINKER_CMD_FILES = $(SDIR)/$(SOC_DIR)/linker_mem_map.cmd
ifeq ($(TARGET_OS),SYSBIOS)
  LINKER_CMD_FILES += $(SDIR)/linker.cmd
else ifeq ($(TARGET_OS),FREERTOS)
  LINKER_CMD_FILES += $(SDIR)/linker_freertos.cmd
endif

STATIC_LIBS += ethfw
STATIC_LIBS += ethfw_callbacks
STATIC_LIBS += eth_intervlan
STATIC_LIBS += lib_remoteswitchcfg_server
STATIC_LIBS += lib_remote_device

ifeq ($(TARGET_OS),FREERTOS)
  STATIC_LIBS += ethfw_lwip
endif

ifeq ($(TARGET_OS),SYSBIOS)
  DEFS += SYSBIOS
else ifeq ($(TARGET_OS),FREERTOS)
  DEFS += MAKEFILE_BUILD FREERTOS
endif

# Comment out to use RMII port instead of QSGMII ports in J721E EVM
DEFS += ENABLE_QSGMII_PORTS

ifeq ($(TARGET_OS),SYSBIOS)
  ENET_APPUTILS_LIB = enet_example_utils_tirtos
else ifeq ($(TARGET_OS),FREERTOS)
  ENET_APPUTILS_LIB = enet_example_utils_freertos
endif

ifeq ($(TARGET_OS),FREERTOS)
  ifeq ($(ETHFW_INTERCORE_ETH_SUPPORT),yes)
    DEFS += ETHAPP_ENABLE_INTERCORE_ETH
  endif
endif

include $(ETHFW_PATH)/apps/concerto_inc.mak

include $(FINALE)

endif
endif

########################################################################

ifeq ($(BUILD_CPU_MCU2_0),yes)
ifneq (,$(filter $(TARGET_CPU),R5F R5Ft))

CPU_ID=mcu2_0

# This name becomes the suffix of final _MODULE name
_MODULE=ccs

include $(PRELUDE)

TARGET      := app_remoteswitchcfg_server_ccs
TARGETTYPE  := exe

CSOURCES    := main.c
ifeq ($(TARGET_OS),SYSBIOS)
  CSOURCES    += $(foreach cfile,$(call all-c-files-in,$(SDIR)/webdata),webdata/$(cfile))
  ASSEMBLY    := utilsCopyVecs2ATmc.asm
else ifeq ($(TARGET_OS),FREERTOS)
  CSOURCES    += ../../ipc_cfg/ipc_trace.c
  ifeq ($(TARGET_PLATFORM),J721E)
    CSOURCES    += ../../common/r5f_mpu_j721e_default.c
  else ifeq ($(TARGET_PLATFORM),J7200)
    CSOURCES    += ../../common/r5f_mpu_j7200_default.c
  endif
endif

SOC_DIR     := $(call lowercase,$(TARGET_PLATFORM))

ifeq ($(TARGET_OS),SYSBIOS)
  XDC_BLD_FILE = $(SDIR)/../../bios_cfg/config_$(call lowercase,$(TARGET_CPU)).bld
  XDC_CFG_FILE = $(SDIR)/mcu2_0.cfg
  XDC_INCLUDE_PACKAGES_PATH  = $(SDIR)/../../bios_cfg/
  XDC_INCLUDE_PACKAGES_PATH += $(SDIR)/../../bios_cfg/$(SOC_DIR)/
  XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})
endif

LINKER_CMD_FILES = $(SDIR)/$(SOC_DIR)/linker_mem_map.cmd
ifeq ($(TARGET_OS),SYSBIOS)
  LINKER_CMD_FILES += $(SDIR)/linker.cmd
else ifeq ($(TARGET_OS),FREERTOS)
  LINKER_CMD_FILES += $(SDIR)/linker_freertos.cmd
endif

STATIC_LIBS += ethfw
STATIC_LIBS += ethfw_callbacks
STATIC_LIBS += eth_intervlan
STATIC_LIBS += lib_remoteswitchcfg_server
STATIC_LIBS += lib_remote_device

ifeq ($(TARGET_OS),FREERTOS)
  STATIC_LIBS += ethfw_lwip
endif

ifeq ($(TARGET_OS),SYSBIOS)
  DEFS += SYSBIOS
else ifeq ($(TARGET_OS),FREERTOS)
  DEFS += MAKEFILE_BUILD FREERTOS
endif

# Comment out to use RMII port instead of QSGMII ports in J721E EVM
DEFS += ENABLE_QSGMII_PORTS

ifeq ($(TARGET_OS),SYSBIOS)
  ENET_APPUTILS_LIB = enet_example_utils_full_tirtos
else ifeq ($(TARGET_OS),FREERTOS)
  ENET_APPUTILS_LIB = enet_example_utils_full_freertos
endif

ifeq ($(TARGET_OS),FREERTOS)
  ifeq ($(ETHFW_INTERCORE_ETH_SUPPORT),yes)
    DEFS += ETHAPP_ENABLE_INTERCORE_ETH
  endif
endif

include $(ETHFW_PATH)/apps/concerto_inc.mak

include $(FINALE)

endif
endif
