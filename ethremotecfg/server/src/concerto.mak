include $(PRELUDE)
TARGET      := lib_remoteswitchcfg_server
TARGETTYPE  := library


CSOURCES    := remote_device_server_ethswitch.c
CSOURCES    += cpsw_proxy_server.c

#include $(ETHFW_PATH)/apps/concerto_inc.mak
ifeq ($(TARGET_OS),FREERTOS)
  IDIRS       += $(PDK_PATH)/packages/ti/transport/lwip/lwip-stack/src/include
  IDIRS       += $(PDK_PATH)/packages/ti/transport/lwip/lwip-port/freertos/include
endif
IDIRS       += $(PDK_PATH)/packages
IDIRS       += $(REMOTE_DEVICE_PATH)
IDIRS       += $(ETHFW_PATH)

ifeq ($(TARGET_OS),FREERTOS)
  DEFS += MAKEFILE_BUILD FREERTOS
endif

ifeq ($(ETHFW_PROXY_ARP_SUPPORT),yes)
  DEFS += ETHFW_PROXY_ARP_HANDLING
endif

include $(FINALE)
