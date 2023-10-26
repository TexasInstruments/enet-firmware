/*
 *
 * Copyright (c) 2023 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any object code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*!
 * \file ethremotecfg.h
 *
 * \brief Header file with the Ethernet Firmware remote configuration interface
 *        definition.
 */

#ifndef ETHREMOTECFG_H_
#define ETHREMOTECFG_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \ingroup  ETHFW_MODULE
 * \defgroup ETHFW_ETHREMOTECFG Ethernet Firmware Remote Config Interface
 *
 * \brief EthFw Remote Client message definitions.
 *
 * Remote clients communicate with Ethernet Firmware via IPC based APIs and
 * need a common message structure definitions to understand the communication.
 * This file defines the headers and message structures of remote client
 * communication.
 *
 * @{
 */
/*! @} */

/*!
 * \addtogroup ETHFW_ETHREMOTECFG
 * @{
 */

/* ========================================================================== */
/*                                 Macros                                     */
/* ========================================================================== */

/*!
 * \anchor EthRemoteCfg_EndptInfo
 * \name Ethernet Remote Config IPC Endpoint Info
 * @{
 */
/*! \brief AUTOSAR service endpoint name */
#define ETHREMOTECFG_AUTOSAR_REMOTE_SERVICE_NAME          "ti.autosar.ethdevice"

/*! \brief Linux/RTOS/QNX endpoint name */
#define ETHREMOTECFG_FRAMEWORK_SERVICE_NAME               "ti.ethfw.ethdevice"

/*! \brief Notify service endpoint name */
#define ETHREMOTECFG_REMOTE_NOTIFY_SERVICE                "ti.ethfw.notifyservice"
/*! @} */

/*!
 * \anchor EthRemoteCfg_ErrorCode
 * \name Ethernet Remote Config Error Codes
 * @{
 */
/*! \brief Success */
#define ETHREMOTECFG_CMDSTATUS_OK                         (0)

/*! \brief Try again. Temporary failure of command, client can retry again. */
#define ETHREMOTECFG_CMDSTATUS_EAGAIN                     (-1)

/*! \brief Failure */
#define ETHREMOTECFG_CMDSTATUS_EFAIL                      (-2)

/*! \brief Bad arguments (i.e. NULL pointer). */
#define ETHREMOTECFG_CMDSTATUS_EBADARGS                   (-3)

/*! \brief Insufficient permission. */
#define ETHREMOTECFG_CMDSTATUS_EACCESS                    (-4)

/*! \brief Operation not supported. */
#define ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED              (-5)
/*! @} */

/*! \brief Maximum length of Ethernet device message data in a ping message */
#define ETHREMOTECFG_MESSAGE_DATA_LEN                     (486U)

/*! \brief Packet priority */
#define ETHREMOTECFG_PRIORITY_NUM                         (8U)

/*! \brief MAC Address length in octets */
#define ETHREMOTECFG_MACADDRLEN                           (6U)

/*! \brief IPv4 Address length in octets */
#define ETHREMOTECFG_IPV4ADDRLEN                          (4U)

/*!
 * Indicates to the Ethernet Firmware to use the default VLAN id for
 * the type of port associated with the caller, i.e. virtual MAC or virtual
 * switch.
 */
#define ETHREMOTECFG_ETHSWITCH_VLAN_USE_DFLT              (0xFFFF)

/*! \brief Number of octets in year */
#define ETHREMOTECFG_FWDATE_YEARLEN                       (4U)

/*! \brief Number of octets in month */
#define ETHREMOTECFG_FWDATE_MONTHLEN                      (3U)

/*! \brief Number of octets in date */
#define ETHREMOTECFG_FWDATE_DATELEN                       (2U)

/*! \brief GIT Commit SHA length in octets */
#define ETHREMOTECFG_FW_COMMITSHALEN                      (8U)

/*!
 * \name Ethernet Firmware version information.
 *
 * API version info for the Ethernet Firmware.  Any remote client should check
 * API version is compatible with this API version.
 * @{
 */
/*! \brief Ethernet device API version major version */
#define ETHREMOTECFG_FW_ETHSWITCH_VERSION_MAJOR           (0U)

/*! \brief Ethernet device API version minor version */
#define ETHREMOTECFG_FW_ETHSWITCH_VERSION_MINOR           (4U)

/*! \brief Ethernet device API version minor revision */
#define ETHREMOTECFG_FW_ETHSWITCH_VERSION_REVISION        (0U)
/*! @} */

/*!
 * \anchor EthRemoteCfg_FeatureMask
 * \name Ethernet device supported feature masks
 * @{
 */
/*! \brief Tx checksum offload feature */
#define ETHREMOTECFG_FEATURE_TXCSUM                       (1 << 0)

/*! \brief MAC-only mode feature */
#define ETHREMOTECFG_FEATURE_MAC_ONLY                     (1 << 2)

/*! \brief Multicast filter feature */
#define ETHREMOTECFG_FEATURE_MC_FILTER                    (1 << 3)
/*! @} */

/*! \brief Max IPC message size */
#define ETHREMOTECFG_IPC_MSG_SIZE                         (496U + 32U)

/*! \brief Max payload size in an IPC messsage */
#define ETHREMOTECFG_IPC_MSG_PAYLOAD_SIZE                 (496U)

/*! \brief Max number of IPC buffers */
#define ETHREMOTECFG_IPC_NUM_MSG_BUFS                     (256U)

/*! \brief Buffer size allocated to the channel */
#define ETHREMOTECFG_IPC_MSG_OBJ_SIZE                     (256U)

/*! \brief IPC data size */
#define ETHREMOTECFG_IPC_DATA_SIZE                        (ETHREMOTECFG_IPC_MSG_SIZE * \
                                                           ETHREMOTECFG_IPC_NUM_MSG_BUFS + \
                                                           ETHREMOTECFG_IPC_MSG_OBJ_SIZE)

