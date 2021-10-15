# User Guide {#ethfw_c_ug_top}

This user guide presents the list of features supported by the Ethernet Firmware
(EthFw) and describes the steps required to build and run the EthFw demo
applications.

For additional information about EthFw refer to [EthFw Introduction](@ref ethfw_c_ug_switch).

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# EthFw Demos {#ethfw_c_ug_ethfw_demos}

The EthFw demos showcase the integration and usage of the Ethernet Firmware
which provides a high-level interface for applications to configure and use the
integrated Ethernet switch peripheral (CPSW9G).

The following sample applications are key to demonstrate the capabilities of the
CPSW9G/CPSW5G hardware as well as the EthFw stack.

Demo                               | Comments
-----------------------------------|--------------
L2 Switching | Configures CPSW9G switch to enable switching between its external ports
L2/L3 address based classification | Illustrates traffic steering to A72 (Linux) and R5F (RTOS) based on Layer-2 Ethernet header. iperf tool and web servers are used to demonstrate traffic steering to/from PCs connected to the switch
Inter-VLAN Routing (SW) | Showcases inter-VLAN routing using lookup and forward operations being done in SW (R5F). It also showcases low-level lookup and forwarding on top of Enet LLD
Inter-VLAN Routing (HW) | Illustrates hardware offload support for inter-VLAN routing, demonstrating the CPSW5G/CPSW9G hardware capabilities to achieve line rate routing without additional impact on R5F CPU load


## EthFw Switching & TCP/IP Apps Demo {#ethfw_switching_demo}

This demo showcases switching capabilities of the integrated Ethernet Switch
(CPSW9G or CPSW5G) found in J721E or J7200 devices for features like VLAN,
Multicast, etc.  It also demonstrates lwIP (TCP/IP stack) integration into
the EthFw.


## Inter-VLAN Routing Demo {#ethfw_intervlan_demo}

This demo illustrates hardware and software based inter-VLAN routing.  The
hardware inter-VLAN routing makes use of the CPSW9G/CPSW5G hardware features
which enable line-rate inter-VLAN routing without any additional CPU load on
the EthFw core.  The software inter-VLAN routing is implemented as a
fall-back alternative.

The hardware inter-VLAN route demo exercises the CPSW ALE classifier feature,
which is used per flow to characterize the route and configure the egress
operation.

Available egress operations:
- Replace Destination (MAC) Address
- Replace Source (MAC) Address
- Replace VLAN ID
- Optional decrement of Time To Live (TTL)
- Supports IPv4 (TTL) and IPv6 (Hop Limit) fields
- Packets with 0 or 1 TTL/Hop Limit are sent to the host for error processing

