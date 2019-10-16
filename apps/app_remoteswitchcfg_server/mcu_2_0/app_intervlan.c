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

/*!
 * \file     app_intervlan.c
 *
 * \brief    This file contains the app_switch test implementation.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <xdc/std.h>

#include <xdc/runtime/Error.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/utils/Load.h>

#include <ti/osal/osal.h>
#include <ti/drv/uart/UART_stdio.h>

#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils_cfg.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appsoc.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpswapp_ethutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_mcm.h>
#include <ti/drv/cpsw/cpsw_cfgserver/cpsw_cfgserver.h>

#include <ti/board/board.h>

#include "app_intervlan.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define CPSW_TEST_INTERVLAN_INGRESS_PORT_NUM            (CPSW_MAC_PORT_3)
#define CPSW_TEST_INTERVLAN_EGRESS_PORT_NUM             (CPSW_MAC_PORT_2)

#define CPSW_TEST_INTERVLAN_INGRESS_VLANID              (100)
#define CPSW_TEST_INTERVLAN_EGRESS_VLANID               (200)
#define CPSW_TEST_INTERVLAN_HOSTPORT_PVID               (300)
#define CPSW_TEST_INTERVLAN_MACPORT_PVID_BASE           (400)
#define CPSW_TEST_INTERVLAN_DEFAULT_SHORTIPG_THRESHOLD  (11)

#define CPSW_TEST_INTERVLAN_IPV6_HOP_LIMIT_OFFSET       (7)
#define CPSW_TEST_INTERVLAN_IPV4_TTL_OFFSET             (8)
#define CPSW_TEST_INTERVLAN_IPV6_ETHERTYPE              (0x86DD)
#define CPSW_TEST_INTERVLAN_IPV4_ETHERTYPE              (0x0800)
#define CPSW_TEST_IPV6_OCTET2ARRAY(x)                   0x00, x
#define CPSW_TEST_IPV6_HEXTET2ARRAY(x)                  ((x) & 0xFF00) >> 8, ((x) & 0xFF)

/* ========================================================================== */
/*                         Structures and Enums                               */
/* ========================================================================== */

/* ========================================================================== */
/*                 Internal Function Declarations                             */
/* ========================================================================== */

static int32_t CpswAppInterVlan_setInterVlanUniEgress(Cpsw_Handle hCpsw,
                                                      CpswMacPort_InterVLANRouteId expectedAllocRouteId,
                                                      uint32_t *pNumRoutesUsed,
                                                      CpswCfgServer_InterVlanConfig *pInterVlanCfg);

static int32_t CpswAppInterVlan_setShortIPG(Cpsw_Handle hCpsw);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

static uint8_t testSrcIpv6Addr[16] = {CPSW_TEST_IPV6_HEXTET2ARRAY(0x2000),
                                      CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                      CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                      CPSW_TEST_IPV6_OCTET2ARRAY(0x1),
                                      CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                      CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                      CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                      CPSW_TEST_IPV6_OCTET2ARRAY(0x2)};
uint8_t testDstIpv6Addr[16] = {CPSW_TEST_IPV6_HEXTET2ARRAY(0x2000),
                               CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                               CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                               CPSW_TEST_IPV6_OCTET2ARRAY(0x2),
                               CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                               CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                               CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                               CPSW_TEST_IPV6_OCTET2ARRAY(0x2)};
uint8_t testDstIpv6Addr2[16] = {CPSW_TEST_IPV6_HEXTET2ARRAY(0x2000),
                                CPSW_TEST_IPV6_OCTET2ARRAY(0x1),
                                CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                CPSW_TEST_IPV6_OCTET2ARRAY(0x4),
                                CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                CPSW_TEST_IPV6_OCTET2ARRAY(0x2)};
uint8_t testDstIpv6AddrMcast[] = {CPSW_TEST_IPV6_HEXTET2ARRAY(0x2FFF),
                                  CPSW_TEST_IPV6_OCTET2ARRAY(0x1),
                                  CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                  CPSW_TEST_IPV6_OCTET2ARRAY(0x4),
                                  CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                  CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                  CPSW_TEST_IPV6_OCTET2ARRAY(0x0),
                                  CPSW_TEST_IPV6_OCTET2ARRAY(0x2)};

static uint8_t testHostMacAddr[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x02};

