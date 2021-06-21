include $(PRELUDE)

TARGET      := ethfw_callbacks
TARGETTYPE  := library

ifneq (,$(filter $(TARGET_PLATFORM),J721E J7200 AM65XX))
ifeq ($(TARGET_OS),SYSBIOS)
  CSOURCES := src/ethfw_callbacks_ndk.c
  CSOURCES += src/ethfw_callbacks_nimu.c
else ifeq ($(TARGET_OS),FREERTOS)
  CSOURCES := src/ethfw_callbacks_lwipif.c
endif

IDIRS := ${ETHFW_PATH}
ifeq ($(TARGET_OS),SYSBIOS)
  IDIRS += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages/ti/posix/ccs
  IDIRS += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
  IDIRS += $(XDCTOOLS_PATH)/packages
  IDIRS += $(NDK_PATH)/packages
endif
IDIRS += $(PDK_PATH)/packages

ifeq ($(TARGET_OS),SYSBIOS)
  DEFS += SYSBIOS
else ifeq ($(TARGET_OS),FREERTOS)
  DEFS += MAKEFILE_BUILD FREERTOS
endif
endif

include $(FINALE)
