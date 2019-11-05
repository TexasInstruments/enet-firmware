/*
 *
 * Copyright (c) 2019 Texas Instruments Incorporated
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

#ifndef __ETHREMOTECFG_SERVER_H__
#define __ETHREMOTECFG_SERVER_H__

#include <stdint.h>
#include <string.h>

#include <ethremotecfg/protocol/rpmsg-kdrv-transport-ethswitch.h>

/**
 * \defgroup ETHSWITCH_REMOTE_DEVICE_SERVER_API Ethernet Switch Remote Device Server APIs
 *
 * \brief This section contains APIs for Ethernet Switch Remote Device Server APIs
 *
 * The ethernet switch remote device server resides on the master core and enables
 * clients on remote cores to configure the Ethernet Switch
 *
 *  @{
 */
/* @} */
/*!
 * \addtogroup ETHSWITCH_REMOTE_DEVICE_SERVER_API
 * @{
 */

/*! \brief Max length of ethernet switch remote device exported name 
 *
 * The Remote Device Server advertises a name to each of the remote cores
 * it supports connecting to the server.
 */
#define ETHREMOTECFG_SERVER_MAX_NAME_LEN         (128)

/*! Max number of remote clients supported by ethernet switch remote device server */
#define ETHREMOTECFG_SERVER_MAX_INSTANCES        (4)

/*! Ethernet Switch Remote Device Name Advertised on MCU_2_1 core.Client on MCU_2_1 core should connect with this name */
#define ETHREMOTEDEVICE_DEVICE_NAME_MCU_2_1 "mcu_2_1_ethswitch-device-0"

/*! Ethernet Switch Remote Device Name Advertised on MPU_1_0 core.Client on MPU_1_0 core should connect with this name */
#define ETHREMOTEDEVICE_DEVICE_NAME_MPU_1_0 "mpu_1_0_ethswitch-device-0"

/*! Service name of the remote device framework */
#define ETHREMOTEDEVICE_REMOTEDEVICE_FRAMEWORK_SERVICE "rpmsg-kdrv"

/*!
 * \brief Ethernet Switch Remote device server instance initialization parameters
 *
 * Application on the master core creates one instance of the Ethernet Switch 
 * Remote device.
 */
typedef struct rdevEthSwitchServerInstPrm_s
{
    uint32_t host_id;                               /**< Host Id that should connect to this device */
    uint8_t name[ETHREMOTECFG_SERVER_MAX_NAME_LEN]; /**< Exported name */
} rdevEthSwitchServerInstPrm_t;

/**
 *  \name Ethernet Switch Remote Device Server App Callback function pointers
 *
 *  On recieving a ethernet switch remote device command from  a remote core, the
 *  server will invoke application registered callback for each CMD
 *  @{
 */
/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_ATTACH 
 *  \param host_id Remote Core Id
 *  \param cpsw type CPSW TYPE of type enum rpmsg_kdrv_ethswitch_cpsw_type
 *  \param pId Pointer to Unique Opaque Handle populated by callback handler 
 *  \param pCoreKey Pointer to Core key populated by callback handler
 *  \param pRxMtu   Pointer to Maximum receive packet length . Populated by callback handler
 *  \param pTxMtu   Array of Maximum transmit packet length per priority supported by ethernet switch
 *  \param txMtuArraySize Number of priority supported
 *  \param pFeatures Pointer to feature bitmap. Bitmask of type RPMSG_KDRV_TP_ETHSWITCH_FEATURE_xxx
*/
typedef int32_t (*ethrdev_srv_cb_attach_handler_t)(uint32_t host_id, uint8_t cpsw_type, uint64_t *pId, uint32_t *pCoreKey, uint32_t *pRxMtu, uint32_t *pTxMtu, uint32_t txMtuArraySize,
                                                   uint32_t *pFeatures);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_ATTACH_EXT
 *  \param host_id Remote Core Id
 *  \param cpsw type CPSW TYPE of type enum rpmsg_kdrv_ethswitch_cpsw_type
 *  \param pId Pointer to Unique Opaque Handle populated by callback handler 
 *  \param pCoreKey Pointer to Core key populated by callback handler
 *  \param pRxMtu   Pointer to Maximum receive packet length . Populated by callback handler
 *  \param pTxMtu   Array of Maximum transmit packet length per priority supported by ethernet switch
 *  \param txMtuArraySize Number of priority supported
 *  \param pFeatures Pointer to feature bitmap. Bitmask of type RPMSG_KDRV_TP_ETHSWITCH_FEATURE_xxx
 *  \param pAllocFlowIdx Pointer to allocated Rx Flow Index populated by callback handler
 *  \param pTxCpswPsilDstId Pointer to allocated Tx Channel CPSW PSIL destination thread id populated by callback handler
 *  \param macAddress  Pointer to allocated destination mac address allocated to remote core populated by callback handler
