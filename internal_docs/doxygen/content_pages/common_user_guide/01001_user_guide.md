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
Reset Recovery on CPSW  | Support to reset CPSW and recover it back to a working state from HW lockups.
QoS             | Support for assignment of multiple TX channels/RX flows for clients
Multicore Timesync             | Support for multicore Timesync demonstrated between server and RTOS client.

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
EtherType classifier as match criteria to have PTP traffic routed to dedicated UDMA RX flow.

The remote configuration infrastructure provided by Ethernet Firmware is built using
the *ethremotecfg* framework which uses ETHFW-IPC (EthFw abstraction layer to support
IPC LLD on both Jacinto and Sitara based SoCs, please refer utils/ethfw_abstract for more details).
Ethernet Firmware supports three types of messages namely, 
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

## Resource Utilization {#ethfw_resource_utilization}

The utilization of these resources by Ethernet Firmware on either the Main R5F 0 Core 0 or the Main R5F 0 Core 1 is as follows:

| Resource    | Count  | EthFw Usage (mcu2_0 OR mcu2_1)
|:------------|:------:|:-----------------------------------
| TX channel  |   2    | <ul><li>lwIP netif (1)</li><li>gPTP (1)
| RX flow     |   5    | <ul><li>lwIP netif (1)</li><li>gPTP (1)</li><li>Proxy ARP (1) or VEPA (1) (only for J784S4 and J742S2)</li><li>SW interVLAN (1)</li><li>Reserved flow (1)</li></ul>
| MAC address |   1    | <ul><li>lwIP netif (1)</li></ul>

**Note: Before running Ethernet Firmware on the Main R5F 0 Core 1, make sure that you have allocated the appropriate amount of resources to the core.**

UDMA TX channels are a resource especially limited as there is only a total of 8 TX channels
available.  So, there are 6 TX channels to be shared among the differrent remote client cores
and their virtual ports.

With Ethernet Firmware's default port configuration, the following resources will be used by
Linux remote client on A72 core.

| Resource    | Count  | Linux Client Usage
|:------------|:------:|:-----------------------------------
| TX channel  |   3    | <ul><li>Virtual switch port (2)</li><li>Virtual MAC port (1)</li></ul>
| RX flow     |   2    | <ul><li>Virtual switch port (1)</li><li>Virtual MAC port (1)</li></ul>
| MAC address |   2    | <ul><li>Virtual switch port (1)</li><li>Virtual MAC port (1)</li></ul>

With Ethernet Firmware's default port configuration, the following resources will be used by
RTOS remote client on Main R5F 0 Core 1. The same remote client will run on Main R5f 1 Core 0 if Ethernet Firmware is running on Main R5F 0 Core 1.

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
library which is built using ETHFW-IPC APIs can be located in */ethfw/ethremotecfg/client/*
folder.

The multicore time synchronization mechanism implemented in RTOS client consists
of a linear correction in software of a local timer owned by the RTOS core which
is periodically synchronized with the CPTS clock via HW push event 2.


### Building RTOS client for Main R5F 1 Core 0 {#ethfw_client_rtos_mcu30}

In SoCs with multiple R5F cores, system design may require virtual network support on a 
different R5F core, instead of Main R5F 0 Core 1.
Starting from SDK 10.0, Ethernet Firmware provides RTOS client support on Main R5F 1 core 0 
(*mcu3_0*) in all supported SoCs (i.e. J721E, J784S4 and J742S2).
This is exactly the same FreeRTOS client example application which runs on Main R5F 0 core 1.

RTOS client by default is enabled for Main R5F 0 core 1. `ETHFW_RTOS_MCU3_0_SUPPORT` is the
build flag defined in `<ethfw>/ethfw_build_flags.mak` to enable RTOS client on Main R5F 1 core 0.
Use the following commands to build EthFw with RTOS client on Main R5F 0 core 1:
```C
make -s -j ethfw_all BUILD_SOC_LIST=<J721E/J784S4/J742S2> PROFILE=<release/debug> ETHFW_RTOS_MCU3_0_SUPPORT=yes
```

It's worth noting that RTOS client is sharing the same resources (same virtual switch/mac ports and tx channels) 
among both the cores (*mcu2_1* and *mcu3_0*). Hence RTOS client can be enabled only on 1 core at a time.

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
\ref ethfw_j7200_port_cfg, \ref ethfw_j784s4_port_cfg and \ref ethfw_j742s2_port_cfg subsections, respectively.

The port's default VLAN for MAC ports configured in MAC-only mode is `0`, and for MAC ports
configured in switch mode is `3`. They can be changed via `EthFw_Config::dfltVlanIdMacOnlyPorts`
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
hardware MAC port, as these virtual ports can receive traffic from any MAC port as long as
the packets match the unicast MAC address classification criteria.

SDK 8.0 or older supported only this type of virtual port.

Virtual port (*virtual switch* or *virtual MAC*) are allocated to a specific core.

For example, below code snippet shows the virtual switch configuration:
- For remote_device based clients in `gEthApp_virtPortCfg` where *virtual switch port 0* is allocated
    for A72 core (used by Linux or QNX client), and *virtual switch port 1* is allocated for Main R5F 0 Core 1
    (used by RTOS or AUTOSAR clients). Also *virtual switch port 2* is allocated for MCU R5F 0 Core 0 for
    AUTOSAR client as well. 

It's worth noting that in this specific configuration *virtual switch port 1* can be used by an RTOS
client or AUTOSAR client, depending on the OS running on Main R5F 0 Core 1. Same with *virtual switch port 0*
as well.

```C
static EthFw_VirtPortCfg gEthApp_virtPortCfg[] =
{
    {
        /* SWITCH_PORT_0 is used for both Linux and QNX client */
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_0,
        ...
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_LINUX) | ETHFW_BIT(ETHREMOTECFG_CLIENTID_QNX),
    },
    {
        .remoteCoreId = IPC_MCU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_2,
        ...
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_AUTOSAR),
    },
    {
        /* SWITCH_PORT_1 is used for both RTOS and Autosar client */
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_SWITCH_PORT_1,
        ...
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_AUTOSAR) | ETHFW_BIT(ETHREMOTECFG_CLIENTID_RTOS),
    },
    {
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_MAC_PORT_1,
        ...
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_LINUX) | ETHFW_BIT(ETHREMOTECFG_CLIENTID_QNX),
    },
    {
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_MAC_PORT_4,
        ...
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_RTOS),
    },
};

static int32_t EthApp_initEthFw(void)
{
    EthFw_Config ethFwCfg;

    ...

    /* Set virtual port configuration parameters */
    ethFwCfg.virtPortCfg  = &gEthApp_virtPortCfg[0];
    ethFwCfg.numVirtPorts = ARRAY_SIZE(gEthApp_virtPortCfg);
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
        /* SWITCH_PORT_0 is used for both Linux and QNX client */
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_0,
        ...
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_LINUX) | ETHFW_BIT(ETHREMOTECFG_CLIENTID_QNX),
    },
    {
        .remoteCoreId = IPC_MCU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_2,
        ...
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_AUTOSAR),
    },
    {
        /* SWITCH_PORT_1 is used for both RTOS and Autosar client */
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_SWITCH_PORT_1,
        ...
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_AUTOSAR) | ETHFW_BIT(ETHREMOTECFG_CLIENTID_RTOS),
    },
    {
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_MAC_PORT_1,
        ...
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_LINUX) | ETHFW_BIT(ETHREMOTECFG_CLIENTID_QNX),
    },
    {
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_MAC_PORT_4,
        ...
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_RTOS),
    },
};