/*! \brief Ethernet device C2S request type count */
#define ETHREMOTECFG_CMD_TYPE_COUNT                       (ETHREMOTECFG_CMD_TYPE_LAST + 1)

/*! \brief Ethernet device S2C notify message type count */
#define ETHREMOTECFG_NOTIFY_TYPE_COUNT                    (ETHREMOTECFG_NOTIFYTYPE_LAST + 1)

/*! \brief Remote client token value when none is assigned by the server */
#define ETHREMOTECFG_TOKEN_NONE                           ((uint32_t)0xFFFFFFFFU)

/*! \brief Remote notify service endpoint Id needed by server to send notify messages */
#define ETHREMOTECFG_NOTIFY_SERVICE_ENDPT_ID              (30U)

/*! \brief Maximum length of input arguments for \ref ETHREMOTECFG_IOCTL */
#define ETHREMOTECFG_IOCTL_INARGS_LEN                     (480U)

/*! \brief Maximum length of output arguments for \ref ETHREMOTECFG_IOCTL */
#define ETHREMOTECFG_IOCTL_OUTARGS_LEN                    (480U)

/* ========================================================================== */
/*                         Structures and Enums                               */
/* ========================================================================== */

/*!
 * \brief Ethernet Firmware commands.
 *
 * The commands supported by Ethernet Firmware.  Initially, remote clients will
 * send a command request message to the server which will carry out the operation
 * and then send the command response back.  Both, the request and response
 * messages will carry one of these command ids.
 */