void CpswAppInterVlan_setOpenPrms(Cpsw_Config *pCpswCfg)
{
    Cpsw_MacPort i;

    /* pCpswCfg->aleConfig.policerGlobalConfig.policingEnable SHOULD BE TRUE for interVLan.
     * Set to FALSE to exercise driver internal logic to auto enable policer when interVLan API
     * is invoked
     */
    pCpswCfg->aleConfig.policerGlobalConfig.policingEnable = TRUE;

    pCpswCfg->aleConfig.modeFlags = CPSW_ALE_CONFIG_MASK_ALE_MODULE_ENABLE;

    pCpswCfg->aleConfig.policerGlobalConfig.redDropEnable = FALSE;
    pCpswCfg->aleConfig.policerGlobalConfig.yellowDropEnable = FALSE;
    pCpswCfg->aleConfig.policerGlobalConfig.policerNoMatchMode = CPSW_ALE_POLICER_NOMATCH_MODE_GREEN;
    pCpswCfg->aleConfig.vlanConfig.aleVlanAwareMode = TRUE;
    pCpswCfg->aleConfig.vlanConfig.cpswVlanAwareMode = TRUE;
    pCpswCfg->aleConfig.vlanConfig.unknownVlanMemberListMask = 0;
    pCpswCfg->aleConfig.nwSecCfg.enableVid0Mode = TRUE;

    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].learningCfg.noLearn = FALSE;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].vlanCfg.dropUntagged = FALSE;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].pvidCfg.unregMcastFloodMask = 0x0;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].pvidCfg.regMcastFloodMask = CPSW_ALE_ALL_PORTS_MASK;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].pvidCfg.forceUntaggedEgressMask = CPSW_ALE_ALL_PORTS_MASK;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].pvidCfg.noLearnMask = 0x0;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].pvidCfg.vidIngressCheck = 0x0;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].pvidCfg.limitIPNxtHdr = false;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].pvidCfg.disallowIPFragmentation = false;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].pvidCfg.vlanIdInfo.outerVlanFlag = false;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].pvidCfg.vlanIdInfo.vlanId = CPSW_TEST_INTERVLAN_HOSTPORT_PVID;
    pCpswCfg->aleConfig.portCfg[CPSW_ALE_HOST_PORT_NUM].pvidCfg.vlanMemberList = CPSW_ALE_ALL_PORTS_MASK;

    for (i = CPSW_MAC_PORT_FIRST; i <= CPSW_MAC_PORT_LAST; i++)
    {
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].learningCfg.noLearn = FALSE;
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].vlanCfg.dropUntagged = FALSE;
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].pvidCfg.unregMcastFloodMask = 0x0;
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].pvidCfg.regMcastFloodMask = CPSW_ALE_ALL_PORTS_MASK;
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].pvidCfg.forceUntaggedEgressMask = CPSW_ALE_ALL_PORTS_MASK;
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].pvidCfg.noLearnMask = 0x0;
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].pvidCfg.vidIngressCheck = 0x0;
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].pvidCfg.limitIPNxtHdr = false;
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].pvidCfg.disallowIPFragmentation = false;
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].pvidCfg.vlanIdInfo.outerVlanFlag = false;
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].pvidCfg.vlanIdInfo.vlanId = CPSW_TEST_INTERVLAN_MACPORT_PVID_BASE + CPSW_NORMALIZE_MACPORT(i);
        pCpswCfg->aleConfig.portCfg[CPSW_ALE_MACPORT_TO_ALEPORT(i)].pvidCfg.vlanMemberList = CPSW_ALE_ALL_PORTS_MASK;
    }

    pCpswCfg->hostPortConfig.vlanCfg.portPri = 7;
    pCpswCfg->hostPortConfig.vlanCfg.portCfi = 0;
    pCpswCfg->hostPortConfig.vlanCfg.portVID = CPSW_TEST_INTERVLAN_HOSTPORT_PVID;
    pCpswCfg->vlanConfig.vlanAware = TRUE;
}

void CpswAppInterVlan_setMacConfig(Cpsw_OpenPortLinkInArgs *pLinkArgs,
                                   uint32_t portNum)
{
    pLinkArgs->macConfig.enableLoopback = FALSE;

    pLinkArgs->macConfig.vlanConfig.portPri = CPSW_NORMALIZE_MACPORT(portNum);
    pLinkArgs->macConfig.vlanConfig.portCfi = 0;
    pLinkArgs->macConfig.vlanConfig.portVID = CPSW_TEST_INTERVLAN_MACPORT_PVID_BASE + CPSW_NORMALIZE_MACPORT(portNum);
}

