include $(PRELUDE)

ifeq ($(BUILD_CPU_MCU2_0),yes)
ifneq (,$(filter $(TARGET_CPU),R5F R5Ft))

TARGET     := ethfw
TARGETTYPE := library

CSOURCES := src/ethfw.c

IDIRS := ${ETHFW_PATH}
IDIRS += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages/ti/posix/ccs
IDIRS += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
IDIRS += $(XDCTOOLS_PATH)/packages
IDIRS += $(REMOTE_DEVICE_PATH)
IDIRS += $(NDK_PATH)/packages
IDIRS += $(PDK_PATH)/packages

RPMSG_KDRV_TP_ETHSWITCH_VERSION_LAST_COMMIT := ${shell cd ${ETHFW_PATH};git rev-parse --short=8 HEAD 2>/dev/null}
DEFS += RPMSG_KDRV_TP_ETHSWITCH_VERSION_LAST_COMMIT="\"${RPMSG_KDRV_TP_ETHSWITCH_VERSION_LAST_COMMIT}\""

endif
endif

include $(FINALE)