static int32_t EthApp_initEthFw(void)
{
    EthFw_Config ethFwCfg;

    ...

    /* Set virtual port configuration parameters */
    ethFwCfg.virtPortCfg  = &gEthApp_virtPortCfg[0];
    ethFwCfg.numVirtPorts = ARRAY_SIZE(gEthApp_virtPortCfg);
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
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_RTOS),
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
## J742S2 Port Configuration {#ethfw_j742s2_port_cfg}

J742S2 EVM E1 version does not provide any Enet board/connector.
To test EthFw functionality (\ref ethfw_j742s2_testing) four MAC ports of CPSW9G are 
enabled by default in Ethernet Firmware for J742S2 SoC.

Two MAC ports are configured in MAC-only mode and allocated for A72 (Linux) and Main R5F
Core 1 (RTOS) usage. The remaining two MAC ports are configured in switch mode.

![](EthFw_PortCfg_j784s4_evm.png "J742S2 default port configuration")

The following table shows the full list of MAC ports in J742S2 EVM, the board they are
located and their MAC mode.

| MAC Port    | PHY Addr | Board  | MAC mode
|:------------|:--------:|:------:|:------------
| MAC Port 1  |   16     | QSGMII | MAC-only
| MAC Port 3  |   17     | QSGMII | Switch Port
| MAC Port 4  |   18     | QSGMII | MAC-only
| MAC Port 5  |   19     | QSGMII | Switch Port

MAC ports 2, 6, 7 and 8 are not enabled. Note that J742S2 CPSW supports up to 4 ports.
To maximize pin muxing flexibility, the system designer can choose based on any 
available ports, but must limit the total number of ports used to 4 or less.

[Back To Top](@ref ethfw_c_ug_top)


### ETHFW J742S2 Testing {#ethfw_j742s2_testing}

As J742S2 EVM E1 version does not provide any Enet board/connector, <b>NO</b> functionality 
test has been done on J742S2-EVM as there is no external port to transfer or receive data.

EthFw (built for J742S2) for CPSW_9G is validated on J784S4-EVM post applying this 
[patch](pdk_j742s2_serdes_2.patch) in PDK to configure SERDES_2.

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
 
Please refer to the following code in `<ethfw>/apps/app_remoteswitchcfg_server/main.c`
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

**Note:** Checksum offload is enabled by default for all Jacinto devices in both Tx & Rx. In case of Inter-core 
virtual path (via lwIP bridge) checksum is validated and computed in software, as it will not involve any hardware 
port for transmission.

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

Please refer to the following code in `<ethfw>/apps/app_remoteswitchcfg_client/main.c`
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

VEPA is supported on J784S4 and J742S2 only, starting from SDK 9.1. EthFw provides support to enable VEPA
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

Please refer to the following code in `<ethfw>/apps/app_remoteswitchcfg_server/main.c`
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


**Note:** 
-# No netif instance creation or TAP application is required on RTOS and Linux client respectively when VEPA is enabled on EthFw.

-# When VEPA is enabled, checksum offload is disabled due to hardware Errata i2444 
(Multihost Checksum Issue). Checksum is validated and computed in software.

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

Please refer to the following code in `<ethfw>/apps/app_remoteswitchcfg_server/main.c`:

```C
/* Must not exceed ETHAPP_MAX_SHARED_MCAST_ADDR entries */
static EthFwMcast_McastCfg gEthApp_sharedMcastCfgTable[] =
{
    {
        /* MCast IP ADDR: 224.0.0.1 */
        .macAddr      = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x01},
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        /* MCast IP ADDR: 224.0.0.251 */
        .macAddr      = {0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB},
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        /* MCast IP ADDR: 224.0.0.252 */
        .macAddr      = {0x01, 0x00, 0x5E, 0x00, 0x00, 0xFC},
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        .macAddr      = {0x33, 0x33, 0x00, 0x00, 0x00, 0x01},
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        .macAddr      = {0x33, 0x33, 0xFF, 0x1D, 0x92, 0xC2},
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        .macAddr      = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00},
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        .macAddr      = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x03},
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
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

VLANs are created and configured in a static or dynamic manner. For static VLANs,
they are exclusively set by to Ethernet Firmware. Remote clients cannot 
create again these static VLANs, they can only *join* or *leave* the statically 
configured VLANs.

Parameters such as VLAN id, member lists (physical and virtual ports), registered
and unregistered multicast flood mask and untag mask are required in order to set up
static VLANs on the Ethernet Firmware server side.

The code snippet below shows the static configuration of VLAN 1024, with MAC ports
2 and 3 as members of the VLAN, and virtual switch ports 0, 1 and 2 as virtual members.

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

For dynamic VLAN support, remote clients can call directly 
<b>JOIN_VLAN</b> and <b>LEAVE_VLAN</b> commands without any static configuraton
done in Ethernet Firmware. - For dynamic VLANs, portmask is set to default
all switch ports that are enabled, this is done to be compliant with
a typical linux interface. This also ensure no ABI changes for ethremotecfg.
For disabling forwarding to all switch ports set *dVlanSwtFwdEn* flag to false.

Clients are requested to still allocate static entries
when they want to maintain strict behaviour in forwarding rules.

The updated implementation of VLANs handling is done in
consideration to save more ALE entries and classifiers.

Presently the default VLAN id for MAC only ports is 0 and for switch ports
is 3. For host port the default VLAN id is 1. These values are configurable by user
and can be modified by updating the following macro definitions in the 
*/ethfw/ethremotecfg/server/src/ethfw_api.c* file:

```C
/*! VLAN id used for host port */
#define ETHFW_HOST_PORT_VLAN_ID                       (1U)

/*! VLAN id used for all MAC ports in MAC-only mode */
#define ETHFW_MAC_ONLY_PORTS_VLAN_ID                  (0U)

