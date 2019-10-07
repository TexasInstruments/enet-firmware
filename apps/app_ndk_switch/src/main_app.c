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
 *  \file main_tirtos.c
 *
 *  \brief Main file for TI-RTOS build
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/utils/Load.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Mailbox.h>

/* OSAL Header file */
#include <ti/osal/osal.h>

/* PDK Driver Header files */
#include <ti/drv/sciclient/sciclient.h>
#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils_cfg.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appboardutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_mcm.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpswapp_ethutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appsoc.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apprm.h>
#include <ti/drv/cpsw/nimucpsw/nimu_ndk.h>
#include <ti/drv/cpsw/nimucpsw/ndk2cpsw_appif.h>

/* NDK headers */
#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/ndk/inc/socket.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/tools/servers.h>
#include <ti/ndk/inc/tools/console.h>

#include "app_switch.h"
#include "webpage.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* Test application stack size */
#define APP_TSK_STACK_MAIN              (10U * 1024U)
#define ETHFWAPP_PACKET_POLL_PERIOD_MS  (1U)


#define ENABLE_NDKSERVERS

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct
{
    /* Core Id */
    uint32_t coreId;

    /* CPSW instance type */
    Cpsw_Type cpswType;

    /* MAC ports */
    Cpsw_MacPort *macPorts;

    /* Master port on which NIMU will poll for link
     * Note - This will get removed once NIMU dependency on port is resolved */
    Cpsw_MacPort masterPort;

    /* Number of MAC ports */
    uint32_t numMacPorts;

    /* Multiclient manager handles */
    CpswMcm_CmdIf mcmCmdIf[CPSW_COUNT];

    /* UDMA driver handle */
    Udma_DrvHandle hUdmaDrv;

    /* Use default flow */
    bool    useDefaultRxFlow;
} CpswMain_AppObj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static void CpswApp_setAleConfig(CpswAle_Config *aleConfig);
static void CpswApp_initLinkArgs(Cpsw_OpenPortLinkInArgs *linkArgs,
                                 Cpsw_MacPort macPort);
static int32_t CpswApp_init(void);
void CpswApp_deInit(void);

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */


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
     { &NIMU_NDK_init },
     { NULL },
};

#ifdef ENABLE_NDKSERVERS
static HANDLE hEcho = 0;
static HANDLE hEchoUdp = 0;
static HANDLE hData = 0;
static HANDLE hNull = 0;
static HANDLE hOob = 0;
#endif

char *VerStr = "NIMU CPSW Example";

char gIpAddrStr[20] = "0.0.0.0";

static Cpsw_MacPort gCpswMainAppMacPorts[] = {
#if defined(SOC_AM65XX)
    CPSW_MAC_PORT_0,
#elif defined(SOC_J721E)
    CPSW_MAC_PORT_1,
    CPSW_MAC_PORT_0,
    CPSW_MAC_PORT_2,
    CPSW_MAC_PORT_3,
#endif
};

static CpswMain_AppObj gCpswMainAppObj = {
#if defined(SOC_AM65XX)
    .cpswType = CPSW_2G,
    .masterPort = CPSW_MAC_PORT_0,
#elif defined(SOC_J721E)
    .cpswType = CPSW_9G,
    .masterPort = CPSW_MAC_PORT_2,
#endif
    .macPorts = gCpswMainAppMacPorts,
    .numMacPorts = CPSWAPPUTILS_ARRAY_SIZE(gCpswMainAppMacPorts),
    .useDefaultRxFlow = true,
};

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int main(void)
{
    /* Set ccsHaltFlag to 1 for halting core for CCS connection */
    volatile uint32_t ccsHaltFlag = 0U;
    while(ccsHaltFlag);

    CpswAppBoardUtils_init();

    CpswAppUtils_enableClocks(gCpswMainAppObj.cpswType,
                              MAC_CONN_TYPE_RGMII_FORCE_1000_FULL);

    CpswAppUtils_print("=======================================================\n");
    CpswAppUtils_print ("           EthFw L2 Switching APP          \n");
    CpswAppUtils_print("=======================================================\n");

    /* does not return */
    BIOS_start();

    return(0);
}