*/
typedef int32_t (*ethrdev_srv_cb_attach_ext_handler_t)(uint32_t host_id, uint8_t cpsw_type, uint64_t *pId, uint32_t *pCoreKey, uint32_t *pRxMtu, uint32_t *pTxMtu, uint32_t txMtuArraySize,
                                                       uint32_t *pFeatures, uint32_t *pAllocFlowIdx, uint32_t *pTxCpswPsilDstId, uint8_t *macAddress);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_ALLOC_TX
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param pTxCpswPsilDstId Pointer to allocated Tx Channel CPSW PSIL destination thread id populated by callback handler
*/
typedef int32_t (*ethrdev_srv_cb_alloc_tx_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t *pTxCpswPsilDstId);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_ALLOC_RX
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param pAllocFlowIdx Pointer to allocated Rx Flow Index populated by callback handler
*/
typedef int32_t (*ethrdev_srv_cb_alloc_rx_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t *pAllocFlowIdx);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_ALLOC_MAC
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param macAddress  Pointer to allocated destination mac address allocated to remote core populated by callback handler
*/
typedef int32_t (*ethrdev_srv_cb_alloc_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *mac_address);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_REGISTER_MAC
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param macAddress  Destination mac address to be registered
 *  \param flow_idx  Rx Flow Index to eb associated with the destination MAC address
*/
typedef int32_t (*ethrdev_srv_cb_register_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *mac_address, uint32_t flow_idx);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_MAC
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param macAddress  Destination mac address to be unregistered
 *  \param flow_idx  Rx Flow Index to eb disassociated with the destination MAC address