typedef enum EthRemoteCfg_CmdType_e
{
    /*!
     * \brief Command to offer the virtual ports queried by the remote core.
     *
     * Clients can query EthFw to provide the details regarding allocated virtual switch
     * and virtual MAC ports.
     *
     * Request (C2S): \ref EthRemoteCfg_CommonReq
     * Response (S2C): \ref EthRemoteCfg_OfferVirtPortRes
     */
    ETHREMOTECFG_OFFER_VIRT_PORT,

    /*!
     * \brief Command to attach to the Ethernet device.
     *
     * All remote clients should first attach before issuing any further
     * commands to the Ethernet device.
     *
     * Request (C2S): \ref EthRemoteCfg_AttachReq
     * Response (S2C): \ref EthRemoteCfg_AttachRes
     */
    ETHREMOTECFG_ATTACH,

    /*!
     * \brief Command to attach to the Ethernet device which returns extended
     *        attach info.
     *
     * All remote clients should first attach before issuing any further
     * commands to the Ethernet device.
     *
     * Remote clients that require only a single data path can use a single
     * single \ref ETHREMOTECFG_ATTACH_EXT command which will return all the
     * parameters required to establish data path including destination MAC
     * address, Rx flow, Tx channel.
     *
     * Request (C2S): \ref EthRemoteCfg_AttachReq
     * Response (S2C): \ref EthRemoteCfg_AttachExtRes
     */
    ETHREMOTECFG_ATTACH_EXT,

    /*!
     * \brief Command to detach remote client from the Ethernet device.
     *
     * This is inverse operation of \ref ETHREMOTECFG_ATTACH and
     * \ref ETHREMOTECFG_ATTACH_EXT. All resources allocated to the remote
     * client like Tx channel, Rx flow, MAC address are freed by the server core.
     *
     * Request (C2S): \ref EthRemoteCfg_CommonReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_DETACH,

    /*!
     * \brief Command to query link status of a port.
     *
     * Command allows clients to query for the link status of a port.
     * Server also returns the link speed and duplexity of the virtual port.
     *
     * Request (C2S): \ref EthRemoteCfg_CommonReq
     * Response (S2C): \ref EthRemoteCfg_PortLinkStatusRes
     */
    ETHREMOTECFG_PORT_LINK_STATUS,

    /*!
     * \brief Command to allocate Tx channel.
     *
     * Client should use the allocated Tx channel as Tx DMA channel CPSW PSIL
     * destination thread id when configuring the Tx DMA channel.
     *
     * Request (C2S): \ref EthRemoteCfg_CommonReq
     * Response (S2C): \ref EthRemoteCfg_AllocTxRes
     */
    ETHREMOTECFG_ALLOC_TX,

    /*!
     * \brief Command to allocate Rx flow.
     *
     * Client should use the allocated Rx flow id to configure the DNA Rx flow
     * to establish Rx data flow path on remote client.
     *
     * Request (C2S): \ref EthRemoteCfg_CommonReq
     * Response (S2C): \ref EthRemoteCfg_AllocRxRes
     */
    ETHREMOTECFG_ALLOC_RX,

    /*!
     * \brief Command to allocate a MAC address to the client.
     *
     * Commands allows client to allocate a host port destination MAC address
     * which it can register using \ref ETHREMOTECFG_REGISTER_MAC.
     *
     * Request (C2S): \ref EthRemoteCfg_CommonReq
     * Response (S2C): \ref EthRemoteCfg_AllocMacRes
     */
    ETHREMOTECFG_ALLOC_MAC,

    /*!
     * \brief Command to free previously allocated Tx channel.
     *
     * This is inverse operation of \ref ETHREMOTECFG_ALLOC_TX and frees the
     * previously allocated Tx channels PSIL destination thread.
     *
     * Request (C2S): \ref EthRemoteCfg_FreeTxReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_FREE_TX,

    /*!
     * \brief Command to free previously allocated Rx flow id.
     *
     * This is inverse operation of \ref ETHREMOTECFG_ALLOC_RX and frees the
     * previously allocated Rx flow id.
     *
     * Request (C2S): \ref EthRemoteCfg_FreeRxReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_FREE_RX,

    /*!
     * \brief Command to free previously allocated MAC address.
     *
     * This is inverse operation of \ref ETHREMOTECFG_ALLOC_MAC and frees the
     * previously allocated MAC address.
     *
     * Request (C2S): \ref EthRemoteCfg_FreeMacReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
    */
    ETHREMOTECFG_FREE_MAC,

    /*!
     * \brief Command to register a destination MAC address by the remote
     *        client to a specific Rx flow id.
     *
     * Command allows remote client to register all traffic received on
     * the host port with a specific destination MAC address to be routed to
     * the given Rx flow id.
     *
     * Request (C2S): \ref EthRemoteCfg_MacAddrRxFlowReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_REGISTER_MAC,

    /*!
     * \brief Command to de-register a destination MAC address by the remote
     *        client.
     *
     * This is inverse operation of \ref ETHREMOTECFG_REGISTER_MAC and disables the
     * routing of traffic with given destination MAC address to a specific
     * Rx flow id.
     *
     * Once de-registered further traffic with the given destination MAC address
     * will be routed to default Rx flow.
     *
     * Request (C2S): \ref EthRemoteCfg_MacAddrRxFlowReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_DEREGISTER_MAC,

    /*!
     * \brief Command to register default flow routing to client.
     *
     * Commands allows remote client to receive traffic directed to the default
     * flow.  Default flow is the flow to which Ethernet device will direct
     * traffic if it does not match any classifier which has a thread id
     * configured.
     *
     * Default flow registration is possible only if no core including server
     * core has registered for the default flow.
     *
     * Request (C2S): \ref EthRemoteCfg_RxDefaultFlowRegisterReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_SET_RX_DEFAULTFLOW,

    /*!
     * \brief Command to de-register default flow routing to client.
     *
     * This is inverse operation of \ref ETHREMOTECFG_SET_RX_DEFAULTFLOW and disables
     * the routing of default flow traffic to the given Rx flow id.
     * Once default flow is de-registered, all traffic destined to default flow
     * will be routed to reserved flow and will be dropped.
     *
     * Request (C2S): \ref EthRemoteCfg_RxDefaultFlowRegisterReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_DEL_RX_DEFAULTFLOW,

    /*!
     * \brief Command to associate IPv4 address with MAC address.
     *
     * Command allows server core to register IPv4 address with a MAC address.
     * ARP queries from external nodes will be received on host port. The
     * ARP queries can be routed to only a single core.  This is typically the
     * server core which maintains ARP database of all the IP entries: MAC
     * address mapping used by all clients in all remote cores.
     * The remote client must explicitly register its IP address:MAC address
     * with the server core using this command.
     *
     * Request (C2S): \ref EthRemoteCfg_IPv4AddrRegisterReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_REGISTER_IPv4,

    /*!
     * \brief Command to remove IPv4 address:MAC address mapping
     *
     * Command removes ARP entry of IP address:MAC address mapping.
     * This is inverse operation of \ref ETHREMOTECFG_REGISTER_IPv4.
     *
     * Request (C2S): \ref EthRemoteCfg_IPv4AddrDeregisterReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_DEREGISTER_IPv4,

    /*!
     * \brief Command to add multicast MAC address to receive filter.
     *
     * Command allows remote core client to add a multicast address to
     * receive filter.
     *
     * Server side differentiates three types of multicast addresses:
     * - Exclusive multicast: single client owns multicast traffic.
     *   Packets will be forwarded in hardware via Rx flow.
     * - Shared multicast: traffic is forwarded to multiple core with
     *   server core involvement.
     * - Reserved multicast: clients cannot add them to receive filter,
     *   reserved for server core.
     *
     * Request (C2S): \ref EthRemoteCfg_FilterMacAddReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_ADD_FILTER_MAC,

    /*!
     * \brief Command to delete multicast MAC address from receive filter.
     *
     * Command allows remote client to delete a multicast address from
     * receive filter.
     *
     * Request (C2S): \ref EthRemoteCfg_FilterMacDelReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_DEL_FILTER_MAC,

    /*!
     * \brief Command to enable promiscuous mode.
     *
     * Command allows remote core client to set promiscuous mode.
     * This command is supported only in virtual MAC ports.
     *
     * Request (C2S): \ref EthRemoteCfg_CommonReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_ENABLE_PROMISC,

    /*!
     * \brief Command to disable promiscuous mode.
     *
     * Command allows remote core client to disable promiscuous mode.
     * This command is supported only in virtual MAC ports.
     *
     * Request (C2S): \ref EthRemoteCfg_CommonReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_DISABLE_PROMISC,

    /*!
     * \brief Command to read from an Ethernet device register.
     *
     * Command allows remote core to read a specific register. Server core
     * will check if remote core is permitted to perform register read and
     * will allow register read only if permitted.
     *
     * Request (C2S): \ref EthRemoteCfg_RegReadReq
     * Response (S2C): \ref EthRemoteCfg_RegReadRes
     */
    ETHREMOTECFG_READ_REGISTER,

    /*!
     * \brief Command to write to an Ethernet device register.
     *
     * Command allows remote core to write a specific register. Server core
     * will check if remote core is permitted to perform register write and
     * will allow register write only if permitted.
     *
     * Request (C2S): \ref EthRemoteCfg_RegWriteReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_WRITE_REGISTER,

    /*!
     * \brief Command to setup an EtherType-based packet route.
     *
     * Commands allows remote client to register all traffic received on
     * the host port with a specific EtherType to be routed to the given Rx
     * flow id.
     *
     * Request (C2S): \ref EthRemoteCfg_MatchEthertypeAddReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_REGISTER_MATCH_ETHTYPE,

    /*!
     * \brief Command to tear-down an EtherType-based packet route.
     *
     * This is inverse operation of \ref ETHREMOTECFG_REGISTER_MATCH_ETHTYPE and
     * disables the routing of traffic with given EtherType to a specific
     * Rx flow id.
     *
     * Once unregistered further traffic with the given EtherType will be
     * routed to default Rx flow.
     *
     * Request (C2S): \ref EthRemoteCfg_MatchEthertypeDelReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_DEREGISTER_MATCH_ETHTYPE,

    /*!
     * \brief Command to register a remote timer with EthFw for multicore
     *        time synchronization.
     *
     * Commands allows remote core client to register a timer which it intends
     * to use for multicore time synchronization.  Master core will setup the
     * time sync router for timer periodic events towards CPTS hardware push
     * events.
     *
     * Request (C2S): \ref EthRemoteCfg_RemoteTimerRegisterReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_REGISTER_REMOTE_TIMER,

    /*!
    * \brief Command to de-register a remote timer with EthFw.
    *
    * Commands allows remote core client to de-register a timer used by
    * for multicore time synchronization with CPTS timer.
    *
    * Request (C2S): \ref EthRemoteCfg_RemoteTimerDeregisterReq
    * Response (S2C): \ref EthRemoteCfg_StatusRes
    */
    ETHREMOTECFG_DEREGISTER_REMOTE_TIMER,

    /*!
     * \brief Command to ping the Ethernet device.
     *
     * Command to request the server running on server core to respond to
     * ping request. The server will copy the ping message sent by client and
     * send back ping response.
     *
     * This command is primarily used for debug/heartbeat check purpose.
     *
     * Request (C2S): \ref EthRemoteCfg_PingReq
     * Response (S2C): \ref EthRemoteCfg_PingRes
     */
    ETHREMOTECFG_MESSAGE_PING,

    /*!
     * \brief Command to query the remote configuration server status.
     *
     * Command allows remote client to query what is the current status
     * of the server state machine.
     *
     * Request (C2S): \ref EthRemoteCfg_CommonReq
     * Response (S2C): \ref EthRemoteCfg_ServerStatusRes
     */
    ETHREMOTECFG_GET_SERVER_STATUS,

    /*!
     * \brief Command to invoke ENET LLD IOCTL from remote client
     *
     * Command allows invocation of any ENET LLD IOCTL from the remote core
     * The master core will check if the remote core has permission to invoke
     * the specific IOCTL CMD and IOCTL CMD may fail if remote core does not
     * have the required permission.
     *
     * Request (C2S): \ref EthRemoteCfg_IoctlReq
     * Response (S2C): \ref EthRemoteCfg_IoctlRes
     */
    ETHREMOTECFG_IOCTL,

    /*!
     * \brief Command from remote client to Server to dump CPSW stats
     *
     * Command to dump network statistics, forwarding table, policer table
     * and other server internal tables on master core's UART console.
     *
     * Request (C2S): \ref EthRemoteCfg_CommonReq
     * Response (S2C): \ref EthRemoteCfg_StatusRes
     */
    ETHREMOTECFG_DUMP,

    /*!
     * \brief Last command id.
     */
    ETHREMOTECFG_CMD_TYPE_LAST = ETHREMOTECFG_DUMP,
} EthRemoteCfg_CmdType;