static void CpswApp_setAleConfig(CpswAle_Config *aleConfig)
{
    aleConfig->modeFlags = CPSW_ALE_CONFIG_MASK_ALE_MODULE_ENABLE |
                           CPSW_ALE_CONFIG_MASK_UNKNOWN_UNICAST_FLOOD2HOST;

    aleConfig->agingConfig.enableAutoAging = TRUE;
    aleConfig->agingConfig.agingPeriodInMs = 1000;

    aleConfig->nwSecCfg.enableVid0Mode = FALSE;

    aleConfig->vlanConfig.aleVlanAwareMode = TRUE;
    aleConfig->vlanConfig.cpswVlanAwareMode = FALSE;
    aleConfig->vlanConfig.unknownUnregMcastFloodMask = 0U;
    aleConfig->vlanConfig.unknownRegMcastFloodMask = 0U;
    aleConfig->vlanConfig.unknownVlanMemberListMask = CPSW_ALE_ALL_PORTS_MASK;
    aleConfig->vlanConfig.autoLearnWithVLAN = TRUE;

    aleConfig->policerGlobalConfig.policingEnable = true;
    aleConfig->policerGlobalConfig.yellowDropEnable = false;
    aleConfig->policerGlobalConfig.redDropEnable = true;
    aleConfig->policerGlobalConfig.policerNoMatchMode = CPSW_ALE_POLICER_NOMATCH_MODE_GREEN;

    aleConfig->portCfg[0].learningCfg.noLearn = FALSE;
    aleConfig->portCfg[0].vlanCfg.dropUntagged = FALSE;

    aleConfig->portCfg[1].learningCfg.noLearn = FALSE;
    aleConfig->portCfg[1].vlanCfg.dropUntagged = FALSE;
}

static void CpswApp_initLinkArgs(Cpsw_OpenPortLinkInArgs *linkArgs,
                                 Cpsw_MacPort macPort)
{
    CpswMacPort_Config     *macConfig = &linkArgs->macConfig;
    CpswMacPort_LinkConfig *linkConfig = &linkArgs->linkConfig;
    CpswMacPort_Interface  *interface = &linkArgs->interface;
    CpswPhy_Config         *phyConfig = &linkArgs->phyConfig;

    linkArgs->portNum = macPort;

    Cpsw_initMacPortParams(macConfig);

    CpswAppBoardUtils_setPhyConfig(gCpswMainAppObj.cpswType,
                                   macPort,
                                   interface,
                                   phyConfig);

    if (phyConfig->phyAddr == CPSW_PHY_INVALID_PHYADDR)
    {
        linkConfig->speed     = CPSW_SPEED_1GBIT;
        linkConfig->duplexity = CPSW_DUPLEX_FULL;
    }
    else
    {
        linkConfig->speed     = CPSW_SPEED_AUTO;
        linkConfig->duplexity = CPSW_DUPLEX_AUTO;
    }

}

static int32_t CpswApp_init(void)
{
    CpswMcm_InitConfig cpswMcmCfg;
    Cpsw_Config cpswCfg;
    int32_t status = CPSW_SOK;

    CpswAppUtils_assert(gCpswMainAppObj.numMacPorts <= CPSW_MAC_PORT_NUM);

    /* Set configuration parameters */
    Cpsw_initParams(&cpswCfg);
    cpswCfg.vlanConfig.vlanAware          = false;
    cpswCfg.hostPortConfig.removeCrc      = true;
    cpswCfg.hostPortConfig.padShortPacket = true;
    cpswCfg.hostPortConfig.passCrcErrors  = true;
    CpswAppUtils_initResourceConfig(gCpswMainAppObj.cpswType, gCpswMainAppObj.coreId, &cpswCfg.resourceConfig);

    CpswApp_setAleConfig(&cpswCfg.aleConfig);

    cpswCfg.dmaConfig.rxChInitPrms.dmaPriority = UDMA_DEFAULT_RX_CH_DMA_PRIORITY;

    /* Open UDMA */
    gCpswMainAppObj.hUdmaDrv = CpswAppUtils_udmaOpen(gCpswMainAppObj.cpswType, NULL);
    cpswCfg.dmaConfig.hUdmaDrv = gCpswMainAppObj.hUdmaDrv;

    cpswMcmCfg.pCpswCfg     = &cpswCfg;
    cpswMcmCfg.cpswType     = gCpswMainAppObj.cpswType;
    cpswMcmCfg.setPortLinkCfg = CpswApp_initLinkArgs;
    cpswMcmCfg.numMacPorts  = gCpswMainAppObj.numMacPorts;
    cpswMcmCfg.periodicTaskPeriod = CPSW_PHY_FSM_TICK_PERIOD_MS; /* msecs */

    memcpy(&cpswMcmCfg.macPortList[0U],
           gCpswMainAppObj.macPorts,
           gCpswMainAppObj.numMacPorts);

    status = CpswMcm_init(&cpswMcmCfg);
    CpswAppUtils_assert (status == CPSW_SOK);

    return status;
}


void CpswApp_deInit(void)
{
    CpswAppUtils_udmaclose(gCpswMainAppObj.hUdmaDrv);

    memset(&gCpswMainAppObj, 0U, sizeof(CpswMain_AppObj));
}




