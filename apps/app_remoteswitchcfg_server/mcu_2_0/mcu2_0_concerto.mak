ifeq ($(BUILD_CPU_MCU2_0),yes)
ifeq ($(TARGET_CPU),R5F)

CPU_ID=mcu2_0

TARGET      := app_remoteswitchcfg_server
TARGETTYPE  := exe 

CSOURCES    := main_tirtos.c

XDC_BLD_FILE = $($(_MODULE)_SDIR)/../../bios_cfg/config_r5f.bld
XDC_CFG_FILE = $($(_MODULE)_SDIR)/mcu2_0.cfg
XDC_INCLUDE_PACKAGES_PATH    = $($(_MODULE)_SDIR)/../../bios_cfg/
XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})

LINKER_CMD_FILES =  $($(_MODULE)_SDIR)/linker_mem_map.cmd
LINKER_CMD_FILES +=  $($(_MODULE)_SDIR)/linker.cmd


SYS_STATIC_LIBS += rtsv7R4_T_le_v3D16_eabi

STATIC_LIBS += lib_remote_device
STATIC_LIBS += lib_remoteswitchcfg_server

include $(ETHFW_PATH)/apps/concerto_inc.mak


endif
endif