/*!
 * \brief Remote core id used by server to identify the application running
 *        on the core.
 */
typedef enum EthRemoteCfg_ClientId_e
{
    /*! No client */
    ETHREMOTECFG_CLIENTID_NONE = 0U,

    /*! AUTOSAR client id */
    ETHREMOTECFG_CLIENTID_AUTOSAR,

    /*! RTOS client id */
    ETHREMOTECFG_CLIENTID_RTOS,

    /*! Linux client id */
    ETHREMOTECFG_CLIENTID_LINUX,

    /*! QNX client id */
    ETHREMOTECFG_CLIENTID_QNX,

    /*! Last client id */
    ETHREMOTECFG_CLIENTID_LAST = ETHREMOTECFG_CLIENTID_QNX
} EthRemoteCfg_ClientId;

/*!
 * \brief Server-to-client (S2C) notify types.
 */
typedef enum EthRemoteCfg_NotifyType_e
{
    /*! Notify type for sending EthFw version info to the remote clients */
    ETHREMOTECFG_NOTIFYTYPE_FWINFO,

    /*! Notify type for sending CPTS HW push events to the remote clients */
    ETHREMOTECFG_NOTIFYTYPE_HWPUSH,

    /*! Custom notify type */
    ETHREMOTECFG_NOTIFYTYPE_CUSTOM,

    /*! Last notification type */
    ETHREMOTECFG_NOTIFYTYPE_LAST = ETHREMOTECFG_NOTIFYTYPE_CUSTOM
} EthRemoteCfg_NotifyType;

/*!
 * \brief Ethernet Firmware server status.
 */
typedef enum EthRemoteCfg_ServerStatus_e
{
    /*! Ethernet Firmware server is initialized and active */
    ETHREMOTECFG_SERVERSTATUS_INIT,

    /*! Underlying Ethernet device in under recovery */
    ETHREMOTECFG_SERVERSTATUS_RECOVERY,

    /*! Ethernet Firmware server is de-initialized and inactive */
    ETHREMOTECFG_SERVERSTATUS_DEINIT
} EthRemoteCfg_ServerStatus;

