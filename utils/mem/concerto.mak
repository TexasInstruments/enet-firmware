ifeq ($(TARGET_PLATFORM),J721E)
ifeq ($(TARGET_OS),SYSBIOS)

include $(PRELUDE)
TARGET      := app_utils_mem
TARGETTYPE  := library
CSOURCES    := $(foreach cfile,$(call all-c-files-in,$($(_MODULE)_SDIR)/src),src/$(cfile))

include $(FINALE)

endif
endif
