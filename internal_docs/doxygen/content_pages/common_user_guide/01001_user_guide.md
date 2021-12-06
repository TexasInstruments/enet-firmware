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
MAC-only         | MAC port configuration in MAC-only mode for traffic exclusively forwarded to host port
Intercore Virtual Ethernet |  TCP/IP communication between cores using inter-core virtual Ethernet driver
Broadcast and Multicast support | Ability to send broadcast and multicast traffic to multiple cores
Remote configuration server | Firmware app hosting the IPC server to serve remote clients like Linux Virtual MAC driver
Resource management library | Resource management library for CPSW resource sharing across cores

[Back To Top](@ref ethfw_c_ug_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# MAC-only {#ethfw_maconly}

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
and \ref ethfw_j7200_port_cfg subsections, respectively.

The port's default VLAN for MAC ports configured in MAC-only mode is `0`, and for MAC ports
configured in switch mode is `1`. They can be changed via `EthFw_Config::dfltVlanIdMacOnlyPorts`
and `EthFw_Config::dfltVlanIdSwitchPorts`, respectively.

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

## Local switch port {#ethfw_local_switch_port}

This is a logical port owned by the Ethernet Firmware.

Ethernet packets are exchanged with the CPSW switch through its *host port* using a UDMA
RX flow and a TX channel.

CPSW's default thread is set to this port's UDMA RX flow, also called *default RX flow*.
Traffic which is not matched by any CPSW classifier gets routed to this port.


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
For example, below code snippet shows a configuration where *virtual switch 0* is allocated
for A72 core, and *virtual switch port 1* is allocated for Main R5F 0 Core 1.

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
shows a configuration where *virtual MAC port 1* is allocated for A72, and *virtual MAC port 2*
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
read from EEPROMs located in the different daughter boards in TI EVM.  Note that a static
MAC address pool is used as a workaround in TI EVMs for cases where I2C bus contention could
happen (i.e. when integrating with Linux).  It's expected that the MAC address pool population
mechanism is adapted when integrating Ethernet Firmware to different platforms.

The utilization of these resources by Ethernet Firmware on Main R5F 0 Core 0 is as follows:

| Resource    | Count  | EthFw Usage (mcu2_0)
|:------------|:------:|:-----------------------------------
| TX channel  |   3    | <ul><li>lwIP netif (1)</li><li>PTP (1)</li><li>SW interVLAN (1)</li></ul>
| RX flow     |   5    | <ul><li>lwIP netif (1)</li><li>Proxy ARP (1)</li><li>PTP (1)</li><li>SW interVLAN (1)</li><li>Enet LLD default flow (1)</li></ul>
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

[Back To Top](@ref ethfw_c_ug_top)


# Default Port Configuration {#ethfw_port_cfg}

## J721E Port Configuration {#ethfw_j721e_port_cfg}

There are four MAC ports enabled by default in Ethernet Firmware for J721E SoC.  These
are the RGMII MAC ports in GESI board.

Two MAC ports are configured in MAC-only mode and allocated for A72 (Linux) and Main R5F
Core 1 (RTOS) usage.  The remaining two MAC ports are configured in switch mode.

![](EthFw_PortCfg_j721e_evm.png "J721E default port configuration")

The following table shows the full list of MAC ports in J721E EVM, the board they are
located and their MAC mode.  It's worth noting that the MAC ports in QSGMII daughter
board are not enabled by default.

| MAC Port    | PHY Addr | Board  | MAC mode
|:------------|:--------:|:------:|:------------
| MAC Port 1  |   12     | GESI   | MAC-only
| <span style="color:gray">MAC Port 2 |   16     | QSGMII | Switch Port</span>
| MAC Port 3  |    0     | GESI   | Switch Port
| MAC Port 4  |    3     | GESI   | MAC-only
| <span style="color:gray">MAC Port 5  |   17     | QSGMII | Switch Port</span>
| <span style="color:gray">MAC Port 6  |   18     | QSGMII | Switch Port</span>
| <span style="color:gray">MAC Port 7  |   19     | QSGMII | Switch Port</span>
| MAC Port 8  |   15     | GESI   | Switch Port


## J7200 Port Configuration {#ethfw_j7200_port_cfg}

All the four MAC ports of CPSW5G are enabled by default in Ethernet Firmware for J7200 SoC.
These are the four QSGMII MAC ports in QSGMII (QpENet) daughter board.

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
# Inter-core Virtual Ethernet {#ethfw_intercore_eth}

Starting with SDK 8.1, the EthFw integrates Inter-core Virtual Ethernet driver which allows TCP/IP
 communication between Ethernet firmware server and client cores. 

-# @ref ethfw_intercore_topology
-# @ref ethfw_intercore_r5server
-# @ref ethfw_intercore_r5client
-# @ref ethfw_intercore_a72client

[Back To Top](@ref ethfw_c_ug_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

##  Topology and Design overview {#ethfw_intercore_topology}

The topology diagram below shows the integration of inter-core virtual Ethernet in Ethernet
firmware.

Inter-core virtual network uses a star topology with the R5F_0 master core (EthFw server)
acting as the central hub. Each node (core) in the network communicates directly with the
master while communication between other nodes (A72 and R5F_1) is routed through the
master. In addition to the Enet LLD network interfaces used to communicate with the CPSW
switch, each participating core creates an inter-core network interface, which allows it
to communicate with another core using standard TCP/IP protocol suite.

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
[<b>multicast filter API</b>](../api_guide/html_/group__CPSW__PROXY__API.html#ga89c45e9fcf6a7927ed1ee4079e4ee9c7)
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

## EthFW Client integration {#ethfw_intercore_r5client}

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

# Multicast and Broadcast Support {#ethfw_mcast_support}

Starting with SDK 8.1, the Ethernet firmware supports client cores to receive multicast and broadcast traffic.

Broadcast support is automatically enabled through inter-core virtual Ethernet
mechanism which allows sending broadcast traffic to all the client cores, provided
that inter-core virtual Ethernet is enabled on that client.

For multicast support, a new
[<b>multicast filter API</b>](../api_guide/html_/group__CPSW__PROXY__API.html#ga89c45e9fcf6a7927ed1ee4079e4ee9c7)
is provided by EthFw which allows client cores to subscribe-to/unsubscribe-from multicast
addresses. The Ethernet firmware differentiates between two types of multicast addresses:

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
[<b>multicast filter API</b>](../api_guide/html_/group__CPSW__PROXY__API.html#ga89c45e9fcf6a7927ed1ee4079e4ee9c7).
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

Flag                             | Description
---------------------------------|------------
`-g`                             | Default behavior. Enables symbolic debugging. The generation of debug information do not impact optimizations. Therefore, generating debug information is enabled by default.
`--endian=little`                | Little Endian
`-mv=7R5`                        | Processor Architecture Cortex-R5
`--abi=eabi`                     | Application binary interface - ELF
`-eo=.obj`                       | Output Object file extension
`--float_support=vfpv3d16`       | VFP co-processor is enabled
`--preproc_with_compile`         | Continue compilation after using -pp`<X>` options
`-D=TARGET_BUILD=2`              | Identifies the build profile as 'debug'
`-D_DEBUG_=1`                    | Identifies as debug build
`-D=SOC_J721E`                   | Identifies the J721E SoC type
`-D=J721E`                       | Identifies the J721E device type
`-D=SOC_J7200`                   | Identifies the J7200 SoC type
`-D=J7200`                       | Identifies the J7200 device type
`-D=R5F="R5F"`                   | Identifies the core type as ARM R5F
`-D=ARCH_32`                     | Identifies the architecture as 32-bit
`-D=SYSBIOS`                     | Identifies as TI RTOS operating system build
`-D=FREERTOS`                    | Identifies as FreeRTOS operating system build
`-D=ETHFW_PROXY_ARP_SUPPORT`     | Enable Proxy ARP support on EthFw server
`-D=ETHFW_INTERCORE_ETH_SUPPORT` | Enable Intercore Ethernet support (disabled if BUILD_QNX_A72 is defined)

[Back To Top](@ref ethfw_c_ug_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Demo Application - Profile: Release {#ethfw_cflag_release}

Flag                             | Description
---------------------------------|------------
`--endian=little`                | Little Endian
`-mv=7R5`                        | Processor Architecture Cortex-R5
`--abi=eabi`                     | Application binary interface - ELF
`-eo=.obj`                       | Output Object file extension
`--float_support=vfpv3d16`       | VFP co-processor is enabled
`--preproc_with_compile`         | Continue compilation after using -pp`<X>` options
`--opt_level=3`                  | Optimization level 3
`--gen_opt_info=2`               | Generate optimizer information file at level 2
`-D=TARGET_BUILD=1`              | Identifies the build profile as 'release'
`-DNDEBUG`                       | Disable standard-C assertions
`-D=SOC_J721E`                   | Identifies the J721E SoC type
`-D=J721E`                       | Identifies the J721E device type
`-D=SOC_J7200`                   | Identifies the J7200 SoC type
`-D=J7200`                       | Identifies the J7200 device type
`-D=R5F="R5F"`                   | Identifies the core type as ARM R5F
`-D=ARCH_32`                     | Identifies the architecture as 32-bit
`-D=SYSBIOS`                     | Identifies as TI RTOS operating system build
`-D=FREERTOS`                    | Identifies as FreeRTOS operating system build
`-D=ETHFW_PROXY_ARP_SUPPORT`     | Enable Proxy ARP support on EthFw server
`-D=ETHFW_INTERCORE_ETH_SUPPORT` | Enable Intercore Virtual Ethernet support (disabled if BUILD_QNX_A72 is defined)


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
1.3      | 01 Dec 2021   | Nitin Sakhuja          | Adedd Inter-core Ethernet support for SDK 8.1

[Back To Top](@ref ethfw_c_ug_top)
(@ref ethfw_c_ug_top)