void stackInitHook(void* hCfg)
{
    int rc;

    /* increase stack size */
    rc = 16384;
    CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_TASKSTKBOOT,
                CFG_ADDMODE_UNIQUE, sizeof(uint32_t), (uint8_t *)&rc, 0 );

    AddWebFiles();
}

void stackDeleteHook(void* hCfg)
{
    RemoveWebFiles();
}

void IpAddrHookFxn (uint32_t IPAddr, uint32_t IfIdx, uint32_t fAdd)
{
    volatile uint32_t ipAddrHex = 0U;

    NtIPN2Str(IPAddr, gIpAddrStr);

    CpswAppUtils_print("\nCPSW NIMU application, IP address I/F 1: %s\n\r", gIpAddrStr);

    /* Not that CPSW is initialized, create UART menu task for user configuration */
    /* Note - We can't call this function from any other tasks as it calls CpswAppIf_getHandles
     *        function but doesn't handle open of flows/tx channels */
    CpswApp_createUartMenuTask(gCpswMainAppObj.cpswType, gCpswMainAppObj.coreId);
}

void netOpenHook()
{
#ifdef ENABLE_NDKSERVERS
    // Create our local servers
    hEcho = DaemonNew(SOCK_STREAMNC, 0, 7, dtask_tcp_echo,
                      OS_TASKPRINORM, OS_TASKSTKNORM, 0, 3);
    hEchoUdp = DaemonNew(SOCK_DGRAM, 0, 7, dtask_udp_echo,
                         OS_TASKPRINORM, OS_TASKSTKNORM, 0, 1);
    hData = DaemonNew(SOCK_STREAM, 0, 1000, dtask_tcp_datasrv,
                      OS_TASKPRINORM, OS_TASKSTKNORM, 0, 3);
    hNull = DaemonNew(SOCK_STREAMNC, 0, 1001, dtask_tcp_nullsrv,
                      OS_TASKPRINORM, OS_TASKSTKNORM, 0, 3);
    hOob  = DaemonNew(SOCK_STREAMNC, 0, 999, dtask_tcp_oobsrv,
                      OS_TASKPRINORM, OS_TASKSTKNORM, 0, 3);
#endif
}

void netCloseHook()
{
#ifdef ENABLE_NDKSERVERS
    DaemonFree(hOob);
    DaemonFree(hNull);
    DaemonFree(hData);
    DaemonFree(hEchoUdp);
    DaemonFree(hEcho);
#endif
}

#if defined (SOC_J721E)
/**
 * \brief PDK-4356 FIX - set to DLFO bit in ACTRL register of R5F
 *
 * This API uses assembly instruction to set DLFO bit in ACTRL register
 * of R5F.
 * This should be called from the Core reset callback.
 *
 */
#pragma CODE_SECTION(CpswApp_setDLFOBitInACTRLReg,".text_boot")
void CpswApp_setDLFOBitInACTRLReg(void)
{
       asm(" MRC p15, #0, r12, c1, c0, #1 ;");
       asm(" ORR r12, r12, #8192 ;");
       asm(" MCR p15, #0, r12, c1, c0, #1 ;");
}
#endif

static bool CpswApp_IsPhyLinked(Cpsw_Handle hCpsw)
{
    return CpswAppUtils_isPortLinkUp(hCpsw, CpswAppSoc_getCoreId(), gCpswMainAppObj.masterPort);
}


