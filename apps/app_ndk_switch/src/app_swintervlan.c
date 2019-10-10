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
 * \file     app_intervlanrouting.c
 *
 * \brief    This file contains the app_intervlanrouting test implementation.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <ti/drv/uart/UART_stdio.h>
#include <ti/csl/csl_cpswitch.h>

#include <ti/drv/udma/udma.h>
#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils_cfg.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appsoc.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpswapp_ethutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_mcm.h>

#include <ti/osal/osal.h>
#include <ti/sysbios/hal/Cache.h>

#include "app_swintervlan.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
/* Un comment the below macro to print the packet count on UART */
//#define APP_PRINTPKTCNT

#define CPSW_FRWD_APP_NUM_PKTS          (192U)
#define RX_TX_COMPLETION_TIMEOUT        (1U)
#define PKT_HEADER_SIZE                 (64U)
#define APP_TSK_STACK_SIZE              (6U * 1024U)

#define APP_INTERVLAN_INGRESS_PORT_NUM  (CPSW_MAC_PORT_3)
#define APP_INTERVLAN_EGRESS_PORT_NUM   (CPSW_MAC_PORT_2)

#define APP_INTERVLAN_INGRESS_VLANID    (100)
#define APP_INTERVLAN_EGRESS_VLANID     (200)

#define APP_INTERVLAN_IPV4_ETHERTYPE    (0x0800)
/* ========================================================================== */
/*                         Structures and Enums                               */
/* ========================================================================== */

typedef struct
{
    Cpsw_Type            cpswType;

    /* CPSW driver handle */
    Cpsw_Handle          hCpsw;

    Udma_DrvHandle       hUdmaDrv;

    CpswDma_RxFlowHandle hIngRxFlow;

    uint32_t             rxFlowStartIdx;

    uint32_t             ingRxFlowIdx;

    CpswDma_PktInfoQ     rxReadyQ;

    CpswDma_TxChHandle   hTxCh;

    uint32_t             txChNum;

    /* Semaphore for signalling packet ready for processing*/
    Semaphore_Handle     completionSem;

    uint8_t              hostMacAddr[ETH_MAC_ADDR_LEN];

    uint32_t             num_pkts;

    uint32_t             coreId;

    uint32_t             coreKey;

    bool                 isDefaultFlow;

} CpswApp_Obj;

/* ========================================================================== */
/*                 Internal Function Declarations                             */
/* ========================================================================== */

static void      CpswApp_pktRxTx(void);
static int32_t   CpswApp_getRxTxHandle(void);
static uint32_t  CpswApp_receivePkts(void);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

CpswApp_Obj gCpswInterVlanAppObj =
{
     .hostMacAddr = {0x02,0x00,0x00,0x00,0x00,0x02},
};

static uint8_t testSrcMacAddr[] = {0x00,0x11,0x01,0x00,0x00,0x01};
static uint8_t testDstMacAddr[] = {0x00,0x11,0x02,0x00,0x00,0x01};


static uint8_t testSrcIpv4Addr[4] = {192,
                             168,
                             1,
                             202};

static uint8_t testDstIpv4Addr[4] = {192,
                             168,
                             1,
                             204};

/* Test application stack */
#pragma DATA_SECTION(gAppTskStackMain,".bss:appStack")
static uint8_t gAppTskStackMain[APP_TSK_STACK_SIZE]
                                __attribute__((aligned(32)));

