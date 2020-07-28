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

/**
 *  \file ethfw_callbacks_nimu.c
 *
 *  \brief Default NIMU callbacks for Ethernet Firmware application.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <stdint.h>

#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils_cfg.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_mcm.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appsoc.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apprm.h>
#include <ti/drv/cpsw/nimucpsw/nimu_ndk.h>
#include <ti/drv/cpsw/nimucpsw/ndk2cpsw_appif.h>

#include <utils/ethfw_callbacks/include/ethfw_callbacks_nimu.h>

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

/*!
 * \brief NIMUDeviceTable
 *
 * \details
 *  This is the NIMU Device Table for the Platform.
 *  This should be defined for each platform. Since the current platform
 *  has a single network Interface; this has been defined here. If the
 *  platform supports more than one network interface this should be
 *  defined to have a list of "initialization" functions for each of the
 *  interfaces.
 */
NIMU_DEVICE_TABLE_ENTRY NIMUDeviceTable[2U] =
{
    /*! \brief NIMU_NDK_Init for this network device */
    {&NIMU_NDK_init},
    {NULL          },
};

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

void EthFwCallbacks_nimuCpswGetHandle(NimuCpswAppIf_GetHandleInArgs *inArgs,
                                      NimuCpswAppIf_GetHandleOutArgs *outArgs)
{
    CpswMcm_CmdIf mcmCmdIf;
    CpswMcm_HandleInfo handleInfo;
    Cpsw_AttachCoreOutArgs attachInfo;
    CpswDma_OpenTxChPrms cpswTxChCfg;
    CpswDma_OpenRxFlowPrms cpswRxFlowCfg;
    CpswDma_UdmaRingPrms *pFqRingPrms;
#if defined(SOC_J721E)
    Cpsw_Type cpswType = CPSW_9G;
#elif defined(SOC_J7200)
    Cpsw_Type cpswType = CPSW_5G;
#endif
    uint8_t *macAddr;
    uint32_t coreId = CpswAppSoc_getCoreId();
    bool useDefaultFlow = true;    /* NDK must handle the default flow */
    bool useRingMon = true;

    /* Get MCM command interface */
    CpswMcm_getCmdIf(cpswType, &mcmCmdIf);
    CpswAppUtils_assert(mcmCmdIf.hMboxCmd != NULL);
    CpswAppUtils_assert(mcmCmdIf.hMboxResponse != NULL);

    /* Get CPSW and UDMA driver handles */
    CpswMcm_acquireHandleInfo(&mcmCmdIf, &handleInfo);

    /* Attach this core, if not done already */
    CpswMcm_coreAttach(&mcmCmdIf, coreId, &attachInfo);

    /* Open TX channel */
    CpswDma_initTxChParams(&cpswTxChCfg);
    cpswTxChCfg.hUdmaDrv            = handleInfo.hUdmaDrv;
    cpswTxChCfg.numTxPkts           = inArgs->txCfg.numPackets;
    cpswTxChCfg.hCbArg              = inArgs->txCfg.cbArg;
    cpswTxChCfg.notifyCb            = inArgs->txCfg.notifyCb;
    cpswTxChCfg.useProxy            = true;
    cpswTxChCfg.disableCacheOpsFlag = false;
    cpswTxChCfg.ringMemAllocFxn     = &CpswAppMemUtils_allocRingMemFxn;
    cpswTxChCfg.ringMemFreeFxn      = &CpswAppMemUtils_freeRingMemFxn;
    cpswTxChCfg.dmaDescAllocFxn     = &CpswAppMemUtils_allocDmaDescFxn;
    cpswTxChCfg.dmaDescFreeFxn      = &CpswAppMemUtils_freeDmaDescFxn;

    CpswAppUtils_openTxCh(handleInfo.hCpsw,
                          attachInfo.coreKey,
                          coreId,
                          &outArgs->txInfo.txChNum,
                          &outArgs->txInfo.hTxChannel,
                          &cpswTxChCfg);

    /* Open RX Flow */
    CpswDma_initRxFlowParams(&cpswRxFlowCfg);
    cpswRxFlowCfg.notifyCb  = inArgs->rxCfg.notifyCb;
    cpswRxFlowCfg.numRxPkts = inArgs->rxCfg.numPackets;
    cpswRxFlowCfg.hUdmaDrv  = handleInfo.hUdmaDrv;
    cpswRxFlowCfg.hCbArg    = inArgs->rxCfg.cbArg;
    cpswRxFlowCfg.useProxy  = true;

    /* Use ring monitor for the CQ ring of RX flow */
    pFqRingPrms                  = &cpswRxFlowCfg.udmaChPrms.fqRingPrms;
    pFqRingPrms->useRingMon      = useRingMon;
    pFqRingPrms->ringMonCfg.mode = TISCI_MSG_VALUE_RM_MON_MODE_THRESHOLD;
    /* Ring mon low threshold */
#if defined _DEBUG_
    /* In debug mode as CPU is processing lesser packets per event, keep threshold more */
    pFqRingPrms->ringMonCfg.data0 = (inArgs->rxCfg.numPackets - 10U);
#else
    pFqRingPrms->ringMonCfg.data0 = (inArgs->rxCfg.numPackets - 20U);
#endif
    /* Ring mon high threshold - to get only low  threshold event, setting high threshold as more than ring depth*/
    pFqRingPrms->ringMonCfg.data1 = inArgs->rxCfg.numPackets;

    cpswRxFlowCfg.disableCacheOpsFlag = false;
    cpswRxFlowCfg.rxFlowMtu           = attachInfo.rxMtu;
    cpswRxFlowCfg.ringMemAllocFxn     = &CpswAppMemUtils_allocRingMemFxn;
    cpswRxFlowCfg.ringMemFreeFxn      = &CpswAppMemUtils_freeRingMemFxn;
    cpswRxFlowCfg.dmaDescAllocFxn     = &CpswAppMemUtils_allocDmaDescFxn;
    cpswRxFlowCfg.dmaDescFreeFxn      = &CpswAppMemUtils_freeDmaDescFxn;

    CpswAppUtils_openRxFlow(handleInfo.hCpsw,
                            attachInfo.coreKey,
                            coreId,
                            useDefaultFlow,
                            &outArgs->rxInfo.rxFlowStartIdx,
                            &outArgs->rxInfo.rxFlowIdx,
                            &outArgs->rxInfo.macAddr[0U],
                            &outArgs->rxInfo.hRxFlow,
                            &cpswRxFlowCfg);

    outArgs->coreId          = coreId;
    outArgs->coreKey         = attachInfo.coreKey;
    outArgs->hCpsw           = handleInfo.hCpsw;
    outArgs->hostPortRxMtu   = attachInfo.rxMtu;
    CPSW_UTILS_ARRAY_COPY(outArgs->txMtu, attachInfo.txMtu);
    outArgs->hUdmaDrv        = handleInfo.hUdmaDrv;
    outArgs->printFxnCb      = &CpswAppUtils_print;
    outArgs->isPortLinkedFxn = &EthFwCallbacks_isPortLinked;

    /* TODO: NIMU's polling timer is getting corrupted at times of sudden burst of
     * traffic, because of which timer callback is never called.
     * With polling timer not functional, packets are never serviced then after.
     * As a workaround setting isRingMonUsed to true (irrespective of ring monitor
     * is enabled or not) to ensure interrupts are used instead of polling.
     * Timer corruption needs to be root-caused and fixed. */
    outArgs->isRingMonUsed = useRingMon;
    outArgs->timerPeriodUs = CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US;

    /* Let NIMU use optimized processing where TX packets are relinquished in next
     * TX submit call */
    outArgs->disableTxEvent = true;

    macAddr = &outArgs->rxInfo.macAddr[0U];
    appLogPrintf("Host MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                 macAddr[0] & 0xFF, macAddr[1] & 0xFF, macAddr[2] & 0xFF,
                 macAddr[3] & 0xFF, macAddr[4] & 0xFF, macAddr[5] & 0xFF);
}

