include $(PRELUDE)
TARGET      := lib_remoteswitchcfg_client
TARGETTYPE  := library


CSOURCES    := remote_device_client_ethswitch.c
CSOURCES    += cpsw_proxy.c

ifeq ($(TARGET_OS),SYSBIOS)
  DEFS += SYSBIOS
else ifeq ($(TARGET_OS),FREERTOS)
  DEFS += MAKEFILE_BUILD FREERTOS
endif

include $(ETHFW_PATH)/apps/concerto_inc.mak

include $(FINALE)
