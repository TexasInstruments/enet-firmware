ifeq ($(TARGET_CPU),R5F)

include $(PRELUDE)

TARGET      := ethfw_app_ndk_tirtos_mcu_1_0
TARGETTYPE  := exe
CSOURCES    := $(foreach cfile,$(call all-c-files-in,$($(_MODULE)_SDIR)/../src),../src/$(cfile))
IDIRS       := $(BIOS_PATH)/packages/ti/posix/ccs
IDIRS       += $(NDK_PATH)/packages

$(warning mcu1_0 CSOURCES: ${CSOURCES})
CPU_ID=mcu1_0

XDC_BLD_FILE = $($(_MODULE)_SDIR)/../bios_cfg/config_r5f.bld
XDC_INCLUDE_PACKAGES_PATH    = $($(_MODULE)_SDIR)/../bios_cfg/
XDC_INCLUDE_PACKAGES_PATH    += $(NDK_PATH)/packages
XDC_INCLUDE_PACKAGES_PATH    += $(BIOS_PATH)/packages/ti/posix/ccs
XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})

XDC_CFG_FILE = $($(_MODULE)_SDIR)/mcu1_0.cfg
ifeq ($(TARGET_PLATFORM),J721E)
XDC_PLATFORM = ti.platforms.cortexR:J7ES
else
ifeq ($(TARGET_PLATFORM),AM65XX)
XDC_PLATFORM = ti.platforms.cortexR:AM65XX
endif
endif

LINKER_CMD_FILES =  $($(_MODULE)_SDIR)/linker.cmd

SYS_STATIC_LIBS += rtsv7R4_T_le_v3D16_eabi

include $(ETHSWITCHFW_PATH)/apps/concerto_inc.mak

include $(FINALE)

endif
