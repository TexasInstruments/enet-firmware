/*
 *
 * Copyright (c) 2017 Texas Instruments Incorporated
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

#include <ethremotecfg/protocol/rpmsg-kdrv-transport-ethswitch.h>

/**
 * \defgroup group_vision_apps_utils_remote_disp Remote Demo APIs
 *
 * \brief This section contains APIs for Remote Demo framework
 *
 * \ingroup group_libs
 *
 * @{
 */

/** \brief Max length of remote demo exported name */
#define ETHREMOTECFG_SERVER_MAX_NAME_LEN         (128)

/** \brief Max number of remote demo instances */
#define ETHREMOTECFG_SERVER_MAX_INSTANCES        (4)

/** \brief Max length of remote demo data */
#define ETHREMOTECFG_SERVER_MAX_DATA_LEN        (128)

#define ETHREMOTEDEVICE_DEVICE_NAME_MCU_2_1 "mcu_2_1_ethswitch-device-0"

#define ETHREMOTEDEVICE_DEVICE_DATA_MCU_2_1 "mcu_2_1_ethswitch-device-0-data"

#define ETHREMOTEDEVICE_DEVICE_NAME_MPU_1_0 "mpu_1_0_ethswitch-device-0"

#define ETHREMOTEDEVICE_DEVICE_DATA_MPU_1_0 "mpu_1_0_ethswitch-device-0-data"

/**
 * \brief Remote demo instance initialization parameters
 */
typedef struct rdevEthSwitchServerInstPrm_s
{
    uint32_t host_id;                               /**< Host Id that should connect to this device */
    uint8_t name[ETHREMOTECFG_SERVER_MAX_NAME_LEN]; /**< Exported name */
} rdevEthSwitchServerInstPrm_t;

typedef int32_t (*ethrdev_srv_cb_attach_handler_t)(uint32_t host_id, uint8_t cpsw_type, uint64_t *pId, uint32_t *pCoreKey, uint32_t *pRxMtu, uint32_t *pTxMtu, uint32_t txMtuArraySize,
                                                   uint32_t *pFeatures);
typedef int32_t (*ethrdev_srv_cb_attach_ext_handler_t)(uint32_t host_id, uint8_t cpsw_type, uint64_t *pId, uint32_t *pCoreKey, uint32_t *pRxMtu, uint32_t *pTxMtu, uint32_t txMtuArraySize,
                                                       uint32_t *pFeatures, uint32_t *pAllocFlowIdx, uint32_t *pTxCpswPsilDstId, uint8_t *macAddress);
typedef int32_t (*ethrdev_srv_cb_alloc_tx_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t *pTxCpswPsilDstId);
typedef int32_t (*ethrdev_srv_cb_alloc_rx_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t *pAllocFlowIdx);
typedef int32_t (*ethrdev_srv_cb_alloc_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *mac_address);
typedef int32_t (*ethrdev_srv_cb_register_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *mac_address, uint32_t flow_idx);
typedef int32_t (*ethrdev_srv_cb_unregister_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *mac_address, uint32_t flow_idx);
typedef int32_t (*ethrdev_srv_cb_register_rx_default_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t flow_idx);
typedef int32_t (*ethrdev_srv_cb_unregister_rx_default_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t flow_idx);
typedef int32_t (*ethrdev_srv_cb_free_tx_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t tx_cpsw_psil_dst_id);
typedef int32_t (*ethrdev_srv_cb_free_rx_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint32_t alloc_flow_idx);
typedef int32_t (*ethrdev_srv_cb_free_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, u8 *mac_address);
typedef int32_t (*ethrdev_srv_cb_detach_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key);
typedef int32_t (*ethrdev_srv_cb_ioctl_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, u32 cmd, const u8 *inargs, u32 inargs_len, u8 *outargs, uint32_t outargs_len);
typedef int32_t (*ethrdev_srv_cb_regwr_handler_t)(uint32_t host_id, uint32_t regaddr, uint32_t regval, uint32_t *pRegval);
typedef int32_t (*ethrdev_srv_cb_regrd_handler_t)(uint32_t host_id, uint32_t regaddr, uint32_t *pRegval);
typedef int32_t (*ethrdev_srv_cb_register_ipv4_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *mac_address, uint8_t *ipv4_addr);
typedef int32_t (*ethrdev_srv_cb_register_ipv6_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *mac_address, uint8_t *ipv6_addr);
typedef int32_t (*ethrdev_srv_cb_unregister_ipv4_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *ipv4_addr);
typedef int32_t (*ethrdev_srv_cb_unregister_ipv6_mac_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, uint8_t *ipv6_addr);
typedef void (*ethrdev_srv_cb_client_notify_handler_t)(uint32_t host_id, uint64_t handle, uint32_t core_key, enum rpmsg_kdrv_ethswitch_client_notify_type notifyid, uint8_t *notify_info,
                                                       uint32_t notify_info_len);