static Task_Handle task;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */
static Void CpswApp_InterVlanRouting(UArg a0, UArg a1)
{
    int32_t       status;
    CpswDma_PktInfoQ fqPktInfoQ, cqPktInfoQ;
    Semaphore_Params semParams;
    CpswMcm_HandleInfo handleInfo;
    Cpsw_AttachCoreOutArgs attachInfo;
    CpswMcm_CmdIf cmdIf;

    CpswMcm_getCmdIf(gCpswInterVlanAppObj.cpswType, &cmdIf);
    CpswAppUtils_assert(cmdIf.hMboxCmd != NULL);
    CpswAppUtils_assert(cmdIf.hMboxResponse != NULL);

    gCpswInterVlanAppObj.coreId = CpswAppSoc_getCoreId();


    CpswMcm_acquireHandleInfo(&cmdIf, &handleInfo);
    CpswMcm_coreAttach(&cmdIf, gCpswInterVlanAppObj.coreId, &attachInfo);

    gCpswInterVlanAppObj.hCpsw          = handleInfo.hCpsw;
    gCpswInterVlanAppObj.hUdmaDrv       = handleInfo.hUdmaDrv;
    gCpswInterVlanAppObj.coreKey        = attachInfo.coreKey;
    gCpswInterVlanAppObj.isDefaultFlow  = false;

    if (gCpswInterVlanAppObj.hCpsw  == NULL)
    {
        CpswAppUtils_print("Failed to open CPSW\n");
        CpswAppUtils_assert(gCpswInterVlanAppObj.hCpsw == NULL);
    }

    if (gCpswInterVlanAppObj.hUdmaDrv == NULL)
    {
        CpswAppUtils_print("Failed to Get UDMA Handle\n");
        CpswAppUtils_assert(gCpswInterVlanAppObj.hUdmaDrv == NULL);
    }

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_COUNTING;

    gCpswInterVlanAppObj.completionSem = Semaphore_create(0, &semParams, NULL);

    status = CpswApp_getRxTxHandle();
    CpswAppUtils_print("Enabled Software Inter-VLAN Routing \n");

    if (status == CPSW_SOK)
    {
        CpswApp_pktRxTx();
    }

    CpswUtils_initQ(&fqPktInfoQ);
    CpswUtils_initQ(&cqPktInfoQ);

    /* Close RX Flow */
    CpswAppUtils_closeRxFlow(gCpswInterVlanAppObj.hCpsw,
                             gCpswInterVlanAppObj.coreKey,
                             gCpswInterVlanAppObj.coreId,
                             true,
                             &fqPktInfoQ,
                             &cqPktInfoQ,
                             gCpswInterVlanAppObj.rxFlowStartIdx,
                             gCpswInterVlanAppObj.ingRxFlowIdx,
                             gCpswInterVlanAppObj.hostMacAddr,
                             gCpswInterVlanAppObj.hIngRxFlow);

    CpswAppUtils_freePktInfoQ(&fqPktInfoQ);
    CpswAppUtils_freePktInfoQ(&cqPktInfoQ);

    /* Close TX channel */
    CpswUtils_initQ(&fqPktInfoQ);
    CpswUtils_initQ(&cqPktInfoQ);

    CpswAppUtils_closeTxCh(gCpswInterVlanAppObj.hCpsw,
                           gCpswInterVlanAppObj.coreKey,
                           gCpswInterVlanAppObj.coreId,
                           &fqPktInfoQ,
                           &cqPktInfoQ,
                           gCpswInterVlanAppObj.hTxCh,
                           gCpswInterVlanAppObj.txChNum);

    CpswAppUtils_freePktInfoQ(&fqPktInfoQ);
    CpswAppUtils_freePktInfoQ(&cqPktInfoQ);

    CpswMcm_releaseHandleInfo(&cmdIf);
}

void CpswApp_ingRxIsrFxn(void *appData)
{
     CpswDma_RxFlowHandle *rxFlow = (CpswDma_RxFlowHandle*) appData;

     /* In Rx completion handler, post the Semaphore for Forwarding task to handle
      * further processing. We also disable further Rx completion events notifications
      * here. Once all pending packets are handled, the Forwarding task will re-enable the
      * interrupt subsequently*/

     /*Step1 : disable any further completion event handling*/
     CpswDma_disableRxEvent(*rxFlow);

     /*Step2 : Post semaphore for signalling the Forwarding task */
     Semaphore_post(gCpswInterVlanAppObj.completionSem);

}

void CpswApp_txIsrFxn(void *appData)
{
     CpswDma_TxChHandle *txChn = (CpswDma_TxChHandle*) appData;

     /*Step1 : Disable Tx completion notification callback. Tx completion will be handled as part
      * of Rx completion handling - this being a forwarding case*/
     CpswDma_disableTxEvent(*txChn);

    /*Step2 : Post semaphore for signalling the Forwarding task */
     Semaphore_post(gCpswInterVlanAppObj.completionSem);

}