void NimuCpswAppCb_getHandle(NimuCpswAppIf_GetHandleInArgs *inArgs,
                             NimuCpswAppIf_GetHandleOutArgs *outArgs)
{
    int32_t status;
    CpswMcm_HandleInfo handleInfo;
    Cpsw_AttachCoreOutArgs attachInfo;
    bool useDefaultFlow = gCpswMainAppObj.useDefaultRxFlow;
    CpswDma_OpenRxFlowPrms cpswRxFlowCfg;
    CpswDma_OpenTxChPrms   cpswTxChCfg;
    CpswDma_UdmaRingPrms *pFqRingPrms;
    uint32_t coreId = CpswAppSoc_getCoreId();


    gCpswMainAppObj.coreId = coreId;
    if (gCpswMainAppObj.mcmCmdIf[gCpswMainAppObj.cpswType].hMboxCmd == NULL)
    {
        status = CpswApp_init();

        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Failed to open CPSW: %d\n", status);
        }
        CpswAppUtils_assert(status == CPSW_SOK);
        CpswMcm_getCmdIf(gCpswMainAppObj.cpswType, &gCpswMainAppObj.mcmCmdIf[gCpswMainAppObj.cpswType]);
    }
    CpswAppUtils_assert(gCpswMainAppObj.mcmCmdIf[gCpswMainAppObj.cpswType].hMboxCmd != NULL);
    CpswAppUtils_assert(gCpswMainAppObj.mcmCmdIf[gCpswMainAppObj.cpswType].hMboxResponse != NULL);


    CpswMcm_acquireHandleInfo(&gCpswMainAppObj.mcmCmdIf[gCpswMainAppObj.cpswType], &handleInfo);
    CpswMcm_coreAttach(&gCpswMainAppObj.mcmCmdIf[gCpswMainAppObj.cpswType], gCpswMainAppObj.coreId  ,&attachInfo);

    /* Open TX channel */
    CpswDma_initTxChParams(&cpswTxChCfg);

    cpswTxChCfg.hUdmaDrv               = handleInfo.hUdmaDrv;
    cpswTxChCfg.numTxPkts              = inArgs->txCfg.numPackets;
    cpswTxChCfg.hCbArg                 = inArgs->txCfg.cbArg;
    cpswTxChCfg.notifyCb               = inArgs->txCfg.notifyCb;
    cpswTxChCfg.useProxy               = true;

    CpswAppUtils_setCommonTxChPrms(&cpswTxChCfg);

    CpswAppUtils_openTxCh(handleInfo.hCpsw,
                          attachInfo.coreKey,
                          coreId,
                          &outArgs->txInfo.txChNum,
                          &outArgs->txInfo.hTxChannel,
                          &cpswTxChCfg);


    /* Open RX Flow */
    CpswDma_initRxFlowParams(&cpswRxFlowCfg);
    cpswRxFlowCfg.notifyCb               = inArgs->rxCfg.notifyCb;
    cpswRxFlowCfg.numRxPkts              = inArgs->rxCfg.numPackets;
    cpswRxFlowCfg.hUdmaDrv               = handleInfo.hUdmaDrv;
    cpswRxFlowCfg.hCbArg                 = inArgs->rxCfg.cbArg;
    cpswRxFlowCfg.useProxy               = true;

    /* Use ring monitor for the CQ ring of RX flow */
    pFqRingPrms = &cpswRxFlowCfg.udmaChPrms.fqRingPrms;
    pFqRingPrms->useRingMon = true;
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

    CpswAppUtils_setCommonRxFlowPrms(&cpswRxFlowCfg);

    CpswAppUtils_openRxFlow(handleInfo.hCpsw,
                            attachInfo.coreKey,
                            coreId,
                            useDefaultFlow,
                            &outArgs->rxInfo.rxFlowStartIdx,
                            &outArgs->rxInfo.rxFlowIdx,
                            &outArgs->rxInfo.macAddr[0U],
                            &outArgs->rxInfo.hRxFlow,
                            &cpswRxFlowCfg);

    outArgs->coreId         = coreId;
    outArgs->coreKey        = attachInfo.coreKey;
    outArgs->hCpsw          = handleInfo.hCpsw;
    outArgs->hostPortRxMtu  = attachInfo.rxMtu;
    CPSW_UTILS_ARRAY_COPY(outArgs->txMtu, attachInfo.txMtu);
    outArgs->hUdmaDrv       = handleInfo.hUdmaDrv;
    outArgs->printFxnCb     = &CpswAppUtils_print;
    outArgs->isPhyLinkedFxn = &CpswApp_IsPhyLinked;

    /* As we are using ring monitor, enable timer to prevent starvation of non-burst
       packets */
    outArgs->isRingMonUsed = true;
    outArgs->clkPeriodMs = ETHFWAPP_PACKET_POLL_PERIOD_MS;

    /* Let NIMU use optimized processing where TX packets are relinquished in next
     * TX submit call */
    outArgs->disbleTxEvent  = true;
    CpswAppUtils_print("Host MAC address: ");
    CpswAppUtils_printMacAddr(&outArgs->rxInfo.macAddr[0U]);
}

void NimuCpswAppCb_releaseHandle(NimuCpswAppIf_ReleaseHandleInfo *releaseInfo)
{
    bool useDefaultFlow = gCpswMainAppObj.useDefaultRxFlow;
    CpswDma_PktInfoQ fqPktInfoQ;
    CpswDma_PktInfoQ cqPktInfoQ;

    CpswAppUtils_assert(gCpswMainAppObj.mcmCmdIf[gCpswMainAppObj.cpswType].hMboxCmd != NULL);
    CpswAppUtils_assert(gCpswMainAppObj.mcmCmdIf[gCpswMainAppObj.cpswType].hMboxResponse != NULL);

    /* Close TX channel */
    {
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
    }

    {
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
    }

    CpswMcm_coreDetach(&gCpswMainAppObj.mcmCmdIf[gCpswMainAppObj.cpswType], releaseInfo->coreId, releaseInfo->coreKey);
    CpswMcm_releaseHandleInfo(&gCpswMainAppObj.mcmCmdIf[gCpswMainAppObj.cpswType]);
}

/* end of file */