typedef void (*ethrdev_srv_cb_init_device_data_t)(uint32_t host_id, struct rpmsg_kdrv_ethswitch_device_data *eth_dev_data);

typedef struct rdevEthSwitchServerCbFxn_s
{
    ethrdev_srv_cb_attach_handler_t attach_handler;
    ethrdev_srv_cb_attach_ext_handler_t attach_ext_handler;
    ethrdev_srv_cb_alloc_tx_handler_t alloc_tx_handler;
    ethrdev_srv_cb_alloc_rx_handler_t alloc_rx_handler;
    ethrdev_srv_cb_alloc_mac_handler_t alloc_mac_handler;
    ethrdev_srv_cb_register_mac_handler_t register_mac_handler;
    ethrdev_srv_cb_unregister_mac_handler_t unregister_mac_handler;
    ethrdev_srv_cb_register_rx_default_handler_t register_rx_default_handler;
    ethrdev_srv_cb_unregister_rx_default_handler_t unregister_rx_default_handler;
    ethrdev_srv_cb_free_tx_handler_t free_tx_handler;
    ethrdev_srv_cb_free_rx_handler_t free_rx_handler;
    ethrdev_srv_cb_free_mac_handler_t free_mac_handler;
    ethrdev_srv_cb_detach_handler_t detach_handler;
    ethrdev_srv_cb_ioctl_handler_t ioctl_handler;
    ethrdev_srv_cb_regwr_handler_t regwr_handler;
    ethrdev_srv_cb_regrd_handler_t regrd_handler;
    ethrdev_srv_cb_register_ipv4_mac_handler_t ipv4_register_mac_handler;
    ethrdev_srv_cb_register_ipv6_mac_handler_t ipv6_register_mac_handler;
    ethrdev_srv_cb_unregister_ipv4_mac_handler_t ipv4_unregister_mac_handler;
    ethrdev_srv_cb_unregister_ipv6_mac_handler_t ipv6_unregister_mac_handler;
    ethrdev_srv_cb_client_notify_handler_t client_notify_handler;
    ethrdev_srv_cb_init_device_data_t init_device_data_handler;
} rdevEthSwitchServerCbFxn_t;

/**
 * \brief Remote demo device initialization parameters
 */
typedef struct rdevEthSwitchServerInitPrm_s
{
    uint32_t num_instances;                                                   /**< Number of instances */
    rdevEthSwitchServerInstPrm_t inst_prm[ETHREMOTECFG_SERVER_MAX_INSTANCES]; /**< List of instances */
    uint32_t rpmsg_buf_size;                                                  /**< Max transport packet size */
    rdevEthSwitchServerCbFxn_t cb;
} rdevEthSwitchServerInitPrm_t;

/**
 * \brief Union of all ethswitch remote device messages. Used internally
 */