/*! 
 * \brief Message type.
 */
typedef enum EthRemoteCfg_MsgType_e
{
    /*! Request message expecting an ACK/response */
    ETHREMOTECFG_MSGTYPE_REQUEST,

    /*! Notify message which notifies the client and expects no ACK */
    ETHREMOTECFG_MSGTYPE_NOTIFY,

    /*! Response message for the request message */
    ETHREMOTECFG_MSGTYPE_RESPONSE
} EthRemoteCfg_MsgType;

/*! @} */

/*!
 * \brief Message header structure.
 *
 * Header structure for any type of messages.
 */
typedef struct EthRemoteCfg_MsgHdr_s
{
    /*! Unique token to identify all operations related to this client */
    uint32_t token;

    /*! Unique client identity for all remote core applications defined
     *  by \ref EthRemoteCfg_ClientId */
    uint32_t clientId;

    /*! Describes if it is a request, response or notify message */
    uint32_t msgType;
} __attribute__((packed)) EthRemoteCfg_MsgHdr;

/*!
 * \brief Request message structure.
 *
 * Structure of every request message and consists of common message header.
 */
typedef struct EthRemoteCfg_ReqHdr_s
{
    /*! Common message header containing details about the remote client */
    EthRemoteCfg_MsgHdr common;

    /*! Request type used to denote the type of request message */
    uint32_t reqType;

    /*! Request id used to compare with respone id, starts from zero and
     *  increments for each request */
    uint32_t reqId;
} __attribute__((packed)) EthRemoteCfg_ReqHdr;

/*!
 * \brief Response message structure
 *
 * Structure of every response message and consists of common message header.
 */
typedef struct EthRemoteCfg_ResHdr_s
{
    /*! Common message header containing details about the remote client */
    EthRemoteCfg_MsgHdr common;

    /*! Response type used to denote the type of respose message */
    uint32_t resType;

    /*! Response id used to compare with the request id */
    uint32_t resId;

    /*! Status of the request */
    int32_t status;
} __attribute__((packed)) EthRemoteCfg_ResHdr;

/*!
 * \brief Notify message structure.
 *
 * Structure of every notify message and consists of common
 * message header - \ref EthRemoteCfg_MsgHdr
 */
typedef struct EthRemoteCfg_NotifyHdr_s
{
    /*! Common message header containing details about the remote client */
    EthRemoteCfg_MsgHdr common;

    /*! Notify type used to denote the type of notify message */
    uint32_t notifyType;
} __attribute__((packed)) EthRemoteCfg_NotifyHdr;

/*!
 * \brief Firmware version info returned by the Ethernet device
 */
typedef struct EthRemoteCfg_FwVerInfo_s
{
    /*! Remote Ethernet device Device API Major version number */
    uint32_t major;

    /*! Remote Ethernet device Device API Minor version number */
    uint32_t minor;

    /*! Remote Ethernet device Device API Revision version number */
    uint32_t rev;

    /*! Remote Ethernet device Device Firmware Build year :
     *  char string in the form YYYY eg: 2019 */
    char year[ETHREMOTECFG_FWDATE_YEARLEN];

    /*! Remote Ethernet device Device Firmware Build month :
     *  char string in the form MON eg: Dec */

    char month[ETHREMOTECFG_FWDATE_MONTHLEN];
    /*! Remote Ethernet device Device Firmware Build month :
     *  char string in the form DD eg: 12 */

    char date[ETHREMOTECFG_FWDATE_DATELEN];
    /*! GIT commit SHA of the firmware: char string in the form fd52c34f */
    char commitHash[ETHREMOTECFG_FW_COMMITSHALEN];
} __attribute__((packed)) EthRemoteCfg_FwVerInfo;

/*!
 * \brief Ethernet device data.
 *
 * The framework will return the below device data to the remote client.
 */
typedef struct EthRemoteCfg_DeviceData_s
{
    /*! Notify message common header */
    EthRemoteCfg_NotifyHdr hdr;

    /*! Ethernet device Firmware version info */
    EthRemoteCfg_FwVerInfo fwVer;

    /*! Flag indicating permission enabled for each command for the
     *  connecting client */
    uint64_t permissionFlags;

    /*! Flag indicating if UART is connected: 1 indicates UART connected,
     *  0 indicates UART not connected  */
    uint32_t uartConnected;

    /*! UART ID used by firmware for log prints */
    uint32_t uartId;
} __attribute__((packed)) EthRemoteCfg_DeviceData;

/*!
 * \brief Common response message structure.
 *
 * Most of the response messages carry the status of the corresponding
 * request message. This structure defines the generic response message
 */
typedef struct EthRemoteCfg_StatusRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;
} __attribute__((packed)) EthRemoteCfg_StatusRes;

/*!
 * \brief Common request message structure.
 *
 * Common message structure for all requests which carry only request header.
 */
typedef struct EthRemoteCfg_CommonReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;
} __attribute__((packed)) EthRemoteCfg_CommonReq;

/*!
 * \brief Response params for \ref ETHREMOTECFG_OFFER_VIRT_PORT command.
 *
 * Virtual port allocation sends the info about the virtual ports assigned
 * to a given remote core.
 */
typedef struct EthRemoteCfg_OfferVirtPortRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! Port mask denoting absolute virtual switch ports allocated */
    uint32_t switchPortMask;

    /*! Port mask denoting absolute virtual MAC ports allocated */
    uint32_t macPortMask;
} __attribute__((packed)) EthRemoteCfg_OfferVirtPortRes;

