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
 *  \file ethfw_vepa_utils.h
 *
 *  \brief Header file for Ethernet Firmware VEPA utils.
 */

#ifndef ETHFW_VEPA_UTILS_H_
#define ETHFW_VEPA_UTILS_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include "lwip/prot/ethernet.h"
#include "netif/ethernet.h"
#include <ethremotecfg/protocol/ethremotecfg_virtport.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \defgroup ETHFW_VEPA_UTILS Ethernet Firmware VEPA Utils
 *
 * \brief This section contains VEPA helper APIs for the Ethernet Firmware.
 *
 * This VEPA utils library provides helper functions to enable VEPA (Virtual
 * Ethernet Port Aggregator) functionality in Jacinto devices with CPSW capable
 * of multihost data flow.  _Multihost_ is a CPSW ALE feature that enables
 * packets to be sent and received on host port, which is mandatory for VEPA.
 *
 * The VEPA utils library allocates a secondary RX flow exclusively for
 * broadcast and registered multicast packets to be routed to.  This flow is
 * in addition to the RX flows used by Ethernet Firmware to receive all other
 * traffic (i.e. TCP/IP, gPTP).
 *
 * Ethernet Firmware server side (_CpswProxyServer_) calls \ref EthFwVepaUtils_addAddr()
 * to register multicast MAC addresses that need to be forwarded to remote
 * clients.  These multicast addresses are added to an internal VEPA table,
 * which can be printed using \ref EthFwVepaUtils_printTable() for debug purpose.
 * An ALE policer entry is also added for each multicast address in the table
 * so that when multicast packets arrive at any of the MAC ports configured in
 * non MAC-only mode, they are routed to its dedicated flow, allocated at
 * init time.
 *
 * When a multicast packet whose MAC address is registered in the VEPA table,
 * it will be passed to a VEPA specific handle function, which then calls
 * \ref EthFwVepaUtils_sendRaw() to send a copy of the multicast packets to
 * all relevant remote cores.
 *
 * @{
 */
/* @} */

/*!
 * \addtogroup ETHFW_VEPA_UTILS
 * @{
 */

/* ========================================================================== */
/*                                 Macros                                     */
/* ========================================================================== */

/*! \brief Flow index used to received packets for duplication is not defined */
#define ETHFW_VEPA_UTILS_PKT_DUP_FLOW_IDX_UNDEFINED      (0xFFFFU)

/* ========================================================================== */
/*                         Structures and Enums                               */
/* ========================================================================== */

/*!
 * \brief Ethernet Firmware VEPA configuration.
 */
typedef struct EthFwVepaUtils_Cfg_s
{
    /*! Private VLAN id for each remote core */
    uint32_t privVlanId[ETHREMOTECFG_SWITCH_PORT_LAST+1];
} EthFwVepaUtils_Cfg;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/*!
 * \brief Initialize VEPA configuration parameters.
 *
 * \param vepaCfg    Configuration parameters to be initialized
 */
void EthFwVepaUtils_initCfg(EthFwVepaUtils_Cfg *vepaCfg);

/*!
 * \brief Initialize VEPA utils module.
 *
 * Initializes VEPA utils module. User should call \ref EthFwVepa_initCfg
 * to initialize configuration parameters and make updates if needed
 * before calling this function.
 *
 * \param vepaCfg    Configuration parameters
 *
 * \returns ENET_SOK if VEPA initialization was successful
 */
int32_t EthFwVepaUtils_init(const EthFwVepaUtils_Cfg *vepaCfg);

/*!
 * \brief De-initialize VEPA utils module.
 *
 * Deinitializes VEPA utils, it must be the last VEPA utils function to be
 * called.
 */
void EthFwVepaUtils_deinit(void);

/*!
 * \brief Sets packet duplication flow.
 *
 * \param flowIdx    RX flow idx to be set
 *
 * \returns ENET_SOK if packet duplication flow was set successful, or a
 *          negative error code if failed.
 */
uint32_t EthFwVepaUtils_setPacketDuplicationFlowIdx(uint32_t flowIdx);

/*!
 * \brief Add a multicast entry in VEPA table for client
 *
 * Adds policer for multicast packets to go to packet duplication flow
 *
 * \param hEnet      Handle to CPSW
 * \param hwAddr     MAC address to add in VEPA table
 * \param vlanId     VLAN id
 * \param hostId     Remote core IPC core id
 * \param virtPort   Remote core virtual port id
 *
 * \returns ENET_SOK if multicast entry addition was successful, or a
 *          negative error code if failed.
 */
int32_t EthFwVepaUtils_addAddr(Enet_Handle hEnet,
                               struct eth_addr *hwAddr,
                               uint16_t vlanId,
                               uint16_t hostId,
                               EthRemoteCfg_VirtPort virtPort);

/*!
 * \brief Removes an entry in VEPA table for client
 *
 * Removes policer which allowed multicast packets to go to packet duplication flow
 *
 * \param hEnet      Handle to CPSW
 * \param hwAddr     MAC address to add in VEPA table
 * \param vlanId     VLAN id
 * \param hostId     Remote core IPC core id
 * \param virtPort   Remote core virtual port id
 *
 * \returns ENET_SOK if multicast entry deletion was successful, or a
 *          negative error code if failed.
 */
int32_t EthFwVepaUtils_delAddr(Enet_Handle hEnet,
                               struct eth_addr *hwAddr,
                               uint16_t vlanId,
                               uint16_t hostId,
                               EthRemoteCfg_VirtPort virtPort);

/*!
 * \brief Print VEPA table with private VLAN associated to each virtual port
 */
void EthFwVepaUtils_printTable(void);

/*!
 * \brief Flush all entries in VEPA table
 */
void EthFwVepaUtils_flushTable(void);

/*!
 * \brief Adds ALE entry and policer with private VLAN when a client registers.
 *
 * \param hEnet      Handle to CPSW
 * \param coreId     Remote core IPC core id
 * \param flowIdx    RX flow id of virtual port
 * \param vlanId     VLAN id
 * \param virtPort   Remote core virtual port id
 * \param srcAddr    MAC address of virtual port
 *
 * \returns ENET_SOK if client registered successfully, or a negative
 *          error code if failed.
 */
int32_t EthFwVepaUtils_registerClient(Enet_Handle hEnet,
                                      uint32_t coreId,
                                      uint32_t flowIdx,
                                      uint16_t vlanId,
                                      EthRemoteCfg_VirtPort virtPort,
                                      struct eth_addr *srcAddr);

/*!
 * \brief Removes ALE entry and policer with private VLAN when
 *        a client de-registers.
 *
 * \param hEnet      Handle to CPSW
 * \param coreId     Remote core IPC core id
 * \param flowIdx    RX flow id of virtual port
 * \param vlanId     VLAN id
 * \param virtPort   Remote core virtual port id
 *
 * \returns ENET_SOK if client de-registered successfully, or a
 *          negative error code if failed.
 */
int32_t EthFwVepaUtils_unregisterClient(Enet_Handle hEnet,
                                        uint32_t coreId,
                                        uint32_t flowIdx,
                                        uint16_t vlanId,
                                        EthRemoteCfg_VirtPort virtPort);

/*!
 * \brief Sends pbuf to all the required virtual switch ports after
 *        inserting their corresponding private VLAN.
 *
 * \param netif      Enet netif on which packet received
 * \param pbuf       Pointer to the data buffer received
 * \param ethSrcAddr Source MAC address
 * \param ethDstAddr Destination MAC address
 *
 * \returns ENET_SOK if packet was sent successfully to required virtual
 *          switch ports, or a negative error code if failed.
 */
int32_t EthFwVepaUtils_sendRaw(struct netif *netif,
                               struct pbuf *pbuf,
                               struct eth_addr *ethSrcAddr,
                               struct eth_addr *ethDstAddr);

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* ETHFW_VEPA_UTILS_H_ */