*/
typedef int32_t (*ethrdev_srv_cb_unregister_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *mac_address, uint32_t flow_idx);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_REGISTER_DEFAULTFLOW
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param flow_idx  Rx Flow Index to which default flow traffic will be routed
*/
typedef int32_t (*ethrdev_srv_cb_register_rx_default_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t flow_idx);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_DEFAULTFLOW
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param flow_idx  Rx Flow Index to which default flow mapping is to be removed.
*/
typedef int32_t (*ethrdev_srv_cb_unregister_rx_default_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t flow_idx);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_FREE_TX
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param tx_cpsw_psil_dst_id Allocated Tx Channel CPSW PSIL destination thread id to be freed
*/
typedef int32_t (*ethrdev_srv_cb_free_tx_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t tx_cpsw_psil_dst_id);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_FREE_RX
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param alloc_flow_idx Allocated Rx Flow Index to be freed
*/
typedef int32_t (*ethrdev_srv_cb_free_rx_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t alloc_flow_idx);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_FREE_MAC
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param mac_address Destination mac address to be freed
*/
typedef int32_t (*ethrdev_srv_cb_free_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, u8 *mac_address);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_DETACH
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 */
typedef int32_t (*ethrdev_srv_cb_detach_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_IOCTL
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param cmd IOCTL id supported by CPSW LLD. Refer CPSW LLD documentation for list of CPSW LLD IOCTLs
 *  \param inargs CPSW IOCTL CMD input arguments .Byte array is typecast to the inArgs structure associated with the IOCTL
 *  \param inargs_len CPSW IOCTL CMD input arguments length
 *  \param outargs CPSW IOCTL CMD output arguments .Byte array is typecast to the outArgs structure associated with the IOCTL. Populated by callback handler
 *  \param outargs_len CPSW IOCTL CMD output arguments length
 */
typedef int32_t (*ethrdev_srv_cb_ioctl_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, u32 cmd, const u8 *inargs, u32 inargs_len, u8 *outargs, uint32_t outargs_len);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_REGWR
 *  \param host_id Remote Core Id
 *  \param regaddr Register address to be written to
 *  \param regval Register value to be written
 *  \param pRegval Pointer to register value after register write. Populated by callback handler
 */
typedef int32_t (*ethrdev_srv_cb_regwr_handler_t)(uint32_t host_id, uint32_t regaddr, uint32_t regval, uint32_t *pRegval);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_REGRD
 *  \param host_id Remote Core Id
 *  \param regaddr Register address to be read from
 *  \param pRegval Pointer to register value read from regaddr. Populated by callback handler
 */
typedef int32_t (*ethrdev_srv_cb_regrd_handler_t)(uint32_t host_id, uint32_t regaddr, uint32_t *pRegval);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_IPV4_MAC_REGISTER
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param mac_address  Destination mac address with which the IPv4 address will be associated in the ARP database
 *  \param ipv4_addr  IPv4 address to be added  in the ARP database with associated MAC address
 */
typedef int32_t (*ethrdev_srv_cb_register_ipv4_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *mac_address, uint8_t *ipv4_addr);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_IPV6_MAC_REGISTER
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param mac_address  Destination mac address with which the IPv6 address will be associated in the ARP database
 *  \param ipv6_addr  IPv6 address to be added  in the ARP database with associated MAC address
 */
typedef int32_t (*ethrdev_srv_cb_register_ipv6_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *mac_address, uint8_t *ipv6_addr);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_IPV4_MAC_UNREGISTER
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param ipv4_addr  IPv4 address to be removed from  the ARP database
 */
typedef int32_t (*ethrdev_srv_cb_unregister_ipv4_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *ipv4_addr);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_IPV6_MAC_UNREGISTER
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param ipv6_addr  IPv6 address to be removed from  the ARP database
 */
typedef int32_t (*ethrdev_srv_cb_unregister_ipv6_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *ipv6_addr);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_C2S_NOTIFY
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param notifyid  Client to server notify id of type enum rpmsg_kdrv_ethswitch_client_notify_type
 *  \param notify_info Notify info associated with the notify id
 *  \param notify_info_len Notify info length
 */
typedef void (*ethrdev_srv_cb_client_notify_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, enum rpmsg_kdrv_ethswitch_client_notify_type notifyid, uint8_t *notify_info,
                                                       uint32_t notify_info_len);

/*! Server Callback Handler for Remote Device Framework Device attach
 *  \param host_id Remote Core Id
 *  \param eth_dev_data Ethernet Switch Remote Device Data to be populated by the callback handler
 */
typedef void (*ethrdev_srv_cb_init_device_data_t)(uint32_t host_id, struct rpmsg_kdrv_ethswitch_device_data *eth_dev_data);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_REGISTER_ETHTYPE
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param ether_type  Ethertype to be registered
 *  \param flow_idx  Rx Flow Index to eb associated with the destination MAC address
*/
typedef int32_t (*ethrdev_srv_cb_register_ethertype_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint16_t ether_type, uint32_t flow_idx);

/*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_ETHTYPE
 *  \param host_id Remote Core Id
 *  \param handle Unique Opaque Handle returned by attach / attach ext CMD
 *  \param core_key  Core key returned by attach / attach ext CMD
 *  \param ether_type  Ethertype to be unregistered
 *  \param flow_idx  Rx Flow Index to eb disassociated with the destination MAC address
*/
typedef int32_t (*ethrdev_srv_cb_unregister_ethertype_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint16_t ether_type, uint32_t flow_idx);


/*  @} */

/*! \brief Ethernet Switch Remote Device Server Callback function table 
 *
 *  The application instantiates a Ethernet Switch Remote Device Server instance
 *  passing a callback function pointer table
 *  Each function pointer is ethernet switch remote device CMD handler which
 *  is called by the server on receiving the associated command
 */
typedef struct rdevEthSwitchServerCbFxn_s
{
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_ATTACH  */
    ethrdev_srv_cb_attach_handler_t attach_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_ATTACH_EXT  */
    ethrdev_srv_cb_attach_ext_handler_t attach_ext_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_ALLOC_TX */
    ethrdev_srv_cb_alloc_tx_handler_t alloc_tx_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_ALLOC_RX */
    ethrdev_srv_cb_alloc_rx_handler_t alloc_rx_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_ALLOC_MAC */
    ethrdev_srv_cb_alloc_mac_handler_t alloc_mac_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_REGISTER_MAC */
    ethrdev_srv_cb_register_mac_handler_t register_mac_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_MAC */
    ethrdev_srv_cb_unregister_mac_handler_t unregister_mac_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_REGISTER_DEFAULTFLOW */
    ethrdev_srv_cb_register_rx_default_handler_t register_rx_default_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_DEFAULTFLOW */
    ethrdev_srv_cb_unregister_rx_default_handler_t unregister_rx_default_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_FREE_TX */
    ethrdev_srv_cb_free_tx_handler_t free_tx_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_FREE_RX */
    ethrdev_srv_cb_free_rx_handler_t free_rx_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_FREE_MAC */
    ethrdev_srv_cb_free_mac_handler_t free_mac_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_DETACH */
    ethrdev_srv_cb_detach_handler_t detach_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_IOCTL */
    ethrdev_srv_cb_ioctl_handler_t ioctl_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_REGWR */
    ethrdev_srv_cb_regwr_handler_t regwr_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_REGRD */
    ethrdev_srv_cb_regrd_handler_t regrd_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_IPV4_MAC_REGISTER */
    ethrdev_srv_cb_register_ipv4_mac_handler_t ipv4_register_mac_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_IPV6_MAC_REGISTER */
    ethrdev_srv_cb_register_ipv6_mac_handler_t ipv6_register_mac_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_IPV4_MAC_UNREGISTER */
    ethrdev_srv_cb_unregister_ipv4_mac_handler_t ipv4_unregister_mac_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_IPV6_MAC_UNREGISTER */
    ethrdev_srv_cb_unregister_ipv6_mac_handler_t ipv6_unregister_mac_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_C2S_NOTIFY */
    ethrdev_srv_cb_client_notify_handler_t client_notify_handler;
    /*! Server Callback Handler for Remote Device Framework Device attach */
    ethrdev_srv_cb_init_device_data_t init_device_data_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_REGISTER_ETHTYPE */
    ethrdev_srv_cb_register_ethertype_handler_t register_ethertype_handler;
    /*! Server Callback Handler for RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_ETHTYPE */
    ethrdev_srv_cb_unregister_ethertype_handler_t unregister_ethertype_handler;

} rdevEthSwitchServerCbFxn_t;

/*!
 * \brief Ethernet Switch Remote device server instance initialization parameters
 */
typedef struct rdevEthSwitchServerInitPrm_s
{
    uint32_t num_instances;                                                   /**< Number of client supported */
    rdevEthSwitchServerInstPrm_t inst_prm[ETHREMOTECFG_SERVER_MAX_INSTANCES]; /**< List of client cores and associated names from which server attach is supported */
    uint32_t rpmsg_buf_size;                                                  /**< Max size of message */
    rdevEthSwitchServerCbFxn_t cb;                                            /**< Ethernet Switch Remote Device Server Callback function table */
} rdevEthSwitchServerInitPrm_t;

/*!
 * \brief Union of all ethswitch remote device messages. Used internally in the server/client implementation
 */
typedef union rdevEthSwitchServerMessageList_u
{
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_ATTACH command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_attach_request attach_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_ATTACH command . Reply from server to client */
    struct rpmsg_kdrv_ethswitch_attach_response attach_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_ATTACH_EXT command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_attach_extended_request attach_ext_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_ATTACH_EXT command . Reply from server to client */
    struct rpmsg_kdrv_ethswitch_attach_extended_response attach_ext_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_ALLOC_TX/RPMSG_KDRV_TP_ETHSWITCH_ALLOC_RX/RPMSG_KDRV_TP_ETHSWITCH_ALLOC_MAC command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_alloc_request alloc_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_ALLOC_RX command . Reply from server to client */
    struct rpmsg_kdrv_ethswitch_alloc_rx_response alloc_rx_res;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_ALLOC_TX command . Reply from server to client */
    struct rpmsg_kdrv_ethswitch_alloc_tx_response alloc_tx_res;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_ALLOC_MAC command . Reply from server to client */
    struct rpmsg_kdrv_ethswitch_alloc_mac_response alloc_mac_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_REGISTER_MAC command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_register_mac_request register_mac_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_REGISTER_MAC command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_register_mac_response register_mac_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_MAC command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_unregister_mac_request unregister_mac_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_MAC command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_unregister_mac_response unregister_mac_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_REGISTER_DEFAULTFLOW command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_register_rx_default_request register_rx_default_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_REGISTER_DEFAULTFLOW command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_register_rx_default_response register_rx_default_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_DEFAULTFLOW command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_unregister_rx_default_request unregister_rx_default_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_DEFAULTFLOW command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_unregister_rx_default_response unregister_rx_default_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_FREE_MAC command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_free_mac_request free_mac_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_FREE_MAC command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_free_mac_response free_mac_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_FREE_TX command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_free_tx_request free_tx_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_FREE_TX command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_free_tx_response free_tx_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_FREE_RX command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_free_rx_request free_rx_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_FREE_RX command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_free_rx_response free_rx_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_DETACH command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_detach_request detach_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_DETACH command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_detach_response detach_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_IOCTL command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_ioctl_request ioctl_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_IOCTL command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_ioctl_response ioctl_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_REGWR command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_regwr_request regwr_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_REGWR command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_regwr_response regwr_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_REGRD command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_regrd_request regrd_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_REGRD command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_regrd_response regrd_res;
    /*! Request Message associated with Remote Device Framework device attach command. Sent from server to client  */
    struct rpmsg_kdrv_ethswitch_device_data device_data;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_PING_REQUEST command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_ping_request ping_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_PING_REQUEST command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_ping_response ping_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_IPV4_MAC_REGISTER command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_ipv4_register_mac_request ipv4_register_mac_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_IPV4_MAC_REGISTER command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_ipv4_register_mac_response ipv4_register_mac_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_IPV6_MAC_REGISTER command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_ipv6_register_mac_request ipv6_register_mac_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_IPV6_MAC_REGISTER command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_ipv6_register_mac_response ipv6_register_mac_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_IPV4_MAC_UNREGISTER command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_ipv4_unregister_mac_request ipv4_unregister_mac_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_IPV4_MAC_UNREGISTER command. Reply from server to client */
    struct rpmsg_kdrv_ethswitch_ipv4_unregister_mac_response ipv4_unregister_mac_res;
    /*! Message associated with Server to Client Notify CMD */
    struct rpmsg_kdrv_ethswitch_s2c_notify s2c_notify;
    /*! Message associated with Client to Server Notify CMD */
    struct rpmsg_kdrv_ethswitch_c2s_notify c2s_notify;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_REGISTER_ETHTYPE command. Sent from client to server */
    struct rpmsg_kdrv_ethswitch_register_ethertype_request register_ethertype_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_REGISTER_ETHTYPE command. Sent from server to client */
    struct rpmsg_kdrv_ethswitch_register_ethertype_response register_ethertype_res;
    /*! Request Message associated with RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_ETHTYPE command. Sent from client to server  */
    struct rpmsg_kdrv_ethswitch_unregister_ethertype_request unregister_ethertype_req;
    /*! Response Message associated with RPMSG_KDRV_TP_ETHSWITCH_UNREGISTER_ETHTYPE command. Sent from server to client  */
    struct rpmsg_kdrv_ethswitch_unregister_ethertype_response unregister_ethertype_res;
} __packed rdevEthSwitchServerMessageList_t;

/**
 * \brief Set EThernet Switch Remote Server init parameters to default state
 *
 * Recommend to call this API before callnig rdevEthSwitchServerInit.
 *
 * \param prm [out] Parameters set to default
 */
static void rdevEthSwitchServerInitPrmSetDefault(rdevEthSwitchServerInitPrm_t *prm)
{
    memset(prm, 0, sizeof(*prm));

    prm->rpmsg_buf_size = 256;
    prm->num_instances = 0;
}

/**
 * \brief Initialize ethernet switch remote device instance
 *
 * \param prm [in] Initialization parameters
 *
 * \return 0 on success, else failure
 */
int32_t rdevEthSwitchServerInit(rdevEthSwitchServerInitPrm_t *prm);

#endif

/* @} */
