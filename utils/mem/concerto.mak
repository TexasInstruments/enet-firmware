ifneq (,$(filter $(TARGET_PLATFORM),J721E AM65XX))
ifeq ($(TARGET_OS),SYSBIOS)

include $(PRELUDE)
TARGET      := app_utils_mem
TARGETTYPE  := library
CSOURCES    := $(foreach cfile,$(call all-c-files-in,$($(_MODULE)_SDIR)/src),src/$(cfile))
IDIRS       := ${ETHSWITCHFW_PATH}
IDIRS       += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
IDIRS       += $(XDCTOOLS_PATH)/packages
IDIRS       += $(PDK_PATH)/packages

include $(FINALE)

endif
endif
