# User Guide {#ethfw_c_ug_top}

This user guide describes EthFw feature list along with steps to build and run
EthFw demo applications.

For additional information about EthFw refer to [EthFw Introduction](@ref ethfw_c_ug_switch)

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# EthFw Demos {#ethfw_c_ug_ethfw_demos}


EthFw Demos showcases usage and integration of EthFw to use & configure CPSW9G IP
in the Jacinto 7 devices. These peripheral/board level sample/demo examples
demonstrates the capabilities of the CPSW9G IP features & EthFw stack.

Listed below are some of the key applications.
Demo                               | Comments
-----------------------------------|--------------
L2 Switching | Configures Switch to enable switching between its external ports
L2/L3 address based classification | Illustrates traffic steering to A72(linux) and R5F(RTOS) based on L2 header. Illustrate with iperf and web server traffic from PC1/PC2
Inter-VLAN Routing (SW)| Showcases Inter-VLAN routing using  lookup and forward operations being done in SW(R5). Showcase low-level lookup and forwarding on top of CPSW LLD
Inter-VLAN Routing (HW)| Illustrates hardware offload support for Inter-VLAN routing. Illustrate Line rate routing with no additional impact on R5 CPU load



## EthFw Switching & TCP/IP Apps Demo {#ethfw_switching_demo}
This demo showcases switching capabilities of the J721E integrated Ethernet Switch
(CPSW9G) for features like VLAN, Multicast etc. This demo also showcases TI NDK (TCP/IP) integration into EthFw. In this demo we showcase usage of HTTP server.

## Inter-VLAN Routing {#ethfw_intervlan_demo}
This demo illustrates Line-rate Inter-VLAN routing in hardware without any additional load on EthFw core.
This demo showcases InterVLAN routing capability of CPSW IP along with software fall-back support.
CPSW ALE classifier feature is used per flow to characterize the route and configure the egress operation.

Available egress operations
- Replace Destination (MAC) Address
- Replace Source (MAC) Address
- Replace VLAN ID
- Optional decrement of Time To Live (TTL)
- Supports IPv4 (TTL) and IPv6 (Hop Limit) fields
- Packets with 0 or 1 TTL/Hop Limit are sent to the host for error processing

For further information, please refer to the @ref demo_l2_switching_ndk_top demo
application documentation.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Supported Features {#ethfw_c_ug_features_list}

Feature         | Comments
----------------|--------------
L2 Switching    | Support for configuration of switch to enable L2 Switching between external ports with VLAN, multi-cast
Inter-VLAN Routing  | Inter-VLAN routing configuration in HW with software fall-back support
NDK Integration | Integration of TCP/IP stack enabling TCP, UDP, HTTP apps
Remote configuration server | Firmware app hosting the IPC server to serve remote clients like Linux Virtual MAC driver
Resource management library | Resource management library for CPSW resource sharing across cores.

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

![](GESI_Board.jpg "J721E EVM GESI Board Top View")

There are four RGMII PHYs in the J721E GESI board as show in the following image.
They will be referred to as **MAC Port 0**, **MAC Port 1**, **MAC Port 2** and
**MAC Port 3** throughout this document.

![](GESI_RJ45_SideView.png "J721E EVM GESI Board connections")

Please refer to the SDK Description for details about installation and getting
started of J721E EVM.

[Back To Top](@ref ethfw_c_ug_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Software Dependencies {#ethfw_depend_sw}

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
moving data between peripherals and memory.

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

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## IDE (CCS) {#ethfw_instal_ccs}

-# Install Code Composer Studio and setup a <b>Target Configuration</b> for
   use with J721E EVM.(refer to @ref ccs_setup_top)

-# Refer to the instructions in @ref ccs_setup_top section for Code Code Composer
   and emulation packs installation as well as Target Configuration File creation.

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

Source code of the EthFw demo applications is in the **apps** directory.
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

- **linker_mem_map.cmd** is auto generated file using PyTI_PSDK_RTOS tool
  which defines memory layout (addresses and sizes)
    + Available at `<ethfw_xx_yy_zz_bb>/apps/app_<name>/<core>/linker_mem_map.cmd`
- **linker.cmd** defines the section mappings used by EthFw application
    + Sets optimal memories for time critical symbols ("text_fast")
    + Available at `<ethfw_xx_yy_zz_bb>/apps/app_<name>/<core>/linker.cmd`

[Back To Top](@ref ethfw_c_ug_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Running Examples {#ethfw_run_eg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Load Example Binaries {#ethfw_run_ccs_load_binary}

Refer to @ref demo_top section for a full list of EthFw demo applications.

For detailed steps to load and run the L2 Switching application, please refer
to its @ref demo_l2_switching_setup_cfg section.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Un Installation {#ethfw_uninstall}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Delete the complete `ethfw_xx_yy_zz_bb` folder.

[Back To Top](@ref ethfw_c_ug_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Known issues {#ethfw_known_issues}

JIRA ID       | Issue Summary | Workaround
--------------|---------------|------------
PROC_BRDS-658 |MAC PORT 0 packet drop seen of Rx/TX - Packet drop is seen on MAC PORT0 (RGMII1) on 1Gbps mode on GESI board of J7 EVM. This port is labelled as PRG1_RGMII1_B (image below) | Don’t connect this port

[Back To Top](@ref ethfw_known_issues)

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
`--float_support=vfpv3d16` | VFP co-processor is enabled
`--preproc_with_compile`   | Continue compilation after using -pp`<X>` options
`-D=TARGET_BUILD=2`        | Identifies the build profile as 'debug'
`-D_DEBUG_=1`              | Identifies as debug build
`-D=SOC_J721E`             | Identifies the SoC type
`-D=J721E`                 | Identifies the device type
`-D=R5F="R5F"`             | Identifies the core type as ARM R5F
`-D=ARCH_32`               | Identifies the architecture as 32-bit
`-D=SYSBIOS`               | Identifies as TI RTOS operating system build

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Demo Application - Profile: Release {#ethfw_cflag_release}

Flag                       | Description
---------------------------|------------
`--endian=little`          | Little Endian
`-mv=7R5`                  | Processor Architecture Cortex-R5
`--abi=eabi`               | Application binary interface - ELF
`-eo=.obj`                 | Output Object file extension
`--float_support=vfpv3d16` | VFP co-processor is enabled
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
0.3      | 12 Jun 2019   | Prasad J               | Updates for EVM demo (.85 release)
0.4      | 17 Jul 2019   | Misael Lopez           | Updates for v.0.09.00

[Back To Top](@ref ethfw_c_ug_top)
(@ref ethfw_c_ug_top)
