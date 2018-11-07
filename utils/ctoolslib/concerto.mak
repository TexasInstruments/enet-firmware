BUILD_CCS_PROJECT := $(and $(wildcard ${CCS_PATH}/.),TRUE)
ifneq (,$(filter $(TARGET_PLATFORM),J721E AM65XX))
ifeq ($(TARGET_CPU),R5F)
ifeq ($(TARGET_OS),SYSBIOS)

ifeq ($(BUILD_CCS_PROJECT),TRUE)

include $(PRELUDE)
TARGET      := ctoolslib
TARGETTYPE  := library

ifeq ($(TARGET_PLATFORM),AM65XX)
    $(_MODULE)_CCS_PJT_PATH := $($(_MODULE)_SDIR)/CPT2Lib/projects/AM654x-R5
    $(_MODULE)_CCS_PJT_PATH += $($(_MODULE)_SDIR)/ETBLib/project/AM654x-R5
    $(_MODULE)_CCS_PJT_NAME := CPT2Lib_AM654x_R5 
    $(_MODULE)_CCS_PJT_NAME += TIETBLib_AM654x_R5
else 
    ifeq ($(TARGET_PLATFORM),J721E)
        $(_MODULE)_CCS_PJT_PATH := $($(_MODULE)_SDIR)/CPT2Lib/projects/J7ES-R5
        $(_MODULE)_CCS_PJT_PATH += $($(_MODULE)_SDIR)/ETBLib/project/J7ES-R5
        $(_MODULE)_CCS_PJT_NAME := CPT2Lib_J7ES_R5 
        $(_MODULE)_CCS_PJT_NAME += TIETBLib_J7ES_R5
    endif
endif

$(_MODULE)_IDIRS := $($(_MODULE)_SDIR)/CPT2Lib/include
$(_MODULE)_IDIRS += $($(_MODULE)_SDIR)/ETBLib/include
$(_MODULE)_IDIRS += $($(_MODULE)_SDIR)/k3_common/atbrep/include
$(_MODULE)_IDIRS += $($(_MODULE)_SDIR)/k3_common/common/include
$(_MODULE)_IDIRS += $($(_MODULE)_SDIR)/k3_common/device_specific
$(_MODULE)_IDIRS += $($(_MODULE)_SDIR)/k3_common/gtc/include

$(_MODULE)_LDIRS := $($(_MODULE)_SDIR)/CPT2Lib/lib
$(_MODULE)_LDIRS += $($(_MODULE)_SDIR)/ETBLib/lib

ifeq ($(TARGET_BUILD),debug)
    ifeq ($(TARGET_PLATFORM),AM65XX)
        STATIC_LIBS := cpt2lib_d.am654x_r5_elf
        STATIC_LIBS += tietb_d.am654x_r5_elf
    else 
        ifeq ($(TARGET_PLATFORM),J721E)
            STATIC_LIBS := cpt2lib_d.j7es_r5_elf
            STATIC_LIBS += tietb_d.j7es_r5_elf
        endif
    endif
endif

include $(FINALE)


endif
endif
endif
endif

