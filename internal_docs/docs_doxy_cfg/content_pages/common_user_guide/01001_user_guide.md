# User Guide {#ethfw_c_ug_top}

[TOC]

# Integrated Switch {#ethfw_c_ug_switch}

SoC's such as J721E, integrates an Ethernet Switch as an chip-in-chip. The
combined features of SOC and Switch IP(CPSW9G) can allow Ethernet switch to
function continuously enabling unaffected switching on external ports regardless
of the state of the rest of the device.
<img src="j7_switch_for_replacement_of_external_switch.png" alt="J721E switch
    replacing external switch" width="500" height="500" title="switch replacing external switch" />
<div class="caption" align="center">J721E Switch as a replacement to external switch  </div>

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# EthFw  {#ethfw_c_ug_fw_architecture}

The Ethernet Firmware is TI RTOS based application for configuration of
Ethernet switch. The package contains remote configuration server, resource management
library, switch resident protocols, proxy layers to handle local and remote API calls
and demonstration applications (EthFw Demos).
The switch software uses PDK CPSW and other drivers for respective IP configuration.
Its is expected to be hosted on Cortex R5F in Main Domain.

<img src="switch_software_stack.png" alt="Switch Architecture" width="500" height="500"
        title="Ethernet Switch Software Architecture" />
<div class="caption" align="center">Ethernet Switch Software Architecture  </div>

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# EthFw Demos {#ethfw_c_ug_ethfw_demos}

Demonstrates usage of software provided for EthFw. These peripheral/board
level sample/demo examples demonstrates the capabilities of the Ethernet Switch
exploiting the IP features.