static void CpswApp_initRxReadyPktQ(void)
{
    CpswDma_PktInfoQ rxFreeQ;
    CpswDma_PktInfoQ rxReadyQ;
    int32_t          status;
    uint32_t         i;
    CpswDma_PktInfo *pPktInfo;

    CpswUtils_initQ(&rxFreeQ);
    CpswUtils_initQ(&rxReadyQ);

    for (i=0U; i<CPSW_FRWD_APP_NUM_PKTS; i++)
    {
        pPktInfo = CpswAppMemUtils_allocEthPktFxn(&gCpswInterVlanAppObj,
                                                  CPSW_APPMEMUTILS_LARGE_POOL_PKT_SIZE,
                                                  UDMA_CACHELINE_ALIGNMENT);
        CpswAppUtils_assert(pPktInfo != NULL);
        CpswUtils_enQ(&rxFreeQ, &pPktInfo->node);
    }

    /* Retrieve any CPSW packets which are ready */
    status = CpswDma_retrieveRxPackets(gCpswInterVlanAppObj.hIngRxFlow, &rxReadyQ);
    CpswAppUtils_assert ( status == CPSW_SOK );
    /* There should not be any packet with DMA during init */
    CpswAppUtils_assert ( CpswUtils_getQCount(&rxReadyQ) == 0U );

    CpswAppUtils_submitRxPackets(gCpswInterVlanAppObj.hIngRxFlow,
                                 &rxFreeQ);
    /* Assert here as during init no. of DMA descriptors should be equal to
     * no. of free Ethernet buffers available with app */

    CpswAppUtils_assert( 0U == CpswUtils_getQCount(&rxFreeQ));

}

static uint32_t CpswAppInterVlan_getIngressVlanMembershipMask(void)
{
    uint32_t memberShipMask;

    memberShipMask =
       (1 << CPSW_ALE_MACPORT_TO_ALEPORT(APP_INTERVLAN_INGRESS_PORT_NUM));
    memberShipMask |= CPSW_ALE_HOST_PORT_MASK;
    return memberShipMask;
}

static uint32_t CpswAppInterVlan_getEgressVlanMembershipMask(void)
{
    uint32_t memberShipMask;

    memberShipMask =
       (1 << CPSW_ALE_MACPORT_TO_ALEPORT(APP_INTERVLAN_EGRESS_PORT_NUM));
    memberShipMask |= CPSW_ALE_HOST_PORT_MASK;
    return memberShipMask;
}

