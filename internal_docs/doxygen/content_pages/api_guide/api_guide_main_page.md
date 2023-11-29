[TOC]

# Introduction {#ethfw_api_introduction}

Ethernet Firmware is a RTOS based server-client application which runs on Cortex
R5F 0 core 0 in main domain of J721E, J7200 and J784S4 SoCs. ETHFW enables multiple
client drivers to run independently on the remaining cores in the system. So the
multiport CPSW switch present in the Jacinto family devices will be shared among
all the participating cores within the SoC. ETHFW owns the CPSW switch and provides
remote configuration infrastructure for other processing cores running different
operating systems.

# Ethernet Remote Configuration {#ethfw_api_ethremotecfg}

Ethernet Firmware is in charge of performing any CPSW switch configuration required
by remote cores on their behalf, which it does through its *ethremotecfg* framework.
This framework is based on IPC low-level driver and is
located at *ethfw/ethremotecfg/* folder. Each and every core participating in the
inter core communication creates a local endpoint(s) which are unique per core
and will be used a landmarks while sending/receiving messages across cores. These
endpoints can be hard-coded or assigned dynamically based on the user requirement.

## Endpoints {#ethfw_api_ethremotecfg_endpts}

Message types and endpoints supported by ETHFW are mentioned below:

| Endpoint Type | Endpoint Service                                                         | Endpoint Owner     | Core                        | Description                                         |
|---------------|--------------------------------------------------------------------------|--------------------|-----------------------------|-----------------------------------------------------|
| Dynamic       | Publishes `"ti.ethfw.ethdevice"` and subscribes `"ti.autosar.ethdevice"` | ETHFW              | Main R5F 0 core 0 (mcu2_0)  | Handles the configurations for the remote clients   |
| Dynamic       | Publishes `"ti.ethfw.notifyservice"`                                     | ETHFW              | Main R5F 0 core 1 (mcu2_0)  | Sends CPTS HW push events for multi-core Timesync   |
| Dynamic       | Subscribes `"ti.ethfw.ethdevice"`                                        | Linux client       | A72 (mpu1_0)                | Requests the remote configurations to server        |
| Static (30U)  | Subscribes `"ti.ethfw.notifyservice"`                                    | Linux client       | A72 (mpu1_0)                | Receives HW push events for RTOS client             |
| Dynamic       | Subscribes `"ti.ethfw.ethdevice"`                                        | QNX io-pkt client  | A72 (mpu1_0)                | Requests the remote configurations to server        |
| Static (30U)  | Subscribes `"ti.ethfw.notifyservice"`                                    | QNX io-pkt client  | A72 (mpu1_0)                | Receives HW push events for RTOS client             |
| Dynamic       | Subscribes `"ti.ethfw.ethdevice"`                                        | RTOS client        | Main R5F 0 core 1 (mcu2_1)  | Requests the remote configurations to server        |
| Static (30U)  | Subscribes `"ti.ethfw.notifyservice"`                                    | RTOS client        | Main R5F 0 core 1 (mcu2_1)  | Receives HW push events for RTOS client             |
| Static (28U)  | Publishes `"ti.autosar.ethdevice"`                                       | AUTOSAR client     | Main R5F 0 core 1 (mcu2_1)  | Used to bind AUTOSAR with ETHFW                     |
| Static (38U)  | Publishes `"ti.autosar.ethdevice"`                                       | AUTOSAR client     | Main R5F 0 core 1 (mcu2_1)  | Used to bind AUTOSAR with ETHFW                     |

This table lists all client types supported by Ethernet Firmware, but no assumptions
should be made for this table alone regarding remote client concurrency.


## Notifications {#ethfw_api_ethremotecfg_notify}

Notification are messages sent by Ethernet Firmware to all clients, they are
exclusively server-to-client direction.

| Notifications                                                       | Params                                    | Switch Port Support | MAC Port Support | Description                         |
|:--------------------------------------------------------------------|:------------------------------------------|:-------------------:|:----------------:|:------------------------------------|
| [FWINFO](\ref ETHREMOTECFG_NOTIFY_FWINFO)                           | \ref EthRemoteCfg_DeviceData              |          Y          |        Y         | ETHFW version info notify           |
| [HWPUSH](\ref ETHREMOTECFG_NOTIFY_HWPUSH)                           | \ref EthRemoteCfg_NotifyServiceHwPushMsg  |          Y          |        Y         | Send CPSW HW push events            |
| [HWERROR](\ref ETHREMOTECFG_NOTIFY_HWERROR)                         | \ref EthRemoteCfg_CommonNotify            |          Y          |        Y         | Hardware error notify               |
| [HWRECOVERY_COMPLETE](\ref ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE) | \ref EthRemoteCfg_CommonNotify            |          Y          |        Y         | Hardware recovery completion notify |


## Commands {#ethfw_api_ethremotecfg_cmds}

Commands are messages sent by remote clients to Ethernet Firmware for it to taken
action on their behalf.  All commands are composed of a *request* (client-to-server
direction) and a *response* (server-to-client direction).

| Commands                                                                   | Request Params                             | Response Params                     | Switch Port\n Support | MAC Port\n Support | Description                               |
|:---------------------------------------------------------------------------|:-------------------------------------------|:------------------------------------|:---------------------:|:------------------:|:------------------------------------------|
| [VIRT_PORT_INFO](\ref ETHREMOTECFG_CMD_VIRT_PORT_INFO)                     | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_OfferVirtPortRes  |         NA            |        NA          | Allocates virtual ports                   |
| [ATTACH](\ref ETHREMOTECFG_CMD_ATTACH)                                     | \ref EthRemoteCfg_AttachReq                | \ref EthRemoteCfg_AttachRes         |         Y             |         Y          | Attachs client to ETHFW                   |
| [ATTACH_EXT](\ref ETHREMOTECFG_CMD_ATTACH_EXT)                             | \ref EthRemoteCfg_AttachReq                | \ref EthRemoteCfg_AttachExtRes      |         Y             |         Y          | Attachs and returns extended attach info  |
| [DETACH](\ref ETHREMOTECFG_CMD_DETACH)                                     | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Detaches the client                       |
| [PORT_LINK_STATUS](\ref ETHREMOTECFG_CMD_PORT_LINK_STATUS)                 | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_PortLinkStatusRes |         Y             |         Y          | Queries link status of the port           |
| [ALLOC_TX](\ref ETHREMOTECFG_CMD_ALLOC_TX)                                 | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_AllocTxRes        |         Y             |         Y          | Allocates Tx channel                      |
| [ALLOC_RX](\ref ETHREMOTECFG_CMD_ALLOC_RX)                                 | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_AllocRxRes        |         Y             |         Y          | Allocates Rx flow                         |
| [ALLOC_MAC](\ref ETHREMOTECFG_CMD_ALLOC_MAC)                               | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_AllocMacRes       |         Y             |         Y          | Allocates MAC Address                     |
| [FREE_TX](\ref ETHREMOTECFG_CMD_FREE_TX)                                   | \ref EthRemoteCfg_FreeTxReq                | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Frees the allocated Tx channel            |
| [FREE_RX](\ref ETHREMOTECFG_CMD_FREE_RX)                                   | \ref EthRemoteCfg_FreeRxReq                | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Frees the allocated Rx channel            |
| [FREE_MAC](\ref ETHREMOTECFG_CMD_FREE_MAC)                                 | \ref EthRemoteCfg_FreeMacReq               | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Frees the allocated MAC Address           |
| [REGISTER_MAC](\ref ETHREMOTECFG_CMD_REGISTER_MAC)                         | \ref EthRemoteCfg_MacAddrRxFlowReq         | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Registers macAddr with the Rx flow ID     |
| [DEREGISTER_MAC](\ref ETHREMOTECFG_CMD_DEREGISTER_MAC)                     | \ref EthRemoteCfg_MacAddrRxFlowReq         | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Unregisters macAddr with the Rx flow ID   |
| [SET_RX_DEFAULTFLOW](\ref ETHREMOTECFG_CMD_SET_RX_DEFAULTFLOW)             | \ref EthRemoteCfg_RxDefaultFlowRegisterReq | \ref EthRemoteCfg_StatusRes         |         Y             |         N          | Registers default flow routing to client  |
| [DEL_RX_DEFAULTFLOW](\ref ETHREMOTECFG_CMD_DEL_RX_DEFAULTFLOW)             | \ref EthRemoteCfg_RxDefaultFlowRegisterReq | \ref EthRemoteCfg_StatusRes         |         Y             |         N          | Unregisters default flow                  |
| [REGISTER_IPv4](\ref ETHREMOTECFG_CMD_REGISTER_IPv4)                       | \ref EthRemoteCfg_IPv4AddrRegisterReq      | \ref EthRemoteCfg_StatusRes         |         Y             |         N          | Associates IP addr with MAC addr          |
| [DEREGISTER_IPv4](\ref ETHREMOTECFG_CMD_DEREGISTER_IPv4)                   | \ref EthRemoteCfg_IPv4AddrDeregisterReq    | \ref EthRemoteCfg_StatusRes         |         Y             |         N          | Deletes the IP addr:MAC addr mapping      |
| [JOIN_VLAN](\ref ETHREMOTECFG_CMD_JOIN_VLAN)                               | \ref EthRemoteCfg_VlanJoinReq              | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Joins a VLAN                              |
| [LEAVE_VLAN](\ref ETHREMOTECFG_CMD_LEAVE_VLAN)                             | \ref EthRemoteCfg_VlanLeaveReq             | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Leaves a VLAN                             |
| [ADD_FILTER_MAC](\ref ETHREMOTECFG_CMD_ADD_FILTER_MAC)                     | \ref EthRemoteCfg_FilterMacAddReq          | \ref EthRemoteCfg_StatusRes         |         Y             |         N          | Adds multicast macAddr to Rx filter       |
| [DEL_FILTER_MAC](\ref ETHREMOTECFG_CMD_DEL_FILTER_MAC)                     | \ref EthRemoteCfg_FilterMacDelReq          | \ref EthRemoteCfg_StatusRes         |         Y             |         N          | Deletes multicast macAddr from Rx filter  |
| [ENABLE_PROMISC](\ref ETHREMOTECFG_CMD_ENABLE_PROMISC)                     | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_StatusRes         |         N             |         Y          | Enables promiscous mode                   |
| [DISABLE_PROMISC](\ref ETHREMOTECFG_CMD_DISABLE_PROMISC)                   | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_StatusRes         |         N             |         Y          | Disables promiscous mode                  |
| [READ_REGISTER](\ref ETHREMOTECFG_CMD_READ_REGISTER)                       | \ref EthRemoteCfg_RegReadReq               | \ref EthRemoteCfg_RegReadRes        |         NA            |         NA         | Reads from a register                     |
| [WRITE_REGISTER](\ref ETHREMOTECFG_CMD_WRITE_REGISTER)                     | \ref EthRemoteCfg_RegWriteReq              | \ref EthRemoteCfg_StatusRes         |         NA            |         NA         | Writes to a register                      |
| [REGISTER_MATCH_ETHTYPE](\ref ETHREMOTECFG_CMD_REGISTER_MATCH_ETHTYPE)     | \ref EthRemoteCfg_MatchEthertypeAddReq     | \ref EthRemoteCfg_StatusRes         |         Y             |         N          | Registers for ethertype based route       |
| [DEREGISTER_MATCH_ETHTYPE](\ref ETHREMOTECFG_CMD_DEREGISTER_MATCH_ETHTYPE) | \ref EthRemoteCfg_MatchEthertypeDelReq     | \ref EthRemoteCfg_StatusRes         |         Y             |         N          | Unregisters the ethertype based route     |
| [REGISTER_REMOTE_TIMER](\ref ETHREMOTECFG_CMD_REGISTER_REMOTE_TIMER)       | \ref EthRemoteCfg_RemoteTimerRegisterReq   | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Registers remote timer for timesync       |
| [DEREGISTER_REMOTE_TIMER](\ref ETHREMOTECFG_CMD_DEREGISTER_REMOTE_TIMER)   | \ref EthRemoteCfg_RemoteTimerDeregisterReq | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Unregisters remote timer for timesync     |
| [MESSAGE_PING](\ref ETHREMOTECFG_CMD_MESSAGE_PING)                         | \ref EthRemoteCfg_PingReq                  | \ref EthRemoteCfg_PingRes           |         NA            |         NA         | Pings Ethernet Firmware                   |
| [GET_SERVER_STATUS](\ref ETHREMOTECFG_CMD_GET_SERVER_STATUS)               | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_ServerStatusRes   |         NA            |         NA         | Provides the remote server status         |
| [TEARDOWN_COMPLETION](\ref ETHREMOTECFG_CMD_TEARDOWN_COMPLETION)           | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Notifies DMA teardown completion          |
| [IOCTL](\ref ETHREMOTECFG_CMD_IOCTL)                                       | \ref EthRemoteCfg_IoctlReq                 | \ref EthRemoteCfg_IoctlRes          |         Y             |         Y          | Invokes ENET LLD IOCTLs by the clients    |
| [DUMP](\ref ETHREMOTECFG_CMD_DUMP)                                         | \ref EthRemoteCfg_CommonReq                | \ref EthRemoteCfg_StatusRes         |         Y             |         Y          | Dumps CPSW Stats                          |

[Back To Top](\ref ethfw_api_introduction)
