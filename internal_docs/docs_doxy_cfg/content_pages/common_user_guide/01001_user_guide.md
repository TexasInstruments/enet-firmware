# User Guide {#ethfw_c_ug_top}

This user guide describes EthFw feature list along with steps to build and run
EthFw demo applications.

For additional information about EthFw refer to [EthFw Introduction](@ref ethfw_c_ug_switch)

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# EthFw Demos {#ethfw_c_ug_ethfw_demos}


EthFw Demos showcases usage and integration of EthFw to use & configure CPSW9G IP
in the Jacinto 7 devices. These peripheral/board level sample/demo examples
demonstrates the capabilities of the CPSW9G IP features & EthFw.

Listed below are some of the key applications.
Demo         | Comments
-------------|--------------
L2 Switching | Configures Switch to enable switching between its external ports


## EthFw Switching & TCP/IP Apps Demo {#ethfw_depend_eg}
This demo showcases switching capabilities of the J721E integrated Ethernet Switch
(CPSW9G) for features like VLAN, Multicast etc. This demo also showcases TI NDK (TCP/IP) integration into EthFw. In this demo we showcase usage of HTTP server.

For further information, please refer to the @ref demo_l2_switching_ndk_top demo
application documentation.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Supported Features {#ethfw_c_ug_features_list}

Feature         | Comments
----------------|--------------
L2 Switching    | Support for configuration of switch to enable L2 Switching between external ports
VLAN            | Support for creating and deleting VLAN
Multicast       | Support for enabling/disabling multicast
NDK Integration | Integration of TCP/IP stack enabling TCP, UDP, HTTP apps

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Dependencies {#ethfw_instal_top}

Dependencies can be categorized as follows:

-# @ref ethfw_depend_hw
-# @ref ethfw_depend_sw

Please note that the dependencies vary depending on the intended use (e.g. for integration
vs running demo applications only).

[Back To Top](@ref ethfw_c_ug_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Hardware Dependencies {#ethfw_depend_hw}

EthFw is supported on the boards/EVM listed below
- @ref ethfw_depend_evm_j721e
- @ref ethfw_depend_evm_gesi_j721e
### J721E EVM {#ethfw_depend_evm_j721e}
![](J7EVM_CPSW_TopView.png "J721E EVM connections")
### J721E EVM GESI Expansion Board {#ethfw_depend_evm_gesi_j721e}
![](GESI_Board.png "J721E EVM GESI Board Top View")
![](GESI_RJ45_SideView.png "J721E EVM GESI Board connections")

<!-- 0_85_BINARY_RELEASE
Please refer to the SDK Description for details about installation
& getting started of J721E EVM.
-->

[Back To Top](@ref ethfw_c_ug_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Software Dependencies {#ethfw_depend_sw}
<!-- 0_85_BINARY_RELEASE

Below listed dependencies are part of Processor SDK package.

### PDK {#ethfw_depend_pdk}
Platform Development Kit (PDK) is a component within the Processor SDK RTOS which provides
Chip Support Library (CSL), Low-Level Drivers (LLD), Boot, Diagnostics, etc.

The following sections list the PDK subcomponents that are required by the EthFw package.

Please refer to the Release Notes that came with this release for the compatible version
of PDK/SDK.


#### CSL {#ethfw_depend_pdk_csl}
Chip Support Library (CSL) implements peripheral register level and functional level
APIs. CSL also provides peripheral base addresses, register offset, C macros to program
peripheral registers.

EthFw uses CSL to determine peripheral addresses and program peripheral registers.

#### UDMA {#ethfw_depend_pdk_udma}
Unified DMA (UDMA) is an integral part of the Jacinto 7 devices and is in charge of
moving data between periperals and memory.

PDK includes an UDMA LLD which provides APIs that the CPSW LLD relies on to send and
receive packets to the CPSW's host port.


#### CPSW LLD {#ethfw_depend_pdk_cpsw}
This is CPSW driver module used to program the CPSW9G(Switch) IP. EthFw receives
commands/configuration from application and uses CPSW LLD to configure CPSW9G.


### NDK {#ethfw_depend_ndk}
The Network Developer's Kit (NDK) is a platform for development of network enabled
applications on TI embedded processors.

NDK provides the TCP/IP stack which used in EthFw for running local TCP/IP applications
and for running switch resident protocols like telnet and EAPoL, as shown in the
Ethernet Switch Software Architecture diagram in the @ref ethfw_c_ug_fw_architecture
section.

-->
[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## IDE (CCS) {#ethfw_instal_ccs}
Code Composer Studio is an integrated development environment (IDE) that supports
TI's Microcontroller and Embedded Processors portfolio. It provides useful tools
to develop and debug embedded applications.

For more information, please visit Code Composer Studio product
[page](http://www.ti.com/tool/ccstudio).

* Download Link - [Download](http://processors.wiki.ti.com/index.php/Download_CCS)

### Setup for J721E EVM {#ethfw_instal_ccs_gel_setup}
-# Supported CCS version is detailed in SDK Release Notes

   ![](ccs_version.png "Code Composer Studio version")

-# Install the CCS Emulation Packs for J7 and J7ES. After installation the
   following versions should be listed

   ![](ccs_emulation_packs.png "Code Composer Studio Emulation Packs")

-#  Creating a Target Configuration File

    1. In Code Composer Studio, go to the **File** menu and select **New** ->
       **Target Configuration File**

    2. Name the Target Configuration file as **J7ES_EVM.ccxml** and click **Finish**
    3. Select the Connection to JTAG emulator which you have connected to J721E
       EVM. For on board emulator use connection as "Texas Instruments XDS110 USB
       Debug Probe"
    4. Set the **Connection** to **JACINTO721E**.

![](ccs_target_configuration.png "Creating CCS Target Configuration File")

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Installation Steps {#ethfw_instal_steps}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

EthFw software is an add-on package, and it's provided as an archive file (tarball).

* Untar the package tarball into SDK base folder, the expected directory structure
  is as show below ([J721E](@ref ethfw_post_install_j721e)).

      tar -zxvf ethfw_xx_yy_zz_bb.tgz

* In case EthFw was installed in a different directory, one could move the complete
  directory as shown:

      mv ethfw_xx_yy_zz_bb ~/ti/.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Directory Structure {#ethfw_dir}

Post installation of EthFw, the following directory would be created. Please
note that this is an indicative snap-shot. Modules could be added/modified.

The top-level EthFw makefile as well as the auxiliary makefiles for build flags
(**ethfw_build_flags.mak**) and build paths (**ethfw_tools_path.mak**)
can be found at the EthFw top-level directory.

## Post Install Directory Structure {#ethfw_post_install_j721e}

![](c_ug_dir_top.png "Top Level Directory Structure")

## Utilities Directory Structure {#ethfw_dir_utils}

The **utils** directory contains miscellaneous utilities required by the EthFw
applications.

![](c_ug_dir_utils.png "Utilities Directory Structure")

## Demo Aplication Sources Directory Structure {#ethfw_dir_demo}

Source code of the EthFw demo applications ben found in the **apps** directory.
For instance, below image shows the directory structure of the L2 Switching
application.

![](c_ug_dir_l2_switching_demo.png "Layer-2 Switching Application Directory Structure")

Pre-compiled binaries are also provided as part of the EthFw release, which can be
found in the **out** directory. For instance, below image shows the directory
structure of the J721E R5F binary.

![](c_ug_dir_j721_r5f_demo.png "Demo Binaries Directory Structure")


[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## EthFw Demonstration Applications {#ethfw_dir_switch_demos}

Refer to @ref demo_top section for a full list of EthFw demo applications.

[Back To Top](@ref ethfw_c_ug_top)


<!-- 0_85_BINARY_RELEASE
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Build {#ethfw_build_top}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EthFw employs Concerto make based build mechanism. When building on Windows based machine,
tool such as [Cygwin](https://www.cygwin.com/) could be used.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Setup Environment {#ethfw_build_setup_env}

The tool paths required by the build system are defined in the `ethfw_tools_path.mak`
makefile. The default paths in `ethfw_tools_path.mak` are defined based on the assumption
that the EthFw package has been installed inside the main Processor SDK directory.

Typically, the Processor SDK installation path is `~/ti` in Linux-based systems.
So, a typical EthFw installation would be at `~/ti/ethfw_xx_yy_zz_bb`. In this case,
no additional environment setup steps are required.

If either Processor SDK or EthFw have been installed at different locations that those
mentioned in previous paragraph, the following variables can be passed to the make
command:

    make <target> PSDK_PATH=<Processor SDK installation path> ETHFW_PATH=<EthFw installation path>

Please refer to the @ref ethfw_build and @ref ethfw_build_clean sections for a list
of recommended targets. Alternatively, run the following command to get the full
list of valid targets:

    make help

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Build {#ethfw_build}

The make commands listed below require the environment setup according to
@ref ethfw_build_setup_env section.

### Build All {#ethfw_build_all}

Build EthFw components as well as its dependencies, including PDK, NDK, etc.

    make ethfw_all

Verbose build can be enabled by setting the **SHOW_COMMANDS** variable as
shown below:

    make ethfw_all SHOW_COMMANDS=1

On successful compilation, the output folder would be created at
`<ethfw_xx.yy.xx.bb>/out`.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Clean {#ethfw_build_clean}

The make commands listed below require the environment setup according to
@ref ethfw_build_setup_env section.


### Clean All {#ethfw_build_clean_all}

Clean EthFw components as well as its dependencies:

    make ethfw_all_clean

### Remove build output {#ethfw_build_clean_binaries}

Remove EthFw build output directory only.

    make scrub

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Profiles {#ethfw_build_profiles}

- **Debug**: Mostly used to development or debugging

      make ethfw_all PROFILE=debug

- **Release**: Recommended to be used for optimized components and production builds

      make ethfw_all PROFILE=release

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Examples Linker File (Select memory location to hold example binary) {#ethfw_build_eg_linker}

The example applications use different memories and this could be changed
and/or reconfigured via linker command files.

- **linker_mem_map.cmd** is autogenerated file using PyTI_PSDK_RTOS tool
  which defines memory layout (addresses and sizes)
    + Available at `<ethfw_xx_yy_zz_bb>/apps/app_<name>/<core>/linker_mem_map.cmd`
- **linker.cmd** defines the section mappings used by EthFw application
    + Sets optimal memories for time critical symbols ("text_fast")
    + Available at `<ethfw_xx_yy_zz_bb>/apps/app_<name>/<core>/linker.cmd`

[Back To Top](@ref ethfw_c_ug_top)

-->
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Running Examples {#ethfw_run_eg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## IDE {#ethfw_run_ccs}

Refer to the instructions in @ref ethfw_instal_ccs section for Code Code Composer
and emulation packs installation as well as Target Configuration File creation.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Load Example Binaries {#ethfw_run_ccs_load_binary}

Refer to @ref demo_top section for a full list of EthFw demo applications.

For detailed steps to load and run the L2 Switching application, please refer
to its @ref demo_l2_switching_steps_top section.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Un Installation {#ethfw_uninstall}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Delete the complete `ethfw_xx_yy_zz_bb` folder.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Compiler Flags used {#ethfw_cflag}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Demo Application - Profile: Debug {#ethfw_cflag_debug}

Flag                       | Description
---------------------------|------------
`-g`                       | Default behavior. Enables symbolic debugging. The generation of debug information do not impact optimizations. Therefore, generating debug information is enabled by default.
`--endian=little`          | Little Endian
`-mv=7R5`                  | Processor Architecture Cortex-R5
`--abi=eabi`               | Application binary interface - ELF
`-eo=.obj`                 | Output Object file extension
`--float_support=vfpv3d16` | VFP coprocessor is enabled
`--preproc_with_compile`   | Continue compilation after using -pp`<X>` options
`-D=TARGET_BUILD=2`        | Identifies the build profile as 'debug'
`-D_DEBUG_=1`              | Identifies as debug build
`-D=SOC_J721E`             | Identifies the SoC type
`-D=J721E`                 | Identifies the device type
`-D=R5F="R5F"`             | Identifies the core type as ARM R5F
`-D=ARCH_32`               | Identifies the architecture as 32-bit
`-D=SYSBIOS`               | Identifies as TI RTOS operating system build
`-D=SIMULATOR`             | Identifies that build is for simulator

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Demo Application - Profile: Release {#ethfw_cflag_release}

Flag                       | Description
---------------------------|------------
`--endian=little`          | Little Endian
`-mv=7R5`                  | Processor Architecture Cortex-R5
`--abi=eabi`               | Application binary interface - ELF
`-eo=.obj`                 | Output Object file extension
`--float_support=vfpv3d16` | VFP coprocessor is enabled
`--preproc_with_compile`   | Continue compilation after using -pp`<X>` options
`--opt_level=3`            | Optimization level 3
`--gen_opt_info=2`         | Generate optimizer information file at level 2
`-D=TARGET_BUILD=1`        | Identifies the build profile as 'release'
`-DNDEBUG`                 | Disable standard-C assertions
`-D=SOC_J721E`             | Identifies the SoC type
`-D=J721E`                 | Identifies the device type
`-D=R5F="R5F"`             | Identifies the core type as ARM R5F
`-D=ARCH_32`               | Identifies the architecture as 32-bit
`-D=SYSBIOS`               | Identifies as TI RTOS operating system build
`-D=SIMULATOR`             | Identifies that build is for simulator


[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Supported Device Families {#ethfw_supported_family}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Device Family | Variant          | Known by other names
--------------|------------------|--------------------
Jacinto 7     | J721E            | -


[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History {#ethfw_rev_history}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


Revision | Date          | Author                 | Description
---------|---------------|------------------------|----------------------
0.1      | 01 Apr 2019   | Prasad J, Misael Lopez | Created for v.0.08.00
0.2      | 02 Apr 2019   | Prasad J               | 0.8 Docs review meeting fixes
0.3      | 12 June 2019  | Prasad J               | Updates for EVM demo (.85 release)

[Back To Top](@ref ethfw_c_ug_top)
