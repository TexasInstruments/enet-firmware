include $(PRELUDE)

TARGET      := ethfw_lwip
TARGETTYPE  := library

ifneq (,$(filter $(TARGET_PLATFORM),J721E J7200 AM65XX))
ifeq ($(TARGET_OS),FREERTOS)
  CSOURCES := src/ethfw_lwip_utils.c

  IDIRS := ${ETHFW_PATH}
  IDIRS += $(PDK_PATH)/packages/ti/transport/lwip/lwip-stack/src/include
  IDIRS += $(PDK_PATH)/packages/ti/drv/enet/lwipif/ports/freertos/include
  IDIRS += $(PDK_PATH)/packages

  DEFS += MAKEFILE_BUILD FREERTOS
endif
endif

include $(FINALE)
