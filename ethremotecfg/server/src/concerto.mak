include $(PRELUDE)
TARGET      := lib_remoteswitchcfg_server
TARGETTYPE  := library


CSOURCES    := remote_device_server_ethswitch.c
CSOURCES    += cpsw_proxy_server.c

#include $(ETHFW_PATH)/apps/concerto_inc.mak
ifeq ($(TARGET_OS),SYSBIOS)
  ifneq (,$(filter $(HOST_COMPILER),TIARMCGT CGT6X CGT7X TMS470 ARP32CGT))
    IDIRS       += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages/ti/posix/ccs
  endif
  IDIRS       += $(NDK_PATH)/packages
  IDIRS       += $(XDCTOOLS_PATH)/packages
  IDIRS       += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
else ifeq ($(TARGET_OS),FREERTOS)
  IDIRS       += $(PDK_PATH)/packages/ti/transport/lwip/lwip-stack/src/include
  IDIRS       += $(PDK_PATH)/packages/ti/drv/enet/lwipif/ports/freertos/include
endif
IDIRS       += $(PDK_PATH)/packages
IDIRS       += $(REMOTE_DEVICE_PATH)
IDIRS       += $(ETHFW_PATH)

ifeq ($(TARGET_OS),SYSBIOS)
  DEFS += SYSBIOS
else ifeq ($(TARGET_OS),FREERTOS)
  DEFS += MAKEFILE_BUILD FREERTOS
endif

include $(FINALE)
