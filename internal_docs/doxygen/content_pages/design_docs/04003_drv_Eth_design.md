# Eth Design Document {#design_eth_top}

[TOC]

# Introduction {#design_eth_intro}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Overview {#design_eth_intro_overview}
The figure below depicts the AUTOSAR layered architecture as 3 distinct layers,
Application, Runtime Environment (RTE) and Basic Software (BSW). The BSW is
further divided into 4 layers, Services, Electronic Control Unit Abstraction,
MicroController Abstraction (MCAL) and Complex Drivers.

![](autosar_acrhitecture_common.png "AUTOSAR Architecture")

MCAL is the lowest abstraction layer of the Basic Software. It contains software
modules that interact with the Microcontroller and its internal peripherals
directly. The ETH driver is part of the Communication Drivers module which is
also part of the Basic Software.  The block diagram below shows the
position of the Ethernet driver in the AUTOSAR Architecture.

![](autosar_acrhitecture_eth.png "AUTOSAR Architecture – Eth MCAL")

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Eth Overview {#design_eth_eth_overview}

As described in the AUTOSAR Ethernet Driver specification, the Ethernet Driver
(Eth) is in charge of providing a uniform, hardware independent interface to the
upper layer, the Ethernet Interface (EthIf). Thus, the Ethernet Interface may
access the underlying bus system in a uniform manner.

The driver provides bus specific functionality for controller initialization,
configuration, data transmission, data reception, statistics gathering, etc.
A single Ethernet Driver module supports only one type of controller hardware,
but several controllers of the same type. Figure below shows the lower part of
the Ethernet stack.

![](eth_stack.png "Lower-level of the Ethernet stack ")

This Ethernet Driver implementation shall support an Ethernet controller based
on the two-port Gigabit Ethernet Switch (CPSW) peripheral present in the MCU
domain of the DRA80x and TDA4x family of devices.

Refer section [2] (@ref design_eth_references) for more details on Eth operation.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Two-Port Gigabit Ethernet Switch

The DRA80x and TDA4x family of devices have an integrated the two-port Gigabit
Ethernet Switch (CPSW) into the device MCU domain named MCU_CPSW0. Please refer
to figure below for a block diagram of the peripheral.

Port 0 is the internal Communications Port Programming Interface (CPPI) host
port. Port 1 is the Ethernet port which supports RGMII and RMII interfaces.

The CPSW peripheral natively supports features like IEEE 1588 Clock
Synchronization, Address Lookup Engine (ALE), and Ethernet Statistics.
Further information on these and several other features supported by the CPSW
subsystem can be found in the device TRM.

![](eth_cpsw0_block.png "MCU_CPSW0 Block Diagram")

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## References {#design_eth_references}

Sl No | Specification | Comment / Link
-----------|----------------|----------
1 | AUTOSAR 4.2.1 | AUTOSAR Specification for Eth Driver [Intranet Link](http://www-open.india.ti.com/~pspcm/data_pspdocs/PDP/MCAL/Documents/Autosar/V4.2/SW_Architecture_Communication_Stack/Standard_Specifications/AUTOSAR_SWS_EthernetInterface.pdf)
2 | AUTOSAR 4.2.1 | Specification of the Ethernet Interface,[Intranet Link](http://www-open.india.ti.com/~pspcm/data_pspdocs/PDP/MCAL/Documents/Autosar/V4.2/SW_Architecture_Communication_Stack/Standard_Specifications/AUTOSAR_SWS_EthernetInterface.pdf)
3 | AUTOSAR 4.2.1 BSW | General Specification of Basic Software Modules,[Intranet Link](http://www-open.india.ti.com/~pspcm/data_pspdocs/PDP/MCAL/Documents/Autosar/V4.2/SW_Architecture_General/Standard_Specifications/AUTOSAR_SWS_BSWGeneral.pdf)
4 | AUTOSAR 4.2.1 | List of Basic Software Modules, AUTOSAR Release ,[Intranet Link](http://www-open.india.ti.com/~pspcm/data_pspdocs/PDP/MCAL/Documents/Autosar/V4.2/SW_Architecture_General/Auxiliary_Material/AUTOSAR_TR_BSWModuleList.pdf)
5 | DRA80x and TDA4x TRM | Technical Reference Manual, Timer module is detailed
6 | BSW General Requirements / Coding guidelines | [Intranet Link](https://confluence.itg.ti.com/display/MCAL/Coding+Guidelines)
7 | Software Product Specification (SPS) | [Intranet Link](https://confluence.itg.ti.com/display/MCAL/Coding+Guidelines) Requirements are derived from [1](@ref design_eth_references)
8 | AUTOSAR 4.2.1 | Specification of the Ethernet State Manager,[Intranet Link](http://www-open.india.ti.com/~pspcm/data_pspdocs/PDP/MCAL/Documents/Autosar/V4.2/SW_Architecture_Communication_Stack/Standard_Specifications/AUTOSAR_SWS_EthernetStateManager.pdf)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Requirements {#design_eth_req}

The Eth driver shall implement as per requirements detailed in
[7](@ref design_eth_references), [1](@ref design_eth_references) and
[3](@ref design_eth_references). It’s recommended to refer
[1](@ref design_eth_references) for clarification.

The Eth driver shall follow coding guidelines listed in [6] (@ref design_eth_references)

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Features Supported {#design_eth_features_supported}

Below listed are some of the key features that are expected to be supported

- Ethernet controller initialization
- Transmission and reception of Ethernet frames
- Interrupt-based hardware error reporting
- MDIO
- Statistics gathering
- Packet time-stamping (PTP)
- VLAN tag
- Hardware-based error detection (collisions, under/over-sized frames, CRC, etc)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_001</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00003, SWS_Eth_00004, SWS_Eth_00006, SWS_Eth_00007,
        SWS_Eth_00009, SWS_Eth_00011, SWS_Eth_00026, SWS_Eth_00119,
        SWS_Eth_00148, SWS_Eth_00149, SWS_Eth_00218
    </td>
  </tr>
</table>

Configuration Requirements
<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_039</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00005, SWS_Eth_00010, SWS_Eth_00012, SWS_Eth_00013,
        SWS_Eth_00121, SWS_Eth_00122, SWS_Eth_00123, SWS_Eth_00124,
        SWS_Eth_00125, SWS_Eth_00126
    </td>
  </tr>
</table>


[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
![](@ref caution.png)
## Features Not Supported / NON Compliance {#design_eth_features_not_supported}
- Link-Time and Post-build variants are not supported in this release.
- HIS subset of the MISRA C Standard not addressed.
- Global Time APIs are not implemented.
  - Eth_GetCurrentTime
<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_026</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td>
	   SWS_Eth_00181, SWS_Eth_00182, SWS_Eth_00183, SWS_Eth_00184, SWS_Eth_00185, SWS_Eth_00210
	</td>
  </tr>
</table>

  - Eth_EnableEgressTimeStamp
<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_027</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td>
      SWS_Eth_00186, SWS_Eth_00187, SWS_Eth_00188, SWS_Eth_00189, SWS_Eth_00211
	</td>
  </tr>
</table>

  - Eth_GetEgressTimeStamp
<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_028</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td>
	  SWS_Eth_00190, SWS_Eth_00191, SWS_Eth_00192, SWS_Eth_00193, SWS_Eth_00194, SWS_Eth_00212
	</td>
  </tr>
</table>

  - Eth_GetIngressTimeStamp
<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_029</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td>
	  SWS_Eth_00195, SWS_Eth_00196, SWS_Eth_00197, SWS_Eth_00198, SWS_Eth_00199, SWS_Eth_00213
	</td>
  </tr>
</table>

  - Eth_SetCorrectionTime
<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_030</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td>
	  SWS_Eth_00200, SWS_Eth_00201, SWS_Eth_00202, SWS_Eth_00203, SWS_Eth_00204, SWS_Eth_00214
	</td>
  </tr>
</table>

  - Eth_SetGlobalTime
<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_031</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td>
	  SWS_Eth_00205, SWS_Eth_00206, SWS_Eth_00207, SWS_Eth_00208, SWS_Eth_00209, SWS_Eth_00215
	</td>
  </tr>
</table>

- The hardware offloaded checksum computation is not supported for either ingress or egress traffic.

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_025</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00216, SWS_Eth_00217
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Assumptions {#design_eth_assumptions}
Below listed are assumed to valid for this design/implementation, exceptions and
other deviations are listed for each explicitly. Care should be taken to ensure
these assumptions are addressed.

-# The CPSW module is expected to be properly clocked before calling any
    Ethernet Driver API. The driver shall not perform any power and/or clock
    related configuration.
-# The Ethernet driver shall not perform any pinmux related settings which are
    assumed to be done by other entities.
-# The Ethernet driver shall rely on the UDMA driver/utility for all DMA related
    transactions which includes configuration and operation of the UDMAP-P,
    PSI-L, Ring Accelerator.
-# The Ethernet driver shall only be responsible of enabling and handling
    interrupt sources on the CPSW side. ISR registration as well as MCU
    interrupt configuration shall be performed by other entities.
-# The MCU_CPSW0 module base address, register offsets, SoC definitions and
    basic helper functions shall be provided by the CSL library


[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Design Description {#design_eth_description}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

The Ethernet driver relies on the TI Chip Support Library (CSL) as well as the PDK UDMA driver. CSL provides register addresses, definitions and helper functions for several modules and sub-modules of the device, including CPSW, CPGMAC, etc. The UDMA driver provides an interface to configure and manage DMA transfers which facilitates and hides the complexity of the programming of the UDMA, Ring Accelerator, PSI-L and other DMA related modules.

The Ethernet driver implementation shall be divided into a number of files that mirror the different functionality of the CPSW peripheral. The block diagram below shows the driver, its subcomponent as well as the CSL and UDMA dependency.

![](eth_driver_block.png "CPSW Ethernet Driver Block Diagram")

The data flow model of the AUTOSAR Ethernet driver requires that buffers are passed one by one to/from the upper layers. Additionally, there are a number of memory copy operations that need to take place in the lifecycle of packet buffer across the upper layers (i.e. UDP, RTP, etc). This Ethernet driver design aims to being able to scale the driver implementation for higher data throughput

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Dynamic Behavior {#design_eth_desc_dynamic}

The driver maintains the following three states: UNINIT, INIT and ACTIVE. Please
refer to the sequence diagrams shown in the Specification of Ethernet State
Manager [[8](@ref design_eth_references)], for further details on the interactions
and transitions between those states.

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Sequence Diagrams {#design_eth_seq_dia}
The following sequence diagrams the data flow interactions between the EthIf, Eth and the proposed design. These sequence diagrams extended from the diagrams of the Ethernet Interface specification document [2](@ref design_eth_references).

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Data Transmission {#design_eth_seq_tx}

The sequence diagrams below list control / data flow for transmission in polling
and interrupt methods.
![](eth_driver_seq_tx_poll.png "Data Transmission in Polling Mode")

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

![](eth_driver_seq_tx_int.png "Data Transmission in Interrupt Mode")

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Data Reception {#design_eth_seq_rx}

The sequence diagrams below list control / data flow for transmission in polling
and interrupt methods.
![](eth_driver_seq_rx_poll.png "Data Reception in Polling Mode")

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

![](eth_driver_seq_rx_int.png "Data Reception in Interrupt Mode")

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

### Data Transmission {#design_eth_seq_tx_opt}

![](eth_driver_seq_tx_optimized.png "Data Transmission Optimized")

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Data Reception {#design_eth_seq_rx_opt}

![](eth_driver_seq_rx_optimized.png "Data Reception Optimized")

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

### Recommended Methods for Tx & Rx {#design_eth_seq_rx_rec}

The implementation shall implement (@ref design_eth_seq_tx_opt) & (@ref design_eth_seq_rx_opt) for transmission and reception.

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Directory Structure {#design_eth_desc_deter_dir}

The directory structure is as depicted in figures below, the source files can
be categorized under “Driver Implementation” and “Example Application”

![](eth_driver_dir_spec.png "Directory Structure : Sourced from AUTOSAR Specification")

<b>Driver Implemented by </b>
- Eth.h and Eth_Irq.h: Shall implement the interface provided by the driver
- Eth.c, Eth_Irq.c, Eth_Packet.c, Eth_Priv.c, Eth_Packet.h, and Eth_Priv.h: Shall implement the driver functionality
- Eth/src/cpsw : Shall implement internal module specifics functionalities
- Eth/src/cpsw/include: Shall provide the internal module interfaces that shall be
    limited to the driver implementation.
- <b> Please note : </b> The implementation shall provide a separate .c & .h for
    each sub-module of the cpsw.

![](eth_driver_dir.png "Detailed Directory Structure")

<b>Example Application</b>
- Host
    + HostApp.c: Shall implement host / PC based application
    + Gpt_PBcfg.c: Shall implement the generated configuration for post-build
    variant
- Target
    + EthApp.c: Shall implement MCAL Eth demo application, which demonstrates
        use of Eth driver in conjunction with Host Application.
- Pre Compile Configuration
    + Provided by Eth_Cfg.h and Eth_Cfg.c

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_002</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> MCAL-2132
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Interrupt Service Routines {#design_eth_desc_isr}

There are two interrupt service routines defined in the AUTOSAR Ethernet Driver specification: Eth_RxIrqHdlr_<CtrlIdx> and Eth_TxIrqHdlr_<CtrlIdx>. They are defined per controller instance.
This Ethernet driver shall implement Eth_RxIrqHdlr_0 and Eth_TxIrqHdlr_0 as a single controller instance is present in the MCU_CPSW0 module.

The Eth_RxIrqHdlr_0() interrupt service routine shall clear the interrupt and read the frames of all received buffers. This driver design takes into consideration the ability to receive several frames from the DMA engine. The driver shall call the Ethernet Interface callback function EthIf_RxIndication() on each received frame.

The Eth_TxIrqHdlr_0() interrupt service routine shall clear the interrupt and check all filled transmit buffers for successful transmission. The driver shall call the Ethernet Interface callback function EthIf_TxConfirmation() for each transmit frame, if requested.

The CPSW module also provides an interrupt for statistics and CPTS related events. These two interrupts shall also have a corresponding service routine.

The CPTS interrupt is generated by the hardware when a time sync event is pushed onto the CPTS Event FIFO. The service routine for this interrupt shall clear the interrupt, read the event from the FIFO, decode the event type and process it accordingly.

The statistics interrupt is generated when any statistics value is greater than or equal to 0x80000000. The service routine for this interrupt shall clear the interrupt and update the statistics counter(s) accordingly.  The intention of this event is to avoid overflow in the 32-bit wide statistics counter values in hardware.

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Configurator {#design_eth_desc_cfg}

The AUTOSAR GPT Driver Specification details mandatory parameters that shall be
configurable via the configurator. Please refer section 10 of
[1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_024</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> ECUC_Eth_00001, ECUC_Eth_00002, ECUC_Eth_00003, ECUC_Eth_00004, ECUC_Eth_00006, ECUC_Eth_00007, ECUC_Eth_00008, ECUC_Eth_00009, ECUC_Eth_00010, ECUC_Eth_00011, ECUC_Eth_00012, ECUC_Eth_00013, ECUC_Eth_00014, ECUC_Eth_00015, ECUC_Eth_00018, ECUC_Eth_00019, ECUC_Eth_00020, ECUC_Eth_00022, ECUC_Eth_00035, ECUC_Eth_00036, ECUC_Eth_00037
    </td>
  </tr>
</table>

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### NON Standard configurable parameters {#design_eth_desc_cfg_ti}

Following lists this design’s specific configurable parameters

Parameter | Usage comment
-------------|-----------
EthEnableCacheOps | This shall allows integrators configure the Eth driver to perform cache operations.
EthCacheWbInvOps | Once Eth driver is enabled for cache operations, a pointer to a function that is expected to perform Cache-write-back-and-invalidate
EthCacheInvalidateOps | Once Eth driver is enabled for cache operations, a pointer to a function that is expected to perform Cache-invalidate
EthDmaTxChIntrNum | Allows integrators to specify the UDMA transmit interrupt number
EthDmaRxChIntrNum | Allows integrators to specify the UDMA receive interrupt number
EthMdioClkFreq | Allows integrators to specify the MDIO clock frequency. Device TRM details the frequency based on the PLL configurations
EthMdioBusFreq | Allows integrators to specify the MDIO BUS frequency (MDCLK). Device TRM details the frequency based on the PLL configurations
EthConnType | Allows integrators to specify the MII connection type
EthEnableLoopBack | Allows integrators to enable or disable loopback mode of operation. Expected to be used for debug
EthUseDefaultMacAddr | Allows integrators to use MAC address that is present in ROM
EthDefaultOSCounterId | This shall allow integrators to specify the OS counter instance to be used in OS API GetCounterValue () The driver shall implement timed-wait for all waits (e.g. waiting for reset to complete). This timed wait shall use OS API GetCounterValue ()
EthTimeoutDuration | Allow integrators to configure the time duration for which Eth-Busy should wait. Mainly needed for PHY register accesses

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Debug Information {#design_eth_desc_dbg}

The ETH driver shall provide driver status for debugging. The states ETH_STATE_UNINIT and ETH_STATE_INIT can be probed.

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_035</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00159
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Error Classification {#design_eth_desc_error}

Errors are classified in two categories, development error and runtime /
 production error.

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_034</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00008, SWS_Eth_00120
    </td>
  </tr>
</table>

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Development Errors {#design_eth_desc_error_dev}

Development errors are reported to the DET using the service Det_ReportError() if development error detection and reporting are enabled. The reported ETH module ID is 088 [4](@ref design_eth_references).

The following table presents the service IDs and the related services:

<table >
  <tr>
    <td><i><b> Service ID (HEX) </b></i></td>
    <td><i><b> Service Name </b></i></td>
  </tr>
  <tr>
    <td> 0x01 </td>
    <td> Eth_Init </td>
  </tr>
  <tr>
    <td> 0x02 </td>
    <td> Eth_ControllerInit </td>
  </tr>
  <tr>
    <td> 0x03 </td>
    <td> Eth_SetControllerMode </td>
  </tr>
  <tr>
    <td> 0x04 </td>
    <td> Eth_GetControllerMode </td>
  </tr>
  <tr>
    <td> 0x05 </td>
    <td> Eth_WriteMii </td>
  </tr>
  <tr>
    <td> 0x06 </td>
    <td> Eth_ReadMii </td>
  </tr>
  <tr>
    <td> 0x08 </td>
    <td> Eth_GetPhysAddr </td>
  </tr>
  <tr>
    <td> 0x09 </td>
    <td> Eth_ProvideTxBuffer </td>
  </tr>
  <tr>
    <td> 0xA </td>
    <td> Eth_Transmit </td>
  </tr>
  <tr>
    <td> 0x0B </td>
    <td> Eth_Receive </td>
  </tr>
  <tr>
    <td> 0x0C </td>
    <td> Eth_TxConfirmation </td>
  </tr>
  <tr>
    <td> 0xD </td>
    <td> Eth_GetVersionInfo </td>
  </tr>
  <tr>
    <td> 0x10 </td>
    <td> Eth_RxIrqHdlr_<CtrlIdx> </td>
  </tr>
  <tr>
    <td> 0x11 </td>
    <td> Eth_TxIrqHdlr_<CtrlIdx> </td>
  </tr>
  <tr>
    <td> 0x12 </td>
    <td> Eth_UpdatePhysAddrFilter </td>
  </tr>
  <tr>
    <td> 0x13 </td>
    <td> Eth_SetPhysAddr </td>
  </tr>
  <tr>
    <td> 0x14 </td>
    <td> Eth_GetDropCount </td>
  </tr>
  <tr>
    <td> 0x15 </td>
    <td> Eth_GetEtherStats </td>
  </tr>
  <tr>
    <td> 0x16 </td>
    <td> Eth_GetCurrentTime </td>
  </tr>
  <tr>
    <td> 0x17 </td>
    <td> Eth_EnableEgressTimeStamp </td>
  </tr>
  <tr>
    <td> 0x18 </td>
    <td> Eth_GetEgressTimeStamp </td>
  </tr>
  <tr>
    <td> 0x19 </td>
    <td> Eth_GetIngressTimeStamp </td>
  </tr>
  <tr>
    <td> 0x1A </td>
    <td> Eth_SetCorrectionTime </td>
  </tr>
  <tr>
    <td> 0x1B </td>
    <td> Eth_SetGlobalTime </td>
  </tr>
  <tr>
    <td> 0x0A </td>
    <td> Eth_MainFunction </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

### Error Detection {#design_eth_desc_error_dev_detect}

The detection of development errors is configurable (ON / OFF) at pre-compile
time. The switch EthDevErrorDetect will activate or deactivate the detection of
all development errors.

### Error notification (DET) {#design_eth_desc_error_dev_notify}

All detected development errors are reported to Det_ReportError service of the
Development Error Tracer (DET).

#### Parameter Checking

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_003</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00016, SWS_Eth_00036, SWS_Eth_00037, SWS_Eth_00038,
        SWS_Eth_00043, SWS_Eth_00044, SWS_Eth_00048, SWS_Eth_00049, SWS_Eth_00050, SWS_Eth_00054, SWS_Eth_00055, SWS_Eth_00056, SWS_Eth_00140, SWS_Eth_00141, SWS_Eth_00142, SWS_Eth_00164, SWS_Eth_00165, SWS_Eth_00166, SWS_Eth_00060, SWS_Eth_00061, SWS_Eth_00066, SWS_Eth_00067, SWS_Eth_00068, SWS_Eth_00228, SWS_Eth_00229, SWS_Eth_00230, SWS_Eth_00235, SWS_Eth_00236, SWS_Eth_00237, SWS_Eth_00182, SWS_Eth_00183, SWS_Eth_00184, SWS_Eth_00187, SWS_Eth_00188, SWS_Eth_00191, SWS_Eth_00192, SWS_Eth_00193, SWS_Eth_00196, SWS_Eth_00197, SWS_Eth_00198, SWS_Eth_00201, SWS_Eth_00202, SWS_Eth_00203, SWS_Eth_00206, SWS_Eth_00207, SWS_Eth_00208, SWS_Eth_00081, SWS_Eth_00082, SWS_Eth_00083, SWS_Eth_00084, SWS_Eth_00085, SWS_Eth_00090, SWS_Eth_00091, SWS_Eth_00092, SWS_Eth_00093, SWS_Eth_00129, SWS_Eth_00097, SWS_Eth_00098, SWS_Eth_00132, SWS_Eth_00103, SWS_Eth_00104, SWS_Eth_00134, SWS_Eth_00136, SWS_Eth_00111, SWS_Eth_00116
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Runtime Errors {#design_eth_desc_error_runtime}

The Eth driver shall be capable of reporting the following production errors
using the Dem_ReportErrorStatus() interface from the Diagnostics Event Manager
(DEM), as specified by the AUTOSAR Ethernet Driver specification.

<table >
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
    <td> Invalid pointer in the parameter list </td>
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


### Extended Production error {#design_eth_desc_error_dem_extended}

The CPSW module is capable of recording and reporting statistics related to different types of traffic events. A subset of those statistics shall be mapped to the error types required in the AUTOSAR Ethernet driver specification as follows

- ETH_E_ACCESS. Mapped to CPSW’s port 0 enable state and UDMA error condition.
- ETH_E_RX_FRAMES_LOST. Mapped to CPSW’s “RX Bottom of FIFO Drop” which reports the total number of frames received on a port that overran the port’s RX FIFO and were dropped.
- ETH_E_CRC. Mapped to CPSW’s “RX CRC Errors” which reports the total number of frames received on a port that experienced a CRC error.
- ETH_E_UNDERSIZEFRAME. Mapped to CPSW’s “Undersize (Short) RX Frames” which reports the total number of undersized frames received on a port.
- ETH_E_OVERSIZEFRAME. Mapped to CPSW’s “Oversize RX Frames” which reports the total number of oversized frames received on a port.
- ETH_E_ALIGNMENT. Mapped to CPSW’s “RX Align/Code Errors” which reports the total number of frames received on a port that experienced an alignment or code error.
- ETH_E_SINGLECOLLISION. Mapped to CPSW’s “Single Collision TX Frames” which reports the total number of frames transmitted on a port that experienced exactly one collision.
- ETH_E_MULTIPLECOLLISION. Mapped to CPSW’s “Multiple Collision TX Frames” which reports the total number of frames transmitted on a port that experienced multiple (2 to 15) collisions.
- ETH_E_LATECOLLISION. Mapped to CPSW’s “Late Collisions” which reports the total number of frames on a port with an abandoned transfer due to a late collision (512-bit times into the transmission).

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_032</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00173, SWS_Eth_00174, SWS_Eth_00219, SWS_Eth_00220, SWS_Eth_00221, SWS_Eth_00222, SWS_Eth_00223, SWS_Eth_00224, SWS_Eth_00225
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Resource Behavior {#design_eth_desc_res_behave}

- The Ethernet driver shall support one controller corresponding to the single Ethernet port e.g. (port 1) in the MCU_CPSW0 instance.
- The Ethernet driver shall support single buffer per packet.
- <b> Code Size </b>: Implementation of this driver shall not exceed 30 kilo lines
of code and 1 KB of data section.

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_036</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> MCAL-2129
    </td>
  </tr>
</table>

- <b> Stack Size </b>: Worst case stack utilization shall not exceed 4 kilo bytes.

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_037</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> MCAL-2128
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## API's {#design_eth_low_level_api}

For the standard API's please refer 8.3 of [1](@ref design_eth_references).
Sections below highlight other design considerations for the implementation.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_Init {#design_eth_low_level_api_init}

Refer section 8.3.1 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_005</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> WS_Eth_00027, SWS_Eth_00028, SWS_Eth_00029, SWS_Eth_00031
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_ControllerInit {#design_eth_low_level_api_ctrl_init}

Refer section 8.3.2 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_006</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00033, SWS_Eth_00034, SWS_Eth_00035, SWS_Eth_00036, SWS_Eth_00037, SWS_Eth_00038, SWS_Eth_00039, SWS_Eth_00040
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_SetControllerMode {#design_eth_low_level_api_set_ctrl_mode}

Refer section 8.3.3 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_007</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00041, SWS_Eth_00042, SWS_Eth_00043, SWS_Eth_00044, SWS_Eth_00045, SWS_Eth_00168
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_GetControllerMode {#design_eth_low_level_api_get_ctrl_mode}

Refer section 8.3.4 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_008</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00046, SWS_Eth_00047, SWS_Eth_00048, SWS_Eth_00049, SWS_Eth_00050, SWS_Eth_00051
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_WriteMii {#design_eth_low_level_api_wr_mii}

Refer section 8.3.8 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_010</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00058, SWS_Eth_00059, SWS_Eth_00060, SWS_Eth_00061, SWS_Eth_00062, SWS_Eth_00063, SWS_Eth_00241
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_ReadMii {#design_eth_low_level_api_rd_mii}

Refer section 8.3.9 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_011</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00064, SWS_Eth_00065, SWS_Eth_00066, SWS_Eth_00067, SWS_Eth_00068, SWS_Eth_00069, SWS_Eth_00070, SWS_Eth_00242
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_GetPhysAddr {#design_eth_low_level_api_get_phy_addr}

Refer section 8.3.5 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_009</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00052, SWS_Eth_00053, SWS_Eth_00054, SWS_Eth_00055, SWS_Eth_00056, SWS_Eth_00057
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_ProvideTxBuffer {#design_eth_low_level_api_give_tx}

Refer section 8.3.18 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_012</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00077, SWS_Eth_00078, SWS_Eth_00079, SWS_Eth_00080, SWS_Eth_00081, SWS_Eth_00082, SWS_Eth_00083, SWS_Eth_00084, SWS_Eth_00085, SWS_Eth_00086, SWS_Eth_00137
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_Transmit {#design_eth_low_level_api_tx}

Refer section 8.3.19 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_013</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00087, SWS_Eth_00088, SWS_Eth_00089, SWS_Eth_00090, SWS_Eth_00091, SWS_Eth_00092, SWS_Eth_00093, SWS_Eth_00094, SWS_Eth_00129, SWS_Eth_00138
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_Receive {#design_eth_low_level_api_rx}

Refer section 8.3.20 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_014</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00095, SWS_Eth_00096, SWS_Eth_00097, SWS_Eth_00098, SWS_Eth_00099, SWS_Eth_00132, SWS_Eth_00153
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_TxConfirmation {#design_eth_low_level_api_tx_confirm}

Refer section 8.3.21 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_015</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00100, SWS_Eth_00101, SWS_Eth_00102, SWS_Eth_00103, SWS_Eth_00104, SWS_Eth_00105, SWS_Eth_00134
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_GetVersionInfo {#design_eth_low_level_api_get_ver}

Refer section 8.3.22 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_016</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00106, SWS_Eth_00136
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_RxIrqHdlr_<CtrlIdx> {#design_eth_low_level_api_rx_irq}

<table >
  <tr>
    <td><i><b> </b></i></td>
    <td><i><b>Description</b></i></td>
    <td><i><b> Comments</b></i></td>
  </tr>
  <tr>
    <td><i><b>Service Name</b></i></td>
    <td>Eth_RxIrqHdlr_<CtrlIdx> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>Syntax</b></i></td>
    <td>void Eth_RxIrqHdlr_<CtrlIdx>(void) </td>
    <td>Handles frame reception interrupts of the indexed controller</td>
  </tr>
  <tr>
    <td><i><b>Service ID </b></i></td>
    <td>0x10</td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>Sync / Async </b></i></td>
    <td>Synchronous</td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>Reentrancy</b></i></td>
    <td>Non Reentrant</td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>Parameter in</b></i></td>
    <td>None</td>
    <td></td>
  </tr>
  <tr>
    <td><i><b>Parameters out</b></i></td>
    <td>None</td>
    <td>  </td>
  </tr>
  <tr>
    <td><i><b>Return Value</b></i></td>
    <td>None</td>
    <td> </td>
  </tr>
</table>

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_017</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00109, SWS_Eth_00110, SWS_Eth_00111, SWS_Eth_00112, SWS_Eth_00113
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_TxIrqHdlr_<CtrlIdx> {#design_eth_low_level_api_tx_irq}

<table >
  <tr>
    <td><i><b> </b></i></td>
    <td><i><b>Description</b></i></td>
    <td><i><b> Comments</b></i></td>
  </tr>
  <tr>
    <td><i><b>Service Name</b></i></td>
    <td>Eth_TxIrqHdlr_<CtrlIdx> </td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>Syntax</b></i></td>
    <td>void Eth_TxIrqHdlr<CtrlIdx>(void) </td>
    <td>Handles frame transmission interrupts of the indexed controller</td>
  </tr>
  <tr>
    <td><i><b>Service ID </b></i></td>
    <td>0x11</td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>Sync / Async </b></i></td>
    <td>Synchronous</td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>Reentrancy</b></i></td>
    <td>Non Reentrant</td>
    <td> </td>
  </tr>
  <tr>
    <td><i><b>Parameter in</b></i></td>
    <td>None</td>
    <td></td>
  </tr>
  <tr>
    <td><i><b>Parameters out</b></i></td>
    <td>None</td>
    <td>  </td>
  </tr>
  <tr>
    <td><i><b>Return Value</b></i></td>
    <td>None</td>
    <td> </td>
  </tr>
</table>

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_018</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00114, SWS_Eth_00115, SWS_Eth_00116, SWS_Eth_00117, SWS_Eth_00118
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

### Eth_UpdatePhysAddrFilter  {#design_eth_low_level_api_up_phy_addr}

Refer section 8.3.7 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_020</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00144, SWS_Eth_00146, SWS_Eth_00147, SWS_Eth_00150, SWS_Eth_00152, SWS_Eth_00164, SWS_Eth_00165, SWS_Eth_00166, SWS_Eth_00167
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

### Eth_SetPhysAddr {#design_eth_low_level_api_set_phy_addr}

Refer section 8.3.6 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_019</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00139, SWS_Eth_00140, SWS_Eth_00141, SWS_Eth_00142, SWS_Eth_00143, SWS_Eth_00151
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

### Eth_GetDropCount {#design_eth_low_level_api_get_drop_count}

Refer section 8.3.10 of [1](@ref design_eth_references) and in addition, this
function shall read a list with drop counter values of the corresponding controller. The meaning of these values is hardware dependent. However, the list DropCount[] shall contain the following values in the given order, where the maximal possible value shall denote an invalid value, e.g. if this counter is not available:
- dropped packets due to buffer overrun
- dropped packets due to CRC errors
- number of undersize packets which were less than 64 octets long (excluding framing bits, but including FCS octets) and were otherwise will formed. (see IETF RFC 1757)
- number of oversize packets which are longer than 1518 octets (excluding framing bits, but including FCS octets) and were otherwise well formed. (see IETF RFC 1757)
- number of alignment errors, i.e. packets which are received and are not an integral number of octets in length and do not pass the CRC.
- SQE test error according to IETF RFC1643 dot3StatsSQETestErrors
- The number of inbound packets which were chosen to be discarded even though no errors had been detected to prevent their being deliverable to a higher-layer protocol. One possible reason for discarding such a packet could be to free up buffer space. (see IETF RFC 2233 ifInDiscards)
- total number of erroneous inbound packets
- The number of outbound packets which were chosen to be discarded even though no errors had been detected to prevent their being transmitted. One possible reason for discarding such a packet could be to free up buffer space. (see IETF RFC 2233 ifOutDiscards)
- total number of erroneous outbound packets
- Single collision frames: A count of successfully transmitted frames on a particular interface for which transmission is inhibited by exactly one collision. (see IETF RFC1643 dot3StatsSingleCollisionFrames)
- Multiple collision frames: A count of successfully transmitted frames on a particular interface for which transmission is inhibited by more than one collision. (see IETF RFC1643 dot3StatsMultipleCollisionFrames)
- Number of deferred transmission: A count of frames for which the first transmission attempt on a particular interface is delayed because the medium is busy. (see IETF RFC1643 dot3StatsDeferredTransmissions)
- Number of late collisions: The number of times that a collision is detected on a particular interface later than 512 bit-times into the transmission of a packet. (see IETF RFC1643 dot3StatsLateCollisions)
- The following positions in the list can contain hardware dependent counter values


<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_022</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00226, SWS_Eth_00227, SWS_Eth_00228, SWS_Eth_00229, SWS_Eth_00230, SWS_Eth_00231, SWS_Eth_00232
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_GetEtherStats {#design_eth_low_level_api_get_eth_stats}

Refer section 8.3.11 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_023</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00233, SWS_Eth_00234, SWS_Eth_00235, SWS_Eth_00236, SWS_Eth_00237, SWS_Eth_00238, SWS_Eth_00239
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_GetCurrentTime {#design_eth_low_level_api_get_curr_time}

Refer section 8.3.12 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_026</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00181, SWS_Eth_00182, SWS_Eth_00183, SWS_Eth_00184, SWS_Eth_00185, SWS_Eth_00210
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_EnableEgressTimeStamp {#design_eth_low_level_api_en_egress_time}

Refer section 8.3.13 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_027</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00186, SWS_Eth_00187, SWS_Eth_00188, SWS_Eth_00189, SWS_Eth_00211
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_GetEgressTimeStamp {#design_eth_low_level_api_get_egress_time}

Refer section 8.3.14 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_028</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00190, SWS_Eth_00191, SWS_Eth_00192, SWS_Eth_00193, SWS_Eth_00194, SWS_Eth_00212
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_GetIngressTimeStamp {#design_eth_low_level_api_get_ingress_time}

Refer section 8.3.15 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_029</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00195, SWS_Eth_00196, SWS_Eth_00197, SWS_Eth_00198, SWS_Eth_00199, SWS_Eth_00213
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_SetCorrectionTime {#design_eth_low_level_api_set_correction_time}

Refer section 8.3.16 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_030</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00200, SWS_Eth_00201, SWS_Eth_00202, SWS_Eth_00203, SWS_Eth_00204, SWS_Eth_00214
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_SetGlobalTime {#design_eth_low_level_api_set_global_time}

Refer section 8.3.17 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_031</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00205, SWS_Eth_00206, SWS_Eth_00207, SWS_Eth_00208, SWS_Eth_00209, SWS_Eth_00215
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Eth_MainFunction {#design_eth_low_level_api_main_function}

Refer section 8.5.1 of [1](@ref design_eth_references)

<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_031</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00169, SWS_Eth_00170, SWS_Eth_00171, SWS_Eth_00172, SWS_Eth_00240
    </td>
  </tr>
</table>

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Types {#design_eth_low_level_types}

The following types are required as per AUTOSAR Ethernet Driver specification,
8.2 of [1](@ref design_eth_references).

- Eth_ConfigType. Implementation specific structure of the post build configuration.
- Eth_ReturnType. Ethernet Driver specific return type.
- Eth_ModeType. Defines the controller modes.
- Eth_StateType. Status supervision used for Development Error Detection.
- Eth_FrameType. The Ethernet frame type used in the Ethernet frame header. 
- Eth_DataType. The Ethernet data type used for data transmission. Its definition depends on the used CPU.
- Eth_RxStatusType. Used as out parameter in Eth_Receive(). It indicates whether a frame has been received and if so, whether more frames are available or frames got lost.
- Eth_FilterActionType. An enumeration that describes the action to be taken for the MAC address given in the  *PhysAddrPtr.
- Eth_TimeStampQualType. Depending on the HW, quality information regarding the evaluated time stamp might be supported. If not supported, the value shall be always Valid. For Uncertain and Invalid values, the upper layer discards the time stamp.
- Eth_TimeStampType. Used for expressing the time stamps including relative and absolute calendar time.
- Eth_TimeIntDiffType. Used to express time differences in a usual way.
- Eth_RateRatioType. Used to express frequency ratios. 


<table >
  <tr>
    <td><i><b>Design ID</b></i></td>
    <td> DES_ETH_033</td>
  </tr>
  <tr>
    <td><i><b>Requirements Covered</b></i></td>
    <td> SWS_Eth_00156, SWS_Eth_00157, SWS_Eth_00158, SWS_Eth_00159, SWS_Eth_00160, SWS_Eth_00161, SWS_Eth_00162, SWS_Eth_00163, SWS_Eth_00175, SWS_Eth_00177, SWS_Eth_00178, SWS_Eth_00179, SWS_Eth_00180
    </td>
  </tr>
</table>
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Global Variables {#design_eth_low_level_globals}

This design expects that implementation will require to use following global
variables.

Variable | Type          | Description         | Default Value
---------|---------------|---------------------|--------------
gEthDrvStatus | Eth_StateType | ETH driver status | ETH_STATE_UNINIT
gEthDrvObj | Eth_DriverObj | ETH driver object | -
gEthTxBufMem | uint8 | TX packet memory pool | -
gEthRxBufMem | uint8 | RX packet memory pool | -
gEthTxFqRingMem | uint8 | TX free ring memory | -
gEthTxCqRingMem | uint8 | TX completion ring memory | -
gEthTxTdCqRingMem | uint8 | TX teardown completion ring memory | -
gEthRxFqRingMem | uint8 | RX free ring memory | -
gEthRxCqRingMem | uint8 | RX completion ring memory | -
gEthRxTdCqRingMem | uint8 | RX teardown completion ring memory | -
gEthCpswTxPkt | uint8 | TX UDMA Host Packet Descriptor (HPD) memory | -
gEthCpswRxPkt | uint8 | RX UDMA Host Packet Descriptor | -
cpswCfg | Cpsw_Config | CPSW initialization structure | ALE enabled in VLAN aware mode, all ports in forwarding mode. UDMA TX and RX interrupts disabled. MAC port with TX & RX flow control enabled, error and control frames passing to host enabled
Host port with CRC removal, short frame padding

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Decision Analysis & Resolution (DAR) {#design_eth_dar_top}

Sections below list some of the important design decisions and rational behind
those decision.

## Packet Submission & Retrieval to CPSW: Single or Queue {#design_eth_dar_q_or_s}

Traffic throughput can be limited by the underlying mechanism used to pass packets to the Ethernet hardware (CPSW). Unfortunately, the transfer related APIs of the AUTOSAR Ethernet Driver impose a significant constraint on the maximum throughput by processing packets one by one.
However, the driver implementation can be done in a way that facilitates future throughput related improvements and customizations.

### DAR Criteria {#design_eth_dar_q_or_s_criteria}

The ability to achieve higher throughput without significant driver complexity increase.

### Available Alternatives {#design_eth_dar_alternatives_1}

* <b> Single Packet  </b>
    A single packet is passed to the transmission or reception helper functions of the driver.

    * <b> Advantages: </b>
        + Simpler driver implementation, no queue operations need to be implemented
        + Meets the AUTOSAR Ethernet driver requirements
    * <b> Disadvantages: </b>
        + The maximum throughput is limited by having to process one packet at a time
        + Not straightforward way to increase throughput without major driver changes

* <b> Queue of Packets </b>
    A linked-list based queue of packets is passed to the transmission or reception helper functions of the driver. The helper functions take care of creating packet descriptors for each packet in the queue.

    * <b> Advantages: </b>
        + Prepares the driver for higher throughput use-cases
        + Prepares the driver for reduced CPU load in higher throughput use-cases, which is achieved by reducing the overhead per packet by processing them all in a single shot
    * <b> Disadvantages: </b>
        + Upper layers can’t take advantage of this approach unless the mechanism to pass packets to the driver is customized
        + Increases driver complexity

### Decision  {#design_eth_dar_decision_1}
    The capability of scaling the driver for higher Ethernet throughput is a strong argument in favor of the alternative 2 (Queue of Packets). No major overhead is foreseen by using a queue instead of an array of packets. While the driver complexity increases, it’s not significant enough to affect the decision.

[Back To Top](@ref design_eth_intro)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Buffers Per Packet {#design_eth_dar_buf_pack}

The number of data buffers in one Ethernet packet is often a configurable parameter in driver implementations. At this point, the AUTOSAR specification doesn’t require that the driver implementation supports multiple buffers per packet.


### DAR Criteria {#design_eth_dar_buf_pack_criteria}

Buffers per packet configurability without significant driver complexity increase.

### Available Alternatives {#design_eth_dar_alternatives_2}

* <b> One buffer per packet </b>
    No configurability allowed. The driver would create only Host Packet Descriptors.

    * <b> Advantages: </b>
        + Simpler driver implementation, only one type of descriptor needs to be implemented
        + Meets the AUTOSAR Ethernet driver requirements
    * <b> Disadvantages: </b>
        + Less configurability of the packet buffers

* <b> Configurable number of buffers per packet </b>
    The driver would create Host Packet Descriptors for the first buffer in the packet and Host Buffer Descriptor for the rest.

    * <b> Advantages: </b>
        + More configurability of the packet buffers
    * <b> Disadvantages: </b>
        + Upper layers can’t take advantage of this approach unless the mechanism to pass packets to the driver is customized
        + Increases driver complexity

### Decision  {#design_eth_dar_decision_2}
    Adding the support for flexible number of buffers per packet in the Ethernet driver is localized changes. Consequently it makes more sense to add this feature when the Ethernet specification mandates it. Recommended to implement option 1 (One buffer per packet)

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Test Criteria {#design_eth_test_top}

The sections below identify some of the aspects of design that would require
emphasis during testing of this design implementation

* <b> Loopback Test </b>
    + CPSW internal loopback test (transmit to receive). Generally speaking, the loopback tests are easier and faster to run because they don’t involve any additional test setup. It’s recommended that loopback tests are performed before any other type of testing as it can help identify problems more quickly (@ref design_eth_desc_cfg_ti)

* <b> Transmit: Basic and Stress Tests </b>
    + Transmission of 100 packets from the DUT
        + Packets shall be received in a PC machine (i.e. running Wireshark or similar application)
        + Manual inspection shall be done to ensure packet content is correct (i.e. it matches an expected pattern)
    + Transmission of 500k packets from the DUT
        + Packets shall be received in a PC
        + Received packet count must match
    + Transmission of packets with injected errors
        + The test application shall print TX error messages from CPSW statistics
    + This will exercise (@ref design_eth_seq_tx_opt)

* <b> Reception: Basic and Stress Tests </b>
    + Reception of N packets on the DUT
        + Packets shall be sent from a PC machine (i.e. running packeETH or similar application)
        + The test application shall inspect packet content to ensure content is correct (i.e. it matches an expected pattern)
    + Reception of 500k packets from the DUT
        + Packets shall be received in a PC
        + Received packet count must match
    + Reception of packets with injected errors
        + The test application shall print RX error messages from CPSW statistics
    + This will exercise (@ref design_eth_seq_rx_opt)

* <b> Polling and Interrupt Mode Test </b>
    + Basic transmission and reception tests with the Eth driver in interrupt mode
    + Basic transmission and reception tests with the Eth driver in polling mode

* <b> Global Time Support Tests </b>
    + Reception of PTP packets
        + PTP packets sent from PC
        + The test application shall inspected all incoming packets and identify the time sync events related
        + The test application shall print the time stamp of the identified packets

[Back To Top](@ref design_eth_intro)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Document Revision History {#design_eth_rev_hist}

Revision | Date          | Author     | Description         | Status
---------|---------------|------------|---------------------|-------
0.1      | 28 Jun 2018   | Misael Lopez   | First version | Approved
0.2      | 04 Oct 2018   | Sujith S   | Format change and re-order | Approved