static int32_t CpswApp_addAleEntries(void)
{
    int32_t          status;
    Cpsw_IoctlPrms prms;
    CpswAle_AddEntryOutArgs setUcastOutArgs;
    CpswAle_SetUcastEntryInArgs setUcastInArgs;

    /* Add ALE entry for GW/Router that enables Inter VLAN routing */
    memcpy (&setUcastInArgs.addr.addr[0U], gCpswInterVlanAppObj.hostMacAddr, sizeof (setUcastInArgs.addr.addr));
    setUcastInArgs.addr.vlanId = 0U;
    setUcastInArgs.info.portNum = 0U;
    setUcastInArgs.info.blocked = false;
    setUcastInArgs.info.secure = false;
    setUcastInArgs.info.super = 0U;
    setUcastInArgs.info.ageable = false;
    setUcastInArgs.info.trunk = false;

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setUcastInArgs, &setUcastOutArgs);

    status       = Cpsw_ioctl(gCpswInterVlanAppObj.hCpsw,
                              gCpswInterVlanAppObj.coreId,
                              CPSW_ALE_IOCTL_ADD_UNICAST,
                              &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
             "%s() failed CPSW_ALE_IOCTL_ADD_UNICAST: %d\n",
             __func__,status);
    }

    memcpy(&setUcastInArgs.addr.addr[0U], testSrcMacAddr,
           sizeof (setUcastInArgs.addr.addr));
    setUcastInArgs.addr.vlanId  = APP_INTERVLAN_EGRESS_VLANID;
    setUcastInArgs.info.portNum = APP_INTERVLAN_EGRESS_PORT_NUM;
    setUcastInArgs.info.blocked = false;
    setUcastInArgs.info.secure  = false;
    setUcastInArgs.info.super   = 0U;
    setUcastInArgs.info.ageable = false;
    setUcastInArgs.info.trunk   = false;

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setUcastInArgs, &setUcastOutArgs);

    status = Cpsw_ioctl(gCpswInterVlanAppObj.hCpsw,
                        gCpswInterVlanAppObj.coreId,
                        CPSW_ALE_IOCTL_ADD_UNICAST,
                        &prms);

    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
            "%s() failed CPSW_ALE_IOCTL_ADD_UNICAST: %d\n",
            __func__,status);
    }

    memcpy(&setUcastInArgs.addr.addr[0U], testDstMacAddr,
           sizeof (setUcastInArgs.addr.addr));
    setUcastInArgs.addr.vlanId  = APP_INTERVLAN_INGRESS_PORT_NUM;
    setUcastInArgs.info.portNum = APP_INTERVLAN_INGRESS_PORT_NUM;
    setUcastInArgs.info.blocked = false;
    setUcastInArgs.info.secure  = false;
    setUcastInArgs.info.super   = 0U;
    setUcastInArgs.info.ageable = false;
    setUcastInArgs.info.trunk   = false;

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setUcastInArgs, &setUcastOutArgs);

    status = Cpsw_ioctl(gCpswInterVlanAppObj.hCpsw,
                        gCpswInterVlanAppObj.coreId,
                        CPSW_ALE_IOCTL_ADD_UNICAST,
                        &prms);

    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
            "%s() failed CPSW_ALE_IOCTL_ADD_UNICAST: %d\n",
            __func__,status);
    }

    if (status == CPSW_SOK)
    {
        CpswAle_VlanEntryInfo   inArgs;
        CpswAle_AddEntryOutArgs outArgs;

        inArgs.vlanIdInfo.vlanId        = APP_INTERVLAN_INGRESS_VLANID;
        inArgs.vlanIdInfo.outerVlanFlag = false;
        inArgs.vlanMemberList           = CpswAppInterVlan_getIngressVlanMembershipMask();
        inArgs.unregMcastFloodMask      = CpswAppInterVlan_getIngressVlanMembershipMask();
        inArgs.regMcastFloodMask        = CpswAppInterVlan_getIngressVlanMembershipMask();
        inArgs.forceUntaggedEgressMask  = 0;
        inArgs.noLearnMask              = 0U;
        inArgs.vidIngressCheck          = false;
        inArgs.limitIPNxtHdr            = false;
        inArgs.disallowIPFragmentation  = false;

        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &outArgs);

        status = Cpsw_ioctl(gCpswInterVlanAppObj.hCpsw,
                            gCpswInterVlanAppObj.coreId,
                            CPSW_ALE_IOCTL_ADD_VLAN,
                            &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print(
                "%s() failed ADD_VLAN ioctl failed: %d\n",
                __func__,status);
        }
    }
    if (status == CPSW_SOK)
    {
        CpswAle_VlanEntryInfo   inArgs;
        CpswAle_AddEntryOutArgs outArgs;

        inArgs.vlanIdInfo.vlanId        = APP_INTERVLAN_EGRESS_VLANID;
        inArgs.vlanIdInfo.outerVlanFlag = false;
        inArgs.vlanMemberList           = CpswAppInterVlan_getEgressVlanMembershipMask();
        inArgs.unregMcastFloodMask      = CpswAppInterVlan_getEgressVlanMembershipMask();
        inArgs.regMcastFloodMask        = CpswAppInterVlan_getEgressVlanMembershipMask();
        inArgs.forceUntaggedEgressMask  = 0;
        inArgs.noLearnMask              = 0U;
        inArgs.vidIngressCheck          = false;
        inArgs.limitIPNxtHdr            = false;
        inArgs.disallowIPFragmentation  = false;

        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &outArgs);

        status = Cpsw_ioctl(gCpswInterVlanAppObj.hCpsw,
                            gCpswInterVlanAppObj.coreId,
                            CPSW_ALE_IOCTL_ADD_VLAN,
                            &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print(
                "%s() failed ADD_VLAN ioctl failed: %d\n",
                __func__,status);
        }
    }
    return status;
}