static uint32_t CpswAppInterVlan_getIngressVlanMembershipMask(CpswCfgServer_InterVlanConfig *pInterVlanCfg)
{
    uint32_t memberShipMask;

    memberShipMask =
        (1 << CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->ingPortNum));
    memberShipMask |= CPSW_ALE_HOST_PORT_MASK;
    return memberShipMask;
}

static uint32_t CpswAppInterVlan_getEgressVlanMembershipMask(CpswCfgServer_InterVlanConfig *pInterVlanCfg)
{
    uint32_t memberShipMask;

    memberShipMask =
        (1 << CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->egrPortNum));
    memberShipMask |= CPSW_ALE_HOST_PORT_MASK;
    return memberShipMask;
}

static int32_t CpswAppInterVlan_addUniEgressAleTableEntries(Cpsw_Handle hCpsw,
                                                            CpswCfgServer_InterVlanConfig *pInterVlanCfg)
{
    int32_t status;
    Cpsw_IoctlPrms prms;
    CpswAle_AddEntryOutArgs setUcastOutArgs;
    CpswAle_SetUcastEntryInArgs setUcastInArgs;

    memcpy(&setUcastInArgs.addr.addr[0U], testHostMacAddr,
           sizeof(setUcastInArgs.addr.addr));
    setUcastInArgs.addr.vlanId = 0;
    setUcastInArgs.info.portNum = CPSW_ALE_HOST_PORT_NUM;
    setUcastInArgs.info.blocked = false;
    setUcastInArgs.info.secure = false;
    setUcastInArgs.info.super = 0U;
    setUcastInArgs.info.ageable = false;
    setUcastInArgs.info.trunk = false;

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setUcastInArgs, &setUcastOutArgs);

    status = Cpsw_ioctl(hCpsw, CpswAppSoc_getCoreId(), CPSW_ALE_IOCTL_ADD_UNICAST,
                        &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
                           "%s() failed CPSW_ALE_IOCTL_ADD_UNICAST: %d\n",
                           __func__, status);
    }

    if (status == CPSW_SOK)
    {
        CpswAle_VlanEntryInfo inArgs;
        CpswAle_AddEntryOutArgs outArgs;

        inArgs.vlanIdInfo.vlanId = pInterVlanCfg->ingVlanId;
        inArgs.vlanIdInfo.outerVlanFlag = false;
        inArgs.vlanMemberList = CpswAppInterVlan_getIngressVlanMembershipMask(pInterVlanCfg);
        inArgs.unregMcastFloodMask = CpswAppInterVlan_getIngressVlanMembershipMask(pInterVlanCfg);
        inArgs.regMcastFloodMask = CpswAppInterVlan_getIngressVlanMembershipMask(pInterVlanCfg);
        inArgs.forceUntaggedEgressMask = 0;
        inArgs.noLearnMask = 0U;
        inArgs.vidIngressCheck = false;
        inArgs.limitIPNxtHdr = false;
        inArgs.disallowIPFragmentation = false;

        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &outArgs);

        status = Cpsw_ioctl(hCpsw, CpswAppSoc_getCoreId(), CPSW_ALE_IOCTL_ADD_VLAN, &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print(
                               "%s() failed ADD_VLAN ioctl failed: %d\n",
                               __func__, status);
        }
    }

    if (status == CPSW_SOK)
    {
        CpswAle_VlanEntryInfo inArgs;
        CpswAle_AddEntryOutArgs outArgs;

        inArgs.vlanIdInfo.vlanId = pInterVlanCfg->egrVlanId;
        inArgs.vlanIdInfo.outerVlanFlag = false;
        inArgs.vlanMemberList = CpswAppInterVlan_getEgressVlanMembershipMask(pInterVlanCfg);
        inArgs.unregMcastFloodMask = CpswAppInterVlan_getEgressVlanMembershipMask(pInterVlanCfg);
        inArgs.regMcastFloodMask = CpswAppInterVlan_getEgressVlanMembershipMask(pInterVlanCfg);
        inArgs.forceUntaggedEgressMask = 0;
        inArgs.noLearnMask = 0U;
        inArgs.vidIngressCheck = false;
        inArgs.limitIPNxtHdr = false;
        inArgs.disallowIPFragmentation = false;

        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &outArgs);

        status = Cpsw_ioctl(hCpsw, CpswAppSoc_getCoreId(), CPSW_ALE_IOCTL_ADD_VLAN, &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print(
                               "%s() failed ADD_VLAN ioctl failed: %d\n",
                               __func__, status);
        }
    }

    return status;
}

