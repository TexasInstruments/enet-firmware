ifneq (,$(filter $(TARGET_PLATFORM),J721E AM65XX))
ifeq ($(TARGET_OS),SYSBIOS)
#App Profile lib supported only for R5F due to use of PMU counters
ifneq (,$(filter ${TARGET_CPU},R5F R5Ft))

include $(PRELUDE)
TARGET      := app_utils_profile
TARGETTYPE  := library
CSOURCES    := $(foreach cfile,$(call all-c-files-in,$($(_MODULE)_SDIR)/src),src/$(cfile))

IDIRS       := ${ETHFW_PATH}
IDIRS       += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
IDIRS       += $(XDCTOOLS_PATH)/packages
IDIRS       += $(PDK_PATH)/packages


include $(FINALE)

endif
endif
endif
