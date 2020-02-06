
include $(PRELUDE)
TARGET      := app_perf_stats
TARGETTYPE  := library

ifneq (,$(filter $(TARGET_PLATFORM),J721E AM65XX))
ifeq ($(TARGET_OS),SYSBIOS)

ifeq ($(TARGET_CPU),$(filter $(TARGET_CPU), R5F C66 C71))
CSOURCES := src/app_perf_stats_sysbios.c
endif

endif
endif

IDIRS       := ${ETHFW_PATH}
IDIRS       += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
IDIRS       += $(XDCTOOLS_PATH)/packages
IDIRS       += $(PDK_PATH)/packages

include $(FINALE)

