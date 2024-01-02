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
 * \file  ethfw_vlan.c
 *
 * \brief This file contains the implementation of VLAN helper functions.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* EthFwTrace id for this module, must be unique within ETHFW */
#define ETHFWTRACE_MOD_ID 0x105

#include <stdint.h>
#include <stdarg.h>

/* PDK header files */
#include <ti/osal/MutexP.h>

/* Enet LLD header files */
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>

/* EthFw header files */
#include <utils/ethfw_common/include/ethfw_trace.h>
#include "ethfw_vlan_priv.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/*!
 * \brief Static VLAN info.
 */
typedef struct EthFwVlan_Vlan_s
{
    /* VLAN id */
    uint16_t vlanId;

    /* ALE member mask of hardware ports */
    uint32_t memberMask;

    /* ALE port mask where registered multicast packets will be flooded to */
    uint32_t regMcastFloodMask;

    /* ALE port mask where unregistered multicast packets will be flooded to */
    uint32_t unregMcastFloodMask;

    /* ALE port mask where VLAN tag must be removed on egress */
    uint32_t untagMask;

    /* Member mask of virtual ports */
    uint32_t virtMemberMask;

    /* Member mask of virtual ports that have joined the VLAN */
    uint32_t virtActiveMask;
} EthFwVlan_Vlan;

/* ETHFW VLAN object */
typedef struct EthFwVlan_Obj_s
{
    /* VLAN configuration */
    EthFwVlan_Vlan vlan[ETHFWVLAN_VLANS_MAX];

    /* Number of valid VLAN configuration entries */
    uint32_t numVlans;

    /*! Mutex object used to protect VLAN configuration table */
    MutexP_Object mutexObj;

    /*! Handle to VLAN table mutex */
    MutexP_Handle hMutex;
} EthFwVlan_Obj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static inline uint32_t EthFwVlan_popcount(uint32_t v);

static EthFwVlan_Vlan *EthFwVlan_getVlan(uint16_t vlanId);

static int32_t EthFwVlan_getCfg(const EthFwVlan_Cfg *cfg);

static int32_t EthFwVlan_setupVlan(Enet_Handle hEnet,
                                   uint16_t vlanId,
                                   uint32_t memberMask,
                                   uint32_t regMcastFloodMask,
                                   uint32_t unregMcastFloodMask,
                                   uint32_t untagMask);

static void EthFwVlan_deleteVlan(Enet_Handle hEnet,
                                 uint16_t vlanId);

static int32_t EthFwVlan_setupClassifier(Enet_Handle hEnet,
                                         const uint8_t *macAddr,
                                         uint32_t vlanId,
                                         uint32_t flowIdxOffset);

static int32_t EthFwVlan_deleteClassifier(Enet_Handle hEnet,
                                          const uint8_t *macAddr,
                                          uint32_t vlanId,
                                          uint32_t flowIdxOffset);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* ETHFW VLAN object */
static EthFwVlan_Obj gEthFwVlanObj;

/* Broadcast address */
static uint8_t gEthFwVlan_bcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int32_t EthFwVlan_init(Enet_Handle hEnet,
                       const EthFwVlan_Cfg *cfg)
{
    EthFwVlan_Vlan *vlan;
    uint32_t i;
    int32_t status = ENET_SOK;

    /* Create mutex to protect VLAN configuration table */
    gEthFwVlanObj.hMutex = MutexP_create(&gEthFwVlanObj.mutexObj);
    if (gEthFwVlanObj.hMutex == NULL)
    {
        status = ENET_EFAIL;
        ETHFWTRACE_ERR(status, "Failed to create mutex");
    }

    /* Check config params and save VLAN configuration */
    if (status == ENET_SOK)
    {
        status = EthFwVlan_getCfg(cfg);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Incorrect static VLAN params");
    }

    /* Add VLAN and broadcast entries in ALE */
    if (status == ENET_SOK)
    {
        for (i = 0U; i < gEthFwVlanObj.numVlans; i++)
        {
            vlan = &gEthFwVlanObj.vlan[i];

            /* Exclude host post from the VLAN, will be added when virtual port(s) join */
            status = EthFwVlan_setupVlan(hEnet,
                                         vlan->vlanId,
                                         vlan->memberMask & ~CPSW_ALE_HOST_PORT_MASK,
                                         vlan->regMcastFloodMask & ~CPSW_ALE_HOST_PORT_MASK,
                                         vlan->unregMcastFloodMask & ~CPSW_ALE_HOST_PORT_MASK,
                                         vlan->untagMask & ~CPSW_ALE_HOST_PORT_MASK);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to setup VLANs in ALE");
        }

        ETHFWTRACE_INFO_IF((status == ETHFW_SOK), "%u VLAN entries added in ALE table", i);
    }

    return status;
}

