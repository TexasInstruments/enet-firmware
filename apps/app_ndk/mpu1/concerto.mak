ifneq (,$(filter $(TARGET_CPU),A72 A53))

include $(PRELUDE)

TARGET      := ethfw_app_ndk_tirtos_mpu1
TARGETTYPE  := exe
CSOURCES    := $(call all-c-files)
IDIRS       := $(BIOS_PATH)/packages/ti/posix/ccs

CPU_ID=mpu1

XDC_BLD_FILE = $($(_MODULE)_SDIR)/../bios_cfg/config_a72.bld
XDC_INCLUDE_PACKAGES_PATH    = $($(_MODULE)_SDIR)/../bios_cfg/
XDC_INCLUDE_PACKAGES_PATH    += $(NDK_PATH)/packages
XDC_INCLUDE_PACKAGES_PATH    += $(NS_PATH)/packages
XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})
XDC_CFG_FILE = $($(_MODULE)_SDIR)/mpu1.cfg
ifeq ($(TARGET_PLATFORM),J721E)
XDC_PLATFORM = ti.platforms.cortexA:J7ES
else
ifeq ($(TARGET_PLATFORM),AM65XX)
XDC_PLATFORM = ti.platforms.cortexA:AM65X
endif
endif

LINKER_CMD_FILES =  $($(_MODULE)_SDIR)/linker.cmd

LDIRS += $(PDK_PATH)/packages/ti/osal/lib/tirtos/a72/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/csl/lib/j7/a72/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/board/lib/simJ7/a72/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/uart/lib/j7/a72/$(TARGET_BUILD)/

STATIC_LIBS += app_utils_mem
STATIC_LIBS += app_utils_console_io
STATIC_LIBS += app_utils_ipc

SYS_STATIC_LIBS += stdc++ gcc m c rdimon

ADDITIONAL_STATIC_LIBS += ti.osal.aa72fg
ADDITIONAL_STATIC_LIBS += ti.csl.aa72fg
ADDITIONAL_STATIC_LIBS += ti.board.aa72fg
ADDITIONAL_STATIC_LIBS += ti.drv.uart.aa72fg

include $(FINALE)

endif
