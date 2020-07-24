include $(PRELUDE)

ifeq ($(BUILD_CPU_MCU2_0),yes)
ifneq (,$(filter $(TARGET_CPU),R5F R5Ft))

CPU_ID=mcu2_0

TARGET      := app_remoteswitchcfg_server
TARGETTYPE  := exe

CSOURCES    := main_tirtos.c
CSOURCES    += $(foreach cfile,$(call all-c-files-in,$(SDIR)/webdata),webdata/$(cfile))
ASSEMBLY    := utilsCopyVecs2ATmc.asm

SOC_DIR     := $(call lowercase,$(TARGET_PLATFORM))

XDC_BLD_FILE = $(SDIR)/../../bios_cfg/config_$(call lowercase,$(TARGET_CPU)).bld
XDC_CFG_FILE = $(SDIR)/mcu2_0.cfg
XDC_INCLUDE_PACKAGES_PATH  = $(SDIR)/../../bios_cfg/
XDC_INCLUDE_PACKAGES_PATH += $(SDIR)/../../bios_cfg/$(SOC_DIR)/
XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})

LINKER_SOC_DIR = $(call lowercase,$(TARGET_PLATFORM))
LINKER_CMD_FILES = $(SDIR)/$(LINKER_SOC_DIR)/linker_mem_map.cmd
LINKER_CMD_FILES +=  $(SDIR)/linker.cmd

ifeq ($(TARGET_CPU),R5F)
SYS_STATIC_LIBS += rtsv7R4_A_le_v3D16_eabi
else ifeq ($(TARGET_CPU),R5Ft)
SYS_STATIC_LIBS += rtsv7R4_T_le_v3D16_eabi
endif

STATIC_LIBS += ethfw
STATIC_LIBS += ethfw_callbacks
STATIC_LIBS += eth_intervlan
STATIC_LIBS += lib_remote_device
STATIC_LIBS += lib_remoteswitchcfg_server

ifeq ($(TARGET_PLATFORM),J7200)
    DEFS += ENABLE_QSGMII_PORTS
endif

include $(ETHFW_PATH)/apps/concerto_inc.mak

endif
endif

include $(FINALE)