static int32_t CpswApp_addClasifierEntries(void)
{
    int32_t          status;
    Cpsw_IoctlPrms prms;
    CpswAle_SetPolicerEntryOutArgs setPolicerEntryOutArgs;
    CpswAle_SetPolicerEntryInArgs setPolicerEntryInArgs;

    /* Add Policer Entry for Ingress Flow */
    /*TODO: Adding MACPORT based classification is not working, need to debug further */
    setPolicerEntryInArgs.policerMatch.policerMatchEnableMask =(//CPSW_ALE_POLICER_MATCH_PORT |
                                                               CPSW_ALE_POLICER_MATCH_MACSRC |
                                                               CPSW_ALE_POLICER_MATCH_MACDST |
                                                               CPSW_ALE_POLICER_MATCH_IVLAN |
                                                               CPSW_ALE_POLICER_MATCH_ETHERTYPE |
                                                               CPSW_ALE_POLICER_MATCH_IPSRC |
                                                               CPSW_ALE_POLICER_MATCH_IPDST);

    setPolicerEntryInArgs.policerMatch.portNum = APP_INTERVLAN_INGRESS_PORT_NUM;
    setPolicerEntryInArgs.policerMatch.portIsTrunk = false;

    setPolicerEntryInArgs.policerMatch.srcMacAddr.ingressPortNum = APP_INTERVLAN_INGRESS_PORT_NUM;
    setPolicerEntryInArgs.policerMatch.dstMacAddr.egressPortNum  = 0U;

    memcpy (&setPolicerEntryInArgs.policerMatch.srcMacAddr.addr.addr[0U],
            testSrcMacAddr,
            ETH_MAC_ADDR_LEN);

    memcpy (&setPolicerEntryInArgs.policerMatch.dstMacAddr.addr.addr[0U],
            gCpswInterVlanAppObj.hostMacAddr,
            ETH_MAC_ADDR_LEN);

    setPolicerEntryInArgs.policerMatch.srcMacAddr.addr.vlanId = 0U;
    setPolicerEntryInArgs.policerMatch.dstMacAddr.addr.vlanId = 0U;

    setPolicerEntryInArgs.policerMatch.ivlanId = APP_INTERVLAN_INGRESS_VLANID;
    setPolicerEntryInArgs.policerMatch.etherType.etherType = APP_INTERVLAN_IPV4_ETHERTYPE;

    setPolicerEntryInArgs.policerMatch.srcIp.ipAddrtype = CPSW_ALE_IPADDR_CLASSIFIER_IPV4;
    setPolicerEntryInArgs.policerMatch.dstIp.ipAddrtype = CPSW_ALE_IPADDR_CLASSIFIER_IPV4;

    memcpy( &setPolicerEntryInArgs.policerMatch.srcIp.ipv4.ipv4Addr[0U],
            testSrcIpv4Addr,
            sizeof(setPolicerEntryInArgs.policerMatch.srcIp.ipv4.ipv4Addr));

    memcpy( &setPolicerEntryInArgs.policerMatch.dstIp.ipv4.ipv4Addr[0U],
            testDstIpv4Addr,
            sizeof(setPolicerEntryInArgs.policerMatch.dstIp.ipv4.ipv4Addr));

    setPolicerEntryInArgs.policerMatch.srcIp.ipv4.numLSBIgnoreBits = 8U;
    setPolicerEntryInArgs.policerMatch.dstIp.ipv4.numLSBIgnoreBits = 8U;

    setPolicerEntryInArgs.threadIdEnable = true;
    setPolicerEntryInArgs.threadId = gCpswInterVlanAppObj.ingRxFlowIdx;
    setPolicerEntryInArgs.peakRateInBitsPerSec = 0;
    setPolicerEntryInArgs.commitRateInBitsPerSec = 0;

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setPolicerEntryInArgs, &setPolicerEntryOutArgs);

    status = Cpsw_ioctl(gCpswInterVlanAppObj.hCpsw,
                        gCpswInterVlanAppObj.coreId,
                        CPSW_ALE_IOCTL_SET_POLICER,
                        &prms);

    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
            "CpswApp_addClasifierEntries() failed CPSW_ALE_IOCTL_SET_POLICER: %d\n", status);
    }

    if (status == CPSW_SOK)
    {
        status = CpswApp_addAleEntries();
        CpswAppUtils_assert(CPSW_SOK == status);
    }

    return status;
}