static int32_t CpswAppInterVlan_setInterVlanUniEgress(Cpsw_Handle hCpsw,
                                                      CpswMacPort_InterVLANRouteId expectedAllocRouteId,
                                                      uint32_t *pNumRoutesUsed,
                                                      CpswCfgServer_InterVlanConfig *pInterVlanCfg)
{
    int32_t status;
    Cpsw_IoctlPrms prms;
    Cpsw_SetInterVlanRouteUniEgressInArgs inArgs;
    Cpsw_SetInterVlanRouteUniEgressOutArgs outArgs;

    *pNumRoutesUsed = 0;
    status = CpswAppInterVlan_addUniEgressAleTableEntries(hCpsw, pInterVlanCfg);

    if (CPSW_SOK == status)
    {
        /* Set to invalid id and confirm outArgs populated correctly after IOCTL
         * called
         */
        outArgs.egressPortRouteId = CPSW_MACPORT_INTERVLAN_ROUTEID_LAST;

        inArgs.inPktMatchCfg.packetMatchEnableMask = 0;

        inArgs.inPktMatchCfg.ingressPort = (Cpsw_MacPort)pInterVlanCfg->ingPortNum;
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_PORT;

        inArgs.inPktMatchCfg.dstIp.ipAddrtype = CPSW_ALE_IPADDR_CLASSIFIER_IPV6;
        inArgs.inPktMatchCfg.dstIp.ipv6.numLSBIgnoreBits = 8;
        memcpy(inArgs.inPktMatchCfg.dstIp.ipv6.ipv6Addr, testDstIpv6Addr, sizeof(inArgs.inPktMatchCfg.dstIp.ipv6.ipv6Addr));
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_IPDST;

        memcpy(inArgs.inPktMatchCfg.dstMacAddr.addr.addr, testHostMacAddr, sizeof(inArgs.inPktMatchCfg.dstMacAddr.addr));
        inArgs.inPktMatchCfg.dstMacAddr.addr.vlanId = 0;
        inArgs.inPktMatchCfg.dstMacAddr.egressPortNum = CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->egrPortNum);

        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_MACDST;

        inArgs.inPktMatchCfg.srcIp.ipAddrtype = CPSW_ALE_IPADDR_CLASSIFIER_IPV6;
        inArgs.inPktMatchCfg.srcIp.ipv6.numLSBIgnoreBits = 8;
        memcpy(inArgs.inPktMatchCfg.srcIp.ipv6.ipv6Addr, testSrcIpv6Addr, sizeof(inArgs.inPktMatchCfg.srcIp.ipv6.ipv6Addr));
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_IPSRC;

        memcpy(inArgs.inPktMatchCfg.srcMacAddr.addr.addr, &pInterVlanCfg->srcMacAddr[0U], sizeof(inArgs.inPktMatchCfg.srcMacAddr.addr.addr));
        inArgs.inPktMatchCfg.srcMacAddr.addr.vlanId = 0;
        inArgs.inPktMatchCfg.srcMacAddr.ingressPortNum = CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->ingPortNum);
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_MACSRC;

        inArgs.inPktMatchCfg.etherType.etherType = CPSW_TEST_INTERVLAN_IPV6_ETHERTYPE;
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_ETHERTYPE;

        inArgs.inPktMatchCfg.vlanId = pInterVlanCfg->ingVlanId;
        inArgs.inPktMatchCfg.enableTTLCheck = TRUE;

        inArgs.egressCfg.egressPort = (Cpsw_MacPort)pInterVlanCfg->egrPortNum;
        inArgs.egressCfg.outPktModCfg.decrementTTL = TRUE;
        inArgs.egressCfg.outPktModCfg.forceUntaggedEgress = FALSE;
        inArgs.egressCfg.outPktModCfg.replaceDASA = TRUE;
        memcpy(inArgs.egressCfg.outPktModCfg.srcAddr, testHostMacAddr, sizeof(inArgs.egressCfg.outPktModCfg.srcAddr));
        memcpy(inArgs.egressCfg.outPktModCfg.dstAddr, &pInterVlanCfg->dstMacAddr[0U], sizeof(inArgs.egressCfg.outPktModCfg.dstAddr));
        inArgs.egressCfg.outPktModCfg.vlanId = pInterVlanCfg->egrVlanId;

        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &outArgs);

        status = Cpsw_ioctl(hCpsw, CpswAppSoc_getCoreId(), CPSW_IOCTL_SET_INTERVLAN_ROUTE_UNI_EGRESS,
                            &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print(
                               "CpswAppInterVlan_setInterVlanUniEgress() failed CPSW_IOCTL_SET_INTERVLAN_ROUTE_UNI_EGRESS: %d\n",
                               status);
        }
    }

    if (status == CPSW_SOK)
    {
        *pNumRoutesUsed += 1;
        // CpswAppUtils_assert(outArgs.egressPortRouteId == expectedAllocRouteId);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.port == CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->ingPortNum));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.portIsTrunk == FALSE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.egressOpEnabled == TRUE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.egressOpcode == (1 + (outArgs.egressPortRouteId - CPSW_MACPORT_INTERVLAN_ROUTEID_FIRST)));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.enableTTLCheck == TRUE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.destPortMask == (1 << CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->egrPortNum)));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.policerMatchEnableMask == (CPSW_ALE_POLICER_MATCH_PORT |
                                                                                           CPSW_ALE_POLICER_MATCH_MACSRC |
                                                                                           CPSW_ALE_POLICER_MATCH_MACDST |
                                                                                           CPSW_ALE_POLICER_MATCH_IVLAN |
                                                                                           CPSW_ALE_POLICER_MATCH_ETHERTYPE |
                                                                                           CPSW_ALE_POLICER_MATCH_IPSRC |
                                                                                           CPSW_ALE_POLICER_MATCH_IPDST));
    }

    if (status == CPSW_SOK)
    {
        /* Add another route to send packet out on ingress port.
         * Only destIP changes. Rest of params remain same
         */
        inArgs.inPktMatchCfg.dstIp.ipAddrtype = CPSW_ALE_IPADDR_CLASSIFIER_IPV6;
        inArgs.inPktMatchCfg.dstIp.ipv6.numLSBIgnoreBits = 8;
        memcpy(inArgs.inPktMatchCfg.dstIp.ipv6.ipv6Addr, testDstIpv6Addr2, sizeof(inArgs.inPktMatchCfg.dstIp.ipv6.ipv6Addr));
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_IPDST;

        inArgs.egressCfg.egressPort = (Cpsw_MacPort)pInterVlanCfg->ingPortNum;

        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &outArgs);

        status = Cpsw_ioctl(hCpsw, CpswAppSoc_getCoreId(), CPSW_IOCTL_SET_INTERVLAN_ROUTE_UNI_EGRESS,
                            &prms);

        if (status != CPSW_SOK)
        {
            CpswAppUtils_print(
                               "CpswAppInterVlan_setInterVlanUniEgress() failed CPSW_IOCTL_SET_INTERVLAN_ROUTE_UNI_EGRESS: %d\n",
                               status);
        }
    }

    if (status == CPSW_SOK)
    {
        /* Do not increment numRoutesUsed as it is a different egress port */
        // CpswAppUtils_assert(outArgs.egressPortRouteId == expectedAllocRouteId);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.port == CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->ingPortNum));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.portIsTrunk == FALSE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.egressOpEnabled == TRUE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.egressOpcode == (1 + (outArgs.egressPortRouteId - CPSW_MACPORT_INTERVLAN_ROUTEID_FIRST)));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.enableTTLCheck == TRUE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.destPortMask == (1 << CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->ingPortNum)));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.policerMatchEnableMask == (CPSW_ALE_POLICER_MATCH_PORT |
                                                                                           CPSW_ALE_POLICER_MATCH_MACSRC |
                                                                                           CPSW_ALE_POLICER_MATCH_MACDST |
                                                                                           CPSW_ALE_POLICER_MATCH_IVLAN |
                                                                                           CPSW_ALE_POLICER_MATCH_ETHERTYPE |
                                                                                           CPSW_ALE_POLICER_MATCH_IPSRC |
                                                                                           CPSW_ALE_POLICER_MATCH_IPDST));
    }

    /* Add a route in opposite direction to enable bidirectional intervlan switching */
    if (CPSW_SOK == status)
    {
        /* Set to invalid id and confirm outArgs populated correctly after IOCTL
         * called
         */
        outArgs.egressPortRouteId = CPSW_MACPORT_INTERVLAN_ROUTEID_LAST;

        inArgs.inPktMatchCfg.packetMatchEnableMask = 0;

        inArgs.inPktMatchCfg.ingressPort = (Cpsw_MacPort)pInterVlanCfg->egrPortNum;
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_PORT;

        inArgs.inPktMatchCfg.dstIp.ipAddrtype = CPSW_ALE_IPADDR_CLASSIFIER_IPV6;
        inArgs.inPktMatchCfg.dstIp.ipv6.numLSBIgnoreBits = 8;
        memcpy(inArgs.inPktMatchCfg.dstIp.ipv6.ipv6Addr, testSrcIpv6Addr, sizeof(inArgs.inPktMatchCfg.dstIp.ipv6.ipv6Addr));
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_IPDST;

        memcpy(inArgs.inPktMatchCfg.dstMacAddr.addr.addr, testHostMacAddr, sizeof(inArgs.inPktMatchCfg.dstMacAddr.addr));
        inArgs.inPktMatchCfg.dstMacAddr.addr.vlanId = 0;
        inArgs.inPktMatchCfg.dstMacAddr.egressPortNum = CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->egrPortNum);

        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_MACDST;

        inArgs.inPktMatchCfg.srcIp.ipAddrtype = CPSW_ALE_IPADDR_CLASSIFIER_IPV6;
        inArgs.inPktMatchCfg.srcIp.ipv6.numLSBIgnoreBits = 8;
        memcpy(inArgs.inPktMatchCfg.srcIp.ipv6.ipv6Addr, testDstIpv6Addr, sizeof(inArgs.inPktMatchCfg.srcIp.ipv6.ipv6Addr));
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_IPSRC;

        memcpy(inArgs.inPktMatchCfg.srcMacAddr.addr.addr, &pInterVlanCfg->dstMacAddr[0U], sizeof(inArgs.inPktMatchCfg.srcMacAddr.addr.addr));
        inArgs.inPktMatchCfg.srcMacAddr.addr.vlanId = 0;
        inArgs.inPktMatchCfg.srcMacAddr.ingressPortNum = CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->egrPortNum);
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_MACSRC;

        inArgs.inPktMatchCfg.etherType.etherType = 0x86DD;
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_ETHERTYPE;

        inArgs.inPktMatchCfg.vlanId = pInterVlanCfg->egrPortNum;
        inArgs.inPktMatchCfg.enableTTLCheck = TRUE;

        inArgs.egressCfg.egressPort = (Cpsw_MacPort)pInterVlanCfg->ingPortNum;
        inArgs.egressCfg.outPktModCfg.decrementTTL = TRUE;
        inArgs.egressCfg.outPktModCfg.forceUntaggedEgress = FALSE;
        inArgs.egressCfg.outPktModCfg.replaceDASA = TRUE;
        memcpy(inArgs.egressCfg.outPktModCfg.srcAddr, testHostMacAddr, sizeof(inArgs.egressCfg.outPktModCfg.srcAddr));
        memcpy(inArgs.egressCfg.outPktModCfg.dstAddr, &pInterVlanCfg->srcMacAddr[0U], sizeof(inArgs.egressCfg.outPktModCfg.dstAddr));
        inArgs.egressCfg.outPktModCfg.vlanId = pInterVlanCfg->ingVlanId;

        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &outArgs);

        status = Cpsw_ioctl(hCpsw, CpswAppSoc_getCoreId(), CPSW_IOCTL_SET_INTERVLAN_ROUTE_UNI_EGRESS,
                            &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print(
                               "CpswAppInterVlan_setInterVlanUniEgress() failed CPSW_IOCTL_SET_INTERVLAN_ROUTE_UNI_EGRESS: %d\n",
                               status);
        }
    }

    if (status == CPSW_SOK)
    {
        // CpswAppUtils_assert(outArgs.egressPortRouteId == (expectedAllocRouteId + 1));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.port == CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->egrPortNum));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.portIsTrunk == FALSE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.egressOpEnabled == TRUE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.egressOpcode == (1 + (outArgs.egressPortRouteId - CPSW_MACPORT_INTERVLAN_ROUTEID_FIRST)));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.enableTTLCheck == TRUE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.destPortMask == (1 << CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->ingPortNum)));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.policerMatchEnableMask == (CPSW_ALE_POLICER_MATCH_PORT |
                                                                                           CPSW_ALE_POLICER_MATCH_MACSRC |
                                                                                           CPSW_ALE_POLICER_MATCH_MACDST |
                                                                                           CPSW_ALE_POLICER_MATCH_IVLAN |
                                                                                           CPSW_ALE_POLICER_MATCH_ETHERTYPE |
                                                                                           CPSW_ALE_POLICER_MATCH_IPSRC |
                                                                                           CPSW_ALE_POLICER_MATCH_IPDST));
    }

    /* Add IPv4 intervlan switching route */
    if (CPSW_SOK == status)
    {
        /* Set to invalid id and confirm outArgs populated correctly after IOCTL
         * called
         */
        outArgs.egressPortRouteId = CPSW_MACPORT_INTERVLAN_ROUTEID_LAST;

        inArgs.inPktMatchCfg.packetMatchEnableMask = 0;

        inArgs.inPktMatchCfg.srcIp.ipAddrtype = CPSW_ALE_IPADDR_CLASSIFIER_IPV4;
        inArgs.inPktMatchCfg.srcIp.ipv4.numLSBIgnoreBits = 0U;
        memcpy(inArgs.inPktMatchCfg.srcIp.ipv4.ipv4Addr, &pInterVlanCfg->srcIpv4Addr[0U], sizeof(inArgs.inPktMatchCfg.srcIp.ipv4.ipv4Addr));
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_IPSRC;

        inArgs.inPktMatchCfg.dstIp.ipAddrtype = CPSW_ALE_IPADDR_CLASSIFIER_IPV4;
        inArgs.inPktMatchCfg.dstIp.ipv4.numLSBIgnoreBits = 0U;
        memcpy(inArgs.inPktMatchCfg.dstIp.ipv4.ipv4Addr, &pInterVlanCfg->dstIpv4Addr[0U], sizeof(inArgs.inPktMatchCfg.dstIp.ipv4.ipv4Addr));
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_IPDST;

        memcpy(inArgs.inPktMatchCfg.dstMacAddr.addr.addr, testHostMacAddr, sizeof(inArgs.inPktMatchCfg.dstMacAddr.addr));
        inArgs.inPktMatchCfg.dstMacAddr.addr.vlanId = 0;
        inArgs.inPktMatchCfg.dstMacAddr.egressPortNum = CPSW_ALE_HOST_PORT_NUM;

        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_MACDST;

        memcpy(inArgs.inPktMatchCfg.srcMacAddr.addr.addr, &pInterVlanCfg->srcMacAddr[0U], sizeof(inArgs.inPktMatchCfg.srcMacAddr.addr.addr));
        inArgs.inPktMatchCfg.srcMacAddr.addr.vlanId = 0;
        inArgs.inPktMatchCfg.srcMacAddr.ingressPortNum = CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->ingPortNum);
        inArgs.inPktMatchCfg.packetMatchEnableMask |= CPSW_INTERVLAN_INGRESSPKT_MATCH_MACSRC;

        inArgs.inPktMatchCfg.vlanId = pInterVlanCfg->ingVlanId;
        inArgs.inPktMatchCfg.enableTTLCheck = TRUE;

        inArgs.egressCfg.egressPort = (Cpsw_MacPort)pInterVlanCfg->egrPortNum;
        inArgs.egressCfg.outPktModCfg.decrementTTL = TRUE;
        inArgs.egressCfg.outPktModCfg.forceUntaggedEgress = FALSE;
        inArgs.egressCfg.outPktModCfg.replaceDASA = TRUE;
        memcpy(inArgs.egressCfg.outPktModCfg.srcAddr, testHostMacAddr, sizeof(inArgs.egressCfg.outPktModCfg.srcAddr));
        memcpy(inArgs.egressCfg.outPktModCfg.dstAddr, &pInterVlanCfg->dstMacAddr[0U], sizeof(inArgs.egressCfg.outPktModCfg.dstAddr));
        inArgs.egressCfg.outPktModCfg.vlanId = pInterVlanCfg->egrVlanId;

        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &outArgs);

        status = Cpsw_ioctl(hCpsw, CpswAppSoc_getCoreId(), CPSW_IOCTL_SET_INTERVLAN_ROUTE_UNI_EGRESS,
                            &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print(
                               "CpswAppInterVlan_setInterVlanUniEgress() failed CPSW_IOCTL_SET_INTERVLAN_ROUTE_UNI_EGRESS: %d\n",
                               status);
        }
    }

    if (status == CPSW_SOK)
    {
        *pNumRoutesUsed += 1;
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.egressOpEnabled == TRUE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.egressOpcode == (1 + (outArgs.egressPortRouteId - CPSW_MACPORT_INTERVLAN_ROUTEID_FIRST)));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.enableTTLCheck == TRUE);
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.destPortMask == (1 << CPSW_ALE_MACPORT_TO_ALEPORT(pInterVlanCfg->egrPortNum)));
        CpswAppUtils_assert(outArgs.ingressPacketClassifierInfo.policerMatchEnableMask == (CPSW_ALE_POLICER_MATCH_MACSRC |
                                                                                           CPSW_ALE_POLICER_MATCH_MACDST |
                                                                                           CPSW_ALE_POLICER_MATCH_IVLAN |
                                                                                           CPSW_ALE_POLICER_MATCH_IPSRC |
                                                                                           CPSW_ALE_POLICER_MATCH_IPDST));
    }

    return status;
}

