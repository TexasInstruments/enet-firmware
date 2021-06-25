/*
 *
 * Copyright (c) 2021 Texas Instruments Incorporated
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

/**
 *  \file ethfw_callbacks_lwipif.c
 *
 *  \brief Default LwIP interface callbacks for Ethernet Firmware application.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <stdint.h>

#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/dma/udma/enet_udma.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/enet/examples/utils/include/enet_apputils.h>
#include <ti/drv/enet/examples/utils/include/enet_appmemutils_cfg.h>
#include <ti/drv/enet/examples/utils/include/enet_appmemutils.h>
#include <ti/drv/enet/examples/utils/include/enet_mcm.h>
#include <ti/drv/enet/examples/utils/include/enet_appsoc.h>
#include <ti/drv/enet/examples/utils/include/enet_apprm.h>
#include <ti/drv/enet/lwipif/inc/lwipif2enet_appif.h>

#include <utils/ethfw_callbacks/include/ethfw_callbacks_lwipif.h>
#include <utils/console_io/include/app_log.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/*! Remote app packet poll period in milliseconds */
#define CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US         (1000U)

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

void EthFwCallbacks_lwipifCpswGetHandle(LwipifEnetAppIf_GetHandleInArgs *inArgs,
                                        LwipifEnetAppIf_GetHandleOutArgs *outArgs)
{
    LwipifEnetAppIf_RxHandleInfo *rxInfo;
    LwipifEnetAppIf_RxConfig *rxCfg;
    EnetMcm_CmdIf mcmCmdIf;
    EnetMcm_HandleInfo handleInfo;
    EnetPer_AttachCoreOutArgs attachInfo;
    EnetUdma_OpenTxChPrms cpswTxChCfg;
    EnetUdma_OpenRxFlowPrms cpswRxFlowCfg;
    EnetUdma_UdmaRingPrms *pFqRingPrms;
#if defined(SOC_J721E)
    Enet_Type enetType = ENET_CPSW_9G;
#elif defined(SOC_J7200)
    Enet_Type enetType = ENET_CPSW_5G;
#endif
    uint8_t *macAddr;
    uint32_t coreId = EnetSoc_getCoreId();
    bool useDefaultFlow = true;    /* Must handle the default flow */
    bool useRingMon = true;

    /* Get MCM command interface */
    EnetMcm_getCmdIf(enetType, &mcmCmdIf);
    EnetAppUtils_assert(mcmCmdIf.hMboxCmd != NULL);
    EnetAppUtils_assert(mcmCmdIf.hMboxResponse != NULL);

    /* Get CPSW and UDMA driver handles */
    EnetMcm_acquireHandleInfo(&mcmCmdIf, &handleInfo);

    /* Attach this core, if not done already */
    EnetMcm_coreAttach(&mcmCmdIf, coreId, &attachInfo);

    /* Open TX channel */
    EnetDma_initTxChParams(&cpswTxChCfg);
    cpswTxChCfg.hUdmaDrv            = handleInfo.hUdmaDrv;
    cpswTxChCfg.numTxPkts           = inArgs->txCfg.numPackets;
    cpswTxChCfg.cbArg               = inArgs->txCfg.cbArg;
    cpswTxChCfg.notifyCb            = inArgs->txCfg.notifyCb;
    cpswTxChCfg.useProxy            = true;
    cpswTxChCfg.disableCacheOpsFlag = false;
    cpswTxChCfg.ringMemAllocFxn     = &EnetMem_allocRingMem;
    cpswTxChCfg.ringMemFreeFxn      = &EnetMem_freeRingMem;
    cpswTxChCfg.dmaDescAllocFxn     = &EnetMem_allocDmaDesc;
    cpswTxChCfg.dmaDescFreeFxn      = &EnetMem_freeDmaDesc;

    EnetAppUtils_openTxCh(handleInfo.hEnet,
                          attachInfo.coreKey,
                          coreId,
                          &outArgs->txInfo.txChNum,
                          &outArgs->txInfo.hTxChannel,
                          &cpswTxChCfg);

    /* Open first RX channel/flow */
    rxCfg  = &inArgs->rxCfg[0U];
    rxInfo = &outArgs->rxInfo[0U];

    EnetDma_initRxChParams(&cpswRxFlowCfg);
    cpswRxFlowCfg.notifyCb  = rxCfg->notifyCb;
    cpswRxFlowCfg.numRxPkts = rxCfg->numPackets;
    cpswRxFlowCfg.hUdmaDrv  = handleInfo.hUdmaDrv;
    cpswRxFlowCfg.cbArg     = rxCfg->cbArg;
    cpswRxFlowCfg.useProxy  = true;

    /* Use ring monitor for the CQ ring of RX flow */
    pFqRingPrms                  = &cpswRxFlowCfg.udmaChPrms.fqRingPrms;
    pFqRingPrms->useRingMon      = useRingMon;
    pFqRingPrms->ringMonCfg.mode = TISCI_MSG_VALUE_RM_MON_MODE_THRESHOLD;
    /* Ring mon low threshold */
#if defined _DEBUG_
    /* In debug mode as CPU is processing lesser packets per event, keep threshold more */
    pFqRingPrms->ringMonCfg.data0 = (rxCfg->numPackets - 10U);
#else
    pFqRingPrms->ringMonCfg.data0 = (rxCfg->numPackets - 20U);
#endif
    /* Ring mon high threshold - to get only low  threshold event, setting high threshold as more than ring depth*/
    pFqRingPrms->ringMonCfg.data1 = rxCfg->numPackets;

    cpswRxFlowCfg.disableCacheOpsFlag = false;
    cpswRxFlowCfg.rxFlowMtu           = attachInfo.rxMtu;
    cpswRxFlowCfg.ringMemAllocFxn     = &EnetMem_allocRingMem;
    cpswRxFlowCfg.ringMemFreeFxn      = &EnetMem_freeRingMem;
    cpswRxFlowCfg.dmaDescAllocFxn     = &EnetMem_allocDmaDesc;
    cpswRxFlowCfg.dmaDescFreeFxn      = &EnetMem_freeDmaDesc;

    EnetAppUtils_openRxFlow(enetType,
                            handleInfo.hEnet,
                            attachInfo.coreKey,
                            coreId,
                            useDefaultFlow,
                            &rxInfo->rxFlowStartIdx,
                            &rxInfo->rxFlowIdx,
                            &rxInfo->macAddr[0U],
                            &rxInfo->hRxFlow,
                            &cpswRxFlowCfg);

    outArgs->coreId          = coreId;
    outArgs->coreKey         = attachInfo.coreKey;
    outArgs->hEnet           = handleInfo.hEnet;
    outArgs->hostPortRxMtu   = attachInfo.rxMtu;
    ENET_UTILS_ARRAY_COPY(outArgs->txMtu, attachInfo.txMtu);
    outArgs->hUdmaDrv        = handleInfo.hUdmaDrv;
    outArgs->print           = &EnetAppUtils_print;
    outArgs->isPortLinkedFxn = &EthFwCallbacks_isPortLinked;

    /* TODO: Polling timer is getting corrupted at times of sudden burst of
     * traffic, because of which timer callback is never called.
     * With polling timer not functional, packets are never serviced then after.
     * As a workaround setting isRingMonUsed to true (irrespective of ring monitor
     * is enabled or not) to ensure interrupts are used instead of polling.
     * Timer corruption needs to be root-caused and fixed. */
    rxInfo->disableEvent = !useRingMon;
    outArgs->timerPeriodUs = CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US;

    /* Let LwIP interface use optimized processing where TX packets are relinquished
     * in next TX submit call */
    outArgs->txInfo.disableEvent = true;

    macAddr = &rxInfo->macAddr[0U];
    appLogPrintf("Host MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                 macAddr[0] & 0xFF, macAddr[1] & 0xFF, macAddr[2] & 0xFF,
                 macAddr[3] & 0xFF, macAddr[4] & 0xFF, macAddr[5] & 0xFF);
}

