/*
 *  Copyright (c) Texas Instruments Incorporated 2023
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*!
 * \file  ethfw_vlan.h
 *
 * \brief This file contains the type definitions, helper macros and functions
 *        required for Ethernet Firmware VLAN support.
 */

#ifndef ETHFW_VLAN_H_
#define ETHFW_VLAN_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdint.h>
#include <ti/drv/enet/enet.h>
#include <ethremotecfg/protocol/ethremotecfg_virtport.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                                 Macros                                     */
/* ========================================================================== */

/*! Maximum number of VLANs that can be setup statically */
#define ETHFWVLAN_VLANS_MAX                            (10U)

/* ========================================================================== */
/*                         Structures and Enums                               */
/* ========================================================================== */

/*!
 * \brief Static VLAN configuration.
 */
typedef struct EthFwVlan_VlanCfg_s
{
    /*! VLAN id */
    uint16_t vlanId;

    /*! Member mask of hardware ports (Ethernet port and host port):
     *  - Use `CPSW_ALE_MACPORT_TO_ALEPORT()` to set the MAC port bitmask
     *  - Use `CPSW_ALE_HOST_PORT_MASK` to set the host port bitmask */
    uint32_t memberMask;

    /*! Member mask of virtual ports.  Use `ENET_BIT(ETHREMOTECFG_SWITCH_PORT_n)`
     *  to create the member mask.
     *  Note: Currently, only one virtual port is supported */
    uint32_t virtMemberMask;

    /*! Port mask where registered multicast packets will be flooded to.
     *  Must be same as EthFwVlan_VlanCfg::memberMask or a subset of it */
    uint32_t regMcastFloodMask;

    /*! Port mask where unregistered multicast packets will be flooded to.
     *  Must be same as EthFwVlan_VlanCfg::memberMask or a subset of it */
    uint32_t unregMcastFloodMask;

    /*! Port mask where VLAN tag must be removed on egress. Must be same
     *  as EthFwVlan_VlanCfg::memberMask or a subset of it */
    uint32_t untagMask;
} EthFwVlan_VlanCfg;

/*!
 * \brief Static VLAN configuration.
 */
typedef struct EthFwVlan_Cfg_s
{
    /*! Pointer to the static VLANs parameters */
    const EthFwVlan_VlanCfg *vlanCfg;

    /*! Number of static VLANs in \ref EthFwVlan_Cfg::vlanCfg */
    uint32_t numVlans;

    /*! Default VLAN id used by Ethernet Firmware for switch ports
      * (non MAC-only) */
    uint32_t dfltVlanIdSwitchPorts;

    /*! Default VLAN id used by Ethernet Firmware for MAC-only ports */
    uint32_t dfltVlanIdMacOnlyPorts;

    /*! Port mask of ports in switch mode (non MAC-only) */
    uint32_t switchPortMask;

    /*! Port mask of ports in MAC-only mode */
    uint32_t macOnlyPortMask;
} EthFwVlan_Cfg;

/* ========================================================================== */
/*                         Global Variables Declarations                      */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/*!
 * \brief Initialize and configure ETHFW VLAN functionality.
 *
 * Sets up static VLAN functionality required by Ethernet Firmware, that is,
 * VLAN and broadcast entries in ALE table for each VLAN id in the
 * configuration params.  Note that the VLANs won't forward packets to the
 * host port until a remote client joins the VLAN.
 *
 * \param hEnet               Handle to the underlying Enet driver
 * \param cfg                 VLAN configuration parameters
 *
 * \returns ENET_SOK if VLAN was initialized correctly, or a negative error
 *          in case of failure.
 */
int32_t EthFwVlan_init(Enet_Handle hEnet,
                       const EthFwVlan_Cfg *cfg);

/*!
 * \brief Deinitialize ETHFW VLAN functionality.
 *
 * Deletes all static VLANs created by \ref EthFwVlan_init().
 *
 * \param hEnet               Handle to the underlying Enet driver
 */
void EthFwVlan_deinit(Enet_Handle hEnet);

/*!
 * \brief Join a registered VLAN.
 *
 * Joins a virtual port to a registered VLAN.  The virtual port must be
 * allowed in the VLAN in the VLAN configuration provided at init time
 * (see \ref EthFwVlan_VlanCfg::virtMemberMask).
 *
 * A classifier will be created for the client's MAC address and VLAN id,
 * which will route unicast traffic to the provided RX flow.
 *
 * \param hEnet               Handle to the underlying Enet driver
 * \param virtPort            Client's virtual port id
 * \param vlanId              VLAN id to join
 * \param macAddr             Client's MAC address
 * \param flowIdx             RX flow where VLAN packets will be routed
 *
 * \returns ENET_SOK if VLAN was initialized correctly, or a negative error
 *          in case of failure.
 */
int32_t EthFwVlan_join(Enet_Handle hEnet,
                       EthRemoteCfg_VirtPort virtPort,
                       uint16_t vlanId,
                       const uint8_t *macAddr,
                       uint32_t flowIdx);

/*!
 * \brief Leave a registered VLAN.
 *
 * Leaves a virtual port from a registered VLAN.  The virtual port must be
 * allowed in the VLAN in the VLAN configuration provided at init time
 * (see \ref EthFwVlan_VlanCfg::virtMemberMask).
 *
 * The classifier for the client's MAC address and VLAN id will be deleted.
 *
 * \param hEnet               Handle to the underlying Enet driver
 * \param virtPort            Client's virtual port id
 * \param vlanId              VLAN id to leave
 * \param macAddr             Client's MAC address
 * \param flowIdx             RX flow where VLAN packets werere routed
 *
 * \returns ENET_SOK if VLAN was initialized correctly, or a negative error
 *          in case of failure.
 */
int32_t EthFwVlan_leave(Enet_Handle hEnet,
                        EthRemoteCfg_VirtPort virtPort,
                        uint16_t vlanId,
                        const uint8_t *macAddr,
                        uint32_t flowIdx);

/*!
 * \brief Query if virtual port has joined VLAN.
 *
 * Queries if virtual port has joined the VLAN.
 *
 * \param virtPort            Client's virtual port id
 * \param vlanId              VLAN id to leave
 *
 * \returns true if VLAN is valid and virtual port has joined it.
 */
bool EthFwVlan_isInVlan(EthRemoteCfg_VirtPort virtPort,
                        uint16_t vlanId);

#ifdef __cplusplus
}
#endif

#endif /* ETHFW_VLAN_H_ */