static int32_t CpswApp_getRxTxHandle(void)
{

    int32_t status = CPSW_SOK;
    CpswDma_OpenTxChPrms   cpswTxChCfg;
    CpswDma_OpenRxFlowPrms cpswRxFlowCfg;
    CpswDma_UdmaRingPrms *pFqRingPrms;

    /* Open the CPSW TX channel  */
    CpswDma_initTxChParams(&cpswTxChCfg);
    CpswAppUtils_setCommonTxChPrms(&cpswTxChCfg);

    cpswTxChCfg.hUdmaDrv               = gCpswInterVlanAppObj.hUdmaDrv;
    cpswTxChCfg.numTxPkts              = CPSW_FRWD_APP_NUM_PKTS + 32;
    cpswTxChCfg.hCbArg                 = &gCpswInterVlanAppObj.hTxCh;
    cpswTxChCfg.notifyCb               = &CpswApp_txIsrFxn;
    cpswTxChCfg.useProxy               = true;
    cpswTxChCfg.disableCacheOpsFlag    = true;

    CpswAppUtils_openTxCh(gCpswInterVlanAppObj.hCpsw,
                          gCpswInterVlanAppObj.coreKey,
                          gCpswInterVlanAppObj.coreId,
                          &gCpswInterVlanAppObj.txChNum,
                          &gCpswInterVlanAppObj.hTxCh,
                          &cpswTxChCfg);

    CpswAppUtils_assert (NULL != gCpswInterVlanAppObj.hTxCh);
    status = CpswDma_enableTxEvent(gCpswInterVlanAppObj.hTxCh);

    if(status != CPSW_SOK)
    {
        CpswAppUtils_freeTxCh(gCpswInterVlanAppObj.hCpsw,
                              gCpswInterVlanAppObj.coreKey,
                              gCpswInterVlanAppObj.coreId,
                              gCpswInterVlanAppObj.txChNum);
        CpswAppUtils_assert(status != CPSW_SOK);
    }

    /* Open the CPSW RX flow for Ingress */
    if (status == CPSW_SOK)
    {
        CpswDma_initRxFlowParams(&cpswRxFlowCfg);
        CpswAppUtils_setCommonRxFlowPrms(&cpswRxFlowCfg);

        cpswRxFlowCfg.notifyCb               = &CpswApp_ingRxIsrFxn;
        cpswRxFlowCfg.numRxPkts              = CPSW_FRWD_APP_NUM_PKTS;
        cpswRxFlowCfg.hUdmaDrv               = gCpswInterVlanAppObj.hUdmaDrv;
        cpswRxFlowCfg.hCbArg                 = &gCpswInterVlanAppObj.hIngRxFlow;
        cpswRxFlowCfg.useProxy               = true;
        cpswRxFlowCfg.disableCacheOpsFlag    = true;

        /* Use ring monitor for the CQ ring of RX flow */
        pFqRingPrms = &cpswRxFlowCfg.udmaChPrms.fqRingPrms;
        pFqRingPrms->useRingMon = false;
        pFqRingPrms->ringMonCfg.mode = TISCI_MSG_VALUE_RM_MON_MODE_THRESHOLD;
        /* Ring mon low threshold */
    #if defined _DEBUG_
        /* In debug mode as CPU is processing lesser packets per event, keep threshold more */
        pFqRingPrms->ringMonCfg.data0 = (CPSW_FRWD_APP_NUM_PKTS - 10U);
    #else
        pFqRingPrms->ringMonCfg.data0 = (CPSW_FRWD_APP_NUM_PKTS - 20U);
    #endif
        /* Ring mon high threshold - to get only low  threshold event, setting high threshold as more than ring depth*/
        pFqRingPrms->ringMonCfg.data1 = CPSW_FRWD_APP_NUM_PKTS;

         status = CpswAppUtils_allocRxFlow(gCpswInterVlanAppObj.hCpsw,
                                           gCpswInterVlanAppObj.coreKey,
                                           gCpswInterVlanAppObj.coreId,
                                           &gCpswInterVlanAppObj.rxFlowStartIdx,
                                           &gCpswInterVlanAppObj.ingRxFlowIdx);
         CpswAppUtils_assert(status == CPSW_SOK);

         cpswRxFlowCfg.startIdx               = gCpswInterVlanAppObj.rxFlowStartIdx;
         cpswRxFlowCfg.flowIdx                = gCpswInterVlanAppObj.ingRxFlowIdx;

         gCpswInterVlanAppObj.hIngRxFlow = CpswDma_openRxFlow(&cpswRxFlowCfg);
         if (gCpswInterVlanAppObj.hIngRxFlow == NULL)
         {
             CpswAppUtils_freeRxFlow(gCpswInterVlanAppObj.hCpsw,
                                     gCpswInterVlanAppObj.coreKey,
                                     gCpswInterVlanAppObj.coreId,
                                     gCpswInterVlanAppObj.ingRxFlowIdx);

             CpswAppUtils_assert (NULL != gCpswInterVlanAppObj.hIngRxFlow);
         }

         CpswAppUtils_addHostPortEntry(gCpswInterVlanAppObj.hCpsw,
                                       gCpswInterVlanAppObj.coreId,
                                       &gCpswInterVlanAppObj.hostMacAddr[0U]);

         status = CpswAppUtils_registerDstMacRxFlow(gCpswInterVlanAppObj.hCpsw,
                                                    gCpswInterVlanAppObj.coreKey,
                                                    gCpswInterVlanAppObj.coreId,
                                                    gCpswInterVlanAppObj.rxFlowStartIdx,
                                                    gCpswInterVlanAppObj.ingRxFlowIdx,
                                                    &gCpswInterVlanAppObj.hostMacAddr[0U]);

         CpswAppUtils_assert(status == CPSW_SOK);
         if (status == CPSW_SOK)
         {
             CpswApp_initRxReadyPktQ();
         }
    }

    if (status == CPSW_SOK)
    {
        status = CpswApp_addClasifierEntries();
        CpswAppUtils_assert(CPSW_SOK == status);
    }

    return status;
}

