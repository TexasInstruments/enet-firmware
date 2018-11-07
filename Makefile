
include ethswitchfw_tools_path.mak

# Edit below file to change default build options
include ethswitchfw_build_flags.mak


get-num-cores = $(strip $(foreach core-list, $(1), \
                    $(if $(findstring $(2),$(core-list)), $(word 2,$(subst :, ,$(core-list))))))

is-valid-combo =  $(and $(call get-num-cores,$(5),$(1)), $(strip $(filter $(1):$(2):$(3),$(4))))

expand-target-combos =  $(foreach soc,$(1),\
					       $(foreach os,$(2),\
						       $(foreach isa,$(3),\
							       $(foreach profile,$(4),\
								       $(foreach cgt,$(5),\
									       $(if $(call is-valid-combo,$(isa),$(cgt),$(os),$(6),${${soc}_ISA_CORE_COUNT}),$(soc):$(os):$(isa):$(strip $(call get-num-cores,${${soc}_ISA_CORE_COUNT},$(isa))):$(profile):$(cgt)) \
									   )\
								   )\
							   )\
						    )\
					    )	

DIRECTORIES :=
DIRECTORIES += utils
DIRECTORIES += apps

TARGET_COMBOS :=

SOC_LIST := J721E AM65XX
OS_LIST  := SYSBIOS LINUX
ISA_LIST := R5F A72 A53 C66 C71
PROFILE_LIST := debug release
CGT_LIST := TIARMCGT GCC_SYSBIOS_ARM CGT6X CGT7X GCC_LINUX_ARM

J721E_ISA_CORE_COUNT := R5F:3
J721E_ISA_CORE_COUNT += A72:1
J721E_ISA_CORE_COUNT += C66:2
J721E_ISA_CORE_COUNT += C71:1
J721E_BOARD          := j721e_sim
AM65XX_ISA_CORE_COUNT := R5F:1
AM65XX_ISA_CORE_COUNT += A53:1
AM65XX_BOARD         := am65xx_evm


ISA_CGT_OS_VALID_TUPLE   := R5F:TIARMCGT:SYSBIOS
ISA_CGT_OS_VALID_TUPLE   += C66:CGT6X:SYSBIOS
ISA_CGT_OS_VALID_TUPLE   += C71:CGT7X:SYSBIOS
ISA_CGT_OS_VALID_TUPLE   += A72:GCC_SYSBIOS_ARM:SYSBIOS
ISA_CGT_OS_VALID_TUPLE   += A72:GCC_LINUX_ARM:LINUX
ISA_CGT_OS_VALID_TUPLE   += A53:GCC_SYSBIOS_ARM:SYSBIOS
ISA_CGT_OS_VALID_TUPLE   += A53:GCC_LINUX_ARM:LINUX


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

ethfw_all: pdk ndk all
ethfw_all_clean: pdk_clean ndk_clean clean