/*! VLAN id used for all MAC ports in switch mode (non MAC-only mode) */
#define ETHFW_SWITCH_PORTS_VLAN_ID                    (3U)
```

### The below section explains packet handling for each packet type:

#### Unicast handling: 

##### Non-VEPA or VEPA case:
No BV* to be added in ALE entry, where B - broadcast address and V* is VLAN 
requested to be joined. Only U1 classifier to be added, no U1V* based classifier,
where U1 - Unicast address of client, V* VLAN requested to be joined.
Only V* entry in ALE, where V* is VLAN requested to be joined.

This will allow U1V^ go to all clients, where V^ is registered
VLAN by other client, unknown VLAN will be dropped. To tightening
this behaviour got more control of V^, recommended to use classifier.

#### Broadcast handling:

##### Non-VEPA case:
No BV* entry in ALE, where B - broadcast
address and V* is VLAN requested to be joined by virtual clients.
No Classifier for broadcast entry.

##### VEPA case:
No BV* entry in ALE, where B - broadcast
address and V* is VLAN requested to be joined by virtual clients.
Classifier only based on B, not VLAN based.

#### Shared Multicast handling:

##### Non-VEPA case:
Multicast packets will be routed to EthFw default flow without honouring VLANs
as lwip bridge cannot differentiate between tagged and untagged packets.
Packets will  then be re-directed to all clients by the lwip bridge.

##### VEPA case:
Multicast packets will be routed to VEPA flow where software (VEPA Table) will
take care of packet forwarding based on VLANs and only the required recipient
will get VLAN tagged packets.

Post VLAN join, clients are required to update the VEPA table
with MV* entry by calling <b>ETHREMOTECFG_CMD_ADD_FILTER_MAC</b>
commands in order to recieve this traffic.

Post VLAN leave, client are required to delete MV* from VEPA table
by calling <b>ETHREMOTECFG_CMD_DEL_FILTER_MAC</b> command to delete 
VLAN MV entry from VEPA table and ALE entry for M.

#### Exclusive Multicast handling:

##### Non-VEPA or VEPA case:
We are stopping multiple clients to join same exclusive multicast
address M irrespective of VLAN.
In case when the same client registers M in a different VLAN,
we are allowing the same client to register M with different VLANs.
It is worth noting that M can be registered with different VLAN V1 and V2
by same client C1 only, if another client C2 tries to register M with V3
(totally different VLAN) then that request will result an error.
But this will allow MV^ go same client C1, where V^ is registered
VLAN by any client (need not same), unknown VLAN will be dropped.


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

**Note:** When multiple remote clients join different VLAN and we send a unicast packet 
of client_1's mac address say M1 with another client_2's VLAN say V2, then still packet will
be forwarded to client_1. This has been taken as a known limitation so
that we use less number of ALE and Classifier entires out of box.
Note that not all VLAN tagged packets will be received by remote clients.
Unicast packets with a VLAN registered by any client will be received by remote clients.
Unknown VLAN tagged packets will still not be received by remote clients.
Customer can add their own logic on top of current implementation for tighter packet forwarding.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# CPSW Recovery {#ethfw_cpsw_recovery}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Starting with SDK 9.1, Ethernet Firmware supports a mechanism to detect hardware lockups,
reset CPSW and recover it back to a functioning state.  A monitor task periodically monitors
the status of CPSW to detect for any hardware lockups.  In the current implementation,
the recovery process is triggered upon the detection of RX bottom of FIFO drops on any
MAC port, but this can be extended to user specific cases and other types of lockups.

If the nature of the lockup condition is such that it cannot be recovered by any other means,
Ethernet Firmware has to resort to resetting CPSW on-the-fly, while the rest of the SoC remains
running.  CPSW will lose its context (register state and logic) during reset, so Ethernet
Firmware will save and restore the context.  More details about the recovery flow are presented
in the following section.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Recovery Sequence {#ethfw_recovery_sequence}

CPSW recovery process performed on Ethernet Firmware is shown below:

![](reset_recovery_sequence.png "CPSW Recovery Sequence")

-# <b>Monitoring phase</b>: EthFw periodically monitors in 100ms intervals for any hardware
   lockup in CPSW, if detected, it will trigger the recovery mechanism.

-# <b>Remote client notification and DMA tear-down phase</b>: EthFw will send ``ETHREMOTECFG_NOTIFY_HWERROR``
   notification to all clients, waits for clients to take action (perform DMA tear-down).
   Ethernet Firmware will remain in this state until it receives DMA tear-down confirmation
   from all its clients, which clients do by sending [<b>ETHREMOTECFG_CMD_TEARDOWN_COMPLETION</b>](../api_guide/group__ETHFW__ETHREMOTECFG.html#ggacfc53541f27433475f4bbdf233ce4ba7a0f9188c71fa66ec806e9f44ecaf40c3a).

-# <b>Local DMA tear-down phase</b>: EthFw will now proceed to close all MAC ports and tear-down
   its own DMA channels and flows.

-# <b>CPSW context save phase</b>: EthFw saves the context of CPSW by calling ``Enet_saveCtxt()``,
   which will save the state of CPSW submodules such as MDIO, ALE, CPTS, host port, etc.

-# <b>CPSW reset phase</b>: EthFw reset the CPSW peripheral by issuing a reset request via
   SCI client.

-# <b>CPSW context restore phase</b>: EthFw restores the context of CPSW by ``Enet_restoreCtxt()``.
   Which restores the context previously saved via ``Enet_saveCtxt()``.

-# <b>Local DMA reopen phase</b>: EthFw wil open back all its DMA channels and flows that had
   closed prior to CPSW reset.

-# <b>Remote client notification and DMA reopen phase</b>: EthFw will send ``ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE``
   notification to all clients.  Clients can proceed to reopen their channels and flows.

-# <b>Ethernet ports reenable phase</b>: EthFw enables back all ports and updates the CPTS time
   with the time taken during recovery. For details refer to \ref ethfw_recovery_cpts_sync.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Remote Client Requirements {#ethfw_recovery_client_reqs}

Remote clients play a key role in making CPSW recovery successful and themselves being able
to continue using Ethernet after recovery.  Clients are required to tear-down their DMA
resources (channels, flows) as this will prevent stale states in UDMA and interconnect
post CPSW reset.

All remote clients are required to implement the following steps in order to participate
in CPSW recovery:

-# Remote clients must register to receive the following notifications:

   - [<b>ETHREMOTECFG_NOTIFY_HWERROR</b>](../api_guide/group__ETHFW__ETHREMOTECFG.html#gga839ae5df609f6394d9b0b22065032eb9a754a33d75aa7a00475f99967e997a8e1)
     This notification is sent by Ethernet Firmware to inform the client that a hardware error
     has been found and the recovery process is about to start.  Client has to tear-down its
     channels and flows upon reception of this notification.

   - [<b>ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE</b>](../api_guide/group__ETHFW__ETHREMOTECFG.html#gga839ae5df609f6394d9b0b22065032eb9a91d51165b414bf607771b309a0958ca7) notifications.
     This notification is sent by Ethernet Firmware to inform the client that the recovery
     process is complete.  Client has to reopen its channels and flows upon reception of this
     notification.

-# Remote clients must send [<b>ETHREMOTECFG_CMD_TEARDOWN_COMPLETION</b>](../api_guide/group__ETHFW__ETHREMOTECFG.html#ggacfc53541f27433475f4bbdf233ce4ba7a0f9188c71fa66ec806e9f44ecaf40c3a) once their DMA channel
   and flow has been released.  Failure to do so will prevent Ethernet Firmware from continuing
   with the recovery process.

If CPSW recovery is enabled, it's mandatory that all clients implement the requirements
described above.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## CPTS time synchronization {#ethfw_recovery_cpts_sync}

During reset recovery, CPTS needs special handling as CPSW will not be aware of the time for
which it was down when reset was performed.  This will affect CPTS time and can lead to CPTS
sending out incorrect time out other nodes in the network.  To mitigate this, the EthFw does
below list of steps:

-# Get a CPTS time before reset (T0)
-# Get a OS time before reset (T1)
-# Get a OS time post reset (T2)
-# Calculate and set CPTS with updated time (T4). `T4 = T0 + (T2 - T1)*1000U (convert to nanoseconds))`

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Miscellaneous details {#ethfw_recovery_misc}

In current implementation of CPSW context save and restore, the MAC port context is not
saved as MAC ports are expected to be closed before ``Enet_saveCtxt()``, as shown in
step 4 above. With respect to PHY, the state of PHY being alive or linked is not saved.
Similar to MAC ports, PHYs are expected to be closed before ``Enet_saveCtxt()`` is called.
Both, MAC ports and PHYs, will be reconfigured when application explicitly re-enables the
required ports during last stage of CPSW recovery.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Quality of Service (QoS) {#ethfw_qos}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Starting with SDK 9.2, Ethernet Firmware supports QoS mechanism via assignment of multiple TX channels/RX flows for clients.
QoS helps in prioritizing one traffic over the other among multiple/same clients to external ports (TX channels).
This helps in offering dedicated bandwidth, lesser delay, controlled jitter and low latency on higher priority channels.
QoS helps in traffic segregation from external ports to multiple/same clients (RX flows) based on the custom policers (@ref ethfw_ale_classification)
added for each RX flow.

**Note:** Ethernet Firmware is providing information of the allocated resources (numTxChan and numRxFlows) to the clients in response of ATTACH 
command. Clients can then call ALLOC_TX/ALLOC_RX (as many times as numTxChan/numRxFlows allocated) with relative channel/flow number to allocate 
the resource. At the same time there is <b>NO</b> change in ATTACH_EXT command neither in request nor in response. 
ATTACH_EXT command still remains as an extended function of ATTACH command with doing exactly 1 ALLOC_RX, 1 ALLOC_TX and 1 ALLOC_MAC
for any client.

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## TX QoS {#ethfw_tx_qos}

QoS helps in prioritizing one traffic over the other among multiple/same clients to external ports (TX channels).
Ethernet Firmware is mapping specific priority TX channels to virtual clients removing the first come first serve TX channel assignment.
Configuration can set one or more TX channel with varying absolute TX channel priority to a virtual client.

![](Tx_QoS.png "Tx Quality of Service")

Below code snippet show how static allocation of TX channels is achieved for each individual virtual client. 

```C
static EthFw_VirtPortCfg gEthApp_virtPortCfg[] =
{
    {
        /* SWITCH_PORT_0 is used for both Linux and QNX client */
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_0,
        .numTxCh       = 2U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_4, 
                            [1] = ENET_RM_TX_CH_7
                         },
        ...
    },
    {
        .remoteCoreId = IPC_MCU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_2,
        .numTxCh       = 1U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_5,
                         },
        ...
    },
    {
        /* SWITCH_PORT_1 is used for both RTOS and Autosar client */
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_SWITCH_PORT_1,
        .numTxCh       = 1U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_1
                         },
        ...
    },
    {
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_MAC_PORT_1,
        .numTxCh       = 1U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_3
                         },
        ...
    },
    {
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_MAC_PORT_4,
        .numTxCh       = 1U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_2
                         },
        ...
    },
};
```
**Note:** Any 2 virtual ports should not have same TX channel allocated to them (should be ensured by the application).

-# Clients call ALLOC_TX with relative channel number to allocate specific priority TX channel.
For example: Refer how IPC_MPU1_0 client has 2 TX channels allocated to it with absolute priority 4 and 7 respectively.
Now to allocate second channel first, client will call ALLOC_TX with relative channel number as 1. Then client can call
ALLOC_TX with relative channel number as 0 to allocate the first TX channel.

-# **Priority of an untagged packets** is the **psilThreadId (DMA channel id)** at which the packet is put by the clients.
This inherently adds priority between 2 untagged packets.

-# At the same time Ethernet Firmware also honor **VLAN/DSCP packet priority i.e. priority between 2 tagged packets**.

[Back To Top](@ref demo_ethfw_combined_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## RX QoS {#ethfw_rx_qos}

Multiple classifiers (@ref ethfw_ale_classification) can match on a single packet. For example a classifier can be enabled
to match on priority while another classifier could match on IP address. The ALE will return to the
switch the highest classifier entries thread ID that matched with an enabled thread ID number.
This could be used to further host routing of the packet.

QoS helps in traffic segregation from external ports to multiple/same clients (RX flows) based on the custom policers
added for each RX flow.
What it means that when a packet is egressed from host port then custom policer will be matched to put the packet onto a
specific RX flow. In this way traffic segregation is achieved with the help of policers.

For example: Suppose we have 3 RX flow allocated to A72 core. We can have a policer to match that this specific IP (say: 138.24.190.64) should always redirect to A72 core's flow 2.
Refer to this @ref ethfw_rx_qos_linux section for more deails.

Ethernet Firmware provides infrastructure to add static custom policers by the clients on each of the allocated RX flow.
To create static custom policers on RX flows, clients need to give flow information (i.e. numCustomPolicers and customPolicersInArgs) for each 
allocated flow. Each flow can have multiple custom policers given in customPolicersInArgs.
These policers are created at the moment client allocates those specific RX flow using ALLOC_RX command.

![](Rx_QoS.png "Rx Quality of Service")

Below code snippet show how static allocation of RX flows with custom policers can be achieved for each individual virtual client.

```C
/* Custom policers which clients need to provide to add their own policers.
 * Make sure that size of this array is <= ETHFW_UTILS_NUM_CUSTOM_POLICERS */
