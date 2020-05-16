ifeq ($(BUILD_CPU_MCU2_0),yes)
ifneq (,$(filter $(TARGET_CPU),R5F R5Ft))

CPU_ID=mcu2_0

TARGET      := app_remoteswitchcfg_server
TARGETTYPE  := exe 

CSOURCES    := main_tirtos.c
CSOURCES    += app_intervlan.c
CSOURCES    += app_swintervlan.c

ASSEMBLY    := utilsCopyVecs2ATmc.asm

XDC_BLD_FILE = $($(_MODULE)_SDIR)/../../bios_cfg/config_$(call lowercase,$(TARGET_CPU)).bld
XDC_CFG_FILE = $($(_MODULE)_SDIR)/mcu2_0.cfg
XDC_INCLUDE_PACKAGES_PATH    = $($(_MODULE)_SDIR)/../../bios_cfg/
XDC_IDIRS     = $(subst $(SPACE),;,${XDC_INCLUDE_PACKAGES_PATH})

LINKER_CMD_FILES =  $($(_MODULE)_SDIR)/linker_mem_map.cmd
LINKER_CMD_FILES +=  $($(_MODULE)_SDIR)/linker.cmd

ifeq ($(TARGET_CPU),R5F)
SYS_STATIC_LIBS += rtsv7R4_A_le_v3D16_eabi
else ifeq ($(TARGET_CPU),R5Ft)
SYS_STATIC_LIBS += rtsv7R4_T_le_v3D16_eabi
endif

STATIC_LIBS += lib_remote_device
STATIC_LIBS += lib_remoteswitchcfg_server

RPMSG_KDRV_TP_ETHSWITCH_VERSION_LAST_COMMIT := ${shell cd ${ETHFW_PATH};git rev-parse --short=8 HEAD 2>/dev/null}


DEFS +=RPMSG_KDRV_TP_ETHSWITCH_VERSION_LAST_COMMIT="\"${RPMSG_KDRV_TP_ETHSWITCH_VERSION_LAST_COMMIT}\""
include $(ETHFW_PATH)/apps/concerto_inc.mak


endif
endif

