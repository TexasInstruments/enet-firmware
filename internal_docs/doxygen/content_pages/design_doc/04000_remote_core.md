# Remote Core Integration {#ethfw_remotecore_top}

[TOC]

The Ethernet Switch hardware is shared across processing cores of the Jacinto 7
devices using the IPC-based switch configuration. The IPC-based framework defines
two types of components: server and client. They communicate using a protocol
defined by the server and common between all the clients.

In the Ethernet Switch scenario, the Ethernet Firmware is the server running on
a Cortex-R5F core, the *master core*.  On the other hand, the clients can be
other *remote cores* like Cortex-A72 or other Cortex-R5F cores.

The master and remote cores have different privileges with respect to the
Ethernet Switch functionality they can acccess.  Privileges are assigned per
core via CPSW Resource Manager software component.  Similarly, DMA resources
like Ring Acc are also assigned per core by the resource manager.

The *master core* is the sole owner of CPSW hardware register configuration.
The *remote cores* can only request switch control configurations via IPC to be
carried out by the master core.

*Remote cores* can also attach and allocate dedicated DMA resources they will use
to submit and retrieve packets.  Similarly, the *master core* also has its own
DMA resources which are not shared with the remote cores.

The following sequence diagram shows the initialization and de-initialization
sequences performed by the *master core*.

![](MasterCore_InitSequence.png "Master Core Init API Sequence")

<br>

![](MasterCore_DeinitSequence.png "Master Core De-init API Sequence")

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# ETHFW + Remote Client ABIs {#ethfw_remoteclient_abis}

There is a common header file defined by ETHFW and is shared between all the participating
remote cores. This header file contains what type of messages can be exchanged between the
server(ETHFW) and the clients (remote cores).

This header file is located at *ethfw/ethremotecfg/protocol* folder. Once any client binds with
ETHFW they can start sending messages to the server based on the header file definitions.
Each remote core needs to send messages per virtual port, where each virtual port corresponds
to either a MAC-Only port or a switch port. All the core specific resource allocation is
hard-coded in ETHFW. The list of messages including headers for IPC based communication between
the Server and Client(s) is referred to as ABI (Application Binary Interface).

There are primarily 3 types of ABIs:
1. Request messages: client-to-server messages
2. Response messages: server-to-client messages
3. Notify messages: server-to-client messages

Request messages are primarily used by clients to request a certain type of operation/configuration
for the remotecore/virtual port respectively. Once the server recieves the request it handles it
and sends a pre-defined response for that request(if any) and the status of the request.
Notify messages, if sent from server to client will be uni-directional, i.e., there won't be any
ACKs from the client, whereas any notify messages from client to the server will be bi-directional,
i.e., the server will be sending an ACK for the notify message sent by the client.

Following are the defined and supported messages by ETHFW:

|    Request Message Types                    |    Description                                                                 |
|---------------------------------------------|--------------------------------------------------------------------------------|
| `ETHREMOTECFG_CMD_VIRT_PORT_INFO`           |  Server returns the allocated virtual ports to a given remote core             |
| `ETHREMOTECFG_CMD_ATTACH`                   |  Command to attach to the Ethernet device                                      |
| `ETHREMOTECFG_CMD_ATTACH_EXT`               |  Command to attach to the Ethernet device which returns extended attach info   |
| `ETHREMOTECFG_CMD_DETACH`                   |  Command to detach remote client from the Ethernet device                      |
| `ETHREMOTECFG_CMD_PORT_LINK_STATUS`         |  Command to query link status of a port                                        |
| `ETHREMOTECFG_CMD_ALLOC_TX`                 |  Command to allocate Tx channel                                                |
| `ETHREMOTECFG_CMD_ALLOC_RX`                 |  Command to allocate Rx flow                                                   |
| `ETHREMOTECFG_CMD_ALLOC_MAC`                |  Command to allocate a MAC address to the client                               |
| `ETHREMOTECFG_CMD_FREE_TX`                  |  Command to free previously allocated Tx channel                               |
| `ETHREMOTECFG_CMD_FREE_RX`                  |  Command to free previously allocated Rx flow id                               |
| `ETHREMOTECFG_CMD_FREE_MAC`                 |  Command to free previously allocated MAC address                              |
| `ETHREMOTECFG_CMD_REGISTER_MAC`             |  Command to register a destination MAC address to a specific Rx flow id        |
| `ETHREMOTECFG_CMD_DEREGISTER_MAC`           |  Command to de-register a destination MAC address                              |
| `ETHREMOTECFG_CMD_SET_RX_DEFAULTFLOW`       |  Command to register default flow routing to client                            |
| `ETHREMOTECFG_CMD_DEL_RX_DEFAULTFLOW`       |  Command to de-register default flow routing to client                         |
| `ETHREMOTECFG_CMD_REGISTER_IPv4`            |  Command to associate IPv4 address with MAC address                            |
| `ETHREMOTECFG_CMD_DEREGISTER_IPv4`          |  Command to remove IPv4 address:MAC address mapping                            |
| `ETHREMOTECFG_CMD_JOIN_VLAN`                |  Command to join a VLAN                                                        |
| `ETHREMOTECFG_CMD_LEAVE_VLAN`               |  Command to leave a VLAN                                                       |
| `ETHREMOTECFG_CMD_ADD_FILTER_MAC`           |  Command to add multicast MAC address to receive filter                        |
| `ETHREMOTECFG_CMD_DEL_FILTER_MAC`           |  Command to delete multicast MAC address from receive filter                   |
| `ETHREMOTECFG_CMD_ENABLE_PROMISC`           |  Command to enable promiscuous mode                                            |
| `ETHREMOTECFG_CMD_DISABLE_PROMISC`          |  Command to disable promiscuous mode                                           |
| `ETHREMOTECFG_CMD_READ_REGISTER`            |  Command to read from an Ethernet device register                              |
| `ETHREMOTECFG_CMD_WRITE_REGISTER`           |  Command to write to an Ethernet device register                               |
| `ETHREMOTECFG_CMD_REGISTER_MATCH_ETHTYPE`   |  Command to setup an EtherType-based packet route                              |
| `ETHREMOTECFG_CMD_DEREGISTER_MATCH_ETHTYPE` |  Command to tear-down an EtherType-based packet route                          |
| `ETHREMOTECFG_CMD_REGISTER_REMOTE_TIMER`    |  Command to register a remote timer for multicore time synchronization         |
| `ETHREMOTECFG_CMD_DEREGISTER_REMOTE_TIMER`  |  Command to de-register a remote timer with EthFw                              |
| `ETHREMOTECFG_CMD_MESSAGE_PING`             |  Command to ping the ETHFW, generally used for test purposes                   |
| `ETHREMOTECFG_CMD_GET_SERVER_STATUS`        |  Command to query the remote configuration server status                       |
| `ETHREMOTECFG_CMD_TEARDOWN_COMPLETION`      |  Command to notify client's DMA teardown completion                            |
| `ETHREMOTECFG_CMD_IOCTL`                    |  Command to invoke ENET LLD IOCTL from remote client                           |
| `ETHREMOTECFG_CMD_DUMP`                     |  Command from remote client to Server to dump CPSW stats                       |

