#NDK app validated only on AM65xx silicon
ifneq (,$(filter $(TARGET_CPU),A72 A53))

include $(PRELUDE)

TARGET      := ethfw_app_ndk_switch_tirtos_mpu1
TARGETTYPE  := exe
CSOURCES    := $(foreach cfile,$(call all-c-files-in,$($(_MODULE)_SDIR)/../src),../src/$(cfile))
CSOURCES    += bios_mmu.c
IDIRS       := 

CPU_ID=mpu1

ifneq (,$(filter $(TARGET_CPU),A72))
XDC_BLD_FILE = $($(_MODULE)_SDIR)/../bios_cfg/config_a72.bld
else
XDC_BLD_FILE = $($(_MODULE)_SDIR)/../bios_cfg/config_a53.bld
endif
XDC_INCLUDE_PACKAGES_PATH    = $($(_MODULE)_SDIR)/../bios_cfg/
XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})
XDC_CFG_FILE = $($(_MODULE)_SDIR)/mpu1.cfg

LINKER_CMD_FILES =  $($(_MODULE)_SDIR)/linker_mem_map.cmd
LINKER_CMD_FILES +=  $($(_MODULE)_SDIR)/linker.cmd

SYS_STATIC_LIBS += stdc++ gcc m c rdimon nosys

include $(ETHFW_PATH)/apps/concerto_inc.mak

include $(FINALE)

endif