/*!
 * \brief Request params for \ref ETHREMOTECFG_ATTACH or
 *        \ref ETHREMOTECFG_ATTACH_EXT commands.
 *
 * This requests provides the virtual port for which it is requesting the core
 * attach and recevies and handle called token which will be further used by
 * the client to communicate with the server.  Each token is unique for a
 * given virtual port.
 */
typedef struct EthRemoteCfg_AttachReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Virtual port which needs core attach */
    uint32_t virtPort;
} __attribute__((packed)) EthRemoteCfg_AttachReq;

/*!
 * \brief Response structure holding data for \ref ETHREMOTECFG_ATTACH request
 *
 * This response provides the token handle for the requested virtual port which
 * will be placed in the \ref EthRemoteCfg_ResHdr and MTU info of Tx and Rx.
 */
typedef struct EthRemoteCfg_AttachRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! MTU of Rx packet */
    uint32_t rxMtu;

    /*! MTU of Tx packet per priority */
    uint32_t txMtu[ETHREMOTECFG_PRIORITY_NUM];

    /*! Feature bitmask based on defines \ref EthRemoteCfg_FeatureMask */
    uint32_t features;
} __attribute__((packed)) EthRemoteCfg_AttachRes;

/*!
 * \brief Response params for \ref ETHREMOTECFG_ATTACH_EXT command.
 *
 * This response provides additional information to that of
 * \ref EthRemoteCfg_AttachRes. Additionally it provides the allocated RX flow
 * ids, Tx channel id and MAC address.
 */
typedef struct EthRemoteCfg_AttachExtRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! MTU of Rx packet */
    uint32_t rxMtu;

    /*! MTU of Tx packet per priority */
    uint32_t txMtu[ETHREMOTECFG_PRIORITY_NUM];

    /*! Allocated Rx flow index base */
    uint32_t rxFlowIdxBase;

    /*! Allocated flow index offset */
    uint32_t rxFlowIdxOffset;

    /*! Tx PSIL Peer destination thread id which should be paired with the
     *  Tx UDMA channel */
    uint32_t txPsilDstId;

    /*! MAC address allocated */
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];

    /*! Feature bitmask based on \ref EthRemoteCfg_FeatureMask */
    uint32_t features;
} __attribute__((packed)) EthRemoteCfg_AttachExtRes;

/*!
 * \brief Response params \ref ETHREMOTECFG_ALLOC_RX command.
 *
 * This response holds the information of the  allocated Rx flow which is
 * `rxflowIdxbase` and `rxflowIdxoffset` for the given virtual port.
 */
typedef struct EthRemoteCfg_AllocRxRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! Allocated Rx flow index base */
    uint32_t rxFlowIdxBase;

    /*! Allocated flow index offset */
    uint32_t rxFlowIdxOffset;
} __attribute__((packed)) EthRemoteCfg_AllocRxRes;

/*!
 * \brief Response params for \ref ETHREMOTECFG_ALLOC_TX command.
 *
 * This response holds the information of the allocated Tx channel which is
 * PSIL channel id for the given virtual port.
 */
typedef struct EthRemoteCfg_AllocTxRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! Tx PSIL peer destination thread id which should be paired with the
     *  Tx UDMA channel */
    uint32_t txPsilDstId;
} __attribute__((packed)) EthRemoteCfg_AllocTxRes;

/*!
 * \brief Response params for \ref ETHREMOTECFG_ALLOC_MAC command.
 *
 * This response holds the information of the allocated MAC address for the
 * given virtual port.
 */
typedef struct EthRemoteCfg_AllocMacRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! Allocated MAC address */
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];
} __attribute__((packed)) EthRemoteCfg_AllocMacRes;

/*!
 * \brief Request params for \ref ETHREMOTECFG_FREE_RX command.
 *
 * This request provides the rxflowIdxBase and rxFlowIdxoffset of the rxflowIdx
 * which needs to be freed.
 */
typedef struct EthRemoteCfg_FreeRxReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Rx flow index base */
    uint32_t rxFlowIdxBase;

    /*! Rx flow index offset */
    uint32_t rxFlowIdxOffset;
} __attribute__((packed)) EthRemoteCfg_FreeRxReq;

/*!
 * \brief Request params for \ref ETHREMOTECFG_FREE_TX command.
 *
 * This request provides the TX PSIL destination id of the Tx channel which
 * needs to be freed.
 */
typedef struct EthRemoteCfg_FreeTxReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Tx PSIL Peer destination thread id to be freed */
    uint32_t txPsilDstId;
} __attribute__((packed)) EthRemoteCfg_FreeTxReq;

/*!
 * \brief Request params for \ref ETHREMOTECFG_FREE_MAC command.
 *
 * This request provides the MAC address which needs to be freed.
 */
typedef struct EthRemoteCfg_FreeMacReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! MAC address to be freed */
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];
} __attribute__((packed)) EthRemoteCfg_FreeMacReq;

/*!
 * \brief Request structure for \ref ETHREMOTECFG_REGISTER_MAC or
 *        \ref ETHREMOTECFG_DEREGISTER_MAC commands.
 *
 * This structure holds the MAC address and the RxflowId which needs to be
 * registered for a given virtual port.
 */
typedef struct EthRemoteCfg_MacAddrRxFlowReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! MAC address which needs to be registered */
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];

    /*! Rx flow index Base */
    uint32_t flowIdxBase;

    /*! Rx flow index offset to which the MAC address needs to be registered */
    uint32_t flowIdxOffset;
} __attribute__((packed)) EthRemoteCfg_MacAddrRxFlowReq;