static CpswAle_SetPolicerEntryInPartitionInArgs gEthApp_customPolicers[ETHFW_UTILS_NUM_CUSTOM_POLICERS] = 
{
    /* Policer to match dst IP address */
    [0] = {
            .policerMatch.policerMatchEnMask                  = CPSW_ALE_POLICER_MATCH_IPDST,
            /* Dummy IP address for policer */
            .policerMatch.dstIpInfo.ipv4Info.ipv4Addr         = { 138, 24, 190, 64 },
            .policerMatch.dstIpInfo.ipv4Info.numLSBIgnoreBits = 0U,
            .policerMatch.dstIpInfo.ipAddrType                = CPSW_ALE_IPADDR_CLASSIFIER_IPV4,
            .threadIdEn                                       = BTRUE,
            /* Flow id will be updated at the time of policer creation */
            .threadId                                         = 0U,
            .peakRateInBitsPerSec                             = 0U,
            .commitRateInBitsPerSec                           = 0U,
            .policerPartLevel                                 = CPSW_ALE_POLICER_PARTITION_LEVEL_2,
          },
    /* Policer to match dst IP address */
    [1] = {
            .policerMatch.policerMatchEnMask                  = CPSW_ALE_POLICER_MATCH_IPDST,
            /* Dummy IP address for policer */
            .policerMatch.dstIpInfo.ipv4Info.ipv4Addr         = { 148, 24, 190, 64 },
            .policerMatch.dstIpInfo.ipv4Info.numLSBIgnoreBits = 0U,
            .policerMatch.dstIpInfo.ipAddrType                = CPSW_ALE_IPADDR_CLASSIFIER_IPV4,
            .threadIdEn                                       = BTRUE,
            /* Flow id will be updated at the time of policer creation */
            .threadId                                         = 0U,
            .peakRateInBitsPerSec                             = 0U,
            .commitRateInBitsPerSec                           = 0U,
            .policerPartLevel                                 = CPSW_ALE_POLICER_PARTITION_LEVEL_2,
          },
};