void EthFwCallbacks_nimuCpswReleaseHandle(NimuCpswAppIf_ReleaseHandleInfo *releaseInfo)
{
    CpswMcm_CmdIf mcmCmdIf;
    CpswDma_PktInfoQ fqPktInfoQ;
    CpswDma_PktInfoQ cqPktInfoQ;
#if defined(SOC_J721E)
    Cpsw_Type cpswType = CPSW_9G;
#elif defined(SOC_J7200)
    Cpsw_Type cpswType = CPSW_5G;
#endif
    bool useDefaultFlow = true;    /* NDK must handle the default flow */

    /* Get MCM command interface */
    CpswMcm_getCmdIf(cpswType, &mcmCmdIf);
    CpswAppUtils_assert(mcmCmdIf.hMboxCmd != NULL);
    CpswAppUtils_assert(mcmCmdIf.hMboxResponse != NULL);

    /* Close TX channel */
    CpswUtils_initQ(&fqPktInfoQ);
    CpswUtils_initQ(&cqPktInfoQ);
    CpswAppUtils_closeTxCh(releaseInfo->hCpsw,
                           releaseInfo->coreKey,
                           releaseInfo->coreId,
                           &fqPktInfoQ,
                           &cqPktInfoQ,
                           releaseInfo->txInfo.hTxChannel,
                           releaseInfo->txInfo.txChNum);
    releaseInfo->txFreePktCb(releaseInfo->freePktCbArg, &fqPktInfoQ, &cqPktInfoQ);

    /* Close RX Flow */
    CpswUtils_initQ(&fqPktInfoQ);
    CpswUtils_initQ(&cqPktInfoQ);
    CpswAppUtils_closeRxFlow(releaseInfo->hCpsw,
                             releaseInfo->coreKey,
                             releaseInfo->coreId,
                             useDefaultFlow,
                             &fqPktInfoQ,
                             &cqPktInfoQ,
                             releaseInfo->rxInfo.rxFlowStartIdx,
                             releaseInfo->rxInfo.rxFlowIdx,
                             releaseInfo->rxInfo.macAddr,
                             releaseInfo->rxInfo.hRxFlow);
    releaseInfo->rxFreePktCb(releaseInfo->freePktCbArg, &fqPktInfoQ, &cqPktInfoQ);

    CpswMcm_coreDetach(&mcmCmdIf, releaseInfo->coreId, releaseInfo->coreKey);
    CpswMcm_releaseHandleInfo(&mcmCmdIf);
}
