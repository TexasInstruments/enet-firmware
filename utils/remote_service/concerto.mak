
include $(PRELUDE)
TARGET      := app_remote_service
TARGETTYPE  := library

ifneq (,$(filter $(TARGET_PLATFORM),J721E J7200 AM65XX))
ifeq ($(TARGET_OS),SYSBIOS)

ifeq ($(TARGET_CPU),$(filter $(TARGET_CPU), R5F R5Ft C66 C71))
CSOURCES := src/app_remote_service.c
endif

endif
endif

IDIRS       := ${ETHFW_PATH}
IDIRS       += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
IDIRS       += $(XDCTOOLS_PATH)/packages
IDIRS       += $(PDK_PATH)/packages

include $(FINALE)

