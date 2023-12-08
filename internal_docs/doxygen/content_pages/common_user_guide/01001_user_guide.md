# User Guide {#ethfw_c_ug_top}

Ethernet Firmware enables multiple client drivers to run independently on the
remaining cores in the system. For instance, A-cores can run HLOS like Linux
or QNX, and other R5F cores can run FreeRTOS or AUTOSAR software.
Client drivers communicate through the central Ethernet Firmware module for
any necessary switch configuration. Once setup packet are directly steered to
the designated cores based on the flow steered criteria described before.

This user guide presents the list of features supported by the Ethernet Firmware
(EthFw), and describes the steps required to build and run the EthFw demo
applications.

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Supported Features {#ethfw_c_ug_features_list}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Feature         | Comments
----------------|--------------
L2 switching    | Support for configuration of the Ethernet Switch to enable L2 switching between external ports with VLAN, multi-cast
Inter-VLAN routing | Inter-VLAN routing configuration in hardware with software fall-back support
lwIP integration | Integration of TCP/IP stack enabling TCP, UDP.
MAC-only         | Port configuration in MAC-only mode for traffic exclusively forwarded to host port, excludes the designated port(s) from switching logic
Intercore Virtual Ethernet |  Shared memory-based virtual Ethernet adapter communication between cores
Multi-core broadcast an multicast support | Multi-core concurrent reception of broadcast and multicast traffic using SW based fan-out 
^ | Ability to send broadcast and multicast traffic to multiple cores
Remote configuration server | Firmware app hosting the IPC server to serve remote clients like Linux Virtual MAC driver
Resource management library | Resource management library for CPSW resource sharing across cores

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Master Core (EthFw) {#ethfw_master_core}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

The multiport CPSW switch present in devices of the Jacinto family is an Ethernet
peripheral shared among the different processing cores within the SoC.  Ethernet
Firmware acts as the owner of the CPSW switch and provides a remote configuration
infrastructure for other processing cores running different operating systems.

Ethernet Firmware enables TCP/IP stack and gPTP stack, includes software and
hardware interVLAN demos, as well as helper utils libraries (i.e. network statistics).

The following diagram shows the main components of the Ethernet Firmware software
architecture.

![](switch_software_stack.png "Ethernet Firmware software architecture")

The TCP/IP stack integrated in the Ethernet Firmware is based on the open source lwIP
stack enabled on top of Enet LLD.

Ethernet Firmware sets up packet classifiers to route traffic to the different
remote processing cores.  Routing criterias are based on the switch ingress port number
or Layer-2 destination MAC address, depending on the virtual port type requested by
the remote cores.  Packets which don't match any of the configured classifier criteria are
routed to a default UDMA flow that is owned by Ethernet Firmware.

For multicast, if the traffic is exclusively requested by a single core it can be directly
steered to the designated core by programming the hardware classifier module through EthFw.
When multiple cores need to receive the same multicast flow, then it is always steered to
the Ethernet Firmware which plays the role of central hub that replicates and fans out.
Refer to the \ref ethfw_mcast_support section for more information.

Ethernet Firmware runs gPTP stack which operates either as master or slave clock based on the
gPTP configurations set, supporting both software and hardware adjustments for the CPTS clock.
This PTP implementation sets up CPSW ALE classifiers with PTP multicast MAC address and PTP
EtherType PTP classifier as match criteria to have PTP traffic routed to dedicated UDMA RX flow.

The remote configuration infrastructure provided by Ethernet Firmware is built using
the *ethremotecfg* framework which uses IPC LLD. Ethernet Firmware supports three types of messages namely, 
requests, responses and notifications. Requests are primarily sent by the remote clients and
waits for the response from the ETHFW server. The notifications from server to client donot have
any ACKs where from client to server notifies will be returned with a server ACK.
Remote configuration is based on the application defined server-client protocols which
can be found in */ethfw/ethremotecfg/protocol* folder. Ethernet Firmware plays the role of a server
which accepts and processes commands from the remote clients and carry out operations such as
attaching/detaching, registering a MAC address or IP, etc, on the client's behalf.

CPSW register configuration is carried out exclusively by Ethernet Firmware, remote
cores are not expected/allowed to perform any CPSW5G/CPSW9G register access, though that
is currently not enforced.  Ethernet Firmware uses Enet LLD for low-level CPSW5G/CPSW9G
driver support and for Ethernet PHY configuration.  Enet LLD internally uses UDMA LLD for
packet exchange with the CPSW switch.  Along with CPSW remote configuration, it is the responsibilty
of ETHFW to manage and distribute the resources among server and the remote clients.

The utilization of these resources by Ethernet Firmware on Main R5F 0 Core 0 is as follows:

| Resource    | Count  | EthFw Usage (mcu2_0)
|:------------|:------:|:-----------------------------------
| TX channel  |   3    | <ul><li>lwIP netif (1)</li><li>gPTP (1)</li><li>SW interVLAN (1)</li></ul>
| RX flow     |   5    | <ul><li>lwIP netif (1)</li><li>gPTP (1)</li><li>Proxy ARP (1) or VEPA (1) (only for J784S4)</li><li>SW interVLAN (1)</li><li>Enet LLD default flow (1)</li></ul>
| MAC address |   1    | <ul><li>lwIP netif (1)</li></ul>

UDMA TX channels are a resource especially limited as there is only a total of 8 TX channels
available.  So, there are 5 TX channels to be shared among the differrent remote client cores
and their virtual ports.

With Ethernet Firmware's default port configuration, the following resources will be used by
Linux remote client on A72 core.

| Resource    | Count  | Linux Client Usage
|:------------|:------:|:-----------------------------------
| TX channel  |   2    | <ul><li>Virtual switch port (1)</li><li>Virtual MAC port (1)</li></ul>
| RX flow     |   2    | <ul><li>Virtual switch port (1)</li><li>Virtual MAC port (1)</li></ul>
| MAC address |   2    | <ul><li>Virtual switch port (1)</li><li>Virtual MAC port (1)</li></ul>

With Ethernet Firmware's default port configuration, the following resources will be used by
RTOS remote client on Main R5F 0 Core 1.

| Resource    | Count  | RTOS Client Usage
|:------------|:------:|:-----------------------------------
| TX channel  |   2    | <ul><li>Virtual switch lwIP netif (1)</li><li>Virtual MAC port lwIP netif (1)</li></ul>
| RX flow     |   2    | <ul><li>Virtual switch lwIP netif (1)</li><li>Virtual MAC port lwIP netif (1)</li></ul>
| MAC address |   2    | <ul><li>Virtual switch lwIP netif (1)</li><li>Virtual MAC port lwIP netif (1)</li></ul>

For a given remote client in general will require a pair of Tx/Rx channels and a MAC Address per virtual port.

The following diagram shows a view of the Ethernet Firmware components and the
expected ownership.

![](building_block_owners.png "Component ownership in EthFw")

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remote Core Clients {#ethfw_remote_clients}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## RTOS Client {#ethfw_client_rtos}

Ethernet Firmware component in SDK provides a FreeRTOS client example application
running on Main R5F 0 core 1.  This application showcases lwIP TCP/IP stack and
multicore time synchronization built on top of Ethernet Firmware's IPC-based
remote config infrastructure.

![](rtos_client.png "EthFw with RTOS client")

The following lwIP netifs are enabled in the RTOS client application:

  - CPSW client drivers:
     - Virtual MAC port based netif - Dedicated MAC port from CPSW is excluded from
       regular packet switching and allocated exclusive for this R5F core.
     - Virtual switch port based netif - Virtual port which carries unicast RX traffic
       from hardware MAC ports and TX traffic to hardware MAC ports.
  - Shared memory virtual driver:
     - Intercore based netif - Used for broadcast/multicast packet exchange with
       R5F core running Ethernet Firmware. Refer to the \ref ethfw_intercore_eth section
       for more details about intercore Ethernet.

The two CPSW virtual port netifs reuse the same Enet LLD based lwIP implementation.

The RTOS core attaches to the Ethernet Firmware server using the *Eth Remote Config Client* 
library which is built using IPC LLD APIs can be located in */ethfw/ethremotecfg/client/*
folder.

The multicore time synchronization mechanism implemented in RTOS client consists
of a linear correction in software of a local timer owned by the RTOS core which
is periodically synchronized with the CPTS clock via HW push event 2.


### Porting RTOS client to Main R5F 1 Core 0 {#ethfw_client_rtos_mcu30}

Ethernet Firmware provides RTOS client support only on Main R5F 0 core 1 (*mcu2_1*) in
all supported SoCs.  Consequently, it's recommended to use *mcu2_1* processing core
if virtual network interface support is required on an RTOS core.

However, in SoCs with multiple R5F cores, like J721E, system design may require virtual
network support on a different R5F core, instead of Main R5F 0 Core 1.  This requires
porting effort on top of Enet LLD and Ethernet Firmware provided in standard TI SDK.
The scope of the required porting changes will be discussed next, in the context of
Main R5F 1 core 0 (*mcu3_0*) in J721E.

  - Enet LLD:
     - Add *mcu3_0* core in driver's CORELIST.  This will be enable core Enet LLD
       support on Main R5F 1 core 0.
     - Replace *mcu2_1* with *mcu3_0* in intercore and lwipific libraries.  Mainly
       renaming MCU2_1 macros to MCU3_0, but more importantly, updating the IPC
       core id (IPC_MCU2_1 -> IPC_MCU3_0).

  - Ethernet Firmware (server application):
     - Replace IPC core id of the RTOS client (IPC_MCU2_1 -> IPC_MCU3_0) in EthFw
       library, CpswProxy server and client, as well as EthFw server application.
     - Update the IPC core id where virtual switch port and virtual MAC port
       interfaces are enabled on.
     - Update memory addresses used for MCU3_0 IPC (0xA3000000 -> 0xA4000000).

  - RTOS client application:
     - Update IPC multiproc configuration to exclude *mcu3_0* and include *mcu2_1*.
     - Update memory addresses used for MCU3_0 IPC (0xA3000000 -> 0xA4000000).
     - Rename Enet LLD intercore macros as per corresponding changes in Enet LLD
       for mcu3_0 support.

It's worth noting that above list summarizes the software changes needed for running
RTOS client on *mcu3_0* instead of *mcu2_1*.  It must not be confused with adding
*mcu3_0* support in addition to *mcu2_1*, which is not in the scope of this discussion.

The following Enet LLD and Ethernet Firmware [patches](j721e_mcu3_0_rtos_client.zip)
are provided as reference to illustrate the actual software changes corresponding to
the summary shown earlier. These changes are meant as reference for prototyping, and
are provided as is.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Linux Client {#ethfw_client_linux}

TI Linux kernel provides support for the two types of CPSW client drivers, *virtual MAC
port* and *virtual switch port*, through the j721e-cpsw-virt-mac driver.  Both interfaces
types are enabled by default in TI Processor SDK Linux.

The following diagram presents a simplified view of the main components involved
in the Linux client usecase.

![](linux_client.png "EthFw with Linux client")

The *rpmsg* client driver is compatible with the *ethremotecfg* server
side running on RTOS master core (Ethernet Firmware).  This driver is used to exchange
control messages with Ethernet Firmware to establish a virtual port connection.

It's important to note that the Ethernet packet exchange doesn't happen via IPC.
Instead, it happens completely in hardware via UDMA TX channel and RX flow.

For further information, please refer to [CPSWng_virt_mac](http://software-dl.ti.com/jacinto7/esd/processor-sdk-linux-jacinto7/latest/exports/docs/linux/Foundational_Components/Kernel/Kernel_Drivers/Network/CPSWng_virt_mac.html)
documentation in Processor SDK Linux.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## QNX Client {#ethfw_client_qnx}

TI's baseport for QNX provides support for *virtual switch port* network interface
through its *devnp_cpsw9g* driver.  *Virtual MAC port* (MAC-only mode) is currently
not supported by QNX client.

The following diagram shows a simplified view of the main components involved in
the QNX client's virtual port implementation.

![](qnx_client.png "EthFw with QNX client")

TI's *devnp_cpsw9g* driver implements the driver interface of the QNX networking stack
(io-pkt), so the virtual MAC port network interface is exposed transparently to the user
as any other native networking interface.

*devnp_cpsw9g* driver uses Ethernet Firmware's remote configuration infrastructure
in order to attach/detach the virtual port, register its MAC address, IP address, etc.
This is the same remote configuration API used by other remote clients such as RTOS core,
and consequently also sits on top of the *ethremotecfg* framework. The lower level IPC
functionality is provided by the *IPC RM* (QNX resmgr).

Ethernet packet exchange with the CPSW switch happens in hardware through an UDMA TX
channel and RX flow, completely independent of the Ethernet Firmware.  *devnp_cpsw9g*
driver uses Enet LLD data path APIs natively to submit and retrieve Ethernet packets.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## AUTOSAR Client {#ethfw_client_autosar}

Ethernet Firmware is also able to attach to a remote client running AUTOSAR.  The
AUTOSAR client must use TI's MCAL Eth VirtMAC driver.  This is a MCAL Eth driver with
TI customizations for virtual MAC functionality.

A simplified view of the main entities involved in the AUTOSAR remote client usecase
are shown in the following diagram.

![](autosar_client.png "EthFw with AUTOSAR client")

The remote core configuration is implemented on top of TI MCAL IPC CDD using the same
protocol headers defined by ETHFW in *ethfw/ethremotecfg/protocol* folder.

Ethernet packet exchange with the CPSW switch doesn't happen via IPC, but in hardware via
UDMA TX channel and RX flow.

In the current release, AUTOSAR client only supports *virtual switch port*.
*Virtual MAC port* (MAC-only mode) is not supported.

Note that the AUTOSAR client in the SDK has enabled on Main R5F 0 core 1 with remote endpoint id as 28
and MCU R5F 0 core 0 with remote endpoint id as 38.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Component Location {#ethfw_component_location}

The location within the SDK directory structure of the software components which
are relevant for Ethernet Firmware usecases is shown in the following figure.
Note that this figure presents a consolidated view of the Ethernet Firmware and all
the supported remote clients, but that doesn't mean that all clients can be supported
simultaneously.

![](EthFw_component_location.png "Location of the Ethernet Firmware related components in SDK")

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# MAC-only {#ethfw_maconly}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CPSW switch supports a feature called MAC-only mode which allows all incoming traffic from
a given MAC port to be transferred only to the host port.  This effectively excludes the
MAC ports configured in this mode for rest of packet switching happening in the CPSW switch.

Starting with SDK 8.1, Ethernet Firmware has enabled MAC-only mode on selected MAC ports.
To better understand the physical and logical entities involved in a system where MAC-only
mode has been enabled, let's start by defining key concepts:

- \ref ethfw_mac_port - The CPSW switch MAC ports.
- *Logical switch ports* - Defined based on packet header match criteria, typically created
  based on destination MAC address, VLAN IDs, etc.  Two possible types:

   - \ref ethfw_local_switch_port - owned exclusively by Ethernet Firmware.
   - \ref ethfw_virtual_switch_port - owned by remote clients (Linux, QNX, MCAL, RTOS).

- *Logical MAC-only ports* - Defined with 1-to-1 correspondence to physical ports (port
  configured in MAC-only mode), owned by remote clients.

   - \ref ethfw_virtual_mac_port - owned by remote clients (Linux, RTOS).

![](EthFw_PortCfg_generic.png "Ethernet Firmware logical ports and hardware ports")

The default port configuration for J721E and J7200 are shown in \ref ethfw_j721e_port_cfg
\ref ethfw_j7200_port_cfg and \ref ethfw_j784s4_port_cfg subsections, respectively.

The port's default VLAN for MAC ports configured in MAC-only mode is `0`, and for MAC ports
configured in switch mode is `1`. They can be changed via `EthFw_Config::dfltVlanIdMacOnlyPorts`
and `EthFw_Config::dfltVlanIdSwitchPorts`, respectively.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Hardware physical ports {#ethfw_mac_port}

These are the actual hardware MAC ports of the CPSW switch.  They can be configured in MAC-only
or switch (non MAC-only) mode.

The MAC ports which are to be enabled by the Ethernet Firmware as passed as a parameter
of `EthFw_Config` structure.  For example, below code snippet shows a configuration which
enables all 8 MAC ports in J721E CPSW9G.

```C
static Enet_MacPort gEthAppPorts[] =
{
    ENET_MAC_PORT_1, /* RGMII */
    ENET_MAC_PORT_3, /* RGMII */
    ENET_MAC_PORT_4, /* RGMII */
    ENET_MAC_PORT_8, /* RGMII */
#if defined(ENABLE_QSGMII_PORTS)
    ENET_MAC_PORT_2, /* QSGMII main */
    ENET_MAC_PORT_5, /* QSGMII sub */
    ENET_MAC_PORT_6, /* QSGMII sub */
    ENET_MAC_PORT_7, /* QSGMII sub */
#endif
};

static int32_t EthApp_initEthFw(void)
{
    EthFw_Config ethFwCfg;

    ...

    ethFwCfg.ports    = &gEthAppPorts[0];
    ethFwCfg.numPorts = ARRAY_SIZE(gEthAppPorts);

    ...
}
```

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Local switch port {#ethfw_local_switch_port}

This is a logical port owned by the Ethernet Firmware.

Ethernet packets are exchanged with the CPSW switch through its *host port* using a UDMA
RX flow and a TX channel.

CPSW's default thread is set to this port's UDMA RX flow, also called *default RX flow*.
Traffic which is not matched by any CPSW classifier gets routed to this port.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Virtual switch port {#ethfw_virtual_switch_port}

This is the traditional logical port owned by remote client cores, controlled via Ethernet
Firmware's IPC-based remote API.

Ethernet packets are also exchanged with the CPSW switch through its *host port* using a
UDMA RX flow and a TX channel.

RX traffic (to remote core) is segregated via CPSW ALE classifier with *unicast MAC address*
match criteria. TX traffic (from remote core) is sent as *non-directed* packets.

It's worth noting that virtual switch ports are not directly associated with any specific
hardware MAC port, as these virtual ports can received traffic from any MAC port as long as
the packets match the unicast MAC address classification criteria.

SDK 8.0 or older supported only this type of virtual port.

Virtual port (*virtual switch* or *virtual MAC*) are allocated to a specific core.

For example, below code snippet shows the virtual switch configuration:
- For remote_device based clients in `gEthApp_virtPortCfg` where *virtual switch port 0* is allocated
    for A72 core, and *virtual switch port 1* is allocated for Main R5F 0 Core 1.
- For AUTOSAR clients in `gEthApp_autosarVirtPortCfg` where *virtual switch port 1* is allocated for
    Main R5F 0 Core 1 and *virtual switch port 2* is allocated for MCU R5F 0 Core 0.

It's worth noting that in this specific configuration *virtual switch port 1* can be used by an RTOS
client or AUTOSAR client, depending on the OS running on Main R5F 0 Core 1.

```C
static EthFw_VirtPortCfg gEthApp_virtPortCfg[] =
{
    {
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_0,
    },
    {
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_SWITCH_PORT_1,
    },
    {
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_MAC_PORT_1,
    },
    {
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_MAC_PORT_4,
    },
};

static EthFw_VirtPortCfg gEthApp_autosarVirtPortCfg[] =
{
    {
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_SWITCH_PORT_1,
    },
    {
        .remoteCoreId = IPC_MCU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_2,
    },
};

static int32_t EthApp_initEthFw(void)
{
    EthFw_Config ethFwCfg;

    ...

    /* Set virtual port configuration parameters */
    ethFwCfg.virtPortCfg  = &gEthApp_virtPortCfg[0];
    ethFwCfg.numVirtPorts = ARRAY_SIZE(gEthApp_virtPortCfg);

    /* Set AUTOSAR virtual port configuration parameters */
    ethFwCfg.autosarVirtPortCfg  = &gEthApp_autosarVirtPortCfg[0];
    ethFwCfg.numAutosarVirtPorts = ARRAY_SIZE(gEthApp_autosarVirtPortCfg);

   ...
}
```

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Virtual MAC port {#ethfw_virtual_mac_port}

This is also a logical port owned by remote clients and controlled via Ethernet Firmware's
IPC-based remote API.

Ethernet packets are also exchanged with the CPSW switch through its *host port* using a
UDMA RX flow and a TX channel.

RX traffic (to remote core) is segregated via CPSW ALE classifier with *port* match criteria.
TX traffic (from remote core) is sent as *directed* packets.

These virtual ports are directly associated with a hardware MAC port which is configured in
MAC-only mode.

Below code snippet (which is same as shown in previous section for \ref ethfw_virtual_switch_port)
shows a configuration where *virtual MAC port 1* is allocated for A72, and *virtual MAC port 4*
is allocated for Main R5F 0 Core 1. It's worth noting that virtual MAC ports are only supported
in Linux and RTOS client, hence no virtual MAC ports are allocated for AUTOSAR client.

```C
static EthFw_VirtPortCfg gEthApp_virtPortCfg[] =
{
    {
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_0,
    },
    {
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_SWITCH_PORT_1,
    },
    {
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_MAC_PORT_1,
    },
    {
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_MAC_PORT_4,
    },
};

static EthFw_VirtPortCfg gEthApp_autosarVirtPortCfg[] =
{
    {
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_SWITCH_PORT_1,
    },
    {
        .remoteCoreId = IPC_MCU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_2,
    },
};

static int32_t EthApp_initEthFw(void)
{
    EthFw_Config ethFwCfg;

    ...

    /* Set virtual port configuration parameters */
    ethFwCfg.virtPortCfg  = &gEthApp_virtPortCfg[0];
    ethFwCfg.numVirtPorts = ARRAY_SIZE(gEthApp_virtPortCfg);

    /* Set AUTOSAR virtual port configuration parameters */
    ethFwCfg.autosarVirtPortCfg  = &gEthApp_autosarVirtPortCfg[0];
    ethFwCfg.numAutosarVirtPorts = ARRAY_SIZE(gEthApp_autosarVirtPortCfg);

   ...
}
```

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Configuring additional ports in MAC-only mode {#ethfw_additional_maconly}

The default port configuration of Ethernet Firmware can be changed to fit the specific
architecture requirements of each system.

If additional ports need to be configured in MAC-only mode, one needs to follow these steps:

-# Add the new *MAC port* to the port array passed via `EthFw_Config::port` config parameter.
-# Add a new *virtual MAC port* corresponding to the hardware *MAC port* of interest.
   The virtual port configuration is passed via `EthFw_Config::virtPortCfg` config parameter.
   The virtual port mode must be set to `ETHREMOTECFG_MAC_PORT_<n>` which is an enum of
   type `EthRemoteCfg_VirtPort`.

```C
static Enet_MacPort gEthAppPorts[] =
{
    ...
    ENET_MAC_PORT_5, /* new MAC port being added */
};

static EthFw_VirtPortCfg gEthApp_virtPortCfg[] =
{
    ...
    {
        .remoteCoreId = IPC_MCU2_1,              /* new MAC port allocated for MCU2_1 RTOS usage */
        .portId       = ETHREMOTECFG_MAC_PORT_5, /* new MAC port in MAC-only mode */
    },
};

static int32_t EthApp_initEthFw(void)
{
    EthFw_Config ethFwCfg;

    ...

    ethFwCfg.ports        = &gEthAppPorts[0];
    ethFwCfg.numPorts     = ARRAY_SIZE(gEthAppPorts);
    ethFwCfg.virtPortCfg  = &gEthApp_virtPortCfg[0];
    ethFwCfg.numVirtPorts = ARRAY_SIZE(gEthApp_virtPortCfg);

    ...
}
```

On the other hand, if the new MAC port or an existing one needs to be changed from MAC-only mode
to switch mode, one can simply remove it from the `EthFw_VirtPortCfg` array.

Resource availability and allocation must be taken into account when adding additional virtual
ports, not only in MAC-only mode but also in switch mode.  Each virtual port will require one
UDMA TX channel and one UDMA RX flow, both are resources partitioned for each core in the SoC,
hence repartitioning might be needed.  Additionally, each virtual port will require a MAC address
which is also a limited resource.

Ethernet Firmware relies on Enet LLD's utils library to populate its MAC address pool
(see `EnetAppUtils_initResourceConfig()`).  The MAC address pool is populated with addresses
read from EEPROMs located in the different expansion boards in TI EVM.  Note that a static
MAC address pool is used as a workaround in TI EVMs for cases where I2C bus contention could
happen (i.e. when integrating with Linux).  It's expected that the MAC address pool population
mechanism is adapted when integrating Ethernet Firmware to different platforms.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Default Port Configuration {#ethfw_port_cfg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## J721E Port Configuration {#ethfw_j721e_port_cfg}

There are four MAC ports enabled by default in Ethernet Firmware for J721E SoC.  These
are the RGMII MAC ports in GESI board.

Two MAC ports are configured in MAC-only mode and allocated for A72 (Linux) and Main R5F
Core 1 (RTOS) usage.  The remaining two MAC ports are configured in switch mode.

![](EthFw_PortCfg_j721e_evm.png "J721E default port configuration")

The following table shows the full list of MAC ports in J721E EVM, the board they are
located and their MAC mode.

| MAC Port    | PHY Addr | Board  | MAC mode
|:------------|:--------:|:------:|:------------
| MAC Port 1  |   12     | GESI   | MAC-only
| MAC Port 2  |   16     | QSGMII | Switch Port
| MAC Port 3  |    0     | GESI   | Switch Port
| MAC Port 4  |    3     | GESI   | MAC-only
| MAC Port 5  |   17     | QSGMII | Switch Port
| MAC Port 6  |   18     | QSGMII | Switch Port
| MAC Port 7  |   19     | QSGMII | Switch Port
| MAC Port 8  |   15     | GESI   | Switch Port


[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## J7200 Port Configuration {#ethfw_j7200_port_cfg}

All the four MAC ports of CPSW5G are enabled by default in Ethernet Firmware for J7200 SoC.
These are the four QSGMII MAC ports in QSGMII (QpENet) expansion board.

Two MAC ports are configured in MAC-only mode and allocated for A72 (Linux) and Main R5F
Core 1 (RTOS) usage.  The remaining two MAC ports are configured in switch mode.

![](EthFw_PortCfg_j7200_evm.png "J7200 default port configuration")

The following table shows the full list of MAC ports in J7200 EVM, the board they are
located and their MAC mode.

| MAC Port    | PHY Addr | Board  | MAC mode
|:------------|:--------:|:------:|:------------
| MAC Port 1  |   16     | QSGMII | MAC-only
| MAC Port 2  |   17     | QSGMII | Switch Port
| MAC Port 3  |   18     | QSGMII | Switch Port
| MAC Port 4  |   19     | QSGMII | MAC-only

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## J784S4 Port Configuration {#ethfw_j784s4_port_cfg}

J784S4 EVM provides two *Enet* expansion connectors (`ENET-EXP-1` and `ENET-EXP-2`) where
two expansion boards can be connected.  QSGMII (QpENet) board can be connected to either
expansion connector, but two QSGMII boards cannot be connected simultaneously due to board
limitation.

Only four MAC ports of CPSW9G are enabled by default in Ethernet Firmware for J784S4 SoC.
These are the four QSGMII MAC ports in QSGMII (QpENet) expansion board when connected in
slot 1 (`ENET-EXP-1`).

Two MAC ports are configured in MAC-only mode and allocated for A72 (Linux) and Main R5F
Core 1 (RTOS) usage.  The remaining two MAC ports are configured in switch mode.

![](EthFw_PortCfg_j784s4_evm.png "J784S4 default port configuration")

The following table shows the full list of MAC ports in J784S4 EVM, the board they are
located and their MAC mode.

| MAC Port    | PHY Addr | Board  | MAC mode
|:------------|:--------:|:------:|:------------
| MAC Port 1  |   16     | QSGMII | MAC-only
| MAC Port 3  |   17     | QSGMII | Switch Port
| MAC Port 4  |   18     | QSGMII | MAC-only
| MAC Port 5  |   19     | QSGMII | Switch Port

MAC ports 2, 6, 7 and 8 are not enabled.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Inter-core Virtual Ethernet via Shared Memory Transport {#ethfw_intercore_eth}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Starting with SDK 8.1, the EthFw integrates Inter-core Virtual Ethernet driver which allows
shared memory based Ethernet frame exchange between cores.  This is modelled as virtual
Ethernet adapter at each end.

-# @ref ethfw_intercore_topology
-# @ref ethfw_intercore_r5server
-# @ref ethfw_intercore_r5client
-# @ref ethfw_intercore_a72client

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
##  Topology and Design overview {#ethfw_intercore_topology}

Inter-core virtual network uses a star topology with the R5F_0 master core (EthFw server)
acting as the central hub. Each node (core) in the network communicates directly with the
master while communication between other nodes (A72 and R5F_1) is routed through the
master. In addition to the Enet LLD network interfaces used to communicate with the CPSW
switch, each participating core creates an inter-core network interface, which allows it
to communicate with another core using standard TCP/IP protocol suite. This is aimed at
modeling Ethernet-like communication between software running on-chip processing cores
(R5Fs, A72). Traffic external to the SoC is handled through CPSW hardware IP that can
steer traffic based on traffic flows directly to the respective cores.

The topology diagram below shows the integration of inter-core virtual Ethernet in Ethernet
Firmware.

![](Intercore_eth_topology_overview.png "Inter-core Virtual Ethernet Topology")

The main entities shown in this diagram are listed below:

-# <b>R5F_0 master</b>: EthFw server core which forms the central hub of the inter-core
network. Both client cores have a direct inter-core link to the R5F_0 master, as shown
with <span style="color:green"><b>green arrows</b></span>. Inter-core communication between
client cores e.g. A72 Linux client trying to ping R5F_1 client, goes through the R5F_0 master.

-# <b>R5F_1 client</b>: This is the EthFw RTOS remote client.
-# <b>A72 Linux client</b>: This is the EthFw Linux remote client.
-# <b>Shared memory transport</b>: The software based packet transport used by inter-core
network driver to exchange Ethernet packets. There is a dedicated set of shared queues and
shared buffer pools for each pair of directly connected nodes. Please refer to the Enet LLD
user guide for more details on the inter-core virtual Ethernet driver.
-# <b>Multicast replication manager</b>: This software component on R5F_0 master (EthFw server)
manages the fanout of shared multicast packets to the interested cores. It does so by dynamically
updating the lwIP bridge FDB database to add/remove cores to/from the given multicast MAC address
in response to the
[<b>multicast filter API</b>](../api_guide/group__ETHFW__ETHREMOTECFG.html#ggacfc53541f27433475f4bbdf233ce4ba7a20258e51b8d8a2d6f7e569a5b1785383)
commands from the remote cores.
-# <b>Data paths/flows</b>: Different data paths are used to route packets according to the type
of traffic (Unicast, Broadcast and Multicast). The <b>black</b> arrows show core specific dedicated
hardware flows which are used for unicast traffic originating from or bound to a given core as well
as incoming [exclusive multicast](@ref ethfw_exclusive_mcast) traffic for a given core. Please refer
to @ref ethfw_mcast_support for details on [shared multicast](@ref ethfw_shared_mcast) and
[exclusive_multicast](@ref ethfw_exclusive_mcast) traffic.

Broadcast and shared multicast packets are always sent to the R5F_0 master core using the
default flow shown by the <span style="color:red"><b>red arrow</b></span>. The master core
creates copies of such packets in software which is shown by the <span style="color:blue">
<b>blue arrows</b></span> and sends them out to other cores using the inter-core Ethernet
links shown by <span style="color:green"><b>green arrows</b></span>. 


On RTOS cores, the inter-core virtual Ethernet driver provides a standard lwIP netif
(network interface) to the application using which the application can exchange Ethernet
packets with another core. The inter-core netifs are seamlessly integrated in EthFw
(client and server) using lwIP bridgeif interface which allows the inter-core netifs to
co-exist along-side the Enet LLD native or virtual client interface on the server and
client respectively. The bridgeif provides a single unified network interface using which
the application communicates with the CPSW switch or other cores without worrying about
which netif to use for sending and receiving packets.

![](Intercore_virt_eth_rtos.png "Inter-core virtual Ethernet architecture: RTOS <-> RTOS")

Inter-core virtual Ethernet can also be used on Linux through a user space demo application
provided in the SDK. This demo application creates a Linux TAP networking device and passes
Ethernet packets back and forth between the TAP device and the inter-core transport shared
queues to communicate with the inter-core netif on EthFw server. The TAP network interface
can be bridged with the Enet LLD client driver interface to provide a single unified network
interface to the network stack, just like the R5F cores. The bridge will automatically select
the correct interface to send the packets based on the destination IP address.

![](Intercore_virt_eth_linux.png "Inter-core virtual Ethernet architecture: RTOS <-> Linux")

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## EthFw Server integration {#ethfw_intercore_r5server}

The EthFw server acts as the central hub of the inter-core virtual network, therefore it
instantiates two inter-core netifs, one to communicate with the EthFw R5F remote client
and another for the A72 (Linux) remote client. The inter-core netifs, along-with the Enet
LLD netif are all added to the lwIP bridgeif which provides a single unified interface to
the network stack/application. Refer to @ref ethfw_intercore_topology diagram which shows
the various netifs, including the lwIP bridge, created on the R5F_0 server core.

<b>Note</b>: The network stack / application sees only a single set of IP and MAC addresses
which belong to the bridgeif. The individual netifs, including the Enet LLD netif, are neither
visible to the network stack / application, nor do they get IP or MAC addresses.
 
Please refer to the following code in `<ethfw>/apps/app_remoteswitchcfg_server/mcu_2_0/main.c`
to understand how these netifs are instantiated and added to the bridge:

```C
#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
    /* Create Enet LLD ethernet interface */
    netif_add(&netif, NULL, NULL, NULL, NULL, LWIPIF_LWIP_init, tcpip_input);

    /* Create inter-core virtual ethernet interface: MCU2_0 <-> MCU2_1 */
    netif_add(&netif_ic[ETHAPP_NETIF_IC_MCU2_0_MCU2_1_IDX], NULL, NULL, NULL,
              (void*)&netif_ic_state[IC_ETH_IF_MCU2_0_MCU2_1],
              LWIPIF_LWIP_IC_init, tcpip_input);

    /* Create inter-core virtual ethernet interface: MCU2_0 <-> A72 */
    netif_add(&netif_ic[ETHAPP_NETIF_IC_MCU2_0_A72_IDX], NULL, NULL, NULL,
              (void*)&netif_ic_state[IC_ETH_IF_MCU2_0_A72],
              LWIPIF_LWIP_IC_init, tcpip_input);

    /* Create bridge interface */
    bridge_initdata.max_ports = ETHAPP_LWIP_BRIDGE_MAX_PORTS;
    bridge_initdata.max_fdb_dynamic_entries = ETHAPP_LWIP_BRIDGE_MAX_DYNAMIC_ENTRIES;
    bridge_initdata.max_fdb_static_entries = ETHAPP_LWIP_BRIDGE_MAX_STATIC_ENTRIES;
    EnetUtils_copyMacAddr(&bridge_initdata.ethaddr.addr[0U], &gEthAppObj.hostMacAddr[0U]);

    netif_add(&netif_bridge, &ipaddr, &netmask, &gw, &bridge_initdata, bridgeif_init, netif_input);

    /* Add all netifs to the bridge and create coreId to bridge portId map */
    bridgeif_add_port(&netif_bridge, &netif);
    gEthApp_lwipBridgePortIdMap[IPC_MCU2_0] = ETHAPP_BRIDGEIF_CPU_PORT_ID;

    bridgeif_add_port(&netif_bridge, &netif_ic[0]);
    gEthApp_lwipBridgePortIdMap[IPC_MCU2_1] = ETHAPP_BRIDGEIF_PORT1_ID;

    bridgeif_add_port(&netif_bridge, &netif_ic[1]);
    gEthApp_lwipBridgePortIdMap[IPC_MPU1_0] = ETHAPP_BRIDGEIF_PORT2_ID;

    /* Set bridge interface as the default */
    netif_set_default(&netif_bridge);
#else
```
[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## R5F RTOS Client integration {#ethfw_intercore_r5client}

The EthFw client on R5F_1 instantiates only one inter-core netif to communicate directly
with the EthFw server on R5F_0. Similar to the EthFW server, an lwIP bridgeif is created and
both the inter-core netif and the Enet LLD virtual netif are added to the bridge to provide a
unified network interface to the application.

Refer to @ref ethfw_intercore_topology diagram which shows the various netifs, including the
lwIP bridge, created on the R5F_1 client core.

<b>Note</b>: The network stack / application sees only a single set of IP and MAC addresses
which belong to the bridgeif. The individual netifs, including the Enet LLD netif, are neither
visible to the network stack / application, nor do they get IP or MAC addresses.

Please refer to the following code in `<ethfw>/apps/app_remoteswitchcfg_client/mcu_2_1/main.c`
to understand how these netifs are instantiated and added to the bridge:

```C
#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
    /* Create Enet LLD ethernet interface */
    netif_add(netif, NULL, NULL, NULL, NULL, LWIPIF_LWIP_init, tcpip_input);

    /* Create inter-core virtual ethernet interface: MCU2_1 <-> MCU2_0 */
    netif_add(&netif_ic, NULL, NULL, NULL,
              (void*)&netif_ic_state[IC_ETH_IF_MCU2_1_MCU2_0],
              LWIPIF_LWIP_IC_init, tcpip_input);

    /* Create bridge interface */
    bridge_initdata.max_ports = ETHAPP_LWIP_BRIDGE_MAX_PORTS;
    bridge_initdata.max_fdb_dynamic_entries = ETHAPP_LWIP_BRIDGE_MAX_DYNAMIC_ENTRIES;
    bridge_initdata.max_fdb_static_entries = ETHAPP_LWIP_BRIDGE_MAX_STATIC_ENTRIES;
    EnetUtils_copyMacAddr(&bridge_initdata.ethaddr.addr[0U], &virtNetif->macAddr[0U]);

    netif_add(&netif_bridge, &ipaddr, &netmask, &gw, &bridge_initdata, bridgeif_init, netif_input);

    /* Add all network interfaces to the bridge */
    bridgeif_add_port_with_opts(&netif_bridge, netif, BRIDGEIF_PORT_CPSW);
    bridgeif_add_port_with_opts(&netif_bridge, &netif_ic, BRIDGEIF_PORT_VIRTUAL);

    /* Set bridge interface as the default */
    netif_set_default(&netif_bridge);
    netif_set_status_callback(&netif_bridge, EthApp_lwipNetifStatusCb);
#else
```
[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## A72 Linux Client integration {#ethfw_intercore_a72client}

Inter-core virtual Ethernet can also be used on the A72 Linux remote client, however lwIP is
not used on Linux so we cannot use the inter-core virtual driver directly. Instead, the
adaptation layer between the Linux network stack and the inter-core transport is implemented
in a user space demo application called <b>TAP</b>, which is provided under `<ethfw>/apps/tap/`.
This user space application creates a Linux TAP networking device and passes Ethernet packets
back and forth between the TAP device and the inter-core transport shared queues to communicate
with the inter-core netif on EthFw server. Further, the TAP network interface can be bridged
with the Enet LLD client interface to provide a single unified interface to the network stack,
just like the R5F cores.

**Note**: The TAP driver implementation is provided as a reference only to demonstrate
and test the intercore functionality in Linux.  It comes with limited feature support,
such as polling  mode operation only, basic packet handling.

Please refer to the following code in `<ethfw>/apps/tap/tapif.c`:

```C
    /* Open TAP device and get TAP device descriptor */
    tap_fd = tap_open(tap_device_name);
    if (tap_fd < 0) {
        perror("Allocating interface");
        assert(tap_fd >= 0);
    }
    printf("Opened TAP Device successfully\n");
    fflush(stdout);

    /* Try to open the memory and fetch its file descriptor */
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        printf("Failed to open /dev/mem\n");
        fflush(stdout);
        assert(0 && "Failed to access shared memory");
    }

    /*Create a mapping between the physical addresses and virtual addresses */
    /* for the Queue Region using mmap*/
    IcQ_globalQTable_Handle =
                (IcQ_Handle)mmap(NULL, q_len, PROT_READ | PROT_WRITE,
                MAP_SHARED, mem_fd, q_base_addr);
    /* Check for failure in mapping */
    assert(IcQ_globalQTable_Handle != MAP_FAILED && "Queue Mapping Failed");
    printf("Queue Mapping Succeeded\n");
    fflush(stdout);

    /*Create a mapping between the physical addresses and virtual addresses */
    /* for the Buffer Region using mmap*/
    BufpoolTable_Handle = (Bufpool_Handle)mmap(NULL, bufpool_len,
                                PROT_READ | PROT_WRITE, MAP_SHARED,
                                mem_fd, bufpool_base_addr);
    /* Check for failure in mapping */
    assert(BufpoolTable_Handle != MAP_FAILED && "Bufpool Mapping Failed");
    printf("Bufpool Mapping Succeeded\n");
    fflush(stdout);

    /* Define txQ_Handle and rxQ_Handle */
    txQ_Handle =
        (IcQ_Handle)&(IcQ_globalQTable_Handle[tx_q_id]);
    rxQ_Handle =
        (IcQ_Handle)&(IcQ_globalQTable_Handle[rx_q_id]);

    printf("Assigned Queue Handles\n");
```

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Inter-core Virtual Ethernet via VEPA {#ethfw_intercore_vepa}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

VEPA is supported on J784S4 only, starting from SDK 9.1. EthFw provides support to enable VEPA
(Virtual Ethernet Port Aggregator) functionality with CPSW capable of _multihost_ data flow.
_Multihost_ is a CPSW ALE feature that enables packets to be sent and received on host port.
Multihost is the foundational feature to support VEPA.

-# @ref ethfw_intercore_topology_vepa
-# @ref ethfw_intercore_r5server_vepa

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Topology and Design overview {#ethfw_intercore_topology_vepa}

There are two distinctive data paths to consider in the intercore communication: unicast, and
multicast/broadcast.  The former only involves packet forwarding from source core to destination
core, while the latter involves packet duplication in addition to forwarding.

For unicast traffic, inter-core virtual network described in section \ref ethfw_intercore_eth
uses R5F_0 master core (EthFw server) acting as a hub, where each node (core) in the network
communicates directly with the master.  Conversely, in VEPA based intercore, direct communication
between other nodes (i.e. A72 and R5F_1) is <b>NOT</b> routed through the master anymore as
ALE _multihost_ and classifier makes it possible to forward packets directly between cores
without EthFw intervention.

For multicast/broadcast traffic, whenever broadcast or shared multicast packets reach EthFw server,
software duplicates the packet, tags it with a _private VLAN_ and sends the packets back to CPSW.
Each participating core has its own unique private VLAN through which packet forwarding happens.
The ALE classifiers set up by EthFw use the private VLAN id as a match criteria to route traffic
exclusively to the relevant core, hence the need of having one private VLAN per participating core.
The private VLANs are set up with untagging on egress, so it's transparent for the receiving
core as packets will be received without the private VLAN tag.

VEPA based implementation is a better alternative than shared memory transport approach as
it's transparent to remote cores and doesn't require additional shared memory based interfaces.
It also provides better throughput as packet forwarding is always via CPSW hardware, with
packet duplication being the only part being done in software.

It's worth noting that the VEPA implementation can coexist seamlessly with the mechanism
used to steer traffic from external ports to RX flows of the respective cores based on
destination MAC address.

The topology diagram below shows the integration of inter-core virtual Ethernet with VEPA 
in Ethernet Firmware.

![](Intercore_eth_topology_vepa.png "Inter-core Virtual Ethernet Topology with VEPA")

The main entities shown in this diagram are listed below:

-# <b>R5F_0 master</b>: EthFw server core which does packet duplication for broadcast and 
shared multicast traffic to all relevant remote cores. Broadcast and shared multicast packets
reach on a secondary RX flow as shown in <span style="color:red"><b>red arrows</b></span> 
dedicated for packets duplication.
-# <b>R5F_1 client</b>: This is the EthFw RTOS remote client.
-# <b>A72 Linux client</b>: This is the EthFw Linux remote client.
-# <b>Packet Duplication</b>: The software based packet duplication happens here for broadcast 
and shared multicast packets. Packets are duplicated and tagged with individual remote core's 
private VLAN and sent back to host port as shown with  <span style="color:blue"><b>blue 
arrows</b></span>. Packets are then re-routed back to host port as shown in  <span 
style="color:blue"><b>dotted blue arrows</b></span> using VEPA and reach the respective 
cores based on the private VLAN tagged on the packet. 
-# <b>Data paths/flows</b>: Different data paths are used to route packets according to the type
of traffic (Unicast, Broadcast and Multicast). The <b>black</b> arrows show core specific 
dedicated hardware flows which are used for unicast traffic originating from or bound to a 
given core as well as incoming [exclusive multicast](@ref ethfw_exclusive_mcast) traffic for a 
given core. Please refer to @ref ethfw_mcast_support for details on [shared multicast](@ref 
ethfw_shared_mcast) and [exclusive_multicast](@ref ethfw_exclusive_mcast) traffic.

<b>Note</b>: Refer to @ref ethfw_intercore_communication_vepa to get detailed description of various data paths/flows.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## EthFw Server integration {#ethfw_intercore_r5server_vepa}

Ethernet Firmware server creates ALE policer entry based on private VLAN associated to
each registered client. This ensures that when a private VLAN tagged packet comes from
packet duplication function it reaches the relevant registered client. Private VLANs
are configured by Ethernet Firmware based on application's settings related to the
VLAN ids to use.

Ethernet Firmware server registers multicast MAC addresses that need to be forwarded to
remote clients. An ALE entry and ALE policer entry is added for each multicast address
so that when multicast packets arrive, they are routed to secondary dedicated flow for
packet duplication allocated at init time as shown in <span style="color:red"><b> red arrows</b></span>.
When a multicast packet whose MAC address is registered comes on secondary dedicated flow,
it will be passed to a VEPA specific packet duplication handle function, which then calls
`EthFwVepa_sendRaw()` function to send a copy of the multicast packets to all relevant
remote cores.

<b>Note</b>: Unicast and exclusive multicast packets to EthFw or remote cores 
reach directly via dedicated flow as shown in <span style="color:black"><b>black 
arrows</b></span>.
 
Please refer to the following code in `<ethfw>/ethremotecfg/server/include/ethfw_vepa.c`
to understand how packets are tagged with private VLAN and sent back to host port

```C
/* i'th virtual switch port will get the packet
* Also source virtual switch port should not receive the packet */
if (ETHFW_IS_BIT_SET(virtPortMask, i) &&
    !EnetUtils_cmpMacAddr(ethSrcAddr->addr, gEthFwVepaObj.virtPortToMacAddr[i].addr))
{
    status = EthFwVepa_getPrivateVlanId(i, &privVlanId);
    if (status == ETHFW_SOK)
    {
        ethType = lwip_htons(ETHTYPE_VLAN);
        ethHdr = (struct eth_hdr *)copyPbuf->payload;

        /* Adding source and destination for the packet */
        SMEMCPY(&ethHdr->dest, ethDstAddr, ETH_HWADDR_LEN);
        SMEMCPY(&ethHdr->src,  ethSrcAddr, ETH_HWADDR_LEN);

        /* Adding tpid as 0x8100 (16 bits) */
        ethHdr->type = ethType;

        /* Adding priority and VLAN id tags (16 bits) */
        vlanhdr = (struct eth_vlan_hdr *)(((uint8_t *)copyPbuf->payload) + SIZEOF_ETH_HDR);
        vlanhdr->prio_vid = lwip_htons(pcpDei | privVlanId);

        /* Adding EtherType of the packet (16 bits) */
        vlanhdr->tpid = ((struct eth_hdr*)pbuf->payload)->type;

        /* Adding payload in the packet */
        SMEMCPY(copyPbuf->payload + SIZEOF_ETH_HDR + sizeof(struct eth_vlan_hdr),
                pbuf->payload + SIZEOF_ETH_HDR,
                pbuf->tot_len - SIZEOF_ETH_HDR);

        /* Send the packet */
        LOCK_TCPIP_CORE();
        netif->linkoutput(netif, copyPbuf);
        UNLOCK_TCPIP_CORE();
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to get priv VLAN for virtual port %u", i);
    }
}
```

Please refer to the following code in `<ethfw>/apps/app_remoteswitchcfg_server/mcu_2_0/main.c`
to understand how application can configure VEPA configurations (i.e. private VLAN associated to
each virtual switch port)

```C
#if defined(ETHFW_VEPA_SUPPORT)
/* Private VLAN ids used in broadcast/multicast packets sent from ETHFW
 * to remote clients using multihost flow */
static uint32_t gEthApp_remoteClientPrivVlanIdMap[ETHREMOTECFG_SWITCH_PORT_LAST+1] =
{
    [ETHREMOTECFG_SWITCH_PORT_0] = 1100U, /* Linux client */
    [ETHREMOTECFG_SWITCH_PORT_1] = 1200U, /* AUTOSAR or RTOS client */
    [ETHREMOTECFG_SWITCH_PORT_2] = 1300U, /* AUTOSAR client */
};
#endif
```


<b>Note</b>: No netif instance creation or TAP application is required on RTOS and Linux client respectively when VEPA is enabled on EthFw.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Multicast and Broadcast Support {#ethfw_mcast_support}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Starting with SDK 8.1, the Ethernet firmware supports client cores to receive multicast and broadcast traffic.

Broadcast support is automatically enabled through inter-core virtual Ethernet
mechanism which allows sending broadcast traffic to all the client cores, provided
that inter-core virtual Ethernet is enabled on that client.

For multicast support, a new
[<b>multicast filter API</b>](../api_guide/group__ETHFW__ETHREMOTECFG.html#ggacfc53541f27433475f4bbdf233ce4ba7a20258e51b8d8a2d6f7e569a5b1785383)
is provided by EthFw which allows client cores to subscribe-to/unsubscribe-from multicast
addresses. The Ethernet Firmware differentiates between two types of multicast addresses:

-# @ref ethfw_shared_mcast
-# @ref ethfw_exclusive_mcast
-# @ref ethfw_reserved_mcast

Note that the cores requesting a multicast address do not need to know if a particular
multicast address is shared or exclusive. This accounting is handled by the EthFw server
and is completely transparent to the requesting client core.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Shared Multicast {#ethfw_shared_mcast}

Shared multicast allows multiple client cores to subscribe to the same multicast address.
To support this, EthFw maintains a list of pre-defined multicast addresses which are treated as <b>shared</b>.

-# More than one core can request these multicast addresses through the
[<b>multicast filter API</b>](../api_guide/group__ETHFW__ETHREMOTECFG.html#ggacfc53541f27433475f4bbdf233ce4ba7a20258e51b8d8a2d6f7e569a5b1785383).
-# Traffic for these multicast addresses is always routed to the EthFw server from where
it is fanned out to all the client cores that requested that particular multicast address.
-# Shared multicast fanout is performed in software using inter-core virtual Ethernet
mechanism, therefore it is suited for low to medium bandwidth multicast traffic only.
-# The <b>shared multicast address list</b> is defined in source as shown below so the user
will need to modify and rebuild the EthFw binaries if they need to change these addresses:

Please refer to the following code in `<ethfw>/apps/app_remoteswitchcfg_server/mcu_2_0/main.c`:

```C
/* Must not exceed ETHAPP_MAX_SHARED_MCAST_ADDR entries */
static EthApp_SharedMcastAddrTable gEthApp_sharedMcastAddrTable[] =
{
    {
        /* MCast IP ADDR: 224.0.0.1 */
        .macAddr = {0x01,0x00,0x5E,0x00,0x00,0x01},
        .portMask= 0U,
    },
    {
        /* MCast IP ADDR: 224.0.0.251 */
        .macAddr = {0x01,0x00,0x5E,0x00,0x00,0xFB},
        .portMask= 0U,
    },
    {
        /* MCast IP ADDR: 224.0.0.252 */
        .macAddr = {0x01,0x00,0x5E,0x00,0x00,0xFC},
        .portMask= 0U,
    },
    {
        .macAddr = {0x33,0x33,0x00,0x00,0x00,0x01},
        .portMask= 0U,
    },
    {
        .macAddr = {0x33,0x33,0xFF,0x1D,0x92,0xC2},
        .portMask= 0U,
    },
    {
        .macAddr = {0x01,0x80,0xC2,0x00,0x00,0x00},
        .portMask= 0U,
    },
    {
        .macAddr = {0x01,0x80,0xC2,0x00,0x00,0x03},
        .portMask= 0U,
    },
};
```
[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Exclusive Multicast {#ethfw_exclusive_mcast}

Exclusive multicast addresses are allocated to only one core at any given time and the
corresponding multicast traffic is routed to that core directly using a dedicated hardware flow.

-# Any multicast addresses that do not belong to the shared multicast address list are
considered exclusive and ownership of such multicast addresses is granted to the first
requesting core. Any other cores requesting the same exclusive multicast address after
it has already been allocated, will get a failure.
-# Exclusive multicast traffic is routed directly to the allocated core through a dedicated
hardware flow therefore it is suitable for high bandwidth single-core multicast traffic.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Reserved Multicast {#ethfw_reserved_mcast}

Reserved multicast addresses are exclusive multicast addresses that are allocated only
to the core running Ethernet Firmware.  Any other core requesting for a reserved multicast
address will get a failure.

PTP-related multicast addresses are defined as reserved multicast addresses in Ethernet
Firmware's default configuration.  This is needed because Ethernet Firmware runs the
PTP stack and is the sole destination of PTP packets.

```C
/* Note: Must not exceed ETHFW_RSVD_MCAST_LIST_LEN */
static uint8_t gEthApp_rsvdMcastAddrTable[][ENET_MAC_ADDR_LEN] =
{
    /* PTP - Peer delay messages */
    {
        0x01, 0x80, 0xc2, 0x00, 0x00, 0x0E,
    },
    /* PTP - Non peer delay messages */
    {
        0x01, 0x1b, 0x19, 0x00, 0x00, 0x00,
    },
};
```

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# VLAN Support {#ethfw_vlan}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

VLAN support is split in two parts: VLAN creation/configuration and join/leave operations from
remote clients.

## VLAN Configuration {#ethfw_vlan_server_cfg}

VLANs are created and configured in a static manner and it's exclusive to Ethernet
Firmware.  Remote clients cannot create VLANs, they can only *join* or *leave* VLANs.

Parameters such as VLAN id, member lists (physical and virtual ports), registered
and unregistered multicast flood mask and untag mask are required in order to set up
VLANs on the Ethernet Firmware server side.

The code snippet below shows the configuration of VLAN 1024, with MAC ports 2 and 3
as members of the VLAN, and virtual switch ports 0, 1 and 2 as virtual members.

```C
/* VLAN member mask: host port + MAC ports 2 and 3 */
#define ETHAPP_DFLT_PORT_MASK          (CPSW_ALE_HOST_PORT_MASK | \
                                        CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_2) | \
                                        CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_3))