static EthFw_VirtPortCfg gEthApp_virtPortCfg[] =
{
    {
        /* SWITCH_PORT_0 is used for both Linux and QNX client */
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_0,
        /* 3 RX flow allocated for virtual port */
        .numRxFlow     = 3U,
        .rxFlowsInfo = {  
                          /* To create custom policers on RX flows clients need to give flow information (i.e. numCustomPolicers and 
                           * customPolicersInArgs) for each allocated flow. */
                         [0] = {
                                    .numCustomPolicers    = 0U,
                                    .customPolicersInArgs = {}
                                },
                          /* 1 dst IP match based custom policer for this flow */
                         [1] = {
                                    .numCustomPolicers    = 1U,
                                    .customPolicersInArgs = {
                                                                [0] = &gEthApp_customPolicers[0U],
                                                            }
                                },
                          /* 1 dst IP match based custom policer for this flow */
                         [2] = {
                                    .numCustomPolicers    = 1U,
                                    .customPolicersInArgs = {
                                                                [0] = &gEthApp_customPolicers[1U],
                                                            }
                                }
                        },
        ...
    },
    {
        .remoteCoreId = IPC_MCU1_0,
        .portId       = ETHREMOTECFG_SWITCH_PORT_2,
        .numRxFlow     = 1U,
        ...
    },
    {
        /* SWITCH_PORT_1 is used for both RTOS and Autosar client */
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_SWITCH_PORT_1,
        .numRxFlow     = 1U,
        ...
    },
    {
        .remoteCoreId = IPC_MPU1_0,
        .portId       = ETHREMOTECFG_MAC_PORT_1,
        .numRxFlow     = 1U,
        ...
    },
    {
        .remoteCoreId = IPC_MCU2_1,
        .portId       = ETHREMOTECFG_MAC_PORT_4,
        .numRxFlow     = 1U,
        ...
    },
};
```

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### ALE Classification {#ethfw_ale_classification}

The ALE has a number of configurable policer engines. Each policer engine can be used for classification.
Each policer can be enabled to classify on one or more of any of the below packet fields
for classification. 

-# Port or Trunk Group Number
-# Priority extracted from VLAN, mapped from DSCP if enabled or Default Port Priority
-# Organization Network Unique identifier
-# Destination Address
-# Source Address
-# Outer VLANID
-# Inner VLANID
-# Ether Type
-# IP Source Address 
-# IP Destination Address

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
### Primary Flow and Extended Flow

Only difference between a primary flow and an extended flow is that Ethernet Firmware restricts existing commands 
(i.e. REGISTER_MAC, DEREGISTER_MAC, SET_RX_DEFAULTFLOW, DEL_RX_DEFAULTFLOW and ADD_FILTER_MAC)
to be called only on primary flow. 
Clients call ALLOC_RX with relative flow number to allocate the resource. To allocate a primary flow, relative flow number provided by the client
should be <b>0U</b>. Similarly to allocate any extended flow, relative flow number vary from 1U to numRxFlows - 1U
    
**Note:** Number of custom policers per RX flow should be <= ETHREMOTECFG_POLICER_PERFLOW

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

    tickUsecs = SEC_TO_USEC / (uint64_t)configTICK_RATE_HZ;
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
# Port Mirroring Support {#ethfw_port_mirroring}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Port mirroring is used to send a copy of network packets seen on one port 
to another port for debugging or network monitoring purposes. 
Network engineers or administrators can use port mirroring to analyze and debug data or diagnose errors on a network. 
It can help to keep a track on network performance and get alert when problems occur.

Port mirroring is disabled by default in EthFw. 
User will need to modify and rebuild the EthFw binaries if they need to enable port mirroring.
```C
static EthFwPortMirroring_Cfg gEthApp_portMirCfg = 
{
    .mirroringType = DISABLE_PORT_MIRRORING
};
```

CPSW ALE supports three mirroring modes: destination port, source port and table entry.
Refer to Section 12.2.2.4.6.1.14 for more details in TRM.


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Destination port mirroring {#ethfw_dst_port_mirroring}

Destination port mirroring allows packets from any ingress port or trunk which ends up switching to a
particular egress destination port or trunk to be mirrored to yet another egress destination port or trunk. For
example any traffic from any port that is switched to port `A` can be also mirrored to port `B`.

```C
/* Code to enable destination port mirroring on port MAC_PORT_1 (A) with Host port (B) as mirrored port */
static EthFwPortMirroring_Cfg gEthApp_portMirCfg = 
{
    .mirroringMode =  
    {
        .dstPortMirCfg = 
        {
            .dstPortNum = 0U,
            .toPortNum  = CPSW_ALE_MACPORT_TO_ALEPORT(ENET_MAC_PORT_1)
        },
    },
    .mirroringType = DST_PORT_MIRRORING
};
```

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Source port mirroring {#ethfw_src_port_mirroring}

Source port mirroring allows packets received on any enabled ingress source port or trunk to be switched to
the mirror egress port as well as the actual egress destination ports. For example traffic received on ingress port
`A` can be switched to egress port `B` as well as the intended egress destination port.

```C
/* Code to enable source port mirroring on port MAC_PORT_3 (A) with MAC_PORT_1 (B) as mirrored port */
static EthFwPortMirroring_Cfg gEthApp_portMirCfg = 
{
    .mirroringMode =  
    {
        .srcPortMirCfg =
        {
            .srcPortNumMask = 1 << CPSW_ALE_MACPORT_TO_ALEPORT(ENET_MAC_PORT_3),
            .toPortNum  = CPSW_ALE_MACPORT_TO_ALEPORT(ENET_MAC_PORT_1)
        },
        
    },
    .mirroringType = SRC_PORT_MIRRORING
};
```

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Table entry port mirroring {#ethfw_tbl_entry_port_mirroring}

Table entry mirroring allows for any ALE entry that
matches on ingress to be switched to the egress destination as well as the actual egress destination. For
example all traffic with destination MAC `M` can be mirrored to port `B`. 
That is any traffic with destination MAC `M` will be mirrored.

```C
/* Code to match any packet with destination MAC address aa:bb:cc:dd:ee:ff to be mirrored to MAC_PORT_1 */
static EthFwPortMirroring_Cfg gEthApp_portMirCfg = 
{
    .mirroringMode =  
    {
        .tblEntryPortMirCfg =
        {
            .matchParams =
            {
                .entryType = CPSW_ALE_TABLE_ENTRY_TYPE_ADDR,
                .dstMacAddrInfo =
                {
                    .addr =
                    {
                        .addr = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
                        .vlanId = 0U
                    },
                    .portNum = 0U
                }
            },
            .toPortNum  = CPSW_ALE_MACPORT_TO_ALEPORT(ENET_MAC_PORT_1)
        },
    },
    .mirroringType = TBL_ENTRY_PORT_MIRRORING
};
```

**Note:** The mirror port need not be a member of the VLAN ID it is mirroring, 
the ALE will forward traffic to the mirror port after ingress and egress filters are applied.

[Back To Top](@ref ethfw_c_ug_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Heartbeat Mechanism {#ethfw_c_ug_heartbeat_mechanism}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Heartbeat mechanism in EthFw is enabled for remote clients to query the operational state of EthFw. The status of EthFw can be queried by remote clients via [ETHREMOTECFG_CMD_GET_SERVER_STATUS](../api_guide/group__ETHFW__ETHREMOTECFG.html#ggacfc53541f27433475f4bbdf233ce4ba7a2e8efba75b7697ed13c35993cc76cdfa) cmd. EthFw can be in one of the following operational states (`ETHREMOTECFG_SERVERSTATUS_UNINIT` , `ETHREMOTECFG_SERVERSTATUS_READY`, `ETHREMOTECFG_SERVERSTATUS_RECOVERY` and `ETHREMOTECFG_SERVERSTATUS_BAD`). A state diagram showing EthFw's transition to different possible states is shown below.
![](heartbeat_mechanism_stateDiagram.png "Heartbeat Mechanism state diagram.")

Remote Clients can implement a mechanism to periodically monitor the status of EthFw. The `ETHREMOTECFG_CMD_GET_SERVER_STATUS` cmd will return the current state of EthFw [<b>EthRemoteCfg_ServerStatus</b>](../api_guide/group__ETHFW__ETHREMOTECFG.html#gae795a400c19d20a478080a085be49540). Reference of heartbeat mechanism on client core can be taken from @ref ethfw_intercore_r5client.

If EthFw is not responsive and fails to respond to client's commands, a timeout mechanism is implemented on CPSW proxy client which will end the wait loop for response and return `ETHREMOTECFG_SERVERSTATUS_BAD` status to client application via callback function when the configured timeout period for cmd expires. 

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Multicore Timesync {#ethfw_c_ug_mts}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Multocore Timesync is enabled starting SDK 11.0, it demonstrates synchronising a system clock (GTC/GP Timer) with CPTS Clock. The feature is demonstrated between EthFw server and RTOS client. The block diagram of the feature is shown below:

![](MTS_block_diagram.png "Multicore Timesync block diagram.")

The Steps involved in Multicore Timesync:
-# Initialization of Timesync Coupler Client by Remote client with intented Timer Type(GTC/GP Timer)
-# Allocation of CPTS HW PUSH instance by Remote client
-# Registering the Timesync Router for allocated HW Push instance.
-# Timer Handling by Remote client 
-# EthFw sending an IPC notification of CPTS time to Remote client.
-# Software Tuple maintenance by remote client and Rate and Offset calculation to get Synchronized Time.


The Remote Client initialises the Timesync Coupler Client module with the choice of Timer intended to be use. A GTC or a GP Timer are the options available.

The Remote Client that needs to allocated a CPTS HW push instance via ETHREMOTECFG_CMD_ALLOC_CPTS_HW_PUSH cmd. The EthFw server maintains the static allocation for CPSW PUSH instances distributed across different cores. On successful allocation, THE CPTS instance is used by Remote client to query CPTS Time.

The Remote Client based on the intended Timer type and allocated CPTS HW Push instance needs to configure the timesync router, this is done via ETHREMOTECFG_CMD_REGISTER_REMOTE_TIMER cmd.

Baased on the selected Timer type. the Remote client will set the GTC push event value for GTC Timer Type. For GP Timer type, the intented Timer handle is created and PWM is triggered.

On trigger of PWM, the EthFw server receives a CPTS notification of the PWM generated time, which is passed to intented Remote clients via IPC messages.

On reception of CPTS Time, Remote client maintains a Software Tuple of Remote Time and System Time and maintains the relationship to calculate the rate and offset needed to get the synchronized time.

There is a demo test showcasing the accuracy on synchronization between local calculated synchronized time vs Remote time. Enable the macro ETHFW_MTS_DEMO_TEST to run the demo test. The log reference of demo test on RTOS client is given below:


```C
=================================================================
 DMSC Firmware Version 10.1.6--v10.01.06 (Fiery Fox)
 Firmware revision 0xa
 ABI revision 4.0
