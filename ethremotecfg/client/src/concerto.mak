include $(PRELUDE)
TARGET      := lib_remoteswitchcfg_client
TARGETTYPE  := library


CSOURCES    := remote_device_client_ethswitch.c
CSOURCES    += cpsw_proxy.c

include $(ETHFW_PATH)/apps/concerto_inc.mak

include $(FINALE)
