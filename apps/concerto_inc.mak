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
TARGET_OS_FOLDER    := $(call lowercase,$(TARGET_OS))

DEFS+=CPU_$(CPU_ID)

ifeq ($(TARGET_OS),SYSBIOS)
    XDC_INCLUDE_PACKAGES_PATH    += $(NDK_PATH)/packages
    XDC_INCLUDE_PACKAGES_PATH    += $(NS_PATH)/source
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
    endif

    ifeq ($(TARGET_PLATFORM),J7200)
        ifneq (,$(filter ${TARGET_CPU},R5F R5Ft))
            ifneq (,$(filter ${CPU_ID},mcu_1_0 mcu_1_1))
                XDC_PLATFORM = ti.platforms.cortexR:J7200_MCU
            else
                XDC_PLATFORM = ti.platforms.cortexR:J7200_MAIN
            endif
        else
            ifeq (${TARGET_CPU},A72)
                XDC_PLATFORM = ti.platforms.cortexA:J7200
            endif
        endif
    endif

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

ifeq ($(TARGET_OS),SYSBIOS)
    #Include posix header file from sysbios package for TI compilers
    ifneq (,$(filter $(HOST_COMPILER),TIARMCGT CGT6X CGT7X TMS470 ARP32CGT))
        IDIRS += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages/ti/posix/ccs
    endif
    IDIRS += $(NDK_PATH)/packages
    IDIRS += $(XDCTOOLS_PATH)/packages
    IDIRS += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
    IDIRS += $(NS_PATH)/source
    IDIRS += $(NS_PATH)/source/ti/net/bsd
else ifeq ($(TARGET_OS),FREERTOS)
    IDIRS += $(PDK_PATH)/packages/ti/transport/lwip/lwip-stack/src/include
    IDIRS += $(PDK_PATH)/packages/ti/drv/enet/lwipif/ports/freertos/include
endif
IDIRS += $(PDK_PATH)/packages
IDIRS += $(REMOTE_DEVICE_PATH)
IDIRS += $(ETHFW_PATH)

ifeq ($(TARGET_OS),SYSBIOS)
    LDIRS += $(PDK_PATH)/packages/ti/osal/lib/tirtos/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
    LDIRS += $(NS_PATH)/source/ti/net/lib/ccs/r5f/
    LDIRS += $(NS_PATH)/source/ti/net/http/lib/ccs/r5f/
else ifeq ($(TARGET_OS),FREERTOS)
    LDIRS += $(PDK_PATH)/packages/ti/osal/lib/freertos/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
    LDIRS += $(PDK_PATH)/packages/ti/kernel/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
    LDIRS += $(PDK_PATH)/packages/ti/transport/lwip/lwip-stack/lib/${TARGET_OS_FOLDER}/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
    LDIRS += $(PDK_PATH)/packages/ti/transport/lwip/lwip-contrib/lib/${TARGET_OS_FOLDER}/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
endif
LDIRS += $(PDK_PATH)/packages/ti/csl/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/board/lib/${TARGET_BOARD_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/i2c/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/uart/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/enet/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/enet/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/enet/lib/${TARGET_BOARD_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/enet/lib/${TARGET_OS_FOLDER}/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/udma/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/sciclient/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/pm/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/ipc/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(REMOTE_DEVICE_PATH)/out/${TARGET_PLATFORM}/${REMOTE_DEVICE_TARGET_CPU}/${TARGET_OS}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/transport/timeSync/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/transport/timeSync/lib/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/

ifeq ($(TARGET_OS),SYSBIOS)
    STATIC_LIBS += app_utils_mem
    STATIC_LIBS += app_utils_console_io
    STATIC_LIBS += app_utils_profile
    STATIC_LIBS += app_perf_stats
else ifeq ($(TARGET_OS),FREERTOS)
endif
STATIC_LIBS += app_ethfw_stats
STATIC_LIBS += app_remote_service

ifneq (,$(filter ${TARGET_CPU},R5F R5Ft))
    # Same extension is kept for R5F or R5Ft (Thumb mode)
    # in PDK build system for backwards compatibility reasons
    TARGET_CPU_SUFFIX=r5f
    ifeq ($(TARGET_OS),SYSBIOS)
        ADDITIONAL_STATIC_LIBS += nimuenet.ae$(TARGET_CPU_SUFFIX)
        ADDITIONAL_STATIC_LIBS += slnetsock_$(TARGET_BUILD).a
        ADDITIONAL_STATIC_LIBS += httpserver_$(TARGET_BUILD).a
    else ifeq ($(TARGET_OS),FREERTOS)
        ADDITIONAL_STATIC_LIBS += ti.kernel.freertos.ae$(TARGET_CPU_SUFFIX)
        ADDITIONAL_STATIC_LIBS += ti.csl.init.ae$(TARGET_CPU_SUFFIX)
        ADDITIONAL_STATIC_LIBS += lwipstack_freertos.ae$(TARGET_CPU_SUFFIX)
        ADDITIONAL_STATIC_LIBS += lwipcontrib_freertos.ae$(TARGET_CPU_SUFFIX)
        ADDITIONAL_STATIC_LIBS += lwipif_freertos.ae$(TARGET_CPU_SUFFIX)
    endif
    ADDITIONAL_STATIC_LIBS += ti.board.ae$(TARGET_CPU_SUFFIX)
    ADDITIONAL_STATIC_LIBS += enet_cfgserver.ae$(TARGET_CPU_SUFFIX)
    ADDITIONAL_STATIC_LIBS += enetsoc.ae$(TARGET_CPU_SUFFIX)
    ADDITIONAL_STATIC_LIBS += $(CPSW_APPUTILS_LIB).ae$(TARGET_CPU_SUFFIX)
    ADDITIONAL_STATIC_LIBS += enetphy.ae$(TARGET_CPU_SUFFIX)
    ADDITIONAL_STATIC_LIBS += enet.ae$(TARGET_CPU_SUFFIX)
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
        ADDITIONAL_STATIC_LIBS += nimuenet.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
        ADDITIONAL_STATIC_LIBS += enet_cfgserver.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
        ADDITIONAL_STATIC_LIBS += $(CPSW_APPUTILS_LIB).a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
        ADDITIONAL_STATIC_LIBS += enet.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
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
ifeq ($(TARGET_OS),SYSBIOS)
    PDK_LIB_RULES += osal_tirtos
    PDK_LIB_RULES += nimuenet
else ifeq ($(TARGET_OS),FREERTOS)
    PDK_LIB_RULES += osal_freertos
    PDK_LIB_RULES += freertos
    PDK_LIB_RULES += lwipstack_freertos
    PDK_LIB_RULES += lwipcontrib_freertos
    PDK_LIB_RULES += lwipif_freertos
endif
PDK_LIB_RULES += udma
PDK_LIB_RULES += csl
PDK_LIB_RULES += sciclient
PDK_LIB_RULES += enet
PDK_LIB_RULES += enetsoc
PDK_LIB_RULES += enetphy
PDK_LIB_RULES += enet_cfgserver
PDK_LIB_RULES += $(CPSW_APPUTILS_LIB)
PDK_LIB_RULES += uart
PDK_LIB_RULES += board
PDK_LIB_RULES += ipc
PDK_LIB_RULES += timeSync_hal
PDK_LIB_RULES += timeSync_ptp