void EthFwVlan_deinit(Enet_Handle hEnet)
{
    EthFwVlan_Vlan *vlan;
    int32_t i;

    /* Delete all static VLANs */
    for (i = 0U; i < gEthFwVlanObj.numVlans; i++)
    {
        vlan = &gEthFwVlanObj.vlan[i];

        ETHFWTRACE_ERR_IF((vlan->virtActiveMask != 0U), ENET_EUNEXPECTED,
                           "Virtual ports still active 0x%x", vlan->virtActiveMask);

        EthFwVlan_deleteVlan(hEnet, vlan->vlanId);
    }

    /* Delete VLAN configuration table mutex */
    if (gEthFwVlanObj.hMutex != NULL)
    {
        MutexP_delete(gEthFwVlanObj.hMutex);
        gEthFwVlanObj.hMutex = NULL;
    }
}

int32_t EthFwVlan_join(Enet_Handle hEnet,
                       EthRemoteCfg_VirtPort virtPort,
                       uint16_t vlanId,
                       const uint8_t *macAddr,
                       uint32_t flowIdx)
{
    EthFwVlan_Vlan *vlan = NULL;
    int32_t status = ENET_SOK;

    /* MAC address should be client's unicast address */
    if (EnetUtils_isMcastAddr(macAddr))
    {
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "Cannot join VLAN %u with mcast addr %02x:%02x:%02x:%02x:%02x:%02x",
                       vlanId,
                       macAddr[0U], macAddr[1U], macAddr[2U],
                       macAddr[3U], macAddr[4U], macAddr[5U]);
    }

    MutexP_lock(gEthFwVlanObj.hMutex, MutexP_WAIT_FOREVER);

    /* Get VLAN info for the VLAN id that remote client is trying to join */
    if (status == ENET_SOK)
    {
        vlan = EthFwVlan_getVlan(vlanId);
        if (vlan == NULL)
        {
            status = ENET_EINVALIDPARAMS;
            ETHFWTRACE_ERR(status, "VLAN %u is not registered, virtual port %u cannot join", vlanId, virtPort);
        }
    }

    /* Check if the virtual port is allowed in the VLAN */
    if (status == ENET_SOK)
    {
        if (!ENET_IS_BIT_SET(vlan->virtMemberMask, virtPort))
        {
            status = ENET_EPERM;
            ETHFWTRACE_ERR(status, "Virtual port %u is not allowed in VLAN %u", virtPort, vlanId);
        }
    }

    /* Update VLAN and broadcast entries in ALE table with host port added */
    if (status == ENET_SOK)
    {
        status = EthFwVlan_setupVlan(hEnet,
                                     vlan->vlanId,
                                     vlan->memberMask,
                                     vlan->regMcastFloodMask,
                                     vlan->unregMcastFloodMask,
                                     vlan->untagMask);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to update VLAN %u in ALE", vlanId);
    }

    /* Setup ALE classifier for remote client's MAC address and VLAN */
    if (status == ENET_SOK)
    {
        status = EthFwVlan_setupClassifier(hEnet, macAddr, vlanId, flowIdx);
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                          "Failed to setup VLAN %u classifier for virtual port %u", vlanId, virtPort);
    }

    /* Mark virtual port as active in the VLAN */
    if (status == ENET_SOK)
    {
        vlan->virtActiveMask |= ENET_BIT(virtPort);
    }

    MutexP_unlock(gEthFwVlanObj.hMutex);

    return status;
}

