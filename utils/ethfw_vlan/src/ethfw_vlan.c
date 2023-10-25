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

#include <stdint.h>
#include <stdarg.h>

/* PDK header files */
#include <ti/osal/MutexP.h>

/* Enet LLD header files */
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>

/* EthFw utils header files */
#include <utils/ethfw_vlan/include/ethfw_vlan.h>
#include <utils/console_io/include/app_log.h>

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
        appLogPrintf("ETHFW: Failed to create mutex\n");
        status = ENET_EFAIL;
    }

    /* Check config params and save VLAN configuration */
    if (status == ENET_SOK)
    {
        status = EthFwVlan_getCfg(cfg);
        if (status != ENET_SOK)
        {
            appLogPrintf("ETHFW: Incorrect static VLAN params: %d\n", status);
        }
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
            if (status != ENET_SOK)
            {
                appLogPrintf("ETHFW: Failed to setup VLANs in ALE: %d\n", status);
            }
        }

        if (status == ENET_SOK)
        {
            appLogPrintf("ETHFW: %u VLAN entries added in ALE table\n", i);
        }
    }

    return status;
}

void EthFwVlan_deinit(Enet_Handle hEnet)
{
    EthFwVlan_Vlan *vlan;
    int32_t i;

    if (vlan->virtActiveMask != 0U)
    {
        appLogPrintf("ETHFW: Virtual ports still active 0x%x\n",
                     vlan->virtActiveMask);
    }

    /* Delete all static VLANs */
    for (i = 0U; i < gEthFwVlanObj.numVlans; i++)
    {
        vlan = &gEthFwVlanObj.vlan[i];

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
        appLogPrintf("ETHFW: Cannot join VLAN %u with mcast MAC addr macAdd=%x:%x:%x:%x:%x:%x\n",
                     vlanId,
                     macAddr[0U], macAddr[1U], macAddr[2U],
                     macAddr[3U], macAddr[4U], macAddr[5U]);
        status = ENET_EINVALIDPARAMS;
    }

    MutexP_lock(gEthFwVlanObj.hMutex, MutexP_WAIT_FOREVER);

    /* Get VLAN info for the VLAN id that remote client is trying to join */
    if (status == ENET_SOK)
    {
        vlan = EthFwVlan_getVlan(vlanId);
        if (vlan == NULL)
        {
            appLogPrintf("ETHFW: VLAN %u is not register, virtual port %u cannot join\n",
                         vlanId, virtPort);
            status = ENET_EINVALIDPARAMS;
        }
    }

    /* Check if the virtual port is allowed in the VLAN */
    if (status == ENET_SOK)
    {
        if (!ENET_IS_BIT_SET(vlan->virtMemberMask, virtPort))
        {
            appLogPrintf("ETHFW: Virtual port %u is not allowed in VLAN %u\n",
                         virtPort, vlanId);
            status = ENET_EPERM;
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
        if (status != ENET_SOK)
        {
            appLogPrintf("ETHFW: Failed to update VLAN %u in ALE: %d\n", vlanId, status);
        }
    }

    /* Setup ALE classifier for remote client's MAC address and VLAN */
    if (status == ENET_SOK)
    {
        status = EthFwVlan_setupClassifier(hEnet, macAddr, vlanId, flowIdx);
        if (status != ENET_SOK)
        {
            appLogPrintf("ETHFW: Failed to setup VLAN %u classifier for virtual port %u: %d\n",
                         vlanId, virtPort, status);
        }
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
        appLogPrintf("ETHFW: Cannot join VLAN %u with mcast MAC addr macAdd=%x:%x:%x:%x:%x:%x\n",
                     vlanId,
                     macAddr[0U], macAddr[1U], macAddr[2U],
                     macAddr[3U], macAddr[4U], macAddr[5U]);
        status = ENET_EINVALIDPARAMS;
    }

    MutexP_lock(gEthFwVlanObj.hMutex, MutexP_WAIT_FOREVER);

    /* Get VLAN info for the VLAN id that remote client is trying to join */
    vlan = EthFwVlan_getVlan(vlanId);
    if (vlan == NULL)
    {
        appLogPrintf("ETHFW: VLAN %u is not register, virtual port %u cannot join\n",
                     vlanId, virtPort);
        status = ENET_EINVALIDPARAMS;
    }

    /* Check if the virtual port is allowed in the VLAN */
    if (status == ENET_SOK)
    {
        if (!ENET_IS_BIT_SET(vlan->virtMemberMask, virtPort))
        {
            appLogPrintf("ETHFW: Virtual port %u is not allowed in VLAN %u\n",
                         virtPort, vlanId);
            status = ENET_EPERM;
        }
    }

    /* Check if the virtual port had joined the VLAN */
    if (status == ENET_SOK)
    {
        if (!ENET_IS_BIT_SET(vlan->virtActiveMask, virtPort))
        {
            appLogPrintf("ETHFW: Virtual port %u had not joined VLAN %u\n",
                         virtPort, vlanId);
            status = ENET_EINVALIDPARAMS;
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
            if (status != ENET_SOK)
            {
                appLogPrintf("ETHFW: Failed to setup VLANs in ALE: %d\n", status);
            }
        }
    }

    /* Delete the classifier, but keep the ALE VLAN entries */
    if (status == ENET_SOK)
    {
        EthFwVlan_deleteClassifier(hEnet, macAddr, vlanId, flowIdx);
        if (status != ENET_SOK)
        {
            appLogPrintf("ETHFW: Failed to delete classifier for VLAN %u: %d\n",
                         vlanId, flowIdx);
        }
    }

    MutexP_unlock(gEthFwVlanObj.hMutex);

    return status;
}

bool EthFwVlan_isInVlan(EthRemoteCfg_VirtPort virtPort,
                        uint16_t vlanId)
{
    EthFwVlan_Vlan *vlan = NULL;
    bool active = false;

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
        appLogPrintf("ETHFW: Too many VLANs (%u), max is %u\n",
                     cfg->numVlans, ETHFWVLAN_VLANS_MAX);
        status = ENET_EINVALIDPARAMS;
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
                appLogPrintf("ETHFW: VLAN id %u cannot be same as default VLAN ids (%u, %u)\n",
                             vlanId,
                             cfg->dfltVlanIdSwitchPorts,
                             cfg->dfltVlanIdMacOnlyPorts);
                status = ENET_EINVALIDPARAMS;
                break;
            }

            /* Cannot have MAC-only and switch ports in the same VLAN, else leaks */
            if (status == ENET_SOK)
            {
                if (ENET_NOT_ZERO(memberMask & aleSwitchPortMask) &&
                    ENET_NOT_ZERO(memberMask & aleMacOnlyPortMask))
                {
                    appLogPrintf("ETHFW: VLAN %u: member list 0x%x cannot be both MAC-only and switch ports\n",
                                 vlanId, memberMask);
                    status = ENET_EINVALIDPARAMS;
                    break;
                }
            }

            /* Cannot have more than one MAC-only port in the same VLAN, else leaks */
            if (status == ENET_SOK)
            {
                memberMask &= aleMacOnlyPortMask;
                if (EthFwVlan_popcount(memberMask) > 1U)
                {
                    appLogPrintf("ETHFW: VLAN %u: member list %x cannot have multiple MAC-only ports\n",
                                 vlanId, memberMask);
                    status = ENET_EINVALIDPARAMS;
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

                appLogPrintf("ETHFW: VLAN %u member=0x%x virtMember=0x%x "
                             "regMcastFlood=0x%x unregMcastFlood=0x%x untag=0x%x\n",
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
    vlanInArgs.vidIngressCheck         = true;
    vlanInArgs.limitIPNxtHdr           = false;
    vlanInArgs.disallowIPFrag          = true;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &vlanInArgs, &aleEntry);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_ADD_VLAN, &prms);
    if (status != ENET_SOK)
    {
        appLogPrintf("ETHFW: Failed to add ALE VLAN %u entry: %d\n",
                     vlanId, status);
    }

    /* Add broadcast entry for the VLAN */
    if (status == ENET_SOK)
    {
        EnetUtils_copyMacAddr(&mcastInArgs.addr.addr[0], &gEthFwVlan_bcastAddr[0U]);
        mcastInArgs.addr.vlanId     = vlanId;
        mcastInArgs.info.super      = false;
        mcastInArgs.info.fwdState   = CPSW_ALE_FWDSTLVL_FWD;
        mcastInArgs.info.portMask   = memberMask;
        mcastInArgs.info.numIgnBits = 0U;

        ENET_IOCTL_SET_INOUT_ARGS(&prms, &mcastInArgs, &aleEntry);

        status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_ADD_MCAST, &prms);
        if (status != ENET_SOK)
        {
            appLogPrintf("ETHFW: Failed to add VLAN %u bcast ALE entry: %d\n",
                         vlanId, status);
        }
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
    if (status != ENET_SOK)
    {
        appLogPrintf("ETHFW: Failed to delete VLAN %u bcast entry: %d\n",
                     vlanId, status);
    }

    /* Delete VLAN entry */
    vlanInArgs.vlanId  = vlanId;
    vlanInArgs.tagType = ENET_VLAN_TAG_TYPE_INNER;

    ENET_IOCTL_SET_IN_ARGS(&prms, &vlanInArgs);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_REMOVE_VLAN, &prms);
    if (status != ENET_SOK)
    {
        appLogPrintf("ETHFW: Failed to delete ALE VLAN %u entry: %d\n",
                     vlanId, status);
    }
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
    polInArgs.threadIdEn = true;
    polInArgs.threadId   = flowIdxOffset;
    polInArgs.peakRateInBitsPerSec   = 0U;
    polInArgs.commitRateInBitsPerSec = 0U;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &polInArgs, &polOutArgs);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_SET_POLICER, &prms);
    if (status != ENET_SOK)
    {
        appLogPrintf("ETHFW: Failed to setup ALE classifier VLAN %u: %d\n",
                     vlanId, status);
    }

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
    if (status != ENET_SOK)
    {
        appLogPrintf("ETHFW: Failed to find ALE classifier for VLAN %u macAdd=%x:%x:%x:%x:%x:%x: %d\n",
                     vlanId,
                     macAddr[0U], macAddr[1U], macAddr[2U],
                     macAddr[3U], macAddr[4U], macAddr[5U],
                     status);
    }

    /* Check if classifier was routing packets to client's flow */
    if (status == ENET_SOK)
    {
        if ((polOutArgs.threadIdEn != true) ||
            (polOutArgs.threadId != flowIdxOffset))
        {
            appLogPrintf("ETHFW: Invalid VLAN %u policer thread cfg (threadIdEn=%u threadId=%u)\n",
                         vlanId, polOutArgs.threadIdEn, polOutArgs.threadId);
            status = ENET_EUNEXPECTED;
        }
    }

    /* Delete classifier and its VLAN/MAC entry */
    if (status == ENET_SOK)
    {
        delPolInArgs.policerMatch = polMatchInArgs;
        delPolInArgs.aleEntryMask = CPSW_ALE_POLICER_TABLEENTRY_DELETE_ALL;

        ENET_IOCTL_SET_IN_ARGS(&prms, &delPolInArgs);

        status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_DEL_POLICER, &prms);
        if (status != ENET_SOK)
        {
            appLogPrintf("ETHFW: Invalid VLAN %u policer: %d\n",
                         vlanId, status);
        }
    }

    return status;
}

