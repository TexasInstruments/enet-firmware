# Eth & EthTrcv User Guide {#ug_eth_top}

[TOC]

# Introduction {#ug_eth_intro}
This document details AUTOSAR BSW ETH & ETHTRCV module implementations
- Supported AUTOSAR Release         <b>: 4.2.1</b>
- Supported Configuration Variants  <b>: Pre-Compile</b>
- Vendor ID                         <b>: ETH_VENDOR_ID (44), ETHTRCV_VENDOR_ID (44)</b>
- Module ID                         <b>: ETH_MODULE_ID (88), ETHTRCV_VENDOR_ID (73)</b>

The ETH module initializes, configures and controls the Gigabit Ethernet
Switch (CPSW) in the DRA80xx device family as detailed in the AUTOSAR BSW ETH
Driver Specification.

The ETHTRCV module initializes and configures the Ethernet transceiver (PHY)
as detailed in the AUTOSAR BSW ETHTRCV Driver Specification.

Following section highlights key aspects of this implementation, which would
be of interest to an integrator.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Eth Driver Architecture/Design {#ug_eth_design}

Please refer the ETH design page, which is included as part of release [[3]
(@ref design_eth_top)].

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Functional Description {#ug_eth_functional_top}

This ETH driver implementation supports the <i>Gigabit Ethernet Switch</i>
(CPSW) peripheral present in the DRA80xx devices.  The CPSW peripheral has an
Ethernet port (port 1) which supports RGMII and RMII interfaces, and a host
port (port 0) which supports the internal <i>Communications Port Programming
Interface</i> (CPPI). The ETH driver uses the UDMA driver APIs to setup data
transfers to/from the CPPI port.

The ETH driver implements single UDMA channel for data transmission and single
channel (flow) for data reception.  Only the CPSW default thread ID is enabled
and configured according to the UDMA receive channel's default flow. Interrupts
can be enabled for DMA transmit and receive completion events.

The DMA transfers are based on descriptors called <em>Host Mode Packet
Descriptors</em> (HMPD).  The descriptors are given to and retrieved from
the UDMA via Ring Accelerators.  There are three rings used per data direction
in this implementation:

- Transmit
  - Free Queue Ring - Descriptors with the address and length of the buffers
    to be transmitted are queued into this ring.  In normal conditions,
	only CPSW will dequeue descriptors from this queue
  - Completion Queue Ring - Descriptors that correspond to Ethernet frames
    which have already been consumed by the CPSW are placed in this queue.
	CPSW is the producer and host is the consumer of this ring
  - Tear-down Completion Queue Ring - This ring is used only when the UDMA
    channel is torn down
- Receive
  - Free Queue Ring - Descriptors with the address and length of free buffers
    to be filled with incoming Ethernet frames are queued into this ring. In
	normal conditions, only the CPSW will dequeue descriptors from this queue
  - Completion Queue Ring - Descriptors that correspond to buffers filled with
    new data from incoming Ethernet frames are placed in this queue. CPSW is
	the producer and host is the consumer of this ring
  - Tear-down Completion Queue Ring - This ring is used only when the UDMA
    channel is torn down

The depth of each ring as well as its associated memory is configurable. The
ring memories can be any memory in the system, but it's recommended that they
are placed in a fast memory (i.e. OCMRAM or MSMC3). The depth of these rings
is determined by the number of TX and RX buffers set in the driver
configuration (<b>EthTxBufTotal</b> and <b>EthRxBufTotal</b>).

Similarly, the HMPDs can be placed in any memory of the system, but it's
recommended that they are placed in OCMRAM or MSMC3 as well.

The <i>Management Data I/O</i> interface (MDIO) of the CPSW peripheral is used
by the ETH driver to implement the MII register read and write APIs which are
ultimately used by the ETHTRCV driver to configure the Ethernet transceiver
(PHY). The ETH driver handles the MDIO interrupt which indicates the completion
event of the Ethernet transceiver register accesses.

The <i>Address Lookup Engine</i> (ALE) of the CPSW peripheral is used by the ETH
driver to implement the receive filter API. The ALE provides 64 entries that
can be used to set filter rules.

The Statistics submodule of the CPSW peripheral is used by the ETH driver to
implement the statistics and drop count APIs.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Ethernet interrupt to ISR mapping {#ug_eth_functional_id_mapping}

The following table lists the mapping between Ethernet interrupts and the
corresponding interrupt service routines.

Interrupt Number | Description | Associated ISR
-----------------|-------------|---------------
Configurable | DMA RX Completion | Eth_RxIrqHdlr_0
Configurable | DMA TX Completion | Eth_TxIrqHdlr_0
35 | MDIO Access Completion | Eth_MdioIrqHdlr_0

The DMA interrupt numbers can be set via the Ethernet driver configuration
parameters <b>dmaTxChIntrNum</b> and <b>dmaRxChIntrNum</b>.

There are 9 interrupts allocated for the MCU1_0 in this release, starting at
interrupt number MCU0_INTR_NAVSS0_R5_0_PEND_0 + 10 (74).  The interrupt numbers
passed to the Ethernet driver configuration must be within that range,
otherwise the interrupt allocation will fail while enabling the Ethernet
controller.

Please refer to the UDMA Driver Resource Manager allocation for further
details.

There are no interrupts in the ETHTRCV module.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Configuration {#ug_eth_functional_cfg}

The Eth Driver implementation in this release only supports the Pre-Compile
variant.  The driver expects generated <b>Eth_Cfg.h</b> and <b>Eth_Cfg.c</b>
to be present at the locations specified in the
@ref ug_eth_functional_eth_filestruct_top section.

Similarly, the EthTrcv Driver implementation only supports the Pre-Compile
variant.  The driver expects generated <b>EthTrcv_Cfg.h</b> and
<b>EthTrcv_Cfg.c</b> to be present at the locations specified in the
@ref ug_eth_functional_ethtrcv_filestruct_top section.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Variance / Deviation from the specification {#ug_eth_functional_cfg_v}

#### Ethernet Global Time APIs {#ug_eth_functional_cfg_v_globaltime}

This driver implementation doesn't implement the Global Time APIs:
- Eth_GetCurrentTime()
- Eth_EnableEgressTimeStamp()
- Eth_GetEgressTimeStamp()
- Eth_GetIngressTimeStamp()
- Eth_SetCorrectionTime()
- Eth_SetGlobalTime()

[Back To Top](@ref ug_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#### Ethernet Transceiver Wake-Up {#ug_eth_functional_cfg_v_wakeup}

The wake-up related APIs are not implemented in this release:
- EthTrcv_SetTransceiverWakeupMode()
- EthTrcv_GetTransceiverWakeupMode()
- EthTrcv_CheckWakeup()

The wake-up related functionality of other non wake-up specific APIs
(i.e. EthTrcv_TransceiverInit(), EthTrcv_SetTransceiverMode(), etc) are not
implemented in this release either.

[Back To Top](@ref ug_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#### Ethernet Transceiver Manual/Auto-Negotiation Mode {#ug_eth_functional_cfg_v_autoneg}

The current EthTrcv driver implementation only supports auto-negotiation mode.
The following APIs are impacted and partial functionality of the API is
implemented:
- EthTrcv_TransceiverInit()
- EthTrcv_SetTransceiverMode()

[Back To Top](@ref ug_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

### Ethernet Transceiver ECUC {#ug_eth_functional_i_cfg_ecuc}

The following EthTrcv ECUC APIs are not implemented in this release:
- EthTrcvPhysLayerType
- EthTrcvConnNeg
- EthTrcvMainFunctionPeriod

[Back To Top](@ref ug_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

### Non Standard Service APIs {#ug_eth_functional_non_std_api_top}

None.

[Back To Top](@ref ug_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Interrupt Configuration {#ug_eth_functional_cfg_int}

The Ethernet driver doesn't register any interrupt handlers (ISR), it is
expected that consumer of this driver registers the required interrupt handler.

The Ethernet interrupts are:
- TX DMA Completion - Asserted when a packet has been consumed by CPSW for
  transmission. The interrupt number is configurable and is passed via Ethernet
  configuration parameters. Please refer to the resource partitioning
  information for further details about what interrupt numbers are allocated for
  the MCU 
- RX DMA Completion - Asserted when a packet has been received. The interrupt
  number is configurable and is passed via Ethernet configuration parameters.
  Please refer to the resource partitioning information for further details
  about what interrupt numbers are allocated for the MCU
- MDIO Access Completion - Asserted when an Ethernet transceiver (PHY) register
  read or write operation is complete. The interrupt number is 35

Other CPSW interrupts (like the Statistics Pending interrupt) are not handled by
the driver.

Please refer to the EthApp_InterruptConfig() function in Eth demo application
for the implementation details of the interrupt registration.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Power-up {#ug_eth_functional_cfg_pwr}

The driver doesn't configure the functional clock and power for the Ethernet
module. It is expected that the Secondary Bootloader (SBL) powers up the
required modules. Please refer SBL documentation.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Build and Running the Application {#ug_eth_functional_cfg_build}

Please follow steps detailed in section (@ref mcusw_build_top) to build library
or example.

### Building the host-side example application {#ug_eth_functional_hostapp_build}

The GCC compiler is required to build the host-side application.

The steps to build the host-side application in Linux are listed below:

\verbatim
$ cd mcal_drv/mcal/examples/Eth/eth_app/host/
$ make
\endverbatim

### Building the target-side example application in loopback mode {#ug_eth_functional_loopback_build}

The Eth example application can run an internal loopback test when configured
accordingly in the driver's configuration parameters.

The example application per se doesn't need any change for loopback, but the
<b>loopback</b> parameter must be set to TRUE as shown below.  The example
application can then be rebuilt following the regular steps listed in
@ref mcusw_build_top.

\verbatim
diff --git a/mcal_drv/mcal/examples_config/Eth_Demo_Cfg/output/generated/src/Eth_Cfg.c b/mcal_drv/mcal/examples_config/Eth_Demo_Cfg/output/generated/src/Eth_Cfg.c
index 24c046a..21cf137 100755
--- a/mcal_drv/mcal/examples_config/Eth_Demo_Cfg/output/generated/src/Eth_Cfg.c
+++ b/mcal_drv/mcal/examples_config/Eth_Demo_Cfg/output/generated/src/Eth_Cfg.c
@@ -75,7 +75,7 @@ ETH_CONFIG_DATA_SECTION CONST(Eth_ConfigType, ETH_CONFIG_SECTION)
     /**< MDIO bus clock (MDCLK) frequency (in Hz) */
     .connType           =    ETH_MAC_CONN_TYPE_RGMII_FORCE_1000_FULL,
     /**< MII connection type */
-    .loopback           =    FALSE,
+    .loopback           =    TRUE,
     /**< Loopback enable */
     .enableCacheOps     =     (uint32)TRUE,
     /**< Packet memory is cacheable */
\endverbatim

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Steps to run example application {#ug_eth_functional_run_example}

### Running the loopback test {#ug_eth_function_run_loopback_example}

In order to run the loopback test, it's required to set the Ethernet driver's
'loopback' configuration parameter to TRUE as described in
@ref ug_eth_functional_loopback_build. In loopback mode, CPSW is configured
with ALE in bypass mode and loopback is enabled in the MAC port (port 1).

This test doesn't require any additional external setup and the example
can run standalone on the device. The example application will transmit and
receive 1000 frames, and will verify the frame content.

### Running the default (non-loopback) test {#ug_eth_function_run_default_example}

This test consists of two applications: host-side application that runs on a
Linux machine, and a target-side application that is loaded to the device (DUT).

Please refer to the @ref ug_eth_functional_hostapp_build section for
instructions to build the host-side application.

-# Connect a CAT5e/CAT6 Ethernet cable to the <i>MCU ETHERNET</i> connector of
   the DRA80xx EVM and to the PC's Ethernet port
-# Don't setup the Network Connection in Linux, disable automatic connection if
   needed. The intention is to avoid any frames going to the DUT, other than
   those sent by the test application
-# Find the interface name on the host PC side by running the following command:

\verbatim
$ ifconfig -s
\endverbatim
-# Run the host-side application with root privileges (needed for raw sockets) and
   specify the interface name to be used, for instance:
\verbatim
$ cd mcal_drv/mcal/examples/Eth/eth_app/host/
$ sudo ./EthHostApp -i eth2
\endverbatim

-# At this point, the host-side application will be waiting for the DUT
-# Load the target-side application to the device and run it. The host-side
   application should detect that the device is now ready and all tests will be
   run (including frame transmission with and without confirmation, frame
   reception, filtering MAC addresses, transmission and reception of VLAN tagged
   packets, transmit throughput, etc)
-# Check the logs printed in the Linux terminal and verify that the host-side
   application didn't report any errors
-# Check the logs printed in the serial console (or CCS console if UART is not
   enabled) and verify that the target-side application didn't report any errors

The example applications on either side (host or target) will report a fail
status if any test Ethernet frames is lost. So even frames which are lost due
to physical link will cause the example application to report as a failure.

<b>Note:</b> The test steps listed above have been tested in Ubuntu 16.04. Test
errors have been found when running the test in Ubuntu 18.04.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Memory Mapping {#ug_eth_functional_cfg_memmap}

Various objects of this implementation (e.g. variables, functions, constants)
are defined under different sections. The linker command file at
(@ref mcusw_build_eg_linker) defines separate section for these objects.
When the driver is integrated, it is expected that these sections are created
and placed in appropriate memory locations.
(Locations of these objects depend on the system design and performance needs)

<table >
  <tr>
    <td><i><b>Section</b></i></td>
    <td><i><b>ETH_CODE</b></i></td>
    <td><i><b>ETH_VAR</b></i></td>
    <td><i><b>ETH_VAR_NOINIT</b></i></td>
    <td><i><b>ETH_CONST</b></i></td>
    <td><i><b>ETH_CONFIG</b></i></td>
    <td><i><b>ETH_UDMA</b></i></td>
    <td><i><b>ETH_TX_DATA</b></i></td>
    <td><i><b>ETH_RX_DATA</b></i></td>
  </tr>
  <tr>
    <td><i><b>ETH_TEXT_SECTION</b></i></td>
    <td> USED </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETH_ISR_TEXT_SECTION</b></i></td>
    <td> USED </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETH_CONST_32_SECTION</b></i></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> USED </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETH_CONFIG_SECTION</b></i></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> USED </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETH_DATA_INIT_UNSPECIFIED_SECTION</b></i></td>
    <td> </td>
    <td> USED </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETH_DATA_NO_INIT_UNSPECIFIED_SECTION</b></i></td>
    <td> </td>
    <td> </td>
    <td> USED </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETH_UDMA_SECTION</b></i></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> USED </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETH_TX_DATA_SECTION</b></i></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> USED </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETH_RX_DATA_SECTION</b></i></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> USED </td>
  </tr>
</table>

<BR>

<table >
  <tr>
    <td><i><b>Section</b></i></td>
    <td><i><b>ETHTRCV_CODE</b></i></td>
    <td><i><b>ETHTRCV_VAR</b></i></td>
    <td><i><b>ETHTRCV_VAR_NOINIT</b></i></td>
    <td><i><b>ETHTRCV_CONST</b></i></td>
    <td><i><b>ETHTRCV_CONFIG</b></i></td>
  </tr>
  <tr>
    <td><i><b>ETHTRCV_TEXT_SECTION</b></i></td>
    <td> USED </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETHTRCV_CONST_32_SECTION</b></i></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> USED </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETHTRCV_CONFIG_SECTION</b></i></td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> </td>
    <td> USED </td>
  </tr>
  <tr>
    <td><i><b>ETHTRCV_DATA_INIT_UNSPECIFIED_SECTION</b></i></td>
    <td> </td>
    <td> USED </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETHTRCV_DATA_INIT_32_SECTION</b></i></td>
    <td> </td>
    <td> USED </td>
    <td> </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETHTRCV_DATA_NO_INIT_UNSPECIFIED_SECTION</b></i></td>
    <td> </td>
    <td> </td>
    <td> USED </td>
    <td> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>ETHTRCV_DATA_NO_INIT_16_SECTION</b></i></td>
    <td> </td>
    <td> </td>
    <td> USED </td>
    <td> </td>
    <td> </td>
  </tr>
</table>

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Cache {#ug_eth_functional_cfg_cache}

This driver implementation has been validated with cache enabled. For optimal
performance it's recommended to place (@ref ug_eth_functional_cfg_memmap)
sections in cache enabled memory area.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Dependencies on SW Modules {#ug_eth_functional_dep_top}

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### DET {#ug_eth_functional_dep_det}

This implementation depends on the DET in order to report development errors
and can be turned OFF. Refer to the @ref ug_eth_error_dev section for detailed
error codes.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### SchM {#ug_eth_functional_dep_schm}

This implementation requires 1 level of exclusive access to guard critical
sections. Invokes SchM_Enter_Eth_ETH_EXCLUSIVE_AREA_0(),
SchM_Exit_Eth_ETH_EXCLUSIVE_AREA_0() to enter critical section and exit.

In the example implementation (SchM_Eth.c), all the interrupts on CPU are
disabled. However, disabling of the enabled Ethernet related interrupts should
suffice.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## File Structure {#ug_eth_functional_filestruct_top}

### Eth File Structure {#ug_eth_functional_eth_filestruct_top}

![](eth_ug_dir_detailed.png "Ethernet Detailed Directory Structure")

- Ethernet driver
	- Driver implemented by: Eth.c, Eth_Irq.c, Eth_Priv.c, Eth.h, Eth_Irq.h,
	  Eth_Priv.h and CPSW core driver files
	- Example Configuration by: Eth_Cfg.c and Eth_Cfg.h
	- Example Application by: EthApp.c, EthUtils.c, EthUtils.h and
	  EthUtils_Patterns.h

### EthTrcv File Structure {#ug_eth_functional_ethtrcv_filestruct_top}

![](ethtrcv_ug_dir_detailed.png "Ethernet Transceiver Detailed Directory Structure")

- Ethernet Transceiver driver
	- Driver implemented by: EthTrcv.c, EthTrcv_Priv.c, EthTrcv.h and
	  EthTrcv_Priv.h
	- Example Configuration by: EthTrcv_Cfg.c and EthTrcv_Cfg.h
	- Example Application is common for Eth and EthTrcv

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Error Handling {#ug_eth_error_top}

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Development Error Reporting {#ug_eth_error_dev}

Development errors are reported to the DET using the service Det_ReportError(),
when enabled. The driver interface files (Eth.h and EthTrcv.h shown in the
driver directory structure of the @ref ug_eth_functional_filestruct_top section)
lists the service IDs.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Error codes {#ug_eth_error_codes}

Production error are reported to DET via Det_ReportError(). Only the error codes
in the Ethernet and Ethernet Transceiver driver specifications are reported
which are listed below.  There are no implementation specific error codes being
reported.

### Ethernet driver error codes {#ug_eth_drv_error_codes}

<table>
  <tr>
    <td><i><b> Type of Error </b></i></td>
    <td><i><b> Related Error code </b></i></td>
    <td><i><b> Value (Hex)</b></i></td>
  </tr>
  <tr>
    <td> Invalid controller index </td>
    <td> ETH_E_INV_CTRL_IDX </td>
    <td> 0x01 </td>
  </tr>
  <tr>
    <td> Eth module or controller was not initialized </td>
    <td> ETH_E_NOT_INITIALIZED </td>
    <td> 0x02 </td>
  </tr>
  <tr>
    <td> Invalid pointer in parameter list </td>
    <td> ETH_E_PARAM_POINTER </td>
    <td> 0x03 </td>
  </tr>
  <tr>
    <td> Invalid parameter </td>
    <td> ETH_E_INV_PARAM </td>
    <td> 0x04 </td>
  </tr>
  <tr>
    <td> Initialization failure </td>
    <td> ETH_E_INIT_FAILED </td>
    <td> 0x05 </td>
  </tr>
  <tr>
    <td> Invalid mode </td>
    <td> ETH_E_INV_MODE </td>
    <td> 0x06 </td>
  </tr>
</table>

### Ethernet Transceiver driver error codes {#ug_ethtrcv_drv_error_codes}

<table>
  <tr>
    <td><i><b> Type of Error </b></i></td>
    <td><i><b> Related Error code </b></i></td>
    <td><i><b> Value (Hex)</b></i></td>
  </tr>
  <tr>
    <td> Invalid transceiver index </td>
    <td> ETHTRCV_E_INV_TRCV_IDX </td>
    <td> 0x01 </td>
  </tr>
  <tr>
    <td> EthTrcv module was not initialized </td>
    <td> ETHTRCV_E_NOT_INITIALIZED </td>
    <td> 0x02 </td>
  </tr>
  <tr>
    <td> Invalid pointer in parameter list </td>
    <td> ETHTRCV_E_PARAM_POINTER </td>
    <td> 0x03 </td>
  </tr>
  <tr>
    <td> Initialization failure </td>
    <td> ETHTRCV_E_INIT_FAILED </td>
    <td> 0x04 </td>
  </tr>
</table>

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Production Code Error Reporting {#ug_eth_error_prod}

Production error are reported to DEM via the service DEM_ReportErrorStatus().
There are no implementation specific error codes being reported. Only the error
codes in the Ethernet and Ethernet Transceiver driver specifications are
reported which are listed below.

<table>
  <tr>
    <td><i><b> Type of Error </b></i></td>
    <td><i><b> Related Error code </b></i></td>
  </tr>
  <tr>
    <td> Ethernet Controller Access Failure </td>
    <td> ETH_E_ACCESS </td>
  </tr>
  <tr>
    <td> Ethernet Frames Lost </td>
    <td> ETH_E_RX_FRAMES_LOST </td>
  </tr>
  <tr>
    <td> CRC Failure </td>
    <td> ETH_E_CRC </td>
  </tr>
  <tr>
    <td> Frame Size Underflow </td>
    <td> ETH_E_UNDERSIZEFRAME </td>
  </tr>
  <tr>
    <td> Frame Size Overflow </td>
    <td> ETH_E_OVERSIZEFRAME </td>
  </tr>
  <tr>
    <td> Frame Alignment Error </td>
    <td> ETH_E_ALIGNMENT </td>
  </tr>
  <tr>
    <td> Single Frame Collision </td>
    <td> ETH_E_SINGLECOLLISION </td>
  </tr>
  <tr>
    <td> Multiple Frame Collision </td>
    <td> ETH_E_MULTIPLECOLLISION </td>
  </tr>
  <tr>
    <td> Late Frame Collision </td>
    <td> ETH_E_LATECOLLISION </td>
  </tr>
</table>

<BR>

<table>
  <tr>
    <td><i><b> Type of Error </b></i></td>
    <td><i><b> Related Error code </b></i></td>
  </tr>
  <tr>
    <td> Ethernet Transceiver Access Failure </td>
    <td> ETHTRCV_E_ACCESS </td>
  </tr>
</table>

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# API Description {#ug_eth_api_top}

The AUTOSAR BSW Eth Driver specification details the APIs [[1](@ref ug_eth_ref_top)].

The Global Time APIs are not implemented in this release. Refer to the @ref
ug_eth_functional_cfg_v_globaltime section for more details.

The AUTOSAR BSW EthTrcv Driver specification details the APIs [[2](@ref ug_eth_ref_top)].

The wake-up related APIs are not implemented in this release. Refer to the
@ref ug_eth_functional_cfg_v_wakeup section for more details on impacted APIs.

The wake-up related functionality of other non wake-up specific APIs (i.e.
EthTrcv_TransceiverInit(), EthTrcv_SetTransceiverMode(), etc) are not
implemented in this release either.

The current EthTrcv driver implementation only supports auto-negotiation mode.
Refer to the @ref ug_eth_functional_cfg_v_autoneg section for more details on
the impacted APIs.

The following EthTrcv ECUC APIs are not implemented in this release. Refer to
@ref ug_eth_functional_i_cfg_ecuc section for more details.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Example Application {#ug_eth_eg_top}

The example application demonstrate use of Eth module.  The example consists of
an application that runs on the DRA80xx device (DUT) and an application that
runs on a Linux PC (host).  The host and target (DUT) applications communicate
with each other to start the different stages of the example tests.

The Eth example relies on shared utils which provide helper functions (i.e.
filling an Ethernet frame with test data, verifying frame contents, etc) which
are useful on the DUT and host sides.

The following table summarizes the different tests implemented by the example
application.

<table>
  <tr>
    <td><b>Test ID</b></td>
    <td><b>Summary</b></td>
    <td><b>Description</b></td>
  </tr>
  <tr>
	<td>test_0001</td>
    <td>Basic DUT frame reception</td>
    <td>
		Host will send 1000 Ethernet frames, the DUT will receive and verify
	    that the frame content matches the expected patterns
	</td>
  </tr>
  <tr>
	<td>test_0002</td>
    <td>Basic DUT frame transmission</td>
    <td>
		The DUT will send 1000 non-VLAN tagged frames without TX confirmation.
		The host PC will check that all frames are received and that the frame
		content matches the expected patterns
	</td>
  </tr>
  <tr>
	<td>test_0003</td>
    <td>External loopback</td>
    <td>
		The DUT will send frames to the host, the host will receive the frames
		and will send them back to the DUT
	</td>
  </tr>
  <tr>
	<td>test_0004</td>
    <td>Default filter operation</td>
    <td>
		Host will send 500 frames with DUT's MAC address and 500 frames with
		other MAC address. The DUT will reject the 500 frames that don't matches
		DUT's address
	</td>
  </tr>
  <tr>
	<td>test_0005</td>
    <td>Different filter configurations</td>
    <td>
		This test is split in different stages that will exercise different
		filter operations like adding/removing unicast address, entering/exiting
		promiscuous mode, etc.
	</td>
  </tr>
  <tr>
	<td>test_0006</td>
    <td>DUT frame transmission with confirmation</td>
    <td>
		The DUT will send 1000 non-VLAN tagged frames and will request TX
		confirmation. The host PC will check that all frames are received and
		that the frame content matches the expected patterns
	</td>
  </tr>
  <tr>
	<td>test_0007</td>
    <td>Transmission of VLAN tagged frames</td>
    <td>
		The DUT will send 1000 VLAN tagged frames and will request TX
		confirmation. The host PC will check that all frames are received and
		that the frame content matches the expected patterns
	</td>
  </tr>
  <tr>
	<td>test_0008</td>
    <td>Transmission of different frame lengths</td>
    <td>
		The DUT will send non-VLAN tagged frames of different lengths, including
		short frames. The host PC will check that all frames are received and
		that the frame content matches the expected patterns
	</td>
  </tr>
  <tr>
	<td>test_0009</td>
    <td>VLAN tagged frame reception</td>
    <td>
		Host will send 1000 VLAN tagged Ethernet frames, the DUT will receive
		and verify that the frame content matches the expected patterns
	</td>
  </tr>
  <tr>
	<td>test_0010</td>
    <td>Controller mode changes</td>
    <td>
		The application will set the controller to DOWN mode and then back to
		ACTIVE mode. Ethernet frame transmission and reception tests will be
		run afterwards. 10 iterations of these steps will be executed.
	</td>
  </tr>
  <tr>
	<td>test_0100</td>
    <td>Transmit throughput</td>
    <td>
		DUT transmit throughput is measured over 150,000 Ethernet frames sent
		from the host PC.
	</td>
  </tr>
  <tr>
	<td>test_0200</td>
    <td>Internal loopback (MAC)</td>
    <td>
		The DUT will send and receive 1000 Ethernet frames and will check the
		frame correctness against expected patterns. Note: This test runs
		only when Ethernet driver configuration's loopback mode is enabled
	</td>
  </tr>
</table>

The following list identifies key steps performed by the DUT side application:

- EthApp_Startup ()
    + Pin mux configuration
	+ Set Ethernet connection type in MMR
    + Interrupt controller initialization and ISR registration for the following
	  interrupts:
        - TX DMA completion
		- RX DMA completion
		- MDIO completion (register access completion)
    + Initialize counters required for timed operations
- EthApp_init()
    + Ethernet driver
        - Ethernet driver initialization (Eth_Init())
        - Ethernet controller initialization (Eth_ControllerInit())
	    - Put controller to active mode (Eth_SetControllerMode())
	    - Get and print the physical address (Eth_GetPhysAddr())
	+ Ethernet Transceiver driver
	    - Ethernet Transceiver driver initialization (EthTrcv_Init())
	    - Ethernet transceiver initialization (EthTrcv_TransceiverInit())
	    - Put Ethernet transceiver in active mode (EthTrcv_SetTransceiverMode())
	    - Get and print the transceiver mode (EthTrcv_GetTransceiverMode())
		- Get and print the link state (EthTrcv_GetLinkState())
		- Get and print the link baud rate (EthTrcv_GetBaudRate())
		- Get and print the link duplexity (EthTrcv_GetDuplexMode())
- Loop through all tests
    + EthApp_test_0001: Basic DUT frame reception test:
	    - Send test START command
        - Receive frames until the STOP is detected
	+ EthApp_test_0002: Basic DUT frame transmission test:
	    - Send test START command
		- Transmit 1000 non-VLAN tagged frames without confirmation
	+ EthApp_test_0003: External loopback test. The DUT sends frames to the
	  host, the host receives the frames and sends them back to the DUT.
	  The following operations are performed on the DUT side:
	    - Send START command
	    - Send and receive 1000 frames, for each of them:
		   + A different payload is set
		   + EtherType and payload are verified on the received frame
	+ EthApp_test_0004: Default filter operation. Test the operation of the
	  default DUT's filter configuration (only frames with DUT's MAC address are
	  accepted, all others are rejected). The DUT performs these operations:
		- Send START command
		- Receive frames until the STOP is detected
		- The number of frames expected to be received is 500 as only one half
		  of the total frames sent by the host (1000) have the DUT's MAC address
	+ EthApp_test_0005: Different filter configurations test. Test the different
	   modes of the DUT's filter configuration.  The DUT performs these
	   operations:
		- Send START command
		- Part 1
			+ Filter is reset to accept only frames with DUT's MAC address
			+ Host side transmits 1000 frames and then transmits the STOP
			  command
			+ Receive frames until STOP is detected
				- Frame 1: Source MAC address is DUT's. It should be accepted
				- Frame 2: Source MAC address is not DUT's. It should be
				  rejected
				- Expected to receive 500 frames
		- Part 2
			+ A second unicast MAC address is added to the DUT's filter
			+ Host side transmits 1000 frames and then transmits the STOP
			  command
			+ Receive frames until STOP is detected
				- Frame 1: Source MAC address is DUT's. It should be accepted
				- Frame 2: Source MAC address is second valid MAC address. It
				  should be accepted
				- Expected to receive 1000 frames
		- Part 3
			+ A multicast address is added to the DUT's filter
			+ Host side transmits 1000 frames and then transmits the STOP
			  command
			+ Receive frames until STOP is detected
				- Frame 1: Source MAC address is DUT's. It should be accepted
				- Frame 2: Source MAC address is multicast address. It should be
				  accepted
				- Expected to receive 1000 frames
		- Part 4
			+ The multicast address is removed from the DUT's filter
			+ Host side transmits 1000 frames and then transmits the STOP
			  command
			+ Receive frames until STOP is detected
				- Frame 1: Source MAC address is DUT's. It should be accepted
				- Frame 2: Source MAC address is multicast address. It should be
				  rejected
				- Expected to receive 500 frames
		- Part 5
			+ The filter is opened up (promiscuous mode)
 			+ Host side transmits 1000 frames and then transmits the STOP
			  command
			+ Receive frames until STOP is detected
				- Frame 1: Source MAC address is DUT's. It should be accepted
				- Frame 2: Source MAC address is not DUT's. It should be
				  accepted
				- Expected to receive 1000 frames
		- Part 6
			+ The filter is reset back to accept only DUT's unicast address
			+ Host side transmits 1000 frames and then transmits the STOP
			  command
			+ Receive frames until STOP is detected
				- Frame 1: Source MAC address is DUT's. It should be accepted
				- Frame 2: Source MAC address is not DUT's. It should be rejected
				- Expected to receive 500 frames
	+ EthApp_test_0006: Transmission with confirmation test. DUT frame
	  transmission test with TX confirmation.  The DUT performs these
	  operations:
		- Send START command
		- Transmit 1000 non-VLAN tagged frames with confirmation
	+ EthApp_test_0007: Transmission of VLAN tagged frames test. DUT frame
	  transmission test of VLAN tagged frames.  The DUT performs these
	  operations:
		- Send START command
		- Transmit 1000 VLAN tagged frames with confirmation
	+ EthApp_test_0008: Transmission of different frame lengths. DUT frame
	  transmission test of different frame lengths, including short frames (less
	  than 64 octets).  The DUT performs these operations:
		- Send START command
		- Transmit 10 frames for lengths starting at 10 octets in increments of
		  10 octets until 1500 octets
	+ EthApp_test_0009: VLAN tagged frame reception test
		- Host side transmits 1000 frames and then transmits the STOP command
		- The DUT performs these operations:
			+ Send START command
			+ Receive frames until the STOP is detected
		- The number of frames expected to be received is 1000
	+ EthApp_test_0010: Controller mode change test.  The DUT performs the
	  following operations:
		- Send START command
		- Run 10 iterations of:
			+ Set controller mode to DOWN state
			+ Set controller mode to ACTIVE state
			+ Transmit frames (run test 0002)
			+ Receive frames (run test 0001)
	+ EthApp_test_0100: DUT transmit throughput test. Measure DUT transmit
	  throughput over 150000 frames
		- Host side transmits 150000 frames and then transmits the STOP command
		- The DUT performs the following operations:
			+ Get the initial timestamp corresponding to the first frame
			  received
			+ Receive frames until the STOP is detected and get the final
			  timestamp
			+ Compute the transmit throughput from the number of received frames
			  and the time difference between the two captured timestamps
	+ EthApp_test_0200: Internal loopback test (MAC). The DUT performs the
	  following operations:
		- Send and receive 1000 frames, for each of them:
			+ A different payload is set
			+ EtherType and payload are verified on the received frame
- EthApp_showStats()
	+ Get and print the Ethernet controller statistics (Eth_GetEtherStats())
- EthApp_showDropCount()
	+ Get and print the Ethernet controller drop counts (Eth_GetDropCount())
- EthApp_deinit()
    + Put the controller in down mode (Eth_SetControllerMode())
- Check for error status and print the result

EthApp_test_0200 will run only when loopback is enabled via Ethernet driver
configuration. All other tests will not run. Conversely, when loopback is
disabled EthApp_test_0200 will not run.

The host application steps are not presented in this guide, but can be found
documented in the host side code. Please refer to the
@ref ug_eth_functional_filestruct_top section for the directory where the
HostApp.c file can be found.

The configuration files are present can be found at the directories shown in the
@ref ug_eth_functional_filestruct_top section.

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Example Log {#ug_eth_eg_log}

### Loopback example application log

\verbatim
Eth Application Starts
MAC Port 1 Address: f4:84:4c:eb:95:09
test_0200: START
test_0200: completed 1000 of 1000 iterations
test_0200: END
Test 0200: Pass

----------------------------
Controller 0 Statistics
----------------------------
Drop Events       : 0
Octets            : 3036000
Packets           : 2000
Bcast Packets     : 1000
Mcast Packets     : 0
CRC/Align Errors  : 0
Undersized Packets: 0
Oversized Packets : 0
Fragments         : 0
Jabbers           : 0
Collisions        : 0
----------------------------
Controller 0 Drop Counters
----------------------------
Buffer overruns   : -1
CRC errors        : 0
Undersize packets : 0
Oversized packets : 0
Alignment errors  : 0
SQE errors        : 0
Discarded inbound : 0
Erroneous inbound : 0
Discarded outbound: 0
Erroneous outbound: 0
Single collision  : 0
Multiple collision: 0
Deferred transm   : 0
Late collisions   : 0
Excessive colls   : 0
Buffer underrun   : 0
Carrier-sense     : 0
ETH Stack Usage: 2368 bytes
Eth Application Completed
\endverbatim

### Default (non-loopback) example application log

#### Target-side application log

\verbatim
$ sudo ./EthHostApp -i eth0
Interface    : eth0
MAC address  : 5c:26:0a:88:5d:08


Waiting for DUT..
DUT detected: f4:84:4c:eb:95:09


-----------------------------------------------------------
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0003: START
test_0003: looped back 1000 frames
test_0003: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0004: START
test_0004: completed 500 of 500 iterations
test_0004: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0005: START
test_0005: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0006: START
test_0006: received 1000 of 1000 frames
test_0006: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0007: START
test_0007: received 1000 of 1000 frames
test_0007: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0008: START
test_0008: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0009: START
test_0009: transmitted 1000 of 1000 frames
test_0009: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0010: START
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
test_0002: START
test_0002: received 1000 of 1000 frames
test_0002: END
test_0001: START
test_0001: transmitted 1000 of 1000 frames
test_0001: END
test_0010: completed 10 of 10 iterations
test_0010: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0100: START
receiveTput: received 149995 frames in 2.68 secs (56025.19 frames/s, 678.58 Mbps)
test_0100: END
Test Result: Pass
-----------------------------------------------------------
\endverbatim

#### Target-side application log

\verbatim
Eth Application Starts
MAC Port 1 Address: f4:84:4c:eb:95:09
EthTrcv mode: ACTIVE
EthTrcv link state: Up
EthTrcv baud rate: 1000Mbps
EthTrcv duplexity: Full
EthIf_TrcvModeIndication: Active


-----------------------------------------------------------
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0002: START
test_0002: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0003: START
test_0003: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0004: START
test_0004: received 500 of 500 frames
test_0004: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0005: START
test_0005:  Null: Exp1: exp 500, got 500
test_0005:  Null: Exp2: exp   0, got   0
test_0005: Ucast: Exp1: exp 500, got 500
test_0005: Ucast: Exp2: exp 500, got 500
test_0005: Mcast: Exp1: exp 500, got 500
test_0005: Mcast: Exp2: exp 500, got 500
test_0005: Mcast: Exp1: exp 500, got 500
test_0005: Mcast: Exp2: exp   0, got   0
test_0005: Bcast: Exp1: exp 500, got 500
test_0005: Bcast: Exp2: exp 500, got 500
test_0005:  Null: Exp1: exp 500, got 500
test_0005:  Null: Exp2: exp   0, got   0
test_0005: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0006: START
test_0006: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0007: START
test_0007: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0008: START
test_0008: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0009: START
test_0009: received 1000 of 1000 frames
test_0009: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0010: START
EthIf_CtrlModeIndication: DOWN
EthIf_CtrlModeIndication: ACTIVE
test_0002: START
test_0002: END
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
EthIf_CtrlModeIndication: DOWN
EthIf_CtrlModeIndication: ACTIVE
test_0002: START
test_0002: END
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
EthIf_CtrlModeIndication: DOWN
EthIf_CtrlModeIndication: ACTIVE
test_0002: START
test_0002: END
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
EthIf_CtrlModeIndication: DOWN
EthIf_CtrlModeIndication: ACTIVE
test_0002: START
test_0002: END
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
EthIf_CtrlModeIndication: DOWN
EthIf_CtrlModeIndication: ACTIVE
test_0002: START
test_0002: END
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
EthIf_CtrlModeIndication: DOWN
EthIf_CtrlModeIndication: ACTIVE
test_0002: START
test_0002: END
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
EthIf_CtrlModeIndication: DOWN
EthIf_CtrlModeIndication: ACTIVE
test_0002: START
test_0002: END
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
EthIf_CtrlModeIndication: DOWN
EthIf_CtrlModeIndication: ACTIVE
test_0002: START
test_0002: END
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
EthIf_CtrlModeIndication: DOWN
EthIf_CtrlModeIndication: ACTIVE
test_0002: START
test_0002: END
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
EthIf_CtrlModeIndication: DOWN
EthIf_CtrlModeIndication: ACTIVE
test_0002: START
test_0002: END
test_0001: START
test_0001: received 1000 of 1000 frames
test_0001: END
test_0010: completed 10 of 10 iterations
test_0010: END
Test Result: Pass
-----------------------------------------------------------


-----------------------------------------------------------
test_0100: START
test_0100: 150000 frames sent, 0 buffer underflows
test_0100: END
Test Result: Pass
-----------------------------------------------------------

----------------------------
Controller 0 Statistics
----------------------------
Drop Events       : 4000
Octets            : 262074484
Packets           : 21038
Bcast Packets     : 19
Mcast Packets     : 1000
CRC/Align Errors  : 0
Undersized Packets: 0
Oversized Packets : 0
Fragments         : 0
Jabbers           : 0
Collisions        : 0
----------------------------
Controller 0 Drop Counters
----------------------------
Buffer overruns   : -1
CRC errors        : 0
Undersize packets : 0
Oversized packets : 0
Alignment errors  : 0
SQE errors        : 0
Discarded inbound : 0
Erroneous inbound : 0
Discarded outbound: 0
Erroneous outbound: 0
Single collision  : 0
Multiple collision: 0
Deferred transm   : 0
Late collisions   : 0
Excessive colls   : 0
Buffer underrun   : 0
Carrier-sense     : 0
ETH Stack Usage: 2340 bytes
Eth Application Completed
\endverbatim

[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# References {#ug_eth_ref_top}

Sl No | Specification | Comment / Link
-----------|----------------|----------
1 | AUTOSAR 4.2.1 | AUTOSAR Specification for Eth Driver [Intranet Link](http://www-open.india.ti.com/~pspcm/data_pspdocs/PDP/MCAL/Documents/Autosar/V4.2/SW_Architecture_Communication_Stack/Standard_Specifications/AUTOSAR_SWS_EthernetInterface.pdf)
2 | AUTOSAR 4.2.1 | AUTOSAR Specification for EthTrcv Driver [Intranet Link](http://www-open.india.ti.com/~pspcm/data_pspdocs/PDP/MCAL/Documents/Autosar/V4.2/SW_Architecture_Communication_Stack/Standard_Specifications/AUTOSAR_SWS_EthernetTransceiverDriver.pdf)
3 | - | Design Page (@ref design_eth_top)


[Back To Top](@ref ug_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History  {#ug_eth_rev_hist}

Revision | Date          | Author     | Description         | Status
---------|---------------|------------|---------------------|--------
0.1      | 08 Oct 2018   | Misael Lopez | First version | Pending Review
0.2      | 22 Oct 2018   | Misael Lopez | Addressed review comments | Approved
0.3      | 30 Nov 2018   | Misael Lopez | Updated UDMA interrupt information | Pending Review
0.4      | 06 Dec 2018   | Misael Lopez | Updating Status | Approved
