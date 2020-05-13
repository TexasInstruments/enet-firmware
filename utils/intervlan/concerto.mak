include $(PRELUDE)

TARGET      := eth_intervlan
TARGETTYPE  := library

ifneq (,$(filter $(TARGET_PLATFORM),J721E AM65XX))

CSOURCES := src/eth_hwintervlan.c
CSOURCES += src/eth_swintervlan.c

IDIRS := ${ETHFW_PATH}
IDIRS += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
IDIRS += $(XDCTOOLS_PATH)/packages
IDIRS += $(PDK_PATH)/packages

endif

include $(FINALE)
