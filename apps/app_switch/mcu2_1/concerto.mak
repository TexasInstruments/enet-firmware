#MCU_2_1 available only on J721E
ifeq ($(TARGET_PLATFORM),J721E)
ifeq ($(TARGET_CPU),R5F)

include $(PRELUDE)

TARGET      := ethfw_app_switch_tirtos_mcu_2_1
TARGETTYPE  := exe
CSOURCES    := $(foreach cfile,$(call all-c-files-in,$($(_MODULE)_SDIR)/../src),../src/$(cfile))
IDIRS       := 
DEFS        += SOC_${TARGET_PLATFORM}
DEFS        += SIMULATOR

CPU_ID=mcu2_1

XDC_BLD_FILE = $($(_MODULE)_SDIR)/../bios_cfg/config_r5f.bld
XDC_INCLUDE_PACKAGES_PATH    = $($(_MODULE)_SDIR)/../bios_cfg/
XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})

XDC_CFG_FILE = $($(_MODULE)_SDIR)/mcu2_1.cfg

LINKER_CMD_FILES =  $($(_MODULE)_SDIR)/linker_mem_map.cmd
LINKER_CMD_FILES +=  $($(_MODULE)_SDIR)/linker.cmd


SYS_STATIC_LIBS += rtsv7R4_T_le_v3D16_eabi

include $(ETHFW_PATH)/apps/concerto_inc.mak

include $(FINALE)

endif
endif