/*!
 * \brief Request structure for \ref ETHREMOTECFG_REGISTER_IPv4 command.
 *
 * This structure holds IPv4 address-MAC address pair which needs to be added
 * to the proxy ARP table of the server.
 */
typedef struct EthRemoteCfg_IPv4AddrRegisterReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! IP address */
    uint8_t ipAddr[ETHREMOTECFG_IPV4ADDRLEN];

    /*! MAC address associated with the IP address which should be added to
     *  the ARP table */
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];
} __attribute__((packed)) EthRemoteCfg_IPv4AddrRegisterReq;

/*!
 * \brief Request structure for \ref ETHREMOTECFG_DEREGISTER_IPv4 command.
 *
 * This structure holds IPv4 address which will be used as a key to
 * remove the entry from the proxy ARP table of the server.
 */
typedef struct EthRemoteCfg_IPv4AddrDeregisterReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! IP address */
    uint8_t ipAddr[ETHREMOTECFG_IPV4ADDRLEN];
} __attribute__((packed)) EthRemoteCfg_IPv4AddrDeregisterReq;

/*!
 * \brief Request params for \ref ETHREMOTECFG_SET_RX_DEFAULTFLOW or
 *        \ref ETHREMOTECFG_DEL_RX_DEFAULTFLOW commands.
 *
 * This structure holds info about the rxFlowIdx which needs to be registered
 * or de-registred as default flow.
 */
typedef struct EthRemoteCfg_RxDefaultFlowRegisterReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Rx flow index Base */
    uint32_t flowIdxBase;

    /*! Rx flow index offset */
    uint32_t flowIdxOffset;
} __attribute__((packed)) EthRemoteCfg_RxDefaultFlowRegisterReq;

/*!
 * \brief Response params for \ref ETHREMOTECFG_PORT_LINK_STATUS command.
 *
 * Response to a periodic request sent by the client for a given virtual port
 * where the server sends back the link status and the config params
 * (link speed, duplexity) of the port.
 */
typedef struct EthRemoteCfg_PortLinkStatusRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! Link status of the port */
    bool isLinked;

    /*! Link speed */
    uint32_t speed;

    /*! Duplexity */
    uint32_t duplexity;
} __attribute__((packed)) EthRemoteCfg_PortLinkStatusRes;

/*!
 * \brief Request params for \ref ETHREMOTECFG_REGISTER_MATCH_ETHTYPE command.
 *
 * Request message structure for setting an Ethertype classifier for a given
 * Rx flow, hence takes EtherType and the RX flow (base and offset) values as
 * input arguments.
 */
typedef struct EthRemoteCfg_MatchEthertypeAddReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! EtherType to be associated with the flow */
    uint16_t ethertype;

    /*! Rx flow index Base */
    uint32_t flowIdxBase;

    /*! Rx flow index offset */
    uint32_t flowIdxOffset;
} __attribute__((packed)) EthRemoteCfg_MatchEthertypeAddReq;

/*!
 * \brief Request params for \ref ETHREMOTECFG_DEREGISTER_MATCH_ETHTYPE command.
 *
 * Request message structure for deleting the EtherType classifier which has
 * been set for a given Rx flow.
 */
typedef struct EthRemoteCfg_MatchEthertypeDelReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! EtherType to be associated with the flow */
    uint16_t ethertype;
} __attribute__((packed)) EthRemoteCfg_MatchEthertypeDelReq;

/*!
 * \brief Request params for \ref ETHREMOTECFG_ADD_FILTER_MAC command.
 *
 * This structure holds vlanId, MAC address and Rx flowIdx details used for
 * adding multicast MAC address to the receive filter.
 */
typedef struct EthRemoteCfg_FilterMacAddReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Multicast MAC address to be added */
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];

    /*! VLAN id */
    uint16_t vlanId;

    /*! Rx flow index base from which the MAC_address association will be added.
     *  It's applicable only for _exclusive multicast traffic_ */
    uint32_t flowIdxBase;

    /*! Rx flow index offset */
    uint32_t flowIdxOffset;
} __attribute__((packed)) EthRemoteCfg_FilterMacAddReq;

/*!
 * \brief Request params for \ref ETHREMOTECFG_DEL_FILTER_MAC command.
 *
 * This structure holds vlanId, MAC address which will be used for deleting
 * multicast MAC address from the receive filter.
 */
typedef struct EthRemoteCfg_FilterMacDelReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Multicast MAC address to be added */
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];

    /*! VLAN id */
    uint16_t vlanId;
} __attribute__((packed)) EthRemoteCfg_FilterMacDelReq;

/*!
 * \brief Request params for \ref ETHREMOTECFG_MESSAGE_PING command.
 *
 * This structure holds some ping data which will be sent back to the client.
 */
typedef struct EthRemoteCfg_PingReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Data which will be responded back by the server */
    uint32_t data[ETHREMOTECFG_MESSAGE_DATA_LEN];
} __attribute__((packed)) EthRemoteCfg_PingReq;

/*!
 * \brief Response params for \ref ETHREMOTECFG_MESSAGE_PING command.
 *
 * This structure holds some ping data which will be recieved back to the
 * client by the server.
 */
typedef struct EthRemoteCfg_PingRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! Data which will be responded back by the server */
    uint32_t data[ETHREMOTECFG_MESSAGE_DATA_LEN];
} __attribute__((packed)) EthRemoteCfg_PingRes;

/*!
 * \brief Request params for \ref ETHREMOTECFG_REGISTER_REMOTE_TIMER command.
 *
 * This request structure contains hardware push number and timerId for
 * registering HWPUSH callback and setting TimeSyncRouter.
 */
