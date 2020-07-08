
include $(PRELUDE)
TARGET      := app_utils_console_io
TARGETTYPE  := library

ifneq (,$(filter $(TARGET_PLATFORM),J721E J7200 AM65XX))
ifeq ($(TARGET_OS),SYSBIOS)

CSOURCES    := src/app_log_writer.c src/app_log_sysbios.c src/app_log_reader.c src/app_cli_sysbios.c

ifeq ($(TARGET_CPU),A72)
CSOURCES += src/app_log_printf_gcc_sysbios.c   
endif 

ifeq ($(TARGET_CPU),$(filter $(TARGET_CPU), R5F R5Ft C66 C71))
CSOURCES += src/app_log_printf_ticgt_sysbios.c
endif

endif

ifeq ($(TARGET_OS),LINUX)

CSOURCES    := src/app_log_writer.c src/app_log_reader.c src/app_log_linux.c

endif
endif

IDIRS       := ${ETHFW_PATH}
IDIRS       += ${BIOS_PATH_$(TARGET_PLATFORM)}/packages
IDIRS       += $(XDCTOOLS_PATH)/packages
IDIRS       += $(PDK_PATH)/packages

include $(FINALE)

