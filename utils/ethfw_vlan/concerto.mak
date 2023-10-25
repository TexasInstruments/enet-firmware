include $(PRELUDE)

TARGET      := ethfw_vlan
TARGETTYPE  := library
TARGET_OS_LC := $(call lowercase,$(TARGET_OS))

ifneq (,$(filter $(TARGET_PLATFORM),J721E J7200 J784S4))
ifneq ($(filter $(TARGET_OS),FREERTOS SAFERTOS),)
  CSOURCES := src/ethfw_vlan.c
endif

IDIRS := ${ETHFW_PATH}
IDIRS += $(PDK_PATH)/packages

ifneq ($(filter $(TARGET_OS),FREERTOS SAFERTOS),)
  DEFS += MAKEFILE_BUILD
endif

endif

include $(FINALE)