=================================================================
Sciclient_ccs_init Passed.
SCISERVER Board Configuration header population... PASSED
[MAIN_Cortex_R5_0_1] CpswProxy: Local cmd endpt 36, notify endpt 30
CpswProxy: ETHFW services found at core 3 endpts 34 (ti.ethfw.ethdevice) and 24 (ti.ethfw.notifyservice)
: Starting lwIP, local interface IP is dhcp-enabled
CpswProxy: ATTACH | C2S | virtPort=1
CpswProxy: ATTACH | S2C | token=100 rxMtu=1522 features=3 numTxCh=1 numRxFlow=1 status=0
CpswProxy: ALLOC_TX | C2S | token=100 chRelPri=0
CpswProxy: ALLOC_TX | S2C | token=100 txPsil=0xca01 chRelPri=0 status=0
CpswProxy: ALLOC_RX | C2S | token=100
CpswProxy: ALLOC_RX | S2C | token=100 flow=84,8 rxPsil=0x4a00 status=0
CpswProxy: ALLOC_MAC | C2S | token=100
CpswProxy: ALLOC_MAC | S2C | token=100 macAddr=70:22:33:65:80:cc status=0
CpswProxy: REGISTER_MAC | C2S | token=100 flowIdx=84,8
CpswProxy: REGISTER_MAC | S2C | token=100 status=0
[LWIPIF_LWIP] Enet LLD netif initialized successfully
[LWIPIF_LWIP_IC] Interface started successfully
[LWIPIF_LWIP_IC] NETIF INIT SUCCESS
: Added interface 'ti0', IP is 0.0.0.0
: Added interface 'br2', IP is 0.0.0.0
: Starting lwIP, local interface IP is dhcp-enabled
CpswProxy: ATTACH | C2S | virtPort=7
CpswProxy: ATTACH | S2C | token=700 rxMtu=1522 features=1 numTxCh=1 numRxFlow=1 status=0
CpswProxy: ALLOC_TX | C2S | token=700 chRelPri=0
CpswProxy: ALLOC_TX | S2C | token=700 txPsil=0xca02 chRelPri=0 status=0
CpswProxy: ALLOC_RX | C2S | token=700
CpswProxy: ALLOC_RX | S2C | token=700 flow=84,9 rxPsil=0x4a00 status=0
CpswProxy: ALLOC_MAC | C2S | token=700
CpswProxy: ALLOC_MAC | S2C | token=700 macAddr=70:bd:3b:ce:4b:da status=0
CpswProxy: REGISTER_MAC | C2S | token=700 flowIdx=84,9
CpswProxy: REGISTER_MAC | S2C | token=700 status=0
[LWIPIF_LWIP] Enet LLD netif initialized successfully
: Added interface 'ti3', IP is 0.0.0.0
CpswProxy: ALLOC_CPTS_HW_PUSH | C2S | token=100
CpswProxy: ALLOC_CPTS_HW_PUSH | S2C | token=100 hwPushNum=7 status=0
CpswProxy: REGISTER_REMOTE_TIMER | C2S | token=100 timerId=2 hwPushNum=7
CpswProxy: REGISTER_REMOTE_TIMER | S2C | token=100 status=0
TS_CouplerClient: IsrPhcTime: 27365528277 RemotePhcTime: 27365523746 Delta time: 4531
ad: Added interface 'br2', IP is 172.24.227.172
TS_CouplerClient: IsrPhcTime: 32365531871 RemotePhcTime: 32365523746 Delta time: 8125
TS_CouplerClient: IsrPhcTime: 37365535308 RemotePhcTime: 37365523748 Delta time: 11560
TS_CouplerClient: IsrPhcTime: 42365539683 RemotePhcTime: 42365523748 Delta time: 15935
TS_CouplerClient: IsrPhcTime: 47365543902 RemotePhcTime: 47365523750 Delta time: 20152
TS_CouplerClient: IsrPhcTime: 52365547339 RemotePhcTime: 52365523750 Delta time: 23589
TS_CouplerClient: IsrPhcTime: 57365551714 RemotePhcTime: 57365523752 Delta time: 27962
TS_CouplerClient: IsrPhcTime: 62365555777 RemotePhcTime: 62365523752 Delta time: 32025
TS_CouplerClient: IsrPhcTime: 67365559058 RemotePhcTime: 67365523754 Delta time: 35304
TS_CouplerClient: IsrPhcTime: 72365563277 RemotePhcTime: 72365523754 Delta time: 39523
TS_CouplerClient: IsrPhcTime: 77365566714 RemotePhcTime: 77365523756 Delta time: 42958
TS_CouplerClient: IsrPhcTime: 82365570777 RemotePhcTime: 82365523756 Delta time: 47021
TS_CouplerClient: IsrPhcTime: 87365574683 RemotePhcTime: 87365523758 Delta time: 50925
TS_CouplerClient: IsrPhcTime: 92365578746 RemotePhcTime: 92365523758 Delta time: 54988
TS_CouplerClient: IsrPhcTime: 97365582652 RemotePhcTime: 97365523760 Delta time: 58892
TS_CouplerClient: IsrPhcTime: 102365586558 RemotePhcTime: 102365523760 Delta time: 62798
TS_CouplerClient: IsrPhcTime: 107365590777 RemotePhcTime: 107365523762 Delta time: 67015
TS_CouplerClient: IsrPhcTime: 112365594371 RemotePhcTime: 112365523762 Delta time: 70609
TS_CouplerClient: IsrPhcTime: 117365598433 RemotePhcTime: 117365523764 Delta time: 74669
TS_CouplerClient: IsrPhcTime: 122365602183 RemotePhcTime: 122365523764 Delta time: 78419
TS_CouplerClient: IsrPhcTime: 127365526732 RemotePhcTime: 127365523766 Delta time: 2966
TS_CouplerClient: IsrPhcTime: 132365528139 RemotePhcTime: 132365523766 Delta time: 4373
TS_CouplerClient: IsrPhcTime: 137365529701 RemotePhcTime: 137365523766 Delta time: 5935
TS_CouplerClient: IsrPhcTime: 142365531576 RemotePhcTime: 142365523768 Delta time: 7808
TS_CouplerClient: IsrPhcTime: 147365533295 RemotePhcTime: 147365523768 Delta time: 9527
TS_CouplerClient: IsrPhcTime: 152365534389 RemotePhcTime: 152365523770 Delta time: 10619
TS_CouplerClient: IsrPhcTime: 157365535951 RemotePhcTime: 157365523770 Delta time: 12181
TS_CouplerClient: IsrPhcTime: 162365537514 RemotePhcTime: 162365523772 Delta time: 13742
TS_CouplerClient: IsrPhcTime: 167365539076 RemotePhcTime: 167365523772 Delta time: 15304
TS_CouplerClient: IsrPhcTime: 172365540951 RemotePhcTime: 172365523774 Delta time: 17177
TS_CouplerClient: IsrPhcTime: 177365542201 RemotePhcTime: 177365523774 Delta time: 18427
TS_CouplerClient: IsrPhcTime: 182365543764 RemotePhcTime: 182365523776 Delta time: 19988
TS_CouplerClient: IsrPhcTime: 187365545482 RemotePhcTime: 187365523776 Delta time: 21706
TS_CouplerClient: IsrPhcTime: 192365546732 RemotePhcTime: 192365523778 Delta time: 22954
TS_CouplerClient: IsrPhcTime: 197365548764 RemotePhcTime: 197365523778 Delta time: 24986
TS_CouplerClient: IsrPhcTime: 202365550014 RemotePhcTime: 202365523780 Delta time: 26234
TS_CouplerClient: IsrPhcTime: 207365551732 RemotePhcTime: 207365523780 Delta time: 27952
TS_CouplerClient: IsrPhcTime: 212365552982 RemotePhcTime: 212365523782 Delta time: 29200
TS_CouplerClient: IsrPhcTime: 217365555014 RemotePhcTime: 217365523782 Delta time: 31232
TS_CouplerClient: IsrPhcTime: 222365556576 RemotePhcTime: 222365523784 Delta time: 32792
TS_CouplerClient: IsrPhcTime: 227365525815 RemotePhcTime: 227365523784 Delta time: 2031
TS_CouplerClient: IsrPhcTime: 232365525502 RemotePhcTime: 232365523786 Delta time: 1716
TS_CouplerClient: IsrPhcTime: 237365525815 RemotePhcTime: 237365523786 Delta time: 2029
TS_CouplerClient: IsrPhcTime: 242365525815 RemotePhcTime: 242365523786 Delta time: 2029
TS_CouplerClient: IsrPhcTime: 247365525815 RemotePhcTime: 247365523788 Delta time: 2027
TS_CouplerClient: IsrPhcTime: 252365525659 RemotePhcTime: 252365523788 Delta time: 1871
TS_CouplerClient: IsrPhcTime: 257365525815 RemotePhcTime: 257365523790 Delta time: 2025
```


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
(CPSW9G or CPSW5G) found in J721E, J7200, J784S4 and J742S2 devices for features like VLAN,
Multicast, etc. It also demonstrates lwIP (TCP/IP stack) integration into
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

## Enhanced Schedule Traffic {#ethfw_est_demo_support}

ETHFW supports EST demo application which demonstrates how to configure the 
802.1 Qbv (EST) through the TSN yang interface. The yang interface in the TSN
is governed by a module called `uniconf` which runs as a daemon. 
Any application which interacts with the uniconf called a `uniconf client`.
The uniconf client configures Qbv by opening yang database (DB), write config
yang parameters to DB and trigger the uniconf for reading parameters from DB 
and writing to HW. The uniconf reads or writes parameters from or to HW by 
calling TI's Enet LLD driver.

### Configuration parameters
This demo uses `EthFwEstDemoTestParam ` structure which is populated by user as a static configuration.
This structure contains all the params required for setting up EST, once the user fills the required
configuration needed this gets passed to the TSN stack for enabling and scheduling EST.
In the demo application provided out-of-box, following are the config params for EST:

```C
   static EthFwEstDemoTestParam gEthFwEstDemoTestLists[] =
   {
      {
         .list =
         {
               .baseTime    = 0ULL,
               .cycleTime   = 4*MIN_INTERVAL_NS,
               .gateCmdList =
               {
                  { .gateStateMask = ENET_TAS_GATE_MASK(1, 0, 0, 0, 0, 0, 0, 1),
                     .timeInterval =  MIN_INTERVAL_NS
                  },
                  { .gateStateMask = ENET_TAS_GATE_MASK(1, 0, 0, 0, 0, 1, 0, 0),
                     .timeInterval =  MIN_INTERVAL_NS
                  },
                  { .gateStateMask = ENET_TAS_GATE_MASK(1, 0, 0, 0, 0, 0, 0, 1),
                     .timeInterval =  MIN_INTERVAL_NS
                  },
                  { .gateStateMask = ENET_TAS_GATE_MASK(1, 0, 0, 0, 0, 1, 0, 0),
                     .timeInterval =  MIN_INTERVAL_NS
                  },
               },
               .listLength = 4U,
         },
         .stParam =
         {
               .streamParams =
               {
                  /* test appliction sends packet with interval 200us */
                  {.bitRateKbps = CALC_BITRATE_KBPS(AVTP_PKT_PAYLOAD_LEN, 200),
                  .payloadLen = AVTP_PKT_PAYLOAD_LEN,
                  .tc = 0U,
                  .priority = 0U,
                  },
                  /* test appliction sends packet with interval 200us */
                  {.bitRateKbps = CALC_BITRATE_KBPS(AVTP_PKT_PAYLOAD_LEN, 200),
                  .payloadLen = AVTP_PKT_PAYLOAD_LEN,
                  .tc = 2U,
                  .priority = 2U,
                  },
               },
               .nStreams = 2U,
         }
      },
   };
