
include ethfw_tools_path.mak

# Edit below file to change default build options
include ethfw_build_flags.mak

include makerules/ethfw_soc_config.mak

DIRECTORIES :=
DIRECTORIES += utils
DIRECTORIES += apps
DIRECTORIES += ethremotecfg
DIRECTORIES += ethfw

TARGET_COMBOS :=

SOC_LIST := J721E J7200 AM65XX
OS_LIST  := SYSBIOS LINUX FREERTOS
ISA_LIST := R5F R5Ft A72 A53 C66 C71
PROFILE_LIST := debug release
CGT_LIST := TIARMCGT GCC_SYSBIOS_ARM CGT6X CGT7X GCC_LINUX_ARM


#Clear PDK libs and PDK SoC to build.Will get updated by executables
PDK_LIB_RULES :=
PDK_SOC_LIST  :=


ifeq ($(BUILD_TARGET_MODE),yes)
  ifneq ($(PROFILE), $(filter $(PROFILE), debug all))
    PROFILE_LIST := $(filter-out debug,$(PROFILE_LIST))
  endif

  ifneq ($(PROFILE), $(filter $(PROFILE), release all))
    PROFILE_LIST := $(filter-out release,$(PROFILE_LIST))
  endif

  ifneq ($(BUILD_ISA_R5F),yes)
    ISA_LIST := $(filter-out R5F,$(ISA_LIST))
  endif

  ifneq ($(BUILD_ISA_R5Ft),yes)
    ISA_LIST := $(filter-out R5Ft,$(ISA_LIST))
  endif

  ifneq ($(BUILD_ISA_A72),yes)
    ISA_LIST := $(filter-out A72,$(ISA_LIST))
  endif

  ifneq ($(BUILD_ISA_A53),yes)
    ISA_LIST := $(filter-out A53,$(ISA_LIST))
  endif

  ifneq ($(BUILD_ISA_C6x),yes)
    ISA_LIST := $(filter-out C66,$(ISA_LIST))
  endif

  ifneq ($(BUILD_ISA_C7x),yes)
    ISA_LIST := $(filter-out C71,$(ISA_LIST))
  endif

  ifneq ($(BUILD_LINUX_A72),yes)
    OS_LIST := $(filter-out LINUX,$(OS_LIST))
  endif

  ifneq ($(BUILD_APP_TIRTOS),yes)
    OS_LIST := $(filter-out SYSBIOS,$(OS_LIST))
  endif

  ifneq ($(BUILD_APP_FREERTOS),yes)
    OS_LIST := $(filter-out FREERTOS,$(OS_LIST))
  endif

  ifeq (,$(filter $(BUILD_SOC_LIST),J721E))
    SOC_LIST := $(filter-out J721E,$(SOC_LIST))
  endif

  ifeq (,$(filter $(BUILD_SOC_LIST),J7200))
    SOC_LIST := $(filter-out J7200,$(SOC_LIST))
  endif

  ifeq (,$(filter $(BUILD_SOC_LIST),AM65XX))
    SOC_LIST := $(filter-out AM65XX,$(SOC_LIST))
  endif

endif

TARGET_COMBOS := $(foreach combo, $(call expand-target-combos,$(SOC_LIST),$(OS_LIST),$(ISA_LIST),$(PROFILE_LIST),$(CGT_LIST),$(ISA_CGT_OS_VALID_TUPLE)),$(strip $(combo)))

CONCERTO_ROOT ?= concerto
BUILD_MULTI_PROJECT := 1
BUILD_TARGET := concerto/target.mak
BUILD_PLATFORM :=

include $(CONCERTO_ROOT)/rules.mak

# Additional make targets to build various related components
include makerules/makefile_pdk.mak
include makerules/makefile_ndk.mak

#Due to bug in NDK needs patch to build .
#DIsabling NDK build for now
#Add ndk and ndk_clean dependent rules for ethfw_all/ethfw_all_clean once NDK bug is fixed
.NOTPARALLEL:
ethfw_server: pdk_custom_libs remotedevicefw app_remoteswitchcfg_server

ethfw_all: pdk remotedevicefw all
ifeq ($(BUILD_APP_FREERTOS),yes)
remoteswitchcfg_all: | pdk_custom_libs remotedevicefw app_remoteswitchcfg_client app_remoteswitchcfg_server
endif
ifeq ($(BUILD_APP_TIRTOS),yes)
remoteswitchcfg_all: | pdk_custom_libs remotedevicefw ndk app_remoteswitchcfg_client app_remoteswitchcfg_server
endif

ethfw_all_clean: pdk_custom_libs_clean remotedevicefw_clean clean scrub
remoteswitchcfg_all_clean: | pdk_custom_libs_clean remotedevicefw_clean clean scrub

remotedevicefw:
	$(foreach soc, $(call lowercase, $(sort ${SOC_LIST})),\
		$(foreach os, $(sort ${OS_LIST}),\
			make -C ${REMOTE_DEVICE_PATH} SOC=${soc} RTOS=${os} lib_remote_device_client lib_remote_device; \
			make -C ${REMOTE_DEVICE_PATH} SOC=${soc} RTOS=${os} cp_to_lib; \
		) \
	)

remotedevicefw_clean:
	$(foreach soc, $(call lowercase, $(sort ${SOC_LIST})),\
		$(foreach os, $(sort ${OS_LIST}),\
			make -C ${REMOTE_DEVICE_PATH} SOC=${soc}  RTOS=${os} clean scrub; \
		) \
	)

.PHONY: ethfw_all remoteswitchcfg_all ethfw_all_clean remoteswitchcfg_all_clean remotedevicefw remotedevicefw_clean
