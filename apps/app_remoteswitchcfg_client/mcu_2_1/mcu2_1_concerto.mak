ifeq ($(BUILD_CPU_MCU2_1),yes)
ifneq (,$(filter $(TARGET_CPU),R5F R5Ft))

CPU_ID=mcu2_1

TARGET      := app_remoteswitchcfg_client
TARGETTYPE  := exe 

CSOURCES    := main_tirtos.c
ASSEMBLY    := utilsCopyVecs2ATmc.asm

SOC_DIR     := $(call lowercase,$(TARGET_PLATFORM))

XDC_BLD_FILE = $($(_MODULE)_SDIR)/../../bios_cfg/config_$(call lowercase,$(TARGET_CPU)).bld
XDC_CFG_FILE = $($(_MODULE)_SDIR)/mcu2_1.cfg
XDC_INCLUDE_PACKAGES_PATH  = $($(_MODULE)_SDIR)/../../bios_cfg/
XDC_INCLUDE_PACKAGES_PATH += $($(_MODULE)_SDIR)/../../bios_cfg/$(SOC_DIR)/
XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})

LINKER_CMD_FILES =  $($(_MODULE)_SDIR)/$(SOC_DIR)/linker_mem_map.cmd
LINKER_CMD_FILES += $($(_MODULE)_SDIR)/linker.cmd

ifeq ($(TARGET_CPU),R5F)
SYS_STATIC_LIBS += rtsv7R4_A_le_v3D16_eabi
else ifeq ($(TARGET_CPU),R5Ft)
SYS_STATIC_LIBS += rtsv7R4_T_le_v3D16_eabi
endif

STATIC_LIBS += lib_remote_device_client
STATIC_LIBS += lib_remoteswitchcfg_client

# TODO: Client app should be agnostic of port specifics
ifeq ($(TARGET_PLATFORM),J7200)
    DEFS += ENABLE_QSGMII_PORTS
endif

include $(ETHFW_PATH)/apps/concerto_inc.mak


endif
endif

