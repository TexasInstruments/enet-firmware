ifeq ($(TARGET_CPU),R5Ft)
    TARGET_CPU_FOLDER := r5f
    REMOTE_DEVICE_TARGET_CPU := R5F
else
    TARGET_CPU_FOLDER := $(call lowercase,$(TARGET_CPU))
    REMOTE_DEVICE_TARGET_CPU := $(TARGET_CPU)
endif

TARGET_SOC_FOLDER := $(call lowercase,$(TARGET_PLATFORM))
TARGET_BOARD_FOLDER := $(call lowercase,${$(TARGET_PLATFORM)_BOARD})
CPU_ID_FOLDER       := $(strip $(if $(filter $(call lowercase,${CPU_ID}),mpu1),mpu1_0,$(call lowercase,${CPU_ID})))
ifeq ($(TARGET_PLATFORM),J721E)
REMOTE_DEVICE_SOC_FOLDER := J7
else
REMOTE_DEVICE_SOC_FOLDER := 
endif

DEFS+=CPU_$(CPU_ID)

XDC_INCLUDE_PACKAGES_PATH    += $(NDK_PATH)/packages
#Include posix header file from sysbios package for TI compilers
ifneq (,$(filter $(HOST_COMPILER),TIARMCGT CGT6X CGT7X TMS470 ARP32CGT))
XDC_INCLUDE_PACKAGES_PATH    += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages/ti/posix/ccs
endif
XDC_INCLUDE_PACKAGES_PATH    += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages

ifeq ($(TARGET_PLATFORM),J721E)
    ifneq (,$(filter ${TARGET_CPU},R5F R5Ft))
        ifneq (,$(filter ${CPU_ID},mcu_1_0 mcu_1_1))
            XDC_PLATFORM = ti.platforms.cortexR:J7ES_MCU
        else
            XDC_PLATFORM = ti.platforms.cortexR:J7ES_MAIN
        endif
    else
        ifeq (${TARGET_CPU},A72)
        XDC_PLATFORM = ti.platforms.cortexA:J7ES
        endif
    endif
else
    ifeq ($(TARGET_PLATFORM),AM65XX)
        ifneq (,$(filter ${TARGET_CPU},R5F R5Ft))
        XDC_PLATFORM = ti.platforms.cortexR:AM65X
        else
            ifeq (${TARGET_CPU},A53)
            XDC_PLATFORM = ti.platforms.cortexA:AM65X
            endif
        endif
    endif
endif

#Include posix header file from sysbios package for TI compilers
ifneq (,$(filter $(HOST_COMPILER),TIARMCGT CGT6X CGT7X TMS470 ARP32CGT))
IDIRS       += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages/ti/posix/ccs
endif
IDIRS       += $(NDK_PATH)/packages
IDIRS       += $(XDCTOOLS_PATH)/packages
IDIRS       += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
IDIRS       += $(PDK_PATH)/packages
IDIRS       += $(REMOTE_DEVICE_PATH)
IDIRS       += $(ETHFW_PATH)

LDIRS += $(PDK_PATH)/packages/ti/osal/lib/tirtos/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/csl/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/board/lib/${TARGET_BOARD_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/i2c/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/uart/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/cpsw/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/cpsw/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/cpsw/lib/${TARGET_BOARD_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/udma/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/sciclient/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/pm/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/ipc/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(REMOTE_DEVICE_PATH)/out/${REMOTE_DEVICE_SOC_FOLDER}/${REMOTE_DEVICE_TARGET_CPU}/${TARGET_OS}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/transport/timeSync/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/transport/timeSync/lib/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/

STATIC_LIBS += app_utils_mem
STATIC_LIBS += app_utils_console_io
STATIC_LIBS += app_utils_profile
STATIC_LIBS += app_remote_service
STATIC_LIBS += app_perf_stats
STATIC_LIBS += app_ethfw_stats

ifneq (,$(filter ${TARGET_CPU},R5F R5Ft))
     # Same extension is kept for R5F or R5Ft (Thumb mode)
     # in PDK build system for backwards compatibility reasons
     TARGET_CPU_SUFFIX=r5f
     ADDITIONAL_STATIC_LIBS += ti.board.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += nimucpsw.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += cpswsoc.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += cpsw_cfgserver.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += cpsw_apputils.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += cpsw.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += udma.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += ipc.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += sciclient.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += ti.drv.i2c.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += ti.drv.uart.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += ti.csl.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += ti.osal.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += pm_lib.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += ti.timesync.hal.ae$(TARGET_CPU_SUFFIX)
     ADDITIONAL_STATIC_LIBS += ti.timesync.ptp.ae$(TARGET_CPU_SUFFIX)     
else
    CORTEX_A_LIB_SUFFIX := $(if $(filter $(TARGET_BUILD),debug),g,)
    ifneq (,$(filter ${TARGET_CPU},A72 A53))
    ADDITIONAL_STATIC_LIBS += ti.board.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += nimucpsw.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += cpsw_cfgserver.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += cpsw_apputils.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += cpsw.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += udma.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += ipc.ae$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += sciclient.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += ti.drv.i2c.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += ti.drv.uart.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += ti.csl.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += ti.osal.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += pm_lib.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += ti.timesync.hal.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += ti.timesync.ptp.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX) 
    endif
endif


PDK_SOC_LIST += $(TARGET_PLATFORM)
PDK_LIB_RULES += i2c
PDK_LIB_RULES += pm_lib
PDK_LIB_RULES += osal_tirtos
PDK_LIB_RULES += udma
PDK_LIB_RULES += csl
PDK_LIB_RULES += sciclient
PDK_LIB_RULES += cpsw
PDK_LIB_RULES += cpswsoc
PDK_LIB_RULES += nimucpsw
PDK_LIB_RULES += cpsw_cfgserver
PDK_LIB_RULES += cpsw_apputils
PDK_LIB_RULES += uart
PDK_LIB_RULES += board
PDK_LIB_RULES += ipc