static int32_t CpswAppInterVlan_setShortIPG(Cpsw_Handle hCpsw)
{
    Cpsw_IoctlPrms prms;
    Cpsw_SetShortTxIPGConfigInArgs setShortIPGInArgs;
    int32_t status;

    CPSW_IOCTL_SET_IN_ARGS(&prms, &setShortIPGInArgs);
    setShortIPGInArgs.configureGapThreshold = FALSE;
    setShortIPGInArgs.numMacPorts = 2;
    setShortIPGInArgs.portIPGCfg[0].portNum = CPSW_TEST_INTERVLAN_INGRESS_PORT_NUM;
    setShortIPGInArgs.portIPGCfg[0].shortGapCfg.enableTxShortGap = false;
    setShortIPGInArgs.portIPGCfg[0].shortGapCfg.enableTxShortGapLimit = false;

    setShortIPGInArgs.portIPGCfg[1].portNum = CPSW_TEST_INTERVLAN_EGRESS_PORT_NUM;
    setShortIPGInArgs.portIPGCfg[1].shortGapCfg.enableTxShortGap = false;
    setShortIPGInArgs.portIPGCfg[1].shortGapCfg.enableTxShortGapLimit = false;

    status = Cpsw_ioctl(hCpsw, CpswAppSoc_getCoreId(), CPSW_IOCTL_SET_SHORT_IPG_CONFIG,
                        &prms);
    if (CPSW_SOK == status)
    {
        Cpsw_GetShortTxIPGConfigOutArgs getShortIPGOutArgs;

        CPSW_IOCTL_SET_OUT_ARGS(&prms, &getShortIPGOutArgs);

        status = Cpsw_ioctl(hCpsw, CpswAppSoc_getCoreId(), CPSW_IOCTL_GET_SHORT_IPG_CONFIG,
                            &prms);
        if (CPSW_SOK == status)
        {
            uint32_t i;

            CpswAppUtils_assert(getShortIPGOutArgs.ipgTriggerThresholdBlkCount == CPSW_TEST_INTERVLAN_DEFAULT_SHORTIPG_THRESHOLD);
            for (i = 0; i < getShortIPGOutArgs.numMacPorts; i++)
            {
                if ((getShortIPGOutArgs.portIPGCfg[i].portNum == CPSW_TEST_INTERVLAN_EGRESS_PORT_NUM)
                    ||
                    (getShortIPGOutArgs.portIPGCfg[i].portNum == CPSW_TEST_INTERVLAN_INGRESS_PORT_NUM))
                {
                    CpswAppUtils_assert(getShortIPGOutArgs.portIPGCfg[i].shortGapCfg.enableTxShortGap == false);
                    CpswAppUtils_assert(getShortIPGOutArgs.portIPGCfg[i].shortGapCfg.enableTxShortGapLimit == false);
                }
            }
        }
    }

    return status;
}

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

void CpswApp_hwInterVlanRouting(Cpsw_Type cpswType,
                                CpswCfgServer_InterVlanConfig *pInterVlanCfg)
{
    int32_t status = CPSW_SOK;
    uint32_t numRoutesAllocated = 0;
    Cpsw_Handle hCpsw;

    /* Get CPSW Handle */
    hCpsw = Cpsw_getHandle(cpswType);

    status = CpswAppInterVlan_setShortIPG(hCpsw);

    if (CPSW_SOK == status)
    {
        status = CpswAppInterVlan_setInterVlanUniEgress(hCpsw,
                                                        CPSW_MACPORT_INTERVLAN_ROUTEID_FIRST,
                                                        &numRoutesAllocated,
                                                        pInterVlanCfg);
    }

    CpswAppUtils_assert(status == CPSW_SOK);
}