For further information, please refer to the @ref demo_ethfw_combined_top demo
application documentation.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Supported Features {#ethfw_c_ug_features_list}

Feature         | Comments
----------------|--------------
L2 switching    | Support for configuration of the Ethernet Switch to enable L2 switching between external ports with VLAN, multi-cast
Inter-VLAN routing | Inter-VLAN routing configuration in hardware with software fall-back support
lwIP integration | Integration of TCP/IP stack enabling TCP, UDP.
Remote configuration server | Firmware app hosting the IPC server to serve remote clients like Linux Virtual MAC driver
Resource management library | Resource management library for CPSW resource sharing across cores

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
- @ref ethfw_depend_evm_gesi_j721e (J721E only)
- @ref ethfw_depend_evm_quadport_j7200 (J7200 only)


### J721E/J7200 EVM {#ethfw_depend_evm_j721e}

![](J7EVM_CPSW_TopView.png "J721E/J7200 EVM connections")


### J721E GESI Expansion Board {#ethfw_depend_evm_gesi_j721e}

![](GESI_Board.png "J721E EVM GESI Board Top View")

There are four RGMII PHYs in the J721E GESI board as shown in the following image.
They will be referred to as **MAC Port 0**, **MAC Port 1**, **MAC Port 2** and
**MAC Port 3** throughout this document.

![](GESI_RJ45_SideView.png "GESI Board connections")

Please refer to the SDK Description for details about installation and getting
started of J721E EVM.

**Note:** GESI expansion board is also available in J7200 EVM, but only one MAC
port is routed to the CPSW5G in J7200, hence GESI board is not enabled and used
by default in the Ethernet Firmware for J7200.

[Back To Top](@ref ethfw_c_ug_top)


### J7200 Quad-Port Eth Expansion Board {#ethfw_depend_evm_quadport_j7200}

There is one QSGMII PHY in the Quad Port Eth expansion board as shown in the
following image.  It enables four MAC ports which will be referred to as
**MAC Port 0**, **MAC Port 1**, **MAC Port 2** and **MAC Port 3** throughout
this document.

![](QPENet_Board.png "Quad Port Eth Board connections")

Please refer to the SDK Description for details about installation and getting
started of J7200 EVM.

**Note:** Quad Port Eth expansion board is also available in J721E EVM, but it's
not enabled by default in the Ethernet Firmware for J721E.

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

PDK includes an UDMA LLD which provides APIs that the Enet LLD relies on to send and
receive packets to the CPSW's host port.


#### Enet LLD {#ethfw_depend_pdk_enet}

This is Ethernet driver module used to program the CPSW5G or CPSW9G (Switch) IP.
EthFw receives commands/configuration from application and uses Enet LLD to
configure CPSW5G/CPSW9G.

Enet LLD supports other Ethernet peripherals available in TI SoCs and provides a
unified interface to program them.


### lwIP {#ethfw_depend_lwip}

lwIP is a free TCP/IP stack developed by Adam Dunkels at the Swedish Institute of
Computer Science (SICS) and licensed under a modified BSD license (completely
open-source).

The focus of the LwIP TCP/IP implementation is to reduce RAM usage while keeping a
full scale TCP/IP stack thus making it suitable for our requirements.

LwIP supports the following features:

- IPv4 and IPv6 (Internet Protocol v4 and v6)
- ICMP (Internet Control Message Protocol) for network maintenance and debugging
- IGMP (Internet Group Management Protocol) for multicast traffic management
- UDP (User Datagram Protocol)
- TCP (Transmission Control Protocol)
- DNS (Domain Name Server)
- SNMP (Simple Network Management Protocol)
- DHCP (Dynamic Host Configuration Protocol)
- PPP (Point to Point Protocol)
- ARP (Address Resolution Protocol)

Starting in SDK 8.0, Ethernet Firmware has been migrated to lwIP stack.  The actual
integration of lwIP into J721E/J7200 devices is done through Enet LLD, which
implements the lwIP netif driver interface.

The Enet LLD lwIP driver interface implementation can be located at:
`<pdk>/packages/ti/drv/enet/lwipif/src`.

The lwIP configuration file (lwipopts.h) contains the lwIP stack features that are
enabled by default in the Enet LLD driver implementation, such as TCP, UDP, DHCP, etc.
It's located at `<pdk>/packages/ti/transport/lwip/lwip-port/freertos/include/lwipopts.h`.
User should also refer to this file if interested on enabling any of the different
lwIP debug options.

The lwIP pool configuration file (lwippools.h) contains the different pools and their
sizes required by the Enet LLD lwIP interface implementation. This file is located at
`<pdk>/packages/ti/drv/transport/lwip/lwip-port/freertos/include/lwippools.h`.

#### Ethernet Firmware Proxy ARP {#ethfw_depend_lwip_proxyarp}

Enet LLD lwIP interface implementation provides a hook to let application *process*
a packet and indicate whether the packet needs additional handling (i.e. be passed to
the lwIP stack) or if the packet can be recycled (i.e. already handled by the
application).

This feature enables Ethernet Firmware to implement Proxy ARP functionality needed to
respond to ARP Request packets on behalf of Ethernet Firmware's remote core clients
as broadcast packets are passed exclusively to Main R5F core 0, not to each individual
remote core.

Ethernet Firmware sets up a dedicated UDMA RX flow where packets that have ARP
EtherType and broadcast destination MAC address are routed to.  While lwIP interface
is processing packets from this RX flow, it will call the *packet processing* function
registered by Ethernet Firmware.  Ethernet Firmware then checks if the packet is meant
for any of its remote core clients, if so, it responds on its behalf and packet is
recycled as it needs not be passed to lwIP stack.  If the packet is not meant to any
of the remote cores, it's simply passed to the lwIP stack, ARP request packets meant
for Ethernet Firmware itself fall into this processing category.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## IDE (CCS) {#ethfw_instal_ccs}

-# Install Code Composer Studio and setup a <b>Target Configuration</b> for
   use with J721E or J7200 EVM.

-# Refer to the instructions in @ref ccs_setup_top section for Code Code Composer
   and emulation packs installation as well as Target Configuration file creation.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Installation Steps {#ethfw_instal_steps}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Ethernet Firmware and its dependencies are part of the SDK, separate installation is not required.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Directory Structure {#ethfw_dir}

Post installation of SDK, the following directory would be created. Please
note that this is an indicative snap-shot, modules could be added/modified.

The top-level EthFw makefile as well as the auxiliary makefiles for build flags
(**ethfw_build_flags.mak**) and build paths (**ethfw_tools_path.mak**)
can be found at the EthFw top-level directory.


## Post Install Directory Structure {#ethfw_post_install_j721e}

![](c_ug_dir_top.png "Top Level Directory Structure")


## Utilities Directory Structure {#ethfw_dir_utils}

The **utils** directory contains miscellaneous utilities required by the EthFw
applications.

![](c_ug_dir_utils.png "Utilities Directory Structure")


## Demo Application Sources Directory Structure {#ethfw_dir_demo}

Source code of the EthFw demo applications is in the **apps** directory.
For instance, below image shows the directory structure of the server application
which implements L2 switch, inter-VLAN routing, etc.

![](c_ug_dir_l2_switching_demo.png "EthFw Server-side Application Directory Structure")

Pre-compiled binaries are also provided as part of the EthFw release, which can
be found in the **out** directory. For instance, below image shows the EthFw
output directory structure with pre-compiled server and client binaries.

![](c_ug_dir_j721_r5f_demo.png "Demo Binaries Directory Structure")

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## EthFw Demonstration Applications {#ethfw_dir_switch_demos}

Refer to @ref demo_top section for a full list of EthFw demo applications.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Build {#ethfw_build_top}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EthFw employs Concerto makefile-based build system. When building on a Windows based
machine, tools such as [Cygwin](https://www.cygwin.com/) could be used.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Setup Environment {#ethfw_build_setup_env}

The tool paths required by the build system are defined in the `ethfw_tools_path.mak`
makefile. The default paths in `ethfw_tools_path.mak` are defined based on the assumption
that the EthFw package has been installed inside the Processor SDK main directory.

Typically, the Processor SDK installation path is `~/ti` in Linux-based systems.
So, a typical EthFw installation would be at `~/ti/ti-processor-sdk-rtos-j721e-evm-08_xx_yy_zz`.
In this case, no additional environment setup steps are required.

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

Build EthFw components as well as its dependencies, including PDK, lwIP, etc.

    make ethfw_all BUILD_SOC_LIST=J721E

or

    make ethfw_all BUILD_SOC_LIST=J7200

Verbose build can be enabled by setting the **SHOW_COMMANDS** variable as
shown below:

    make ethfw_all BUILD_SOC_LIST=<SOC> SHOW_COMMANDS=1

On successful compilation, the output folder would be created at
`<ethfw>/out`.

### QNX Build {#ethfw_qnx_build_all}

Build EthFw components for QNX OS integration running on A72.

    make ethfw_all BUILD_SOC_LIST=<SOC> BUILD_QNX_A72=yes

For QNX integration, the **BUILD_QNX_A72** flag will make sure that EthFW would not
load the IPC resource table, unlike in Linux.

When building for Linux, the **BUILD_QNX_A72** can be omitted.

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

      make ethfw_all BUILD_SOC_LIST=<SOC> PROFILE=debug

- **Release**: Recommended to be used for optimized components and production builds

      make ethfw_all BUILD_SOC_LIST=<SOC> PROFILE=release

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Examples Linker File (Select memory location to hold example binary) {#ethfw_build_eg_linker}

The example applications use different memories and this could be changed
and/or re-configured via linker command files.

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

For detailed steps to load and run the demo application, please refer to the
@ref demo_ethfw_combined_setup_cfg section.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Un Installation {#ethfw_uninstall}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Delete the complete `ethfw_xx_yy_zz_bb` folder.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Known issues {#ethfw_known_issues}

Please refer to the Ethernet Firmware Release Notes.

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
`-D=SOC_J721E`             | Identifies the J721E SoC type
`-D=J721E`                 | Identifies the J721E device type
`-D=SOC_J7200`             | Identifies the J7200 SoC type
`-D=J7200`                 | Identifies the J7200 device type
`-D=R5F="R5F"`             | Identifies the core type as ARM R5F
`-D=ARCH_32`               | Identifies the architecture as 32-bit
`-D=SYSBIOS`               | Identifies as TI RTOS operating system build
`-D=FREERTOS`              | Identifies as FreeRTOS operating system build

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
`-D=SOC_J721E`             | Identifies the J721E SoC type
`-D=J721E`                 | Identifies the J721E device type
`-D=SOC_J7200`             | Identifies the J7200 SoC type
`-D=J7200`                 | Identifies the J7200 device type
`-D=R5F="R5F"`             | Identifies the core type as ARM R5F
`-D=ARCH_32`               | Identifies the architecture as 32-bit
`-D=SYSBIOS`               | Identifies as TI RTOS operating system build
`-D=FREERTOS`              | Identifies as FreeRTOS operating system build


[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Supported Device Families {#ethfw_supported_family}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Device Family | Variant          | Known by other names
--------------|------------------|--------------------
Jacinto 7     | J721E, J7200     | -


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
0.5      | 15 Oct 2019   | Misael Lopez, Santhana Bharathi | Updates for v.1.00.00
1.0      | 28 Jan 2020   | Misael Lopez           | Updates for SDK 6.02.00
1.1      | 31 Aug 2020   | Misael Lopez           | Added J7200 support for SDK 7.01 EA
1.2      | 02 Nov 2020   | Misael Lopez           | Updated for Enet LLD migration

[Back To Top](@ref ethfw_c_ug_top)
(@ref ethfw_c_ug_top)