int32_t EthFwVlan_leave(Enet_Handle hEnet,
                        EthRemoteCfg_VirtPort virtPort,
                        uint16_t vlanId,
                        const uint8_t *macAddr,
                        uint32_t flowIdx)
{
    EthFwVlan_Vlan *vlan = NULL;
    int32_t status = ENET_SOK;

    /* MAC address should be client's unicast address */
    if (EnetUtils_isMcastAddr(macAddr))
    {
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "Cannot leave VLAN %u with mcast addr %02x:%02x:%02x:%02x:%02x:%02x",
                       vlanId,
                       macAddr[0U], macAddr[1U], macAddr[2U],
                       macAddr[3U], macAddr[4U], macAddr[5U]);
    }

    MutexP_lock(gEthFwVlanObj.hMutex, MutexP_WAIT_FOREVER);

    /* Get VLAN info for the VLAN id that remote client is trying to join */
    vlan = EthFwVlan_getVlan(vlanId);
    if (vlan == NULL)
    {
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "VLAN %u is not registered, virtual port %u cannot leave", vlanId, virtPort);
    }

    /* Check if the virtual port is allowed in the VLAN */
    if (status == ENET_SOK)
    {
        if (!ENET_IS_BIT_SET(vlan->virtMemberMask, virtPort))
        {
            status = ENET_EPERM;
            ETHFWTRACE_ERR(status, "Virtual port %u is not allowed in VLAN %u", virtPort, vlanId);
        }
    }

    /* Check if the virtual port had joined the VLAN */
    if (status == ENET_SOK)
    {
        if (!ENET_IS_BIT_SET(vlan->virtActiveMask, virtPort))
        {
            status = ENET_EINVALIDPARAMS;
            ETHFWTRACE_ERR(status, "Virtual port %u had not joined VLAN %u", virtPort);
        }
    }

    /* Clear virtual port from VLAN active list */
    if (status == ENET_SOK)
    {
        vlan->virtActiveMask &= ~ENET_BIT(virtPort);

        /* Exclude host post from the VLAN until virtual port(s) join again */
        if (vlan->virtActiveMask == 0U)
        {
            status = EthFwVlan_setupVlan(hEnet,
                                         vlan->vlanId,
                                         vlan->memberMask & ~CPSW_ALE_HOST_PORT_MASK,
                                         vlan->regMcastFloodMask & ~CPSW_ALE_HOST_PORT_MASK,
                                         vlan->unregMcastFloodMask & ~CPSW_ALE_HOST_PORT_MASK,
                                         vlan->untagMask & ~CPSW_ALE_HOST_PORT_MASK);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to setup VLANs in ALE");
        }
    }

    /* Delete the classifier, but keep the ALE VLAN entries */
    if (status == ENET_SOK)
    {
        status = EthFwVlan_deleteClassifier(hEnet, macAddr, vlanId, flowIdx);
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                          "Failed to delete classifier for VLAN %u flowIdx %u", vlanId, flowIdx);
    }

    MutexP_unlock(gEthFwVlanObj.hMutex);

    return status;
}

bool EthFwVlan_isInVlan(EthRemoteCfg_VirtPort virtPort,
                        uint16_t vlanId)
{
    EthFwVlan_Vlan *vlan = NULL;
    bool active = BFALSE;

    MutexP_lock(gEthFwVlanObj.hMutex, MutexP_WAIT_FOREVER);

    vlan = EthFwVlan_getVlan(vlanId);
    active = (vlan != NULL) && ENET_IS_BIT_SET(vlan->virtActiveMask, virtPort);

    MutexP_unlock(gEthFwVlanObj.hMutex);

    return active;
}

/* http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan */
static inline uint32_t EthFwVlan_popcount(uint32_t v)
{
    uint32_t c;

    for (c = 0U; v != 0U; c++)
    {
        v &= v - 1U;
    }

    return c;
}