typedef union rdevEthSwitchServerMessageList_u
{
    struct rpmsg_kdrv_ethswitch_attach_request attach_req;
    struct rpmsg_kdrv_ethswitch_attach_response attach_res;
    struct rpmsg_kdrv_ethswitch_attach_extended_request attach_ext_req;
    struct rpmsg_kdrv_ethswitch_attach_extended_response attach_ext_res;
    struct rpmsg_kdrv_ethswitch_alloc_request alloc_req;
    struct rpmsg_kdrv_ethswitch_alloc_rx_response alloc_rx_res;
    struct rpmsg_kdrv_ethswitch_alloc_tx_response alloc_tx_res;
    struct rpmsg_kdrv_ethswitch_alloc_mac_response alloc_mac_res;
    struct rpmsg_kdrv_ethswitch_register_mac_request register_mac_req;
    struct rpmsg_kdrv_ethswitch_register_mac_response register_mac_res;
    struct rpmsg_kdrv_ethswitch_unregister_mac_request unregister_mac_req;
    struct rpmsg_kdrv_ethswitch_unregister_mac_response unregister_mac_res;
    struct rpmsg_kdrv_ethswitch_register_rx_default_request register_rx_default_req;
    struct rpmsg_kdrv_ethswitch_register_rx_default_response register_rx_default_res;
    struct rpmsg_kdrv_ethswitch_unregister_rx_default_request unregister_rx_default_req;
    struct rpmsg_kdrv_ethswitch_unregister_rx_default_response unregister_rx_default_res;
    struct rpmsg_kdrv_ethswitch_free_mac_request free_mac_req;
    struct rpmsg_kdrv_ethswitch_free_mac_response free_mac_res;
    struct rpmsg_kdrv_ethswitch_free_tx_request free_tx_req;
    struct rpmsg_kdrv_ethswitch_free_tx_response free_tx_res;
    struct rpmsg_kdrv_ethswitch_free_rx_request free_rx_req;
    struct rpmsg_kdrv_ethswitch_free_rx_response free_rx_res;
    struct rpmsg_kdrv_ethswitch_detach_request detach_req;
    struct rpmsg_kdrv_ethswitch_detach_response detach_res;
    struct rpmsg_kdrv_ethswitch_ioctl_request ioctl_req;
    struct rpmsg_kdrv_ethswitch_ioctl_response ioctl_res;
    struct rpmsg_kdrv_ethswitch_regwr_request regwr_req;
    struct rpmsg_kdrv_ethswitch_regwr_response regwr_res;
    struct rpmsg_kdrv_ethswitch_regrd_request regrd_req;
    struct rpmsg_kdrv_ethswitch_regrd_response regrd_res;
    struct rpmsg_kdrv_ethswitch_device_data device_data;
    struct rpmsg_kdrv_ethswitch_ping_request ping_req;
    struct rpmsg_kdrv_ethswitch_ping_response ping_res;
    struct rpmsg_kdrv_ethswitch_ipv4_register_mac_request ipv4_register_mac_req;
    struct rpmsg_kdrv_ethswitch_ipv4_register_mac_response ipv4_register_mac_res;
    struct rpmsg_kdrv_ethswitch_ipv6_register_mac_request ipv6_register_mac_req;
    struct rpmsg_kdrv_ethswitch_ipv6_register_mac_response ipv6_register_mac_res;
    struct rpmsg_kdrv_ethswitch_ipv4_unregister_mac_request ipv4_unregister_mac_req;
    struct rpmsg_kdrv_ethswitch_ipv4_unregister_mac_response ipv4_unregister_mac_res;
    struct rpmsg_kdrv_ethswitch_s2c_notify s2c_notify;
    struct rpmsg_kdrv_ethswitch_c2s_notify c2s_notify;
} __packed rdevEthSwitchServerMessageList_t;

/**
 * \brief Set Remote Demo device init parameters to default state
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
 * \brief Initialize remote demo module
 *
 * \param prm [in] Initialization parameters
 *
 * \return 0 on success, else failure
 */
int32_t rdevEthSwitchServerInit(rdevEthSwitchServerInitPrm_t *prm);

#endif

/* @} */