```

   The above structure holds the gate command list, interval time, cycle time 
   and base time which are specific to EST configuration.
   Base time will however be calculated later as future time in multiples of cycle time.
   Additionally, one can set up the stream params (params per priority channel) with the
   bitrate (in kbps) where each packet's size is 1200 Bytes. Here 200us denotes the time
   interval between two consecutive priority packets.
   User can add/remove priorities from gameCmdList and streamParams based on their requirement.
   But the seventh priority channel should always be open for gPTP traffic, which is required
   for keeping listener and talker in sync.

### Yang Configuration for 802.1Qbv

- Yang file for the 802.1Qbv is defined at
  `standard/ieee/draft/802.1/Qcw/ieee802-dot1q-sched-bridge.yang`
  and `standard/ieee/draft/802.1/Qcw/ieee802-dot1q-sched.yang`
  from the <a href="https://github.com/YangModels/yang.git">EST YangModels</a>

- Since Enet driver supports to configure the ``admin-control-list``,
  `baseTime` (`admin-base-time`) and cycleTime (`admin-cycle-time`),
  this section only describes the parameters of the ``admin-control-list``
  Here are parameters of the ``admin-control-list`` after converting
  parameters from yang to to xml:

```C
   <?xml version='1.0' encoding='UTF-8'?>
      <data xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">
         <interfaces xmlns="urn:ietf:params:xml:ns:yang:ietf-interfaces">
            <interface>
            <name/>
            <bridge-port xmlns="urn:ieee:std:802.1Q:yang:ieee802-dot1q-bridge">
               <traffic-class>
                  <traffic-class-table>
                  <number-of-traffic-classes/>
                  <priority0/>
                  ...
                  <priority7/>
                  </traffic-class-table>
                  <tc-data>
                  <tc/>
                  <lqueue/>
                  ...
                  </tc-data>
                  <number-of-pqueues/>
                  <pqueue-map>
                  <pqueue/>
                  <lqueue/>
                  </pqueue-map>
               </traffic-class>
               <gate-parameter-table xmlns="urn:ieee:std:802.1Q:yang:ieee802-dot1q-sched-bridge">
                  <gate-enabled></gate-enabled>
                  <admin-control-list>
                  <gate-control-entry>
                     <index/>
                     <operation-name/>
                     <time-interval-value/>
                     <gate-states-value/>
                  </gate-control-entry>
                  </admin-control-list>
                  <admin-cycle-time>
                  <numerator/>
                  <denominator/>
                  </admin-cycle-time>
                  <admin-base-time>
                  <seconds/>
                  <nanoseconds/>
                  </admin-base-time>
               </gate-parameter-table>
            </bridge-port>
            </interface>
         </interfaces>
      </data>