static EthFwVlan_Vlan *EthFwVlan_getVlan(uint16_t vlanId)
{
    EthFwVlan_Vlan *vlan = NULL;
    uint32_t i;

    /* Find VLAN info for the given VLAN id */
    for (i = 0U; i < gEthFwVlanObj.numVlans; i++)
    {
        if (gEthFwVlanObj.vlan[i].vlanId == vlanId)
        {
            vlan = &gEthFwVlanObj.vlan[i];
        }
    }

    return vlan;
}

static int32_t EthFwVlan_getCfg(const EthFwVlan_Cfg *cfg)
{
    const EthFwVlan_VlanCfg *vlanCfg = NULL;
    EthFwVlan_Vlan *vlan = NULL;
    uint32_t aleSwitchPortMask = 0U;
    uint32_t aleMacOnlyPortMask = 0U;
    uint32_t memberMask = 0U;
    uint16_t vlanId = 0U;
    uint32_t i;
    int32_t status = ENET_SOK;

    aleSwitchPortMask  = (cfg->switchPortMask << 1U);
    aleMacOnlyPortMask = (cfg->macOnlyPortMask << 1U);

    /* Check that number of VLAN has not been exceeded (this is a software limitation) */
    if (cfg->numVlans > ETHFWVLAN_VLANS_MAX)
    {
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "Too many VLANs (%u), max is %u", cfg->numVlans, ETHFWVLAN_VLANS_MAX);
    }

    /* Check all VLAN config params and add them to local VLAN info table */
    if (status == ENET_SOK)
    {
        for (i = 0U; i < cfg->numVlans; i++)
        {
            vlanCfg = &cfg->vlanCfg[i];

            vlanId = vlanCfg->vlanId;
            memberMask = vlanCfg->memberMask;

            /* VLANs cannot be same as default VLAN ids for switch and MAC-only ports */
            if ((vlanId == cfg->dfltVlanIdSwitchPorts) ||
                (vlanId == cfg->dfltVlanIdMacOnlyPorts))
            {
                status = ENET_EINVALIDPARAMS;
                ETHFWTRACE_ERR(status, "VLAN id %u cannot be same as default VLAN ids (%u, %u)",
                               vlanId, cfg->dfltVlanIdSwitchPorts, cfg->dfltVlanIdMacOnlyPorts);
                break;
            }

            /* Cannot have MAC-only and switch ports in the same VLAN, else leaks */
            if (status == ENET_SOK)
            {
                if (ENET_NOT_ZERO(memberMask & aleSwitchPortMask) &&
                    ENET_NOT_ZERO(memberMask & aleMacOnlyPortMask))
                {
                    status = ENET_EINVALIDPARAMS;
                    ETHFWTRACE_ERR(status, "VLAN %u member list 0x%x cannot be both MAC-only and switch ports",
                                   vlanId, memberMask);
                    break;
                }
            }

            /* Cannot have more than one MAC-only port in the same VLAN, else leaks */
            if (status == ENET_SOK)
            {
                memberMask &= aleMacOnlyPortMask;
                if (EthFwVlan_popcount(memberMask) > 1U)
                {
                    status = ENET_EINVALIDPARAMS;
                    ETHFWTRACE_ERR(status, "VLAN %u member list %x cannot have multiple MAC-only ports",
                                   vlanId, memberMask);
                    break;
                }
            }

            /* All checks passed, save it */
            if (status == ENET_SOK)
            {
                vlan = &gEthFwVlanObj.vlan[i];

                /* Cap masks to member mask */
                vlan->vlanId              = vlanCfg->vlanId;
                vlan->memberMask          = vlanCfg->memberMask;
                vlan->virtMemberMask      = vlanCfg->virtMemberMask;
                vlan->regMcastFloodMask   = vlanCfg->regMcastFloodMask & vlanCfg->memberMask;
                vlan->unregMcastFloodMask = vlanCfg->unregMcastFloodMask & vlanCfg->memberMask;
                vlan->untagMask           = vlanCfg->untagMask &vlanCfg->memberMask;
                vlan->virtActiveMask      = 0U;
                gEthFwVlanObj.numVlans++;

                ETHFWTRACE_INFO("VLAN %u member=0x%x virtMember=0x%x "
                                "regMcastFlood=0x%x unregMcastFlood=0x%x untag=0x%x",
                                vlan->vlanId,
                                vlan->memberMask,
                                vlan->virtMemberMask,
                                vlan->regMcastFloodMask,
                                vlan->unregMcastFloodMask,
                                vlan->untagMask);
            }
        }
    }

    return status;
}