/* Default virtual port mask for shared multicast addresses: all virtual switch ports */
#define ETHAPP_DFLT_VIRT_PORT_MASK     (ETHFW_BIT(ETHREMOTECFG_SWITCH_PORT_0) | \
                                        ETHFW_BIT(ETHREMOTECFG_SWITCH_PORT_1) | \
                                        ETHFW_BIT(ETHREMOTECFG_SWITCH_PORT_2))

/* Test VLAN config */
EthFwVlan_VlanCfg gEthApp_vlanCfg[] =
{
    {
        .vlanId              = 1024U,
        .memberMask          = ETHAPP_DFLT_PORT_MASK,
        .regMcastFloodMask   = ETHAPP_DFLT_PORT_MASK,
        .unregMcastFloodMask = ETHAPP_DFLT_PORT_MASK,
        .virtMemberMask      = ETHAPP_DFLT_VIRT_PORT_MASK,
        .untagMask           = 0U,
    },
};

void EthApp_myFunc(void)
{
    ...

    /* Set static VLAN configuration parameters */
    ethFwCfg.vlanCfg  = &gEthApp_vlanCfg[0];
    ethFwCfg.numVlans = ARRAY_SIZE(gEthApp_vlanCfg);

    ...
}
```

For more information about VLAN configuration, please refer to the
[VLAN API Guide](../api_guide/group__ETHFW__SERVER__VLAN.html).

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Joining and leaving VLANs {#ethfw_vlan_join_leave}

Remote clients cannot create VLANs, but they can *join* or *leave* any of the VLANs
created by Ethernet Firmware through remote commands:
[<b>JOIN_VLAN</b>](../api_guide/group__ETHFW__ETHREMOTECFG.html#ggacfc53541f27433475f4bbdf233ce4ba7a979616846aa588ce0618c56662773eb8)
and [<b>LEAVE_VLAN</b>](../api_guide/group__ETHFW__ETHREMOTECFG.html#ggacfc53541f27433475f4bbdf233ce4ba7a57f5df65c855a42980f5ee253e9e93dd).

The remote client must be a member of the VLAN in order to be able to successfully
join the VLAN.  The virtual port membership is set through `virtMemberMask` parameter
in the [VLAN configuration](../api_guide/structEthFwVlan__VlanCfg.html) at
VLAN creation time on Ethernet Firmware server side.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Trace Support {#ethfw_tracing}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Ethernet Firmware supports two types of trace levels:

- **Build-time trace level** is set via `ETHFW_CFG_TRACE_LEVEL` and determines the
  traces that are built in Ethernet Firmware libraries, both for server
  (`ethfw_remotecfg_server`) and client (`ethfw_remotecfg_client`).

- **Runtime trace level** is set via `EthFwTrace_setLevel()` function and can be set
  to the build-time trace level or lower (less verbose levels).

Trace functionality must be initialized via `EthFwTrace_init()` before any other
Ethernet Firmware API, either on server or client sides.

Traces can be optionally timestamped if the trace format is `ETHFW_CFG_TRACE_FORMAT_DFLT_TS`,
`ETHFW_CFG_TRACE_FORMAT_FUNC_TS`, `ETHFW_CFG_TRACE_FORMAT_FILE_TS` or
`ETHFW_CFG_TRACE_FORMAT_FULL_TS`.  Application must pass a timestamp provider at init time
that returns timestamps in microseconds.

Ethernet Firmware also supports generation of unique error codes in its server library
(`ethfw_remotecfg_server`) which are 32-bit values composed of file id, line number and
status value.  Application must pass a callback function (`EthFwTrace_Cfg::extTraceFunc`)
in order to get unique error codes.  Note that a number of error codes will be reported
as the error cascades back through the call sequences.  This functionality can be used
for tracing purposes in error diagnostics.

The code snippet below shows trace feature initialization with FreeRTOS based timestamping
and a unique error code callback function.

```C
#define SEC_TO_USEC (1000000ULL)

