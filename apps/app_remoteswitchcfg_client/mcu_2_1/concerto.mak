########################################################################

include $(PRELUDE)

ifeq ($(BUILD_CPU_MCU2_1),yes)
ifneq (,$(filter $(TARGET_CPU),R5F R5Ft))

CPU_ID=mcu2_1

TARGET      := app_remoteswitchcfg_client
TARGETTYPE  := exe

CSOURCES    := main.c
ifeq ($(TARGET_OS),SYSBIOS)
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
  XDC_CFG_FILE = $(SDIR)/mcu2_1.cfg
  XDC_INCLUDE_PACKAGES_PATH  = $(SDIR)/../../bios_cfg/
  XDC_INCLUDE_PACKAGES_PATH += $(SDIR)/../../bios_cfg/$(SOC_DIR)/
  XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})
endif

LINKER_CMD_FILES =  $(SDIR)/$(SOC_DIR)/linker_mem_map.cmd
ifeq ($(TARGET_OS),SYSBIOS)
  LINKER_CMD_FILES += $(SDIR)/linker.cmd
else ifeq ($(TARGET_OS),FREERTOS)
  LINKER_CMD_FILES += $(SDIR)/linker_freertos.cmd
endif

STATIC_LIBS += lib_remoteswitchcfg_client
STATIC_LIBS += lib_remote_device_client

# TODO: Client app should be agnostic of port specifics
ifeq ($(TARGET_PLATFORM),J7200)
  DEFS += ENABLE_QSGMII_PORTS
endif

ifeq ($(TARGET_OS),SYSBIOS)
  DEFS += SYSBIOS
else ifeq ($(TARGET_OS),FREERTOS)
  DEFS += MAKEFILE_BUILD FREERTOS
endif

ifeq ($(TARGET_OS),SYSBIOS)
  ENET_APPUTILS_LIB = enet_example_utils_tirtos
else ifeq ($(TARGET_OS),FREERTOS)
  ENET_APPUTILS_LIB = enet_example_utils_freertos
endif

include $(ETHFW_PATH)/apps/concerto_inc.mak

endif
endif

include $(FINALE)