static int32_t EthFwVlan_setupVlan(Enet_Handle hEnet,
                                   uint16_t vlanId,
                                   uint32_t memberMask,
                                   uint32_t regMcastFloodMask,
                                   uint32_t unregMcastFloodMask,
                                   uint32_t untagMask)
{
    CpswAle_VlanEntryInfo vlanInArgs;
    CpswAle_SetMcastEntryInArgs mcastInArgs;
    Enet_IoctlPrms prms;
    uint32_t aleEntry;
    uint32_t coreId;
    int32_t status = ENET_SOK;

    coreId = EnetSoc_getCoreId();

    /* Add ALE VLAN entry */
    memset(&vlanInArgs, 0U, sizeof (CpswAle_VlanEntryInfo));
    vlanInArgs.vlanIdInfo.vlanId       = vlanId;
    vlanInArgs.vlanIdInfo.tagType      = ENET_VLAN_TAG_TYPE_INNER;
    vlanInArgs.vlanMemberList          = memberMask;
    vlanInArgs.regMcastFloodMask       = regMcastFloodMask;
    vlanInArgs.unregMcastFloodMask     = unregMcastFloodMask;
    vlanInArgs.forceUntaggedEgressMask = untagMask;
    vlanInArgs.noLearnMask             = 0U;
    vlanInArgs.vidIngressCheck         = BTRUE;
    vlanInArgs.limitIPNxtHdr           = BFALSE;
    vlanInArgs.disallowIPFrag          = BTRUE;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &vlanInArgs, &aleEntry);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_ADD_VLAN, &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to add ALE VLAN %u entry", vlanId);

    /* Add broadcast entry for the VLAN */
    if (status == ENET_SOK)
    {
        EnetUtils_copyMacAddr(&mcastInArgs.addr.addr[0], &gEthFwVlan_bcastAddr[0U]);
        mcastInArgs.addr.vlanId     = vlanId;
        mcastInArgs.info.super      = BFALSE;
        mcastInArgs.info.fwdState   = CPSW_ALE_FWDSTLVL_FWD;
        mcastInArgs.info.portMask   = memberMask;
        mcastInArgs.info.numIgnBits = 0U;

        ENET_IOCTL_SET_INOUT_ARGS(&prms, &mcastInArgs, &aleEntry);

        status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_ADD_MCAST, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to add VLAN %u bcast ALE entry", vlanId);
    }

    return status;
}

static void EthFwVlan_deleteVlan(Enet_Handle hEnet,
                                 uint16_t vlanId)
{
    CpswAle_MacAddrInfo mcastInArgs;
    CpswAle_VlanIdInfo vlanInArgs;
    Enet_IoctlPrms prms;
    uint32_t coreId;
    int32_t status = ENET_SOK;

    coreId = EnetSoc_getCoreId();

    /* Delete VLAN broadcast entry */
    EnetUtils_copyMacAddr(&mcastInArgs.addr[0], &gEthFwVlan_bcastAddr[0U]);
    mcastInArgs.vlanId = vlanId;

    ENET_IOCTL_SET_IN_ARGS(&prms, &mcastInArgs);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_REMOVE_ADDR, &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to delete VLAN %u bcast entry", vlanId);

    /* Delete VLAN entry */
    vlanInArgs.vlanId  = vlanId;
    vlanInArgs.tagType = ENET_VLAN_TAG_TYPE_INNER;

    ENET_IOCTL_SET_IN_ARGS(&prms, &vlanInArgs);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_REMOVE_VLAN, &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to delete ALE VLAN %u entry", vlanId);
}