void EthFwCallbacks_lwipifCpswReleaseHandle(LwipifEnetAppIf_ReleaseHandleInfo *releaseInfo)
{
    LwipifEnetAppIf_RxHandleInfo *rxInfo;
    Lwip2EnetAppIf_FreePktInfo *freePktInfo;
    EnetMcm_CmdIf mcmCmdIf;
    EnetDma_PktQ fqPktInfoQ;
    EnetDma_PktQ cqPktInfoQ;
#if defined(SOC_J721E)
    Enet_Type enetType = ENET_CPSW_9G;
#elif defined(SOC_J7200)
    Enet_Type enetType = ENET_CPSW_5G;
#endif
    bool useDefaultFlow = true;    /* Must handle the default flow */

    /* Get MCM command interface */
    EnetMcm_getCmdIf(enetType, &mcmCmdIf);
    EnetAppUtils_assert(mcmCmdIf.hMboxCmd != NULL);
    EnetAppUtils_assert(mcmCmdIf.hMboxResponse != NULL);

    /* Close TX channel */
    EnetQueue_initQ(&fqPktInfoQ);
    EnetQueue_initQ(&cqPktInfoQ);
    EnetAppUtils_closeTxCh(releaseInfo->hEnet,
                           releaseInfo->coreKey,
                           releaseInfo->coreId,
                           &fqPktInfoQ,
                           &cqPktInfoQ,
                           releaseInfo->txInfo.hTxChannel,
                           releaseInfo->txInfo.txChNum);
    releaseInfo->txFreePkt.cb(releaseInfo->txFreePkt.cbArg, &fqPktInfoQ, &cqPktInfoQ);

    /* Close first RX channel/flow */
    rxInfo = &releaseInfo->rxInfo[0U];
    freePktInfo = &releaseInfo->rxFreePkt[0U];

    EnetQueue_initQ(&fqPktInfoQ);
    EnetQueue_initQ(&cqPktInfoQ);
    EnetAppUtils_closeRxFlow(enetType,
                             releaseInfo->hEnet,
                             releaseInfo->coreKey,
                             releaseInfo->coreId,
                             useDefaultFlow,
                             &fqPktInfoQ,
                             &cqPktInfoQ,
                             rxInfo->rxFlowStartIdx,
                             rxInfo->rxFlowIdx,
                             rxInfo->macAddr,
                             rxInfo->hRxFlow);
    freePktInfo->cb(freePktInfo->cbArg, &fqPktInfoQ, &cqPktInfoQ);

    EnetMcm_coreDetach(&mcmCmdIf, releaseInfo->coreId, releaseInfo->coreKey);
    EnetMcm_releaseHandleInfo(&mcmCmdIf);
}