static void CpswApp_pktRxTx(void)
{
    int32_t          status = CPSW_SOK;
    CpswDma_PktInfoQ txSubmitQ;
    CpswDma_PktInfoQ rxFreeQ;
    CpswDma_PktInfo *pktInfo;
    EthVlanFrame    *frame;
    uint32_t         rxReadyCnt;
    bool             isSemPosted;
    uint32_t         iterationCount = 0;

    /*  The packet handling loop is structured as described below
     *  The outer loop waits for semaphore notification from RX completion
     *  ISR. At this moment the Rx completion interrupt is disabled and we switch
     *  into processing loop, handling all packets received, Tx completed packets.
     *
     *  For each packet received, we perform header inspection, header modification and
     *  enqueue packet for transmission.
     *
     *  We then handle Tx packet completion - for each TX packets transmitted, when the buffer
     *  is reclaimed we then add it back to Rx Free Q (this implicitly acts SW flow control)
     */
    CpswUtils_initQ(&gCpswInterVlanAppObj.rxReadyQ);
    Task_setPri(Task_self(),14);

    do
    {

     /*TODO: add a voluntary yield here for other tasks at same priority to run
      * if we are continuosly handling a stream of packets*/

    /* rxReadyQ should be empty here as we would have processed and queued all packets from
     * last iteration. */
     CpswAppUtils_assert(CpswUtils_getQCount(&gCpswInterVlanAppObj.rxReadyQ) == 0);

     /* Initialize the Tx Submission, Rx Free SW Qs*/
     CpswUtils_initQ(&txSubmitQ);
     CpswUtils_initQ(&rxFreeQ);


     /* Get the packets received so far */
     rxReadyCnt = CpswApp_receivePkts();

     if (0 == rxReadyCnt)
     {
        /* If we get here, it means that we have processed all received packets.
         * Need to switch on interrupt notification for Rx pkts and move to
         * blocked state until new pkt reception is signalled
         */

        /* Re-enable the Rx completion notification from ISR here */
        CpswDma_enableRxEvent(gCpswInterVlanAppObj.hIngRxFlow);


        /* Re-enable the Tx completion notification from ISR here */
        CpswDma_enableTxEvent(gCpswInterVlanAppObj.hTxCh);


        /* Pend on sempahore nofitication event from Rx Completion ISR */
        isSemPosted = Semaphore_pend(gCpswInterVlanAppObj.completionSem, RX_TX_COMPLETION_TIMEOUT);

        if (false == isSemPosted)
        {
            iterationCount++;
            if ((iterationCount & 0x7FF) == 0)
            {
                #ifdef APP_PRINTPKTCNT
                CpswAppUtils_print("# pkts=%d\n", gCpswInterVlanAppObj.num_pkts);
                #endif
            }

        }

     }
     else
     {
        gCpswInterVlanAppObj.num_pkts += rxReadyCnt;

        /* Consume the received packets and release them */
        pktInfo = (CpswDma_PktInfo *)CpswUtils_deQ(&gCpswInterVlanAppObj.rxReadyQ);

        /*Processing loop for received packets, we will perform Header inspection,
         * mangling and enqueue the same for Tx Processing */
        while (NULL != pktInfo)
        {
            /* Consume the received packets and release them */
            /* TODO: Invalidate for the header portion*/
            frame = (EthVlanFrame *)pktInfo->bufPtr;
            Cache_inv((Ptr)frame, PKT_HEADER_SIZE, Cache_Type_L1D, TRUE);

            /* Step2: Modify SA, DA fields of ethernet header*/
            /* Modify DASA
             * Modify VLAN ID
             * Modify TTL
             */
            memcpy(frame->hdr.dstMac, testDstMacAddr, ETH_MAC_ADDR_LEN);
            memcpy(frame->hdr.srcMac, gCpswInterVlanAppObj.hostMacAddr, ETH_MAC_ADDR_LEN);
            status = EthFrame_changeVlanId(frame,
                                           APP_INTERVLAN_EGRESS_VLANID);
            if (status == CPSW_SOK)
            {
                /* Decrement TTL by 1U*/
                EthFrame_decrementTTL(frame);

                /* TODO: Flush the cache contents for header region */
                Cache_wbInv((Ptr)frame, PKT_HEADER_SIZE, Cache_Type_L1D, FALSE);

                /* Step3: Enq the modified frame for transmission */
                CpswUtils_enQ(&txSubmitQ, &pktInfo->node);

                pktInfo = (CpswDma_PktInfo *)CpswUtils_deQ(&gCpswInterVlanAppObj.rxReadyQ);
            }

        } /* end of while loop*/

        Cache_wait();

        /* Submit the list of Packets to be Tx to HW */
        status = CpswAppUtils_submitTxPackets(gCpswInterVlanAppObj.hTxCh,
                                                           &txSubmitQ);
        CpswAppUtils_assert(CpswUtils_getQCount(&txSubmitQ) == 0);
     }/*end of else condition*/

   /* Reclaim Transmitted packets and add the reclaimed buffers to Rx FreeQ */
    status = CpswDma_retrieveTxDonePackets(gCpswInterVlanAppObj.hTxCh, &rxFreeQ);
    CpswAppUtils_submitRxPackets(gCpswInterVlanAppObj.hIngRxFlow,
                                                  &rxFreeQ);
    CpswAppUtils_assert(CpswUtils_getQCount(&rxFreeQ) == 0);

   } while(true);

}