typedef struct EthRemoteCfg_RemoteTimerRegisterReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Hardware Push Number to be used for timesync router configuration */
    uint8_t hWPushNum;

    /*! Timer Id to be used for timesync router configuration */
    uint8_t timerId;
} __attribute__((packed)) EthRemoteCfg_RemoteTimerRegisterReq;

/*!
 * \brief Request params for \ref ETHREMOTECFG_DEREGISTER_REMOTE_TIMER command.
 *
 * This request structure contains hardware push number de-registering HWPUSH
 * callback and clearing TimeSyncRouter.
 */
typedef struct EthRemoteCfg_RemoteTimerDeregisterReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Hardware Push number to be used for timesync router configuration */
    uint8_t hWPushNum;
} __attribute__((packed)) EthRemoteCfg_RemoteTimerDeregisterReq;

/*!
 * \brief Request params for \ref ETHREMOTECFG_IOCTL command.
 *
 * This is request structure holding the CMD, inArgs, inArgsLen, outArgsLen,
 * which is required for calling an IOCTL request. This request returns a
 * response providing the outArgs of the corresponding IOCTL command.
 */
typedef struct EthRemoteCfg_IoctlReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Enet LLD IOCTL command id. Refer Enet LLD documentation for list of
     *  available IOCTLs */
    uint32_t cmd;

    /*! Enet LLD IOCTL command input arguments length */
    uint32_t inArgsLen;

    /*! Enet LLD IOCTL command input arguments. Byte array is typecast to the
     *  inArgs structure associated with the IOCTL */
    uint64_t inArgs[(ETHREMOTECFG_IOCTL_INARGS_LEN / sizeof(uint64_t))];

    /*! Enet LLD IOCTL command output arguments length */
    uint32_t outArgsLen;
}  __attribute__((packed)) EthRemoteCfg_IoctlReq;

/*!
 * \brief Response params for \ref ETHREMOTECFG_IOCTL command.
 *
 * This is response structure for \ref EthRemoteCfg_IoctlReq.
 */
typedef struct EthRemoteCfg_IoctlRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! Enet LLD IOCTL command id. Refer Enet LLD documentation for list of
     *  available IOCTLs */
    uint32_t cmd;

    /*! Enet LLD IOCTL command output arguments length */
    uint32_t outArgsLen;

    /*! Enet LLD IOCTL command output arguments. Byte array is typecast to the
     *  outArgs structure associated with the IOCTL */
    uint64_t outArgs[(ETHREMOTECFG_IOCTL_OUTARGS_LEN / sizeof(uint64_t))];
}  __attribute__((packed)) EthRemoteCfg_IoctlRes;

/*!
 * \brief Request params for ETHREMOTECFG_REGISTER_WRITE command.
 *
 * Request structure holds the address of the register and the value which
 * needs to be written into the register.
 */
typedef struct EthRemoteCfg_RegWriteReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Register address */
    uint32_t addr;

    /*! Value which needs to be written */
    uint32_t val;
} __attribute__((packed)) EthRemoteCfg_RegWriteReq;

/*!
 * \brief Request params for ETHREMOTECFG_REGISTER_READ command.
 *
 * Request structure holds the address of the register which needs to be
 * read and returns the value of the register as response.
 */
typedef struct EthRemoteCfg_RegReadReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;

    /*! Register address */
    uint32_t addr;
} __attribute__((packed)) EthRemoteCfg_RegReadReq;

/*!
 * \brief Request params for ETHREMOTECFG_REGISTER_READ command.
 *
 * The response structure for EthRemoteCfg_RegReadReq.
 */
typedef struct EthRemoteCfg_RegReadRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! Value which has been read */
    uint32_t val;
} __attribute__((packed)) EthRemoteCfg_RegReadRes;

/*!
 * \brief Response params for \ref ETHREMOTECFG_GET_SERVER_STATUS command.
 *
 * This response returns the status of Ethernet Firmware server, see
 * \ref EthRemoteCfg_ServerStatus.
 */
typedef struct EthRemoteCfg_ServerStatusRes_s
{
    /*! Response message common header */
    EthRemoteCfg_ResHdr hdr;

    /*! Server status */
    uint32_t status;
} __attribute__((packed)) EthRemoteCfg_ServerStatusRes;

/*!
 * \brief Request params for \ref ETHREMOTECFG_DUMP command.
 */
typedef struct EthRemoteCfg_DumpReq_s
{
    /*! Request message common header */
    EthRemoteCfg_ReqHdr hdr;
} __attribute__((packed)) EthRemoteCfg_DumpReq;

/*!
 * \brief Notify params for \ref ETHREMOTECFG_NOTIFYTYPE_HWPUSH notification.
 *
 * This is S2C notify message which notifies the remote client everytime there
 * is a hardware push notification with a event timestamp for a given
 * hWPushNum.
 */
typedef struct EthRemoteCfg_NotifyServiceHwPushMsg_s
{
    /*! Common notify message header */
    EthRemoteCfg_NotifyHdr hdr;

    /*! CPTS hardware push number */
    uint32_t hwPushNum;

    /*! CPTS hardware push event timestamp  */
    uint64_t timeStamp;
} __attribute__((packed)) EthRemoteCfg_NotifyServiceHwPushMsg;

/* ========================================================================== */
/*                         Global Variables Declarations                      */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                        Deprecated Function Declarations                    */
/* ========================================================================== */

/* None */

#ifdef __cplusplus
}
#endif

/*! @} */

#endif /* ETHREMOTECFG_H_ */
