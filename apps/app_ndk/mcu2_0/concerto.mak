ifeq ($(TARGET_CPU),R5F)
ifeq ($(TARGET_PLATFORM),J721E)

include $(PRELUDE)

TARGET      := ethfw_app_ndk_tirtos_mcu_2_0
TARGETTYPE  := exe
CSOURCES    := $(call all-c-files)
IDIRS       := $(BIOS_PATH)/packages/ti/posix/ccs

CPU_ID=mcu2_0

XDC_BLD_FILE = $($(_MODULE)_SDIR)/../bios_cfg/config_r5f.bld
XDC_INCLUDE_PACKAGES_PATH    = $($(_MODULE)_SDIR)/../bios_cfg/
XDC_INCLUDE_PACKAGES_PATH    += $(NDK_PATH)/packages
XDC_INCLUDE_PACKAGES_PATH    += $(NS_PATH)/packages
XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})
XDC_CFG_FILE = $($(_MODULE)_SDIR)/mcu2_0.cfg
XDC_PLATFORM = ti.platforms.cortexR:J7ES
LINKER_CMD_FILES =  $($(_MODULE)_SDIR)/linker.cmd

LDIRS += $(PDK_PATH)/packages/ti/osal/lib/tirtos/r5f/$(TARGET_BUILD)/

STATIC_LIBS += app_utils_mem
STATIC_LIBS += app_utils_console_io
STATIC_LIBS += app_utils_ipc


SYS_STATIC_LIBS += rtsv7R4_T_le_v3D16_eabi

ADDITIONAL_STATIC_LIBS += ti.osal.aer5f

include $(FINALE)

endif
endif