static int32_t EthFwVlan_setupClassifier(Enet_Handle hEnet,
                                         const uint8_t *macAddr,
                                         uint32_t vlanId,
                                         uint32_t flowIdxOffset)
{
    CpswAle_SetPolicerEntryInArgs polInArgs;
    CpswAle_SetPolicerEntryOutArgs polOutArgs;
    Enet_IoctlPrms prms;
    uint32_t coreId;
    int32_t status = ENET_SOK;

    coreId = EnetSoc_getCoreId();

    /* MAC destination + VLAN match */
    polInArgs.policerMatch.policerMatchEnMask         = CPSW_ALE_POLICER_MATCH_MACDST;
    polInArgs.policerMatch.dstMacAddrInfo.portNum     = CPSW_ALE_HOST_PORT_NUM;
    polInArgs.policerMatch.dstMacAddrInfo.addr.vlanId = vlanId;
    EnetUtils_copyMacAddr(&polInArgs.policerMatch.dstMacAddrInfo.addr.addr[0U], macAddr);

    /* Route to remote clients flow */
    polInArgs.threadIdEn = BTRUE;
    polInArgs.threadId   = flowIdxOffset;
    polInArgs.peakRateInBitsPerSec   = 0U;
    polInArgs.commitRateInBitsPerSec = 0U;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &polInArgs, &polOutArgs);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_SET_POLICER, &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to setup ALE classifier VLAN %u", vlanId);

    return status;
}

static int32_t EthFwVlan_deleteClassifier(Enet_Handle hEnet,
                                          const uint8_t *macAddr,
                                          uint32_t vlanId,
                                          uint32_t flowIdxOffset)
{
    CpswAle_PolicerMatchParams polMatchInArgs;
    CpswAle_PolicerEntryOutArgs polOutArgs;
    CpswAle_DelPolicerEntryInArgs delPolInArgs;
    Enet_IoctlPrms prms;
    uint32_t coreId;
    int32_t status = ENET_SOK;

    coreId = EnetSoc_getCoreId();

    /* Get policer for MAC destination + VLAN match */
    polMatchInArgs.policerMatchEnMask         = CPSW_ALE_POLICER_MATCH_MACDST;
    polMatchInArgs.dstMacAddrInfo.portNum     = CPSW_ALE_HOST_PORT_NUM;
    polMatchInArgs.dstMacAddrInfo.addr.vlanId = vlanId;
    EnetUtils_copyMacAddr(&polMatchInArgs.dstMacAddrInfo.addr.addr[0U], macAddr);

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &polMatchInArgs, &polOutArgs);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_GET_POLICER, &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                      "Failed to find ALE classifier for VLAN %u macAdd=%02x:%02x:%02x:%02x:%02x:%02x",
                      vlanId,
                      macAddr[0U], macAddr[1U], macAddr[2U],
                      macAddr[3U], macAddr[4U], macAddr[5U]);

    /* Check if classifier was routing packets to client's flow */
    if (status == ENET_SOK)
    {
        if ((polOutArgs.threadIdEn != BTRUE) ||
            (polOutArgs.threadId != flowIdxOffset))
        {
            status = ETHFW_EUNEXPECTED;
            ETHFWTRACE_ERR(status, "Invalid VLAN %u policer thread cfg (threadIdEn=%u threadId=%u)",
                           vlanId, polOutArgs.threadIdEn, polOutArgs.threadId);
        }
    }

    /* Delete classifier and its VLAN/MAC entry */
    if (status == ENET_SOK)
    {
        delPolInArgs.policerMatch = polMatchInArgs;
        delPolInArgs.aleEntryMask = CPSW_ALE_POLICER_TABLEENTRY_DELETE_ALL;

        ENET_IOCTL_SET_IN_ARGS(&prms, &delPolInArgs);

        status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_DEL_POLICER, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Invalid VLAN %u policer", vlanId);
    }

    return status;
}