static uint32_t CpswApp_receivePkts(void)
{
    int32_t          status;
    uint32_t         rxReadyCnt = 0U;

    /* we fetch all Rx ready pkts from HW Q and populate the list in SW Q
     * The SW Q would have been empty before this call, as we would have serviced all
     * pending Rx packets from SW Q before going back to check on the HW Q status*/

    /* Retrieve any CPSW packets which are ready */
    status = CpswDma_retrieveRxPackets(gCpswInterVlanAppObj.hIngRxFlow, &gCpswInterVlanAppObj.rxReadyQ);
    if (status == CPSW_SOK)
    {
        rxReadyCnt = CpswUtils_getQCount(&gCpswInterVlanAppObj.rxReadyQ);
    }
    else
    {
        CpswAppUtils_print("receivePkts() failed to retrieve pkts: %d\n",
                           status);
    }

    return rxReadyCnt;
}

void CpswApp_swInterVlanRouting(Cpsw_Type cpswType)
{
    Task_Params params;
    Error_Block eb;

    gCpswInterVlanAppObj.cpswType = cpswType;
    Error_init(&eb);

    /* Initialize the task params. Set the task priority higher than the
     * default priority (1) */
    Task_Params_init(&params);
    params.priority       = 2U;
    params.stack          = gAppTskStackMain;
    params.stackSize      = sizeof(gAppTskStackMain);
    params.instance->name = "Forwarding_Task";

    task = Task_create(CpswApp_InterVlanRouting, &params, &eb);
    if (task == NULL)
    {
        BIOS_exit(0);
    }

}


/* end of file */
