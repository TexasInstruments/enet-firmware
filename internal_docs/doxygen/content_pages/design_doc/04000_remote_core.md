# Remote Core Integration {#ethfw_remotecore_top}

[TOC]

The Ethernet Switch hardware is shared across processing cores of the Jacinto 7
devices using the Remote Device framework.  The Remote Device framework defines
two types of components: server and client, which communicate using a protocol
specific for the underlying remote device type.

In the Ethernet Switch scenario, the Ethernet Firmware is the server running on
a Cortex-R5F core, the *master core*.  On the other hand, the clients can be
other *remote cores* like Cortex-A72 or other Cortex-R5F cores.

The master and remote cores have different privileges with respect to the
Ethernet Switch functionality they can acccess.  Privileges are assigned per
core via CPSW Resource Manager software component.  Similarly, DMA resources
like Ring Acc are also assigned per core.

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

[Back To Top](@ref ethfw_remotecore_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Linux {#ethfw_remotecore_linux}

Linux remote core support is enabled via virtual mac netdev driver.  The virtual
mac netdev driver is a standard netdev driver that plugins into the Linux kernel
network stack.  The netdev driver will attach to the master core running the
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
# TI-RTOS {#ethfw_remotecore_tirtos}

The `app_remoteswitchcfg_client` application demonstrates direct data path to 
remote cores running TI-RTOS.  The application uses Ethernet Switch remote device
client APIs to communicate with the master core.

The remote core runs standard TI NDK stack.  The NDK/NIMU layer invokes an
application callback at open time.  The callback opens Rx Flow and Tx Channel
handles.  The resources required to open a Tx channel and Rx flow such as Tx DMA
channel, CPSW PSIL destination thread, Rx flow Id, destination MAC address are
allocated by invoking the Ethernet Switch remote device client API.  The client
APIs send IPC msg to the master core to allocate resources.  The client APIs
also support remote core invocation of all CPSW LLD runtime IOCTLs.

The application `app_remoteswitchcfg_client` demonstrates the remote core IOCTL
invocation using the `CPSW_IOCTL_IS_PORT_LINK_UP` to query PHY status.

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