Listed below are some of the key applications.
Demo   | Comments
-------|---------
L2 Switching  |  Configures Switch to enable switching between it's external ports.
Eth Profiling |  Application to determine the CPU load transmission & reception of Ethernet messages

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Supported Features {#ethfw_c_ug_features_list}

Feature   | Comments
---------|---------
L2 Switching      |  Support for configuration of switch to enable L2 Switching between external ports
Profiling      |  Support for profiling of Ethernet firmware

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Dependencies {#ethfw_instal_top}

Dependencies can be categorized as listed below. Please note that
depending on the intended use, the dependencies vary (e.g. for integration vs
running demo applications only)

-# Hardware Dependencies (@ref ethfw_depend_hw)
-# Software Dependencies (@ref ethfw_depend_sw)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Hardware Dependencies {#ethfw_depend_hw}

EthFw is supported on the boards/EVM listed below
- J721E VLAB (@ref ethfw_depend_vlab_j721e)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### J721E VLAB {#ethfw_depend_vlab_j721e}
Please refer SDK VLAB Description for details about installation & getting started
of VLAB.
Below are steps to run basic switching demo on the VLAB with MCU2_1.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Software Dependencies {#ethfw_depend_sw}

Below listed dependencies are part of Processor SDK package.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### PDK {#ethfw_depend_pdk}
"PDK" is component within Processor SDK. Following section list the sub-components
of PDK that are used / required by EthFw.

Please check release note that came with this release for the compatible version
of PDK/SDK
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#### CPSW LLD {#ethfw_depend_pdk_cpsw}
This is CPSW driver module used to program the CPSW9G(Switch) IP. EthFw receives
commands/configuration from application and uses CPSW LLD to configure CPSW9G.
- The CPSW LLD module relies on UDMA driver

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#### CSL {#ethfw_depend_pdk_csl}
Chip Support Library : Implements peripheral register level and functional level
    API's. CSL also provides peripheral base addresses, register offset,
    C MACROS to program peripheral registers

- EthFw uses CSL to determine peripheral addresses and program peripheral registers.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#### UDMA {#ethfw_depend_pdk_udma}
UDMA is used to move data between peripherals and memory.

- The CPSW LLD module relies on UDMA driver

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### NDK {#ethfw_depend_ndk}
"NDK" is TI TCP/IP stack used in EthFw for running local TCP/IP applications and
for running switch resident protocols like telnet and EAPOL etc. as shown is
switch software stack architecture diagram @ref ethfw_c_ug_fw_architecture

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#### EthFw Basic Switching Application {#ethfw_depend_eg}

- Switching application is based on TI RTOS and build is XDC based.
- Applications rely on UART driver to print on console

[Back To Top](@ref ethfw_c_ug_top)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#### EthFw SW Demo Application {#ethfw_demo_depend_eg}

- Applications rely on TI RTOS for OS features such as
    - Task's
    - Sempahores
    - Interrupt handling
- Applications rely on PDK UART driver to print on console

[Back To Top](@ref ethfw_c_ug_top)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

[Back To Top](@ref ethfw_c_ug_top)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## IDE (CCS) {#ethfw_instal_ccs}
Code Composer Studio is an integrated development environment (IDE) that supports TI's Microcontroller and Embedded Processors portfolio.

* [CCS Link](http://www.ti.com/tool/ccstudio?jktype=recommendedresults)
* [Download](http://processors.wiki.ti.com/index.php/Download_CCS)

### J721E {#ethfw_instal_ccs_gel_setup}
-# Supported CCS version is detailed in SDK Release Notes
-# Installation and configuration of GEL files is detailed in SDK How To

[Back To Top](@ref ethfw_c_ug_top)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


# Installation Steps {#ethfw_instal_steps}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EthFW software is an add-on package,

* Unzip the package zip into SDK base folder, the expected directory structure is as show below ([J721E](@ref mcuw_post_install_j721e))
* In case EthFw was installed in a different directory, one could move the complete directory as shown.Please note that this is an indicative snap-shot.Modules/Drivers versions could be modified for each release.

## J721E {#mcuw_post_install_j721e}
![](c_ug_install_dir_1.png "Directory structure post installation")


[Back To Top](@ref ethfw_c_ug_top)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Directory Structure {#ethfw_dir}

Post installation of EthFw, the following directory would be created. Please
note that this is an indicative snap-shot. Modules could be added/modified.

![](c_ug_dri_1.png "Top Level Directory Structure")


[Back To Top](@ref ethfw_c_ug_top)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

![](c_ug_dri_5.png "EthFw Demo")

## EthFw Demonstration Applications {#ethfw_dir_switch_demos}

### Utilities {#ethfw_dir_ethfw_utils}

- <b>demo_utils</b> : Utility functions

### Demos {#ethfw_dir_ethfw_demos_apps}

- <b>Basic Switching </b> : Demo application which configures the CPSW9G (Switch)
                            for switching between external ports.

- <b>Profiling</b> : Applications used to measure performance of NDK TCP and UDP
                     apps.

[Back To Top](@ref ethfw_dir_switch_demos)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Build {#ethfw_build_top}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EthFw employs concerto make based build mechanism. When building on Windows based machine,
tool such as [Cygwin](https://www.cygwin.com/) could be used.

## Setup Environment {#ethfw_build_setup_env}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Following changes are required to be performed in Rules.make to build

-# <b> Rules.make </b> can be found at (SDK Install Directory)/ethfw_xx.yy.xx.bb/build, When building on Windows environment ensure to update variables under <b>ifeq ($(OS),Windows_NT)</b>
-# Choose to build <b> either </b> EthFw Demo Applications
    + EthFw demo application uses TI RTOS (sysBios)
    + Set <b>BUILD_OS_TYPE = tirtos</b> to build MUCSS demo applications
    + Set <b>BUILD_OS_TYPE = baremetal</b> to build MCAL examples/libraries
    + <img style="float: left;" src="caution.png"> To switch between MCAL & EthFw demo applications a <b> clean build </b> would be required. As the EthFw demo applications will use custom MCAL configurations.
-# Specify the location of the compiler
    + Typically PDK includes the required compiler
    + In Rules.make, update <b> SDK_INSTALL_PATH </b> to specify location of the SDK installation.
    + One can override compiler path by updating variable <b>TOOLCHAIN_PATH_R5</b>
-# Specify Version of PDK being used
    + In Rules.make, update <b> PDK_PACKAGE_VER </b> to specify version of the pdk
    + (@ref ethfw_depend_sw) lists the components of pdk that are required for MCAL.
-# Specify Version of EthFw being used
    + In Rules.make, update <b> ETHFW_PACKAGE_VER </b> to specify version of the EthFw
-# Enable / Disable logging messages to UART
    + Flag <b>EthFw_UART_ENABLE</b>, when set to TRUE directs the messages from example application to UART console. When set to FALSE, these messages are displayed in CCS console.
-# Windows environment only
    + Specify location of the Cygwin utils, update <b> utils_PATH </b>
    + All the required utils are provided with CCS installation. The default value
        in Rules.make/utils_PATH specify the path with in CCS installation.

-# Also refer [CCS One Time Setup] (@ref ethfw_instal_ccs_gel_setup)

## Build Everything MCAL {#ethfw_build_all_mcal}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
With steps listed at (@ref ethfw_build_setup_env) all MCAL modules can
be built
- Ensure <b>BUILD_OS_TYPE = baremetal</b>
- Go to folder (SDK Install Directory)/ethfw_xx.yy.xx.bb/build
- Linux <b>make -s all</b> and <b>gmake -s all</b> for Windows
- Use option -j to speed up compilation
- On Successful compilation, binary folder would be created in (SDK Install Directory)/ethfw_xx.yy.xx.bb/binary/(driver name)_app/bin/am65xx_evm

## Build Everything Demos {#ethfw_build_all_demos}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
With steps listed at (@ref ethfw_build_setup_env) completed MCAL modules can
be built
- Ensure <b>BUILD_OS_TYPE = tirtos</b>
- Go to folder (SDK Install Directory)/ethfw_xx.yy.xx.bb/build
- Linux <b>make -s all</b> and <b>gmake -s all</b> for Windows
- Use option -j to speed up compilation
- On Successful compilation, binary folder would be created in (SDK Install Directory)/ethfw_xx.yy.xx.bb/binary/(driver name)_app/bin/am65xx_evm

## Profiles {#ethfw_build_profiles}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
- Debug : Mostly used to development or debugging
    + **make -s BUILD_PROFILE=debug**
    + **gmake -s BUILD_PROFILE=debug** for Windows
- Release : Recommended to be used for production
    + **make -s BUILD_PROFILE=release**
    + **gmake -s BUILD_PROFILE=release** for Windows

## Other useful commands {#ethfw_build_lib}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-# To Build all examples (all MCAL drivers and their associated examples)
    + **gmake -s examples**
-# To build libraries only (only MCAL driver are built as library)
    + **gmake -s ethfw_libs**
-# To build specific example (associated MCAL driver will also be built)
    + **gmake -s can_app**
        + Other examples dio_app, eth_app, gpt_app, mcspi_app
        + The above list is indicative and examples could be added/deleted
        + To determine example name, refer <b> makefile </b> in subdirectory of (SDK Install Directory)/ethfw_xx.yy.xx.bb/mcal/examples/
            + gmake -help, will list available target names
    + **gmake can_profile_app -s**

## Examples Linker File (Select memory location to hold example binary) {#ethfw_build_eg_linker}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
The example applications use different memory and this could be changed/re-configured.

- **linker_r5.lds** defines the memory locations used by the MCAL examples.
    + The linker file is specific to a device and available at (SDK Install Directory)/ethfw_xx.yy.xx.bb/build/(device family name)/linker_r5.lds
    + In case of DRA80x,(SDK Install Directory)/ethfw_xx.yy.xx.bb/build/am65xx/linker_r5.lds
- **linker_r5_sysbios.lds** defines the memory locations used by the MCU SS
    demo applications
    + The linker file is specific to a device and available at (SDK Install Directory)/ethfw_xx.yy.xx.bb/build/(device family name)/linker_r5_sysbios.lds
    + In case of DRA80x,(SDK Install Directory)/ethfw_xx.yy.xx.bb/build/am65xx/linker_r5_sysbios.lds

- .text and .bss sections are stored in MSMC (Multicore Shared Memory Controller, please refer device specific TRM for details)
- all other sections are stored in cores internal memory

# Running Examples {#ethfw_run_eg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## IDE {#ethfw_run_ccs}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

### CCS {#ethfw_run_ccs_setup}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Please refer (@ref ethfw_instal_ccs) for CCS and GEL setup

#### Load Example Binaries {#ethfw_run_ccs_load_binary}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
##### DRA80x
-# In core MCU_PULSAR_Cortex_R5_0, load binary <b>(driver name)_app_mcu1_0_(release or debug).xer5f</b>
    + DRA80x MCAL Binaries is available at (SDK Install Directory)/ethfw_xx.yy.xx.bb/binary/(driver name)_app/bin/am65xx_evm
-# Run example
    + Expect to see prints on CCS console or UART console (@ref ethfw_build_setup_env)

## SBL {#ethfw_run_sbl}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### DRA80x
PDK contains a Secondary Boot Loader (SBL) and example applications running on EthFw can be booted using the SBL.
Detail about SBL can be found from [SDK SBL User Guide] (http://software-dl.ti.com/processor-sdk-rtos/esd/docs/latest/rtos/Foundational_Components.html#am65x)

<img style="float: left;" src="caution.png"> By default the MCAL example application prints messages on CCS console, when using SBL to run example applications, MCUSW should be configured to send messages to UART. To enable messages to be sent to UART, set <b>MCUSW_UART_ENABLE ?= TRUE</b> in <b>Rules.make</b> (Refer @ref ethfw_build_setup_env)

Once an EthFw application is built, it generates *.appimage in binary folder (SDK Install Directory)/ethfw_xx.yy.xx.bb/binary/(example_name)/bin/am65xx_evm/(example_name)_mcu1_0_release.appimage
e.g. <b>can_app_mcu1_0_release.appimage</b>

<img style="float: left;" src="caution.png"> Depending on the required boot media, ensure to configure the boot mode switches, [Refer Section "Boot Modes"] (http://www.ti.com/lit/pdf/spruim7)
- For MMC SD boot mode
    + <b>SW3.2</b> and <b>SW3.3</b> should be ON, others can be OFF
    + <b>SW2.3</b> should be ON, others can be OFF
- For OSPI boot mode
    + <b>SW3.1</b> and <b>SW3.8</b> should be ON, others can be OFF
    + <b>SW2.x</b> not required

## Preparing SBL for MMCSD Or OSPI {#ethfw_run_sbl_prepare_image}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

To build the SBL binary for SD/MMC or OSPI boot media, please use the following command:
\verbatim
$cd (SDK Install Directory)/ethfw_xx.yy.xx.bb/build
$make sbl_mmcsd
$make sbl_ospi
\endverbatim

Post compilation of SBL, the SBL binary can be found at (SDK Install Directory)/(pdk-install-folder)/packages/ti/boot/sbl/binary/(boot-media)/am65xx_evm/

- <b> MMC SD </b>: Section [Booting Via SD Card] (http://software-dl.ti.com/processor-sdk-rtos/esd/docs/latest/rtos/Foundational_Components.html#am65x) details the steps to load *.appimage and sbl_mmcsd_img_mcu1_0_release.tiimage into an MMC SD card and boot

- <b> OSPI </b>: Section [Booting Via OSPI flash] (http://software-dl.ti.com/processor-sdk-rtos/esd/docs/latest/rtos/Foundational_Components.html#am65x) details the step to load *.appimage and sbl_ospi_img_mcu1_0_release.tiimage in to OSPI flash and booted.

    + [Steps to programe the OSPI Flash] (http://software-dl.ti.com/processor-sdk-rtos/esd/docs/latest/rtos/Board_EVM_Abstration.html#uniflash)


# Un Installation {#ethfw_uninstall}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Run <b>uninstall.exe</b> or delete the complete ethfw_xx_yy_zz_bb folder.

# Compiler Flags used {#ethfw_cflag}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## MCAL Drivers - Profile : Release {#ethfw_cflag_drv_rel}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Flag | Description
-----|------------
-g  | Default behavior. Enables symbolic debugging. The generation of debug information do not impact optimizations. Therefore, generating debug information is enabled by default.
-ms | Place each function in a separate subsection
-DMAKEFILE_BUILD    | Defines the build support infra i.e makefile based.
-c  | Disables linking
-qq | Super Quite Mode
-pdsw225    | Categorizes the diagnostic identified by num as a warning
--endian=little | Little Endian
-mv7R5  | Processor Architecture Cortex-R5
--abi=eabi  | Application binary interface - ELF
-eo.oer5f   | Output Object file extension
-ea.ser5f   | Output assembly file extension
--symdebug:dwarf    | Generate symbolic debug in DWARF format
--embed_inline_assembly | Embed inline assembly in code for optimization
--float_support=vfpv3d16    | VFP coprocessor is enabled
--emit_warnings_as_errors   | Treat warning as errors
-ms | Place each function in a separate sub section
-oe | Required by PDK
-O3 | Optimization Level
-op0    | Specifies that functions provided in a C file could potentially be called from different C file.
-os | Interlists optimizer comments with assembly statements
--optimize_with_debug   | Optimize fully in the presence of debug
--inline_recursion_limit=20 | Required by PDK
-DBUILD_MCU1_0  | Identifies Core in the domain
-DBUILD_MCU | Identifies Domain
-DSOC_AM65XX    | Device Identifier

## MCAL Examples - Profile : Release {#ethfw_cflag_eg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Same flags that were used for driver (@ref ethfw_cflag_drv_rel), additional flags listed below
- UART_ENABLED : Configure example application to use UART / Console to display messages.

# Supported Device Families {#ethfw_supported_family}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Device Family | Variant          | Know by other names
---------|---------------|---------------------------
DRA80x    | dra80xx   | AM65x

# Document Revision History {#ethfw_rev_history}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Revision | Date          | Author             | Description
---------|---------------|--------------------|------------
0.1      | 25 Mar 2019   | Prasad J           | Created for V 00 08 00