```

- All the supported parameters for 802.1Qbv are from yang standard except the mechanism for mapping
  traffic class and HW queue are augmented. To configure all parameters of 802.1Qbv above, application
  calls the `yang_db_runtime_put_oneline` to write data to DB.

Please refer [Enhanced Schedule Traffic demo](@ref ethfw_est_demo) section on how to run and build EST demo app.

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
J742S2  | \ref ethfw_depend_evm_j784s4  | None


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


### J784S4/J742S2 EVM {#ethfw_depend_evm_j784s4}

![](J784S4EVM_CPSW_TopView.png "J784S4/J742S2 EVM connections")


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

For references to the Enet LLD driver, please refer [Enet LLD User Guide](https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-jacinto7/10_01_00_04/exports/docs/pdk_jacinto_10_01_00_25/docs/userguide/jacinto/modules/enet/enet.html) and [PHY Integration Guide](https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-jacinto7/10_01_00_04/exports/docs/pdk_jacinto_10_01_00_25/docs/apiguide/j721e/html/enetphy_guide_top.html#enetphy_guide_intro)

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
integration of lwIP into J721E/J7200/J784S4/J742S2 devices is done through Enet LLD, which
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

Starting from SDK 9.0, a new gPTP stack is integrated on top of Enet LLD in PDK, it can be
located at: `<pdk>/packages/ti/transport/tsn/tsn-stack`.
The previous gPTP test stack used in SDK 8.x and older releases is no longer supported
and has been fully removed from both, Enet LLD and Ethernet Firmware.

The new gPTP stack provides time synchronization for CPSW5G/CPSW9G on Main R5F0 core 0
for J721E, J7200, J784S4 and J742S2. The stack is composed of the following modules:

  - **tsn_unibase** : Universal utility libraries that are platform-independent.
  - **tsn_combase** : Communication utility libraries that provide support for functions
    like sockets, mutexes, and semaphores.
  - **tsn_uniconf**: Universal configuration daemon for Yang, provides APIs for developing
    a client application which retreives/writes yang parameters from/to database
  - **tsn_gptp**: Implementation of the IEEE 802.1 AS gptp protocol.

This stack can be used for production and testing purposes. For more information about
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

**Note:** There is no SafeRTOS support for J742S2

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## IDE (CCS) {#ethfw_instal_ccs}

Install Code Composer Studio and setup a <b>Target Configuration</b> for use with
J721E, J7200 or J784S4 EVM.  Refer to the instructions in *CCS Setup* section of
the Processor SDK RTOS documentation.

**Note:** There is no CCS support for J742S2

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

The above will generate binary for running ethernet firmware on the Main R5F 0 Core 0.
To run ethernet firmware on Main R5F 0 Core 1, user must provide an additional
`ETHFW_RTOS_MCU2_1_SUPPORT` flag.

    make ethfw_all BUILD_SOC_LIST=<SOC> PSDK_TOOLS_PATH=$HOME/ti ETHFW_RTOS_MCU2_1_SUPPORT=yes

**Note: Ethernet Firmware support on the Main R5F 0 Core 1 is provided only for J721E and J784S4 SOCs.**

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

For J742S2:

    make ethfw_all BUILD_SOC_LIST=J742S2


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
`-D=SOC_J742S2`                  | Identifies the J742S2 SoC type
`-D=J742S2`                      | Identifies the J742S2 device type
`-D=R5Ft="R5Ft"`                 | Identifies the core type as ARM R5F with Thumb2 enabled
`-D=TARGET_NUM_CORES=2`          | Identifies the core id as mcu2_0 (ETHFW server)
`-D=TARGET_NUM_CORES=3`          | Identifies the core id as mcu2_1 (RTOS client)
`-D=TARGET_ARCH=32`              | Identifies the target architecture as 32-bit
`-D=ARCH_32`                     | Identifies the architecture as 32-bit
`-D=FREERTOS`                    | Identifies as FreeRTOS operating system build
`-D=SAFERTOS`                    | Identifies as SafeRTOS operating system build
`-D=ETHFW_PROXY_ARP_SUPPORT`     | Enable Proxy ARP support on EthFw server
`-D=ETHFW_CPSW_VEPA_SUPPORT`     | Enable VEPA support on EthFw server (only applicable to J784S4 and J742S2)
`-D=ETHAPP_ENABLE_INTERCORE_ETH` | Enable Intercore Virtual Ethernet support (disabled in QNX images)
`-D=ETHAPP_ENABLE_IPERF_SERVER`  | Enable lwIP iperf server support (TCP only)
`-D=ENABLE_QSGMII_PORTS`         | Enable QSGMII ports in QpENet expansion board (applicable only to J721E)
`-D=ETHFW_BOOT_TIME_PROFILING`   | Enable special ETHFW configuration for boot time profiling (TI internal)
`-D=ETHFW_DEMO_SUPPORT`          | Enable ETHFW demos, such as hardware and software interVLAN, GUI configurator tool, etc.
`-D=ETHFW_MONITOR_SUPPORT`       | Enable ETHFW Monitor to detect and handle any HW lockups and perform reset recovery.
`-D=ETHFW_MTS_SUPPORT`           | Enable ETHFW Multicore Timesync support.
`-D=ETHFW_MTS_DEMO_TEST`         | Enable Multicore Timesync Demo test on RTOS client.

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
`-D=SOC_J742S2`                  | Identifies the J742S2 SoC type
`-D=J742S2`                      | Identifies the J742S2 device type
`-D=R5Ft="R5Ft"`                 | Identifies the core type as ARM R5F with Thumb2 enabled
`-D=TARGET_NUM_CORES=2`          | Identifies the core id as mcu2_0 (ETHFW server)
`-D=TARGET_NUM_CORES=3`          | Identifies the core id as mcu2_1 (RTOS client)
`-D=TARGET_ARCH=32`              | Identifies the target architecture as 32-bit
`-D=ARCH_32`                     | Identifies the architecture as 32-bit
`-D=FREERTOS`                    | Identifies as FreeRTOS operating system build
`-D=SAFERTOS`                    | Identifies as SafeRTOS operating system build
`-D=ETHFW_PROXY_ARP_SUPPORT`     | Enable Proxy ARP support on EthFw server
`-D=ETHFW_CPSW_VEPA_SUPPORT`     | Enable VEPA support on EthFw server (only applicable to J784S4 and J742S2)
`-D=ETHAPP_ENABLE_INTERCORE_ETH` | Enable Intercore Virtual Ethernet support (disabled in QNX images)
`-D=ETHAPP_ENABLE_IPERF_SERVER`  | Enable lwIP iperf server support (TCP only)
`-D=ENABLE_QSGMII_PORTS`         | Enable QSGMII ports in QpENet expansion board (applicable only to J721E)
`-D=ETHFW_BOOT_TIME_PROFILING`   | Enable special ETHFW configuration for boot time profiling (TI internal)
`-D=ETHFW_DEMO_SUPPORT`          | Enable ETHFW demos, such as hardware and software interVLAN, GUI configurator tool, etc.
`-D=ETHFW_MONITOR_SUPPORT`       | Enable ETHFW Monitor to detect and handle any HW lockups and perform reset recovery.
`-D=ETHFW_MTS_SUPPORT`           | Enable ETHFW Multicore Timesync support.
`-D=ETHFW_MTS_DEMO_TEST`         | Enable Multicore Timesync Demo test on RTOS client.

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
^             | J742S2               | -

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
1.8      | 28 Aug 2024   | Vaibhav Jindal         | Added J742S2 support for SDK 10.0.1

[Back To Top](@ref ethfw_c_ug_top)
(@ref ethfw_c_ug_top)
