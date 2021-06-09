#
# Utility makefile to build PDK libaries and related components
#
# Edit this file to suit your specific build needs
#

ifeq ($(PROFILE), $(filter $(PROFILE),release all))
  PDK_BUILD_PROFILE_LIST_ALL+=release
endif
ifeq ($(PROFILE), $(filter $(PROFILE),debug all))
  PDK_BUILD_PROFILE_LIST_ALL+=debug
endif


NDK_ENV_SETTINGS =
# Uncomment target that need to be built
ifeq ($(BUILD_ISA_C6x),yes)
  NDK_ENV_SETTINGS  += ti.targets.elf.C66=${CGT6X_ROOT}
endif

#NDK_ENV_SETTINGS  += ti.targets.elf.C66_big_endian=${CGT6X_ROOT}
#NDK_ENV_SETTINGS  += ti.targets.elf.C674=${CGT6X_ROOT}
#NDK_ENV_SETTINGS  += ti.targets.arm.elf.Arm9=${TIARMCGT_ROOT}
#NDK_ENV_SETTINGS  += ti.targets.arm.elf.A8F=${TIARMCGT_ROOT}
#NDK_ENV_SETTINGS  += ti.targets.arm.elf.A8Fnv=${TIARMCGT_ROOT}
#NDK_ENV_SETTINGS  += ti.targets.arm.elf.M3=${TIARMCGT_ROOT}
#NDK_ENV_SETTINGS  += ti.targets.arm.elf.M4=${TIARMCGT_ROOT}
#NDK_ENV_SETTINGS  += ti.targets.arm.elf.M4F=${TIARMCGT_ROOT}

ifeq ($(BUILD_ISA_R5F),yes)
  NDK_ENV_SETTINGS  += ti.targets.arm.elf.R5F=${TIARMCGT_ROOT_$(TARGET_PLATFORM)}
endif

ifeq ($(BUILD_ISA_R5Ft),yes)
  NDK_ENV_SETTINGS  += ti.targets.arm.elf.R5Ft=${TIARMCGT_ROOT_$(TARGET_PLATFORM)}
endif

#NDK_ENV_SETTINGS  += gnu.targets.arm.M3=${GCC_SYSBIOS_ARM_ROOT}
#NDK_ENV_SETTINGS  += gnu.targets.arm.M4=${GCC_SYSBIOS_ARM_ROOT}
#NDK_ENV_SETTINGS  += gnu.targets.arm.M4F=${GCC_SYSBIOS_ARM_ROOT}
#NDK_ENV_SETTINGS  += gnu.targets.arm.A8F=${GCC_SYSBIOS_ARM_ROOT}
#NDK_ENV_SETTINGS  += gnu.targets.arm.A9F=${GCC_SYSBIOS_ARM_ROOT}
#NDK_ENV_SETTINGS  +=  gnu.targets.arm.A15F=${GCC_SYSBIOS_ARM_ROOT}

ifeq ($(BUILD_ISA_A72),yes)
  NDK_ENV_SETTINGS  +=  gnu.targets.arm.A53F=${GCC_SYSBIOS_ARM_ROOT}
endif

ifeq ($(BUILD_ISA_A53),yes)
  NDK_ENV_SETTINGS  +=  gnu.targets.arm.A53F=${GCC_SYSBIOS_ARM_ROOT}
endif

NDK_ENV_SETTINGS += XDC_INSTALL_DIR=${XDCTOOLS_PATH} \
                    SYSBIOS_INSTALL_DIR=${BIOS_PATH_$(TARGET_PLATFORM)}  \
                    NS_INSTALL_DIR=${NS_PATH}



ndk:
	make -C $(NDK_PATH) -f ndk.mak all $(sort ${NDK_ENV_SETTINGS}) 

ndk_clean:
	make -C $(NDK_PATH) -f ndk.mak clean $(sort ${NDK_ENV_SETTINGS})

.PHONY: ndk ndk_clean