<br>

| Notify Message Types (Server to client)     |    Description                                                                 |
|---------------------------------------------|--------------------------------------------------------------------------------|
| `ETHREMOTECFG_NOTIFY_FWINFO`                |  Send EthFw version info to the remote clients                                 |
| `ETHREMOTECFG_NOTIFY_HWPUSH`                |  Send CPTS HW push events to the remote clients                                |

For more details please refer to the ETHFW API guide.


[Back To Top](@ref ethfw_remotecore_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Linux {#ethfw_remotecore_linux}

Linux remote core support is enabled via virtual mac netdev driver.  The virtual
mac netdev driver is a standard netdev driver that plugins into the Linux kernel
network stack. The netdev driver will attach to the master core running the
Ethernet Firmware and will allocate resources to setup data path from CPSW host
port to the Cortex-A72 core. A destination MAC address is allocated to the A72
core and all packets with this destination MAC address received on the host port
will be routed to the A72 virtual mac netdev driver.

The Linux remote core interaction with the master core during initialization is
shown in the following call sequence diagram.

![](RemoteCore_Linux_InitSequence.png "Linux Remote Core Init API Sequence")

The Linux remote core interaction with the master core during teardown is shown
in the following call sequence diagram.

![](RemoteCore_Linux_DeinitSequence.png "Linux Remote Core De-init API Sequence")

[Back To Top](@ref ethfw_remotecore_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# RTOS {#ethfw_remotecore_rtos}

The `app_remoteswitchcfg_client` application demonstrates direct data path to 
remote cores running FreeRTOS. The application uses proxy client APIs to communicate
with the master core.

The remote core runs standard lwIP TCP/IP stack.  The lwIP adaptation layer invokes
an application callback at open time.  The callback opens Rx Flow and Tx Channel
handles. The resources required to open a Tx channel and Rx flow such as Tx DMA
channel, CPSW PSIL destination thread, Rx flow Id, destination MAC address are
allocated by invoking the Ethernet Switch proxy client APIs.  The client
APIs send IPC msg to the master core to allocate resources.  The client APIs
also support remote core invocation of all Enet LLD runtime IOCTLs.

The application `app_remoteswitchcfg_client` demonstrates the remote core IOCTL
invocation using the `ENET_PER_IOCTL_IS_PORT_LINK_UP` to query PHY status.

[Back To Top](@ref ethfw_remotecore_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# AUTOSAR {#ethfw_remotecore_autosar}

The Ethernet Firmware is capable of interfacing with a remote core running an
AUTOSAR stack.  The remote core side must implement a MCAL Ethernet driver with
support for virtual MAC.

In virtual MAC mode, the role of the Etherner driver is restricted from any
configuration of CPSW registers as well as management of Ethernet PHYs, which
are solely owned by the Ethernet Firmware.

The virtual MAC Ethernet driver communicates with the master core via IPC to
establish a data path by requesting DMA resources such as Tx channel and Rx
flow, destination MAC address, etc.  The IPC communication is enabled in the
AUTOSAR stack through a MCAL Complex Device Driver called CddIpc.

The following diagram shows the AUTOSAR / MCAL data setup sequence.

![](RemoteCore_AUTOSAR_DataSetupSequence.png "AUTOSAR/MCAL Remote Core Data Path Setup API Sequence")

Please refer to the TI MCUSW package for further details on the AUTOSAR / MCAL
side of the integration and enablement of the virtual MAC feature.

[Back To Top](@ref ethfw_remotecore_top)
