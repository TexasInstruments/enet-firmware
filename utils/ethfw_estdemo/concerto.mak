include $(PRELUDE)

TARGET      := ethfw_estdemo
TARGETTYPE  := library

ifneq (,$(filter $(TARGET_PLATFORM),J721E J7200 J784S4))
ifeq ($(ETHFW_EST_DEMO_SUPPORT),yes)
CSOURCES += src/ethfw_estdemo.c
endif

IDIRS := ${ETHFW_PATH}
IDIRS += $(PDK_PATH)/packages
IDIRS += $(PDK_PATH)/packages/ti/transport/tsn/tsn-stack
IDIRS += $(PDK_PATH)/packages/ti/transport/tsn/tsn-stack/tsn_combase/tilld/jacinto

LDIRS += $(PDK_PATH)/packages/ti/transport/tsn/lib/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
endif

# Feature flags: ETHFW EST demo - should be supported with gPTP
ifeq ($(ETHFW_EST_DEMO_SUPPORT),yes)
  ifeq ($(ETHFW_GPTP_SUPPORT),yes)
    DEFS += ETHFW_EST_DEMO_SUPPORT
  endif
endif

include $(FINALE)