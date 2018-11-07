TARGET_CPU_FOLDER := $(call lowercase,$(TARGET_CPU))
TARGET_SOC_FOLDER := $(call lowercase,$(TARGET_PLATFORM))
TARGET_BOARD_FOLDER := $(call lowercase,${$(TARGET_PLATFORM)_BOARD})

LDIRS += $(PDK_PATH)/packages/ti/osal/lib/tirtos/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/csl/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/board/lib/${TARGET_BOARD_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/uart/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/cpsw/lib/${TARGET_SOC_FOLDER}/${TARGET_CPU_FOLDER}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/udma/lib/${TARGET_SOC_FOLDER}/${CPU_ID}/$(TARGET_BUILD)/
LDIRS += $(PDK_PATH)/packages/ti/drv/sciclient/lib/${TARGET_SOC_FOLDER}/${CPU_ID}/$(TARGET_BUILD)/

STATIC_LIBS += app_utils_mem
STATIC_LIBS += app_utils_console_io



ADDITIONAL_STATIC_LIBS += ti.osal.ae$(TARGET_CPU)
ADDITIONAL_STATIC_LIBS += ti.csl.ae$(TARGET_CPU)
ADDITIONAL_STATIC_LIBS += ti.board.ae$(TARGET_CPU)
ADDITIONAL_STATIC_LIBS += ti.drv.uart.ae$(TARGET_CPU)
ADDITIONAL_STATIC_LIBS += cpsw_apputils.ae$(TARGET_CPU)
ADDITIONAL_STATIC_LIBS += cpsw.ae$(TARGET_CPU)
ADDITIONAL_STATIC_LIBS += nimucpsw.ae$(TARGET_CPU)
ADDITIONAL_STATIC_LIBS += sciclient.ae$(TARGET_CPU)

PDK_LIB_RULES += osal_tirtos
PDK_LIB_RULES += udma
PDK_LIB_RULES += csl
PDK_LIB_RULES += sciclient
PDK_LIB_RULES += cpsw
PDK_LIB_RULES += cpswnimu
PDK_LIB_RULES += cpswapputils
PDK_LIB_RULES += uart




