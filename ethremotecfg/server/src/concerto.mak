include $(PRELUDE)
TARGET      := lib_remoteswitchcfg_server
TARGETTYPE  := library


CSOURCES    := remote_device_server_ethswitch.c

include $(ETHFW_PATH)/apps/concerto_inc.mak

include $(FINALE)
