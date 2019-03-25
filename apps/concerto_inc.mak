TARGET_CPU_FOLDER := $(call lowercase,$(TARGET_CPU))
TARGET_SOC_FOLDER := $(call lowercase,$(TARGET_PLATFORM))
TARGET_BOARD_FOLDER := $(call lowercase,${$(TARGET_PLATFORM)_BOARD})
CPU_ID_FOLDER       := $(strip $(if $(filter $(call lowercase,${CPU_ID}),mpu1),mpu1_0,$(call lowercase,${CPU_ID})))


XDC_INCLUDE_PACKAGES_PATH    += $(NDK_PATH)/packages
#Include posix header file from sysbios package for TI compilers
ifneq (,$(filter $(HOST_COMPILER),TIARMCGT CGT6X CGT7X TMS470 ARP32CGT))
XDC_INCLUDE_PACKAGES_PATH    += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages/ti/posix/ccs
endif
XDC_INCLUDE_PACKAGES_PATH    += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages

ifeq ($(TARGET_PLATFORM),J721E)
    ifeq (${TARGET_CPU},R5F)
    XDC_PLATFORM = ti.platforms.cortexR:J7ES
    else 
	    ifeq (${TARGET_CPU},A72)
        XDC_PLATFORM = ti.platforms.cortexA:J7ES
        endif
    endif
else
    ifeq ($(TARGET_PLATFORM),AM65XX)
        ifeq (${TARGET_CPU},R5F)
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

LDIRS += $(PDK_PATH)/packages/ti/osal/lib/tirtos/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/csl/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/board/lib/${TARGET_BOARD_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/uart/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/cpsw/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/udma/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/sciclient/lib/${TARGET_SOC_FOLDER}/${CPU_ID_FOLDER}/$(TARGET_BUILD)/

STATIC_LIBS += app_utils_mem
STATIC_LIBS += app_utils_console_io

ifeq (${TARGET_CPU},R5F)
ADDITIONAL_STATIC_LIBS += ti.board.ae$(call lowercase,$(TARGET_CPU))
ADDITIONAL_STATIC_LIBS += nimucpsw.ae$(call lowercase,$(TARGET_CPU))
ADDITIONAL_STATIC_LIBS += cpsw_apputils.ae$(call lowercase,$(TARGET_CPU))
ADDITIONAL_STATIC_LIBS += cpsw.ae$(call lowercase,$(TARGET_CPU))
ADDITIONAL_STATIC_LIBS += udma.ae$(call lowercase,$(TARGET_CPU))
ADDITIONAL_STATIC_LIBS += sciclient.ae$(call lowercase,$(TARGET_CPU))
ADDITIONAL_STATIC_LIBS += ti.drv.uart.ae$(call lowercase,$(TARGET_CPU))
ADDITIONAL_STATIC_LIBS += ti.csl.ae$(call lowercase,$(TARGET_CPU))
ADDITIONAL_STATIC_LIBS += ti.osal.ae$(call lowercase,$(TARGET_CPU))
else
    CORTEX_A_LIB_SUFFIX := $(if $(filter $(TARGET_BUILD),debug),g,)
    ifneq (,$(filter ${TARGET_CPU},A72 A53))
    ADDITIONAL_STATIC_LIBS += ti.board.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += nimucpsw.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += cpsw_apputils.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += cpsw.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += udma.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += sciclient.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += ti.drv.uart.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += ti.csl.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    ADDITIONAL_STATIC_LIBS += ti.osal.a$(call lowercase,$(TARGET_CPU))f$(CORTEX_A_LIB_SUFFIX)
    endif
endif


PDK_SOC_LIST += $(TARGET_PLATFORM)
PDK_LIB_RULES += osal_tirtos
PDK_LIB_RULES += udma
PDK_LIB_RULES += csl
PDK_LIB_RULES += sciclient
PDK_LIB_RULES += cpsw
PDK_LIB_RULES += nimucpsw
PDK_LIB_RULES += cpsw_apputils
PDK_LIB_RULES += uart
PDK_LIB_RULES += board




