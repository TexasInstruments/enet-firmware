include $(PRELUDE)

TARGET      := ethfw_callbacks
TARGETTYPE  := library

ifneq (,$(filter $(TARGET_PLATFORM),J721E AM65XX))

CSOURCES := src/ethfw_callbacks_ndk.c
CSOURCES += src/ethfw_callbacks_nimu.c

IDIRS := ${ETHFW_PATH}
IDIRS += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages/ti/posix/ccs
IDIRS += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
IDIRS += $(XDCTOOLS_PATH)/packages
IDIRS += $(NDK_PATH)/packages
IDIRS += $(PDK_PATH)/packages

endif

include $(FINALE)
