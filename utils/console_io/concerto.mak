
include $(PRELUDE)
TARGET      := app_utils_console_io
TARGETTYPE  := library

ifeq ($(TARGET_PLATFORM),J721E)
ifeq ($(TARGET_OS),SYSBIOS)

CSOURCES    := src/app_log_writer.c src/app_log_sysbios.c src/app_log_reader.c src/app_cli_sysbios.c

ifeq ($(TARGET_CPU),A72)
CSOURCES += src/app_log_printf_gcc_sysbios.c   
endif 

ifeq ($(TARGET_CPU),$(filter $(TARGET_CPU), R5F C66 C71))
CSOURCES += src/app_log_printf_ticgt_sysbios.c
endif

endif

ifeq ($(TARGET_OS),LINUX)

CSOURCES    := src/app_log_writer.c src/app_log_reader.c src/app_log_linux.c

endif
endif

include $(FINALE)

