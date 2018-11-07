ifeq ($(TARGET_PLATFORM),J721E)

include $(PRELUDE)
TARGET      := app_utils_ipc
TARGETTYPE  := library

CSOURCES    := $(foreach cfile,$(call all-c-files-in,$($(_MODULE)_SDIR)/src),src/$(cfile))

DEFS=SOC_J7

include $(FINALE)

endif