static uint64_t EthApp_traceTs(void)
{
    static uint64_t ts = 0ULL;
    TickType_t tickCnt;
    uint64_t tickUsecs;

    tickUsecs = SEC_TO_USEC / uint64_t)configTICK_RATE_HZ;
    tickCnt = xTaskGetTickCount();
    ts = (uint64_t)tickCnt * tickUsecs;

    return ts;
}

void EthApp_extTraceFunc(uint32_t errCode)
{
  appLogPrintf("Error code: %x\n", errCode);
}


/* Trace configuration */
static EthFwTrace_Cfg gEthApp_traceCfg =
{
    .print        = appLogPrintf,
    .traceTsFunc  = EthApp_traceTs,
    .extTraceFunc = EthApp_extTraceFunc,
};

void EthApp_myTask(...)
{
    ...

    EthFwTrace_init(&gEthApp_traceCfg);

    ...
}
```

For more information, refer to the [EthFwTrace API guide](../api_guide/group__ETHFW__UTILS__TRACE.html).

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# EthFw Demos {#ethfw_c_ug_ethfw_demos}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

The EthFw demos showcase the integration and usage of the Ethernet Firmware
which provides a high-level interface for applications to configure and use the
integrated Ethernet switch peripheral (CPSW5G/CPSW9G).

The following sample applications are key to demonstrate the capabilities of the
CPSW9G/CPSW5G hardware as well as the EthFw stack.

Demo                               | Comments
-----------------------------------|--------------
L2 Switching | Configures CPSW5G/CPSW9G switch to enable switching between its external ports
L2/L3 address based classification | Illustrates traffic steering to A72 (Linux) and R5F (RTOS) based on Layer-2 Ethernet header. iperf tool and web servers are used to demonstrate traffic steering to/from PCs connected to the switch
Inter-VLAN Routing (SW) | Showcases inter-VLAN routing using lookup and forward operations being done in SW (R5F). It also showcases low-level lookup and forwarding on top of Enet LLD
Inter-VLAN Routing (HW) | Illustrates hardware offload support for inter-VLAN routing, demonstrating the CPSW5G/CPSW9G hardware capabilities to achieve line rate routing without additional impact on R5F CPU load

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## EthFw Switching & TCP/IP Apps Demo {#ethfw_switching_demo}

This demo showcases switching capabilities of the integrated Ethernet Switch
(CPSW9G or CPSW5G) found in J721E, J7200 and J784S4 devices for features like VLAN,
Multicast, etc.  It also demonstrates lwIP (TCP/IP stack) integration into
the EthFw.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
# Dependencies {#ethfw_instal_top}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Dependencies can be categorized as follows:

-# @ref ethfw_depend_hw
-# @ref ethfw_depend_sw

Please note that the dependencies vary depending on the intended use (e.g. for integration
vs running demo applications only).

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Hardware Dependencies {#ethfw_depend_hw}

EthFw is supported on the following EVMs and expansion boards listed below:

SoC     | EVM                           | Expansion boards
--------|-------------------------------|--------------------------------
J721E   | \ref ethfw_depend_evm_j721e   | \ref ethfw_depend_evm_gesi_j721e
^       | ^                             | \ref ethfw_depend_evm_quadport_j721e
J7200   | \ref ethfw_depend_evm_j721e   | \ref ethfw_depend_evm_quadport_j7200
J784S4  | \ref ethfw_depend_evm_j784s4  | \ref ethfw_depend_evm_quadport_j784s4


**Note:** Quad-Port Eth expansion board is supported in all EVMs, but with different
MAC port number assignments, hence they are listed in separate sections.


### J721E/J7200 EVM {#ethfw_depend_evm_j721e}

![](J7EVM_CPSW_TopView.png "J721E/J7200 EVM connections")


### J721E GESI Expansion Board {#ethfw_depend_evm_gesi_j721e}

![](GESI_Board.png "J721E EVM GESI Board Top View")

There are four RGMII PHYs in the J721E GESI board as shown in the following image.
They will be referred to as **MAC Port 1**, **MAC Port 3**, **MAC Port 4** and
**MAC Port 8** throughout this document.

![](GESI_RJ45_SideView.png "GESI Board connections")

Please refer to the SDK Description for details about installation and getting
started of J721E EVM.

**Note:** GESI expansion board is also available in J7200 EVM, but only one MAC
port is routed to the CPSW5G in J7200, hence GESI board is not enabled and used
by default in the Ethernet Firmware for J7200.

[Back To Top](@ref ethfw_c_ug_top)


### J721E Quad-Port Eth Expansion Board {#ethfw_depend_evm_quadport_j721e}

The Quad-Port Eth expansion board in J721E EVM provides four MAC ports in addition
to the four MAC ports in GESI board.

It enables four MAC ports: **MAC Port 2**, **MAC Port 5**, **MAC Port 6** and
**MAC Port 7**.

![](J721E_QPENet_Board.png "Quad Port Eth Board connections in J7200 EVM")

Please refer to the SDK for more details about installation and getting started on
J721E EVM.

[Back To Top](@ref ethfw_c_ug_top)


### J7200 Quad-Port Eth Expansion Board {#ethfw_depend_evm_quadport_j7200}

The Quad-Port Eth expansion board provides the connectivity to the four MAC ports
in J7200's CPSW5G: **MAC Port 1**, **MAC Port 2**, **MAC Port 3** and **MAC Port 4**.

![](J7200_QPENet_Board.png "Quad Port Eth Board connections in J7200 EVM")

Please refer to the SDK for more details about installation and getting started on
J7200 EVM.

[Back To Top](@ref ethfw_c_ug_top)


### J784S4 EVM {#ethfw_depend_evm_j784s4}

![](J784S4EVM_CPSW_TopView.png "J784S4 EVM connections")


### J784S4 Quad-Port Eth Expansion Board {#ethfw_depend_evm_quadport_j784s4}

Currently, Ethernet Firmware supports only one Quad-Port Eth expansion board
connected in expansion connectors labeled as `ENET-EXP-1`.

It enables four MAC ports: **MAC Port 1**, **MAC Port 3**, **MAC Port 4** and
**MAC Port 5**.

![](J784S4_QPENet_Board.png "Quad Port Eth Board connections in J784S4 EVM")

Please refer to the SDK for more details about installation and getting started on
J784S4 EVM.

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
integration of lwIP into J721E/J7200/J784S4 devices is done through Enet LLD, which
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


### TSN stack {#ethfw_depend_tsn}

\note SDK 9.0 provides support only for gPTP stack. No other TSN protocol is supported.

Starting in SDK 9.0, a new gPTP stack is integrated on top of Enet LLD in PDK, it can be
located at: `<pdk>/packages/ti/transport/tsn/tsn-stack`.
The previous gPTP test stack used in SDK 8.x and older releases is no longer supported
and has been fully removed from both, Enet LLD and Ethernet Firmware.

The new gPTP stack provides time synchronization for CPSW5G/CPSW9G on Main R5F0 core 0
for J721E, J7200 and J784S4. The stack is composed of the following modules:

  - **tsn_unibase** : Universal utility libraries that are platform-independent.
  - **tsn_combase** : Communication utility libraries that provide support for functions
    like sockets, mutexes, and semaphores.
  - **tsn_gptp**: Implementation of the IEEE 802.1 AS gptp protocol.

This stack can be used for production and testing purposes.  For more information about
the stack, please refer to PDK documentation:

  - API Guide is located under *Time Sensitive Networking (TSN) Stack* section of
    PDK API Guide.
  - User's Guide is located under *TSN Integration* section of the ENET module in
    PDK User's Guide.

The utilisation of these resources by gPTP stack on Ethernet Firmware is as follows:

| Resource    | Count  | gPTP Usage (mcu2_0)
|:------------|:------:|:-----------------------------------
| TX channel  |   1    | To transmit PTP packets
| RX flow     |   1    | To receive PTP packets (filtered by PTP multicast and EtherType)
| MAC address |   1    | Shared with TCP/IP lwIP netif

\note The gPTP stack is supported only in FreeRTOS.  It's not supported in SafeRTOS.


### Ethernet Firmware Proxy ARP {#ethfw_depend_lwip_proxyarp}

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


### SafeRTOS {#ethfw_depend_safertos}

Ethernet Firmware requires the following SafeRTOS kernel versions, depending on the
SoC being used.

  SoC  | ISA | SafeRTOS package version
-------|-----|--------------------------
J721E  | R5F | 009-004-199-024-219-001
J7200  | R5F | 009-002-199-024-243-001
J784S4 | R5F | 009-004-199-024-251-001

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## IDE (CCS) {#ethfw_instal_ccs}

Install Code Composer Studio and setup a <b>Target Configuration</b> for use with
J721E, J7200 or J784S4 EVM.  Refer to the instructions in *CCS Setup* section of
the Processor SDK RTOS documentation.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Installation Steps {#ethfw_instal_steps}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Ethernet Firmware and its dependencies are part of the SDK, separate installation is not required.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Directory Structure {#ethfw_dir}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Post installation of SDK, the following directory would be created. Please
note that this is an indicative snap-shot, modules could be added/modified.

The top-level EthFw makefile as well as the auxiliary makefiles for build flags
(**ethfw_build_flags.mak**) and build paths (**ethfw_tools_path.mak**)
can be found at the EthFw top-level directory.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Post Install Directory Structure {#ethfw_post_install_j721e}

![](c_ug_dir_top.png "Top Level Directory Structure")

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Utilities Directory Structure {#ethfw_dir_utils}

The **utils** directory contains miscellaneous utilities required by the EthFw
applications.

![](c_ug_dir_utils.png "Utilities Directory Structure")

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

The tool paths required by the build system are defined in `<ethfw>/ethfw_tools_path.mak`.
When building ETHFW component standalone, user must provide the location of the
compiler through `PSDK_TOOLS_PATH` variable (by default it's set to `../ethfw`):

    make ethfw_all BUILD_SOC_LIST=<SOC> PSDK_TOOLS_PATH=$HOME/ti


User can run the following command to get the full list of valid targets:

    make help

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Build {#ethfw_build}

The make commands listed below require the environment setup according to
@ref ethfw_build_setup_env section.


### Build All {#ethfw_build_all}

Build EthFw components as well as its dependencies, including PDK, lwIP, etc.

For J721E:

    make ethfw_all BUILD_SOC_LIST=J721E

For J7200:

    make ethfw_all BUILD_SOC_LIST=J7200

For J784S4:

    make ethfw_all BUILD_SOC_LIST=J784S4


By default, above commands will build Ethernet Firmware for FreeRTOS.

Verbose build can be enabled by setting the **SHOW_COMMANDS** variable as
shown below:

    make ethfw_all BUILD_SOC_LIST=<SOC> SHOW_COMMANDS=1

On successful compilation, the output folder would be created at
`<ethfw>/out`.


### SafeRTOS Build {#ethfw_safertos_build_all}

The RTOS used in Ethernet Firmware build is determined by the following flags, which
can be set in `ethfw_build_flags.mk` or passed to the make command:

- `BUILD_APP_FREERTOS` enables FreeRTOS build of EthFw and RTOS client.
- `BUILD_APP_SAFERTOS` enables SafeRTOS build of EthFw and RTOS client. It requires
  SafeRTOS kernel installed in SDK installation path.

The location of the SafeRTOS package can be changed through the `SAFERTOS_KERNEL_INSTALL_r5f_<SOC>`
variable in `ethfw_tools_path.mak`.  The SafeRTOS version validated for each SoC
can also be found in `ethfw_tools_path.mak`.

Build for SafeRTOS only, FreeRTOS build disabled:

    make ethfw_all BUILD_SOC_LIST=<SOC> BUILD_APP_FREERTOS=no BUILD_APP_SAFERTOS=yes

Build for SafeRTOS and FreeRTOS:

    make ethfw_all BUILD_SOC_LIST=<SOC> BUILD_APP_FREERTOS=yes BUILD_APP_SAFERTOS=yes


### QNX Build {#ethfw_qnx_build_all}

Ethernet Firmware for QNX OS client integration on A72 is built with the standard
make command:

    make ethfw_all BUILD_SOC_LIST=<SOC>

It's worth noting that above command also builds EthFw binaries for integration
with Linux and CCS, as well as RTOS client.

Alternatively, user may choose to build Ethernet Firmware server for QNX only
using below command:

    make ethfw_server_qnx BUILD_SOC_LIST=<SOC>

There are two main differences between QNX and Linux builds of Ethernet Firmware:

- For QNX integration, EthFW would not load the IPC resource table, unlike in Linux.
- \ref ethfw_intercore_eth is disabled, as QNX virtual client currently doesn't support
  this feature.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Clean {#ethfw_build_clean}

The make commands listed below require the environment setup according to
@ref ethfw_build_setup_env section.


### Clean All {#ethfw_build_clean_all}

Clean EthFw components as well as its dependencies:

    make ethfw_all_clean BUILD_SOC_LIST=<SOC>


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
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Please refer to the Ethernet Firmware Release Notes.

[Back To Top](@ref ethfw_known_issues)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Compiler Flags used {#ethfw_cflag}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Demo Application - Profile: Debug {#ethfw_cflag_debug}

Flag                             | Description
---------------------------------|------------
`-O0`                            | Optimization level 0
`-D=MAKEFILE_BUILD`              | Makefile-based build type
`-D=TARGET_BUILD=2`              | Identifies the build profile as 'debug'
`-D_DEBUG_=1`                    | Identifies as debug build
`-D=ETHFW_CCS`                   | Identifies ETHFW build for CCS boot, disabled for U-Boot/SBL build
`-D=SOC_J721E`                   | Identifies the J721E SoC type
`-D=J721E`                       | Identifies the J721E device type
`-D=SOC_J7200`                   | Identifies the J7200 SoC type
`-D=J7200`                       | Identifies the J7200 device type
`-D=SOC_J784S4`                  | Identifies the J784S4 SoC type
`-D=J784s4`                      | Identifies the J784S4 device type
`-D=R5Ft="R5Ft"`                 | Identifies the core type as ARM R5F with Thumb2 enabled
`-D=TARGET_NUM_CORES=2`          | Identifies the core id as mcu2_0 (ETHFW server)
`-D=TARGET_NUM_CORES=3`          | Identifies the core id as mcu2_1 (RTOS client)
`-D=TARGET_ARCH=32`              | Identifies the target architecture as 32-bit
`-D=ARCH_32`                     | Identifies the architecture as 32-bit
`-D=FREERTOS`                    | Identifies as FreeRTOS operating system build
`-D=SAFERTOS`                    | Identifies as SafeRTOS operating system build
`-D=ETHFW_PROXY_ARP_SUPPORT`     | Enable Proxy ARP support on EthFw server
`-D=ETHFW_CPSW_VEPA_SUPPORT`     | Enable VEPA support on EthFw server (only applicable to J784S4)
`-D=ETHAPP_ENABLE_INTERCORE_ETH` | Enable Intercore Virtual Ethernet support (disabled in QNX images)
`-D=ETHAPP_ENABLE_IPERF_SERVER`  | Enable lwIP iperf server support (TCP only)
`-D=ENABLE_QSGMII_PORTS`         | Enable QSGMII ports in QpENet expansion board (applicable only to J721E)
`-D=ETHFW_BOOT_TIME_PROFILING`   | Enable special ETHFW configuration for boot time profiling (TI internal)
`-D=ETHFW_DEMO_SUPPORT`          | Enable ETHFW demos, such as hardware and software interVLAN, GUI configurator tool, etc.
`-D=ETHFW_MONITOR_SUPPORT`       | Enable ETHFW Monitor.

Other common flags:

```
-Wno-extra -Wno-exceptions -ferror-limit=100 -Wno-parentheses-equality -Wno-unused-command-line-argument -Wno-gnu-variable-sized-type-not-at-end -Wno-unused-function -Wno-inconsistent-missing-override -Wno-address-of-packed-member -Wno-self-assign -Wno-ignored-attributes -Wno-bitfield-constant-conversion -Wno-unused-const-variable -Wno-unused-variable -Wno-format-security -Wno-excess-initializers -Wno-sometimes-uninitialized -Wno-empty-body -Wno-extern-initializer -Wno-absolute-value -Wno-missing-braces -Wno-ti-macros -Wno-pointer-sign -Wno-macro-redefined -Wno-main-return-type -Werror -O0 -ggdb3 -mfloat-abi=hard -mfpu=vfpv3 -D16 -mcpu=cortex-r5 -march=armv7-r -mthumb -fno-strict-aliasing  -ffunction-sections
```

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Demo Application - Profile: Release {#ethfw_cflag_release}

Flag                             | Description
---------------------------------|------------
`-O3`                            | Optimization level 3
`-D=MAKEFILE_BUILD`              | Makefile-based build type
`-D=TARGET_BUILD=1`              | Identifies the build profile as 'release'
`-D=ETHFW_CCS`                   | Identifies ETHFW build for CCS boot, disabled for U-Boot/SBL build
`-D=SOC_J721E`                   | Identifies the J721E SoC type
`-D=J721E`                       | Identifies the J721E device type
`-D=SOC_J7200`                   | Identifies the J7200 SoC type
`-D=J7200`                       | Identifies the J7200 device type
`-D=SOC_J784S4`                  | Identifies the J784S4 SoC type
`-D=J784S4`                      | Identifies the J784S4 device type
`-D=R5Ft="R5Ft"`                 | Identifies the core type as ARM R5F with Thumb2 enabled
`-D=TARGET_NUM_CORES=2`          | Identifies the core id as mcu2_0 (ETHFW server)
`-D=TARGET_NUM_CORES=3`          | Identifies the core id as mcu2_1 (RTOS client)
`-D=TARGET_ARCH=32`              | Identifies the target architecture as 32-bit
`-D=ARCH_32`                     | Identifies the architecture as 32-bit
`-D=FREERTOS`                    | Identifies as FreeRTOS operating system build
`-D=SAFERTOS`                    | Identifies as SafeRTOS operating system build
`-D=ETHFW_PROXY_ARP_SUPPORT`     | Enable Proxy ARP support on EthFw server
`-D=ETHFW_CPSW_VEPA_SUPPORT`     | Enable VEPA support on EthFw server (only applicable to J784S4)
`-D=ETHAPP_ENABLE_INTERCORE_ETH` | Enable Intercore Virtual Ethernet support (disabled in QNX images)
`-D=ETHAPP_ENABLE_IPERF_SERVER`  | Enable lwIP iperf server support (TCP only)
`-D=ENABLE_QSGMII_PORTS`         | Enable QSGMII ports in QpENet expansion board (applicable only to J721E)
`-D=ETHFW_BOOT_TIME_PROFILING`   | Enable special ETHFW configuration for boot time profiling (TI internal)
`-D=ETHFW_DEMO_SUPPORT`          | Enable ETHFW demos, such as hardware and software interVLAN, GUI configurator tool, etc.
`-D=ETHFW_MONITOR_SUPPORT`       | Enable ETHFW Monitor.

Other common flags:

```
-Wno-extra -Wno-exceptions -ferror-limit=100 -Wno-parentheses-equality -Wno-unused-command-line-argument -Wno-gnu-variable-sized-type-not-at-end -Wno-unused-function -Wno-inconsistent-missing-override -Wno-address-of-packed-member -Wno-self-assign -Wno-ignored-attributes -Wno-bitfield-constant-conversion -Wno-unused-const-variable -Wno-unused-variable -Wno-format-security -Wno-excess-initializers -Wno-sometimes-uninitialized -Wno-empty-body -Wno-extern-initializer -Wno-absolute-value -Wno-missing-braces -Wno-ti-macros -Wno-pointer-sign -Wno-macro-redefined -Wno-main-return-type -Werror -O3 -mfloat-abi=hard -mfpu=vfpv3 -D16 -mcpu=cortex-r5 -march=armv7-r -mthumb -fno-strict-aliasing  -ffunction-sections
```

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Supported Device Families {#ethfw_supported_family}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Device Family | Variant              | Known by other names
--------------|----------------------|--------------------
Jacinto 7     | J721E                | -
^             | J7200                | -
^             | J784S4               | -

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
1.3      | 01 Dec 2021   | Nitin Sakhuja          | Adedd Inter-core Ethernet support for SDK 8.1
1.4      | 07 Dec 2021   | Misael Lopez           | Adedd MAC-only, server and client doc
1.5      | 01 Jul 2021   | Misael Lopez           | Updates for J784S4 support and SDK 8.02.01
1.6      | 10 Feb 2023   | Misael Lopez           | Added SafeRTOS build info
1.7      | 29 Nov 2023   | Misael Lopez           | SDK 9.1 and VLAN, trace support

[Back To Top](@ref ethfw_c_ug_top)
(@ref ethfw_c_ug_top)
