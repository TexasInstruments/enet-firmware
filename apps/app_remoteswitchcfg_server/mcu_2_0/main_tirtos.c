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
#include <stdint.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Error.h>
/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <ti/osal/SemaphoreP.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/utils/Load.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Mailbox.h>

#include <ti/osal/osal.h>

#include <ti/drv/ipc/ipc.h>

#include <server-rtos/remote-device.h>
#include <ethremotecfg/server/include/ethremotecfg_server.h>
#include <ethremotecfg/server/include/cpsw_proxy_server.h>

#include <apps/ipc_cfg/app_ipc_rsctable.h>

/* PDK Driver Header files */
#include <ti/drv/sciclient/sciclient.h>
#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils_cfg.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appboardutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_networkutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_mcm.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpswapp_ethutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appsoc.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apprm.h>
#include <ti/drv/cpsw/nimucpsw/nimu_ndk.h>
#include <ti/drv/cpsw/nimucpsw/ndk2cpsw_appif.h>

#include <ti/drv/cpsw/cpsw_cfgserver/cpsw_cfgserver.h>

/* NDK headers */
#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/ndk/inc/socket.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/tools/servers.h>
#include <ti/ndk/inc/tools/console.h>

#include <utils/remote_service/include/app_remote_service.h>
#include <utils/perf_stats/include/app_perf_stats.h>
#include <utils/ethfw_stats/include/app_ethfw_stats_sysbios.h>

#include "webpage.h"
#include "app_intervlan.h"
#include "app_swintervlan.h"
#include <utils/profile/include/app_profile.h>

#define IPC_RPMESSAGE_OBJ_SIZE  (256U)
#define VQ_BUF_SIZE             (2048U)
#define REMOTE_DEVICE_ENDPT     (26U)
#define AUTOSAR_ETHDRIVER_DEVICE_ENDPT     (28U)
#define RPMSG_DATA_SIZE         ((256U * 512U) + IPC_RPMESSAGE_OBJ_SIZE)

static uint8_t g_ipcStackBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));
static uint8_t g_vdevMonStackBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));
static uint8_t ctrlTaskBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));

static uint8_t sysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section("ipc_data_buffer"), aligned(8)));
static uint8_t gCntrlBuf[RPMSG_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned(8)));

static uint8_t g_vringMemBuf[IPC_VRING_MEM_SIZE] __attribute__ ((section(".bss:ipc_vring_mem"), aligned(8192)));


static uint32_t selfProcId = IPC_MCU2_0;
static uint32_t gRemoteProc[] =
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
};
static uint32_t gNumRemoteProc = sizeof(gRemoteProc) / sizeof(uint32_t);

/* Test application stack size */
#define APP_TSK_STACK_MAIN                             (10U * 1024U)
#define ETHFWAPP_PACKET_POLL_PERIOD_MS                 (1U)
#define ETHFWAPP_UART_READ_TIMEOUT                     (5U)
#define ETHFWAPP_UART_WRITE_TIMEOUT                    (5U)

#define ENABLE_NDKSERVERS

#define CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US         (1000U)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct
{
    /* Core Id */
    uint32_t coreId;

    /* CPSW instance type */
    Cpsw_Type cpswType;

    /* Multiclient manager handles */
    CpswMcm_CmdIf mcmCmdIf[CPSW_COUNT];

    /* UDMA driver handle */
    Udma_DrvHandle hUdmaDrv;

    /* Use default rx flow */
    bool useDefaultRxFlow;
} CpswMain_AppObj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static void CpswApp_setAleConfig(CpswAle_Config *aleConfig);

static void CpswApp_initLinkArgs(Cpsw_OpenPortLinkInArgs *linkArgs,
                                 Cpsw_MacPort macPort);

static int32_t CpswApp_init(Cpsw_Type cpswType);

void CpswApp_deInit(void);

static void  app_ethrdev_srv_print_ethfw_device_data(uint32_t host_id);

static int32_t CpswApp_proxyServerInit(void);
static int32_t CpswApp_initPerfRemoteService(void);

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
    {&NIMU_NDK_init},
    {NULL          },
};

#ifdef ENABLE_NDKSERVERS
typedef void *HANDLE;
typedef char INT8;
typedef short INT16;
typedef int INT32;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

typedef UINT32 IPN;
typedef struct sockaddr *PSA;

static HANDLE hEcho = 0;
static HANDLE hEchoUdp = 0;
static HANDLE hData = 0;
static HANDLE hNull = 0;
static HANDLE hOob = 0;
static HANDLE hSock = 0;
#endif

char *VerStr = "NIMU CPSW Example";

static const Cpsw_MacPort gCpswMainAppMacPorts[] =
{
    CPSW_MAC_PORT_0,
#if defined(SOC_J721E)
    /* On J721E EVM to use all 8 ports simultaneously, we use below configuration
       RGMII Ports - 1,3,4,8. QSGMII ports - 2,5,6,7 */
    CPSW_MAC_PORT_2, /* RGMII */
    CPSW_MAC_PORT_3, /* RGMII */
    CPSW_MAC_PORT_7, /* RGMII */
#if defined(ENABLE_QSGMII_PORTS) //kept it disabled for 6.2
    CPSW_MAC_PORT_1, /* QSGMII main */
    CPSW_MAC_PORT_4, /* QSGMII sub */
    CPSW_MAC_PORT_5, /* QSGMII sub */
    CPSW_MAC_PORT_6, /* QSGMII sub */
#endif
#endif
};

static CpswMain_AppObj gCpswMainAppObj =
{
#if defined(SOC_AM65XX)
    .cpswType         = CPSW_2G,
#elif defined(SOC_J721E)
    .cpswType         = CPSW_9G,
#endif
    .useDefaultRxFlow = true,
};

void appLogPrintf(const char *format,
                  ...)
{
    va_list args;

    va_start(args, format);
    System_vprintf(format, args);
    CpswAppUtils_print(format, args);
    va_end(args);
}

static int32_t CpswApp_initPerfRemoteService(void)
{
    int32_t status;
    app_remote_service_init_prms_t remoteServicePrms;

    appRemoteServiceInitSetDefault(&remoteServicePrms);
    status = appRemoteServiceInit(&remoteServicePrms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("Remote service init failed: %d !!!\n", status);
    }

    if (status == CPSW_SOK)
    {
        status = appPerfStatsInit();
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Perf stats init failed: %d !!!\n", status);
        }
    }

    if (status == CPSW_SOK)
    {
        status = appPerfStatsRemoteServiceInit();
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Perf stats remote service init failed: %d !!!\n", status);
        }
    }

    if (status == CPSW_SOK)
    {
        status = appEthfwStatsInit(gCpswMainAppObj.cpswType);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Ethfw stats init failed: %d !!!\n", status);
        }
    }

    if (status == CPSW_SOK)
    {
        status = appEthfwStatsRemoteServiceInit();
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Ethfw stats remote service init failed: %d !!!\n", status);
        }
    }
    return status;
}

static void rpmsg_vdevMonitorFxn(UArg arg0,
                                 UArg arg1)
{
    int32_t status;

    /* Wait for Linux VDev ready... */
    while (!Ipc_isRemoteReady(IPC_MPU1_0))
    {
        Task_sleep(10);
    }

    /* Create the VRing now ... */
    status = Ipc_lateVirtioCreate(IPC_MPU1_0);
    if (status != IPC_SOK)
    {
        CpswAppUtils_print("%s: Ipc_lateVirtioCreate failed\n", __func__);
        return;
    }

    status = RPMessage_lateInit(IPC_MPU1_0);
    if (status != IPC_SOK)
    {
        CpswAppUtils_print("%s: RPMessage_lateInit failed\n", __func__);
        return;
    }

    status = appRemoteDeviceLateAnnounce(IPC_MPU1_0);
    if (status != IPC_SOK)
    {
        CpswAppUtils_print("%s: RPMessage_announce() failed\n", __func__);
        return;
    }

    /* Register the services after remote core is Ready */
    status = CpswApp_initPerfRemoteService();
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("%s: Performance Remote service init failed\n", __func__);
        return;
    }
}

static Void ipc_init(UArg a0,
                     UArg a1)
{
    Task_Params params;
    uint32_t numProc = gNumRemoteProc;
    Ipc_VirtIoParams vqParam;
    int32_t status;

    /* Step1 : Initialize the multiproc */
    Ipc_mpSetConfig(selfProcId, numProc, &gRemoteProc[0]);

    CpswAppUtils_print("IPC_echo_test (core : %s) .....\r\n",
                       Ipc_mpGetSelfName());

    Ipc_init(NULL);
    Ipc_loadResourceTable(appGetIpcResourceTable());

    /* Step2 : Initialize Virtio */
    vqParam.vqObjBaseAddr = (void *)&sysVqBuf[0];
    vqParam.vqBufSize = numProc * Ipc_getVqObjMemoryRequiredPerCore();
    vqParam.vringBaseAddr = (void *)g_vringMemBuf;
    vqParam.vringBufSize = sizeof(g_vringMemBuf);
    vqParam.timeoutCnt = 100;     /* Wait for counts */
    Ipc_initVirtIO(&vqParam);

    /* Step 3: Initialize RPMessage */
    RPMessage_Params cntrlParam;

    /* Initialize the param */
    RPMessageParams_init(&cntrlParam);

    /* Set memory for HeapMemory for control task */
    cntrlParam.buf = &gCntrlBuf[0];
    cntrlParam.bufSize = RPMSG_DATA_SIZE;
    cntrlParam.stackBuffer = &ctrlTaskBuf[0];
    cntrlParam.stackSize = IPC_TASK_STACKSIZE;
    RPMessage_init(&cntrlParam);

    status = CpswApp_proxyServerInit();
    CpswAppUtils_assert(status == CPSW_SOK);

    status = CpswProxyServer_start();
    CpswAppUtils_assert(status == CPSW_SOK);

    Task_Params_init(&params);
    params.priority = 3;
    params.stackSize = IPC_TASK_STACKSIZE;
    params.stack = &g_vdevMonStackBuf[0];
    params.stackSize = IPC_TASK_STACKSIZE;
    Task_create(rpmsg_vdevMonitorFxn, &params, NULL);
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
    aleConfig->vlanConfig.autoLearnWithVLAN = false;

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
    CpswMacPort_Config *macConfig = &linkArgs->macConfig;
    CpswMacPort_LinkConfig *linkConfig = &linkArgs->linkConfig;
    CpswMacPort_Interface *interface = &linkArgs->interface;
    CpswPhy_Config *phyConfig = &linkArgs->phyConfig;

    linkArgs->portNum = macPort;

    Cpsw_initMacPortParams(macConfig);

    CpswAppBoardUtils_setPhyConfig(gCpswMainAppObj.cpswType,
                                   linkArgs->portNum,
                                   macConfig,
                                   interface,
                                   phyConfig);

    if (phyConfig->phyAddr == CPSW_PHY_INVALID_PHYADDR)
    {
        linkConfig->speed = CPSW_SPEED_1GBIT;
        linkConfig->duplexity = CPSW_DUPLEX_FULL;
    }
    else
    {
        linkConfig->speed = CPSW_SPEED_AUTO;
        linkConfig->duplexity = CPSW_DUPLEX_AUTO;
    }

    CpswAppInterVlan_setMacConfig(linkArgs, macPort);
}

static int32_t CpswApp_init(Cpsw_Type cpswType)
{
    CpswMcm_InitConfig cpswMcmCfg;
    Cpsw_Config cpswCfg;
    int32_t status = CPSW_SOK;

    CpswAppUtils_assert(CPSW_UTILS_ARRAYSIZE(gCpswMainAppMacPorts) <= CPSW_MAC_PORT_NUM);

    /* Set configuration parameters */
    Cpsw_initParams(&cpswCfg);
    cpswCfg.vlanConfig.vlanAware = true;
    cpswCfg.hostPortConfig.removeCrc = true;
    cpswCfg.hostPortConfig.padShortPacket = true;
    cpswCfg.hostPortConfig.passCrcErrors = true;
    cpswCfg.hostPortConfig.enableCsumOffload = true;
    CpswAppUtils_initResourceConfig(cpswType, CpswAppSoc_getCoreId(), &cpswCfg.resourceConfig);

    CpswApp_setAleConfig(&cpswCfg.aleConfig);

    /* Use high priority RX channel to get higher priority on the UDMA */
    cpswCfg.dmaConfig.rxChInitPrms.dmaPriority = TISCI_MSG_VALUE_RM_UDMAP_CH_SCHED_PRIOR_HIGH;

    CpswAppInterVlan_setOpenPrms(&cpswCfg);

    /* Policer Config */
    cpswCfg.aleConfig.policerGlobalConfig.policingEnable     = true;
    cpswCfg.aleConfig.policerGlobalConfig.yellowDropEnable   = false;
    cpswCfg.aleConfig.policerGlobalConfig.redDropEnable      = true;
    cpswCfg.aleConfig.policerGlobalConfig.policerNoMatchMode = CPSW_ALE_POLICER_NOMATCH_MODE_GREEN;

    /* Open UDMA */
    gCpswMainAppObj.hUdmaDrv = CpswAppUtils_udmaOpen(cpswType, NULL);
    cpswCfg.dmaConfig.hUdmaDrv = gCpswMainAppObj.hUdmaDrv;

    cpswMcmCfg.pCpswCfg = &cpswCfg;
    cpswMcmCfg.cpswType = cpswType;
    cpswMcmCfg.setPortLinkCfg = CpswApp_initLinkArgs;
    cpswMcmCfg.numMacPorts = CPSW_UTILS_ARRAYSIZE(gCpswMainAppMacPorts);
    cpswMcmCfg.periodicTaskPeriod = CPSW_PHY_FSM_TICK_PERIOD_MS; /* msecs */

    memcpy(&cpswMcmCfg.macPortList[0U], &gCpswMainAppMacPorts[0U], sizeof(gCpswMainAppMacPorts));

    status = CpswMcm_init(&cpswMcmCfg);
    CpswAppUtils_assert(status == CPSW_SOK);

    return status;
}

void CpswApp_deInit(void)
{
    CpswAppUtils_udmaclose(gCpswMainAppObj.hUdmaDrv);

    memset(&gCpswMainAppObj, 0U, sizeof(CpswMain_AppObj));
}

static bool CpswApp_isAllPortLinked(Cpsw_Handle hCpsw)
{
    uint32_t i;
    bool isPhyLinked = false;

    for (i = 0; i < CPSW_UTILS_ARRAYSIZE(gCpswMainAppMacPorts); i++)
    {
        isPhyLinked = (isPhyLinked ||
                       CpswAppUtils_isPortLinkUp(hCpsw, CpswAppSoc_getCoreId(),
                       gCpswMainAppMacPorts[i]));
    }

    return isPhyLinked;
}

void NimuCpswAppCb_getHandle(NimuCpswAppIf_GetHandleInArgs *inArgs,
                             NimuCpswAppIf_GetHandleOutArgs *outArgs)
{
    int32_t status;
    CpswMcm_HandleInfo handleInfo;
    Cpsw_AttachCoreOutArgs attachInfo;
    uint32_t coreId = CpswAppSoc_getCoreId();
    bool useDefaultFlow = gCpswMainAppObj.useDefaultRxFlow;
    Cpsw_Type cpswType = gCpswMainAppObj.cpswType;
    CpswDma_OpenTxChPrms cpswTxChCfg;
    CpswDma_OpenRxFlowPrms cpswRxFlowCfg;
    CpswDma_UdmaRingPrms *pFqRingPrms;

    if (gCpswMainAppObj.mcmCmdIf[cpswType].hMboxCmd == NULL)
    {
        status = CpswApp_init(cpswType);

        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Failed to open CPSW: %d\n", status);
        }

        CpswAppUtils_assert(status == CPSW_SOK);
        CpswMcm_getCmdIf(cpswType, &gCpswMainAppObj.mcmCmdIf[cpswType]);
    }

    CpswAppUtils_assert(gCpswMainAppObj.mcmCmdIf[cpswType].hMboxCmd != NULL);
    CpswAppUtils_assert(gCpswMainAppObj.mcmCmdIf[cpswType].hMboxResponse != NULL);

    CpswMcm_acquireHandleInfo(&gCpswMainAppObj.mcmCmdIf[cpswType], &handleInfo);
    CpswMcm_coreAttach(&gCpswMainAppObj.mcmCmdIf[cpswType], coreId, &attachInfo);

    /* Open TX channel */
    CpswDma_initTxChParams(&cpswTxChCfg);

    cpswTxChCfg.hUdmaDrv = handleInfo.hUdmaDrv;
    cpswTxChCfg.numTxPkts = inArgs->txCfg.numPackets;
    cpswTxChCfg.hCbArg = inArgs->txCfg.cbArg;
    cpswTxChCfg.notifyCb = inArgs->txCfg.notifyCb;
    cpswTxChCfg.useProxy = true;

    cpswTxChCfg.disableCacheOpsFlag = false;

    cpswTxChCfg.ringMemAllocFxn = &CpswAppMemUtils_allocRingMemFxn;
    cpswTxChCfg.ringMemFreeFxn = &CpswAppMemUtils_freeRingMemFxn;

    cpswTxChCfg.dmaDescAllocFxn = &CpswAppMemUtils_allocDmaDescFxn;
    cpswTxChCfg.dmaDescFreeFxn = &CpswAppMemUtils_freeDmaDescFxn;

    CpswAppUtils_openTxCh(handleInfo.hCpsw,
                          attachInfo.coreKey,
                          coreId,
                          &outArgs->txInfo.txChNum,
                          &outArgs->txInfo.hTxChannel,
                          &cpswTxChCfg);

    /* Open RX Flow */
    CpswDma_initRxFlowParams(&cpswRxFlowCfg);
    cpswRxFlowCfg.notifyCb = inArgs->rxCfg.notifyCb;
    cpswRxFlowCfg.numRxPkts = inArgs->rxCfg.numPackets;
    cpswRxFlowCfg.hUdmaDrv = handleInfo.hUdmaDrv;
    cpswRxFlowCfg.hCbArg = inArgs->rxCfg.cbArg;
    cpswRxFlowCfg.useProxy = true;

    /* Use ring monitor for the CQ ring of RX flow */
    pFqRingPrms = &cpswRxFlowCfg.udmaChPrms.fqRingPrms;
    pFqRingPrms->useRingMon = false;

    cpswRxFlowCfg.disableCacheOpsFlag = false;
    cpswRxFlowCfg.rxFlowMtu = attachInfo.rxMtu;

    cpswRxFlowCfg.ringMemAllocFxn = &CpswAppMemUtils_allocRingMemFxn;
    cpswRxFlowCfg.ringMemFreeFxn = &CpswAppMemUtils_freeRingMemFxn;

    cpswRxFlowCfg.dmaDescAllocFxn = &CpswAppMemUtils_allocDmaDescFxn;
    cpswRxFlowCfg.dmaDescFreeFxn = &CpswAppMemUtils_freeDmaDescFxn;

    CpswAppUtils_openRxFlow(handleInfo.hCpsw,
                            attachInfo.coreKey,
                            coreId,
                            useDefaultFlow,
                            &outArgs->rxInfo.rxFlowStartIdx,
                            &outArgs->rxInfo.rxFlowIdx,
                            &outArgs->rxInfo.macAddr[0U],
                            &outArgs->rxInfo.hRxFlow,
                            &cpswRxFlowCfg);

    CpswAppUtils_print("Host MAC address: ");
    CpswAppUtils_printMacAddr(&outArgs->rxInfo.macAddr[0U]);

    outArgs->coreId = coreId;
    outArgs->coreKey = attachInfo.coreKey;
    outArgs->hCpsw = handleInfo.hCpsw;
    outArgs->hostPortRxMtu = attachInfo.rxMtu;
    CPSW_UTILS_ARRAY_COPY(outArgs->txMtu, attachInfo.txMtu);
    outArgs->hUdmaDrv = handleInfo.hUdmaDrv;
    outArgs->printFxnCb = &CpswAppUtils_print;
    outArgs->isPortLinkedFxn = &CpswApp_isAllPortLinked;
    /* TODO: NIMU's polling timer is getting corrupted at times of sudden burst of
     * traffic, because of which timer callback is never called.
     * With polling timer not functional, packets are never serviced then after.
     * As a workaround setting isRingMonUsed to true (irrespective of ring monitor
     * is enabled or not) to ensure interrupts are used instead of polling.
     * Timer corruption needs to be root-caused and fixed.
     */
    outArgs->isRingMonUsed = true;
    outArgs->timerPeriodUs = CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US;
}

void NimuCpswAppCb_releaseHandle(NimuCpswAppIf_ReleaseHandleInfo *releaseInfo)
{
    CpswDma_PktInfoQ fqPktInfoQ;
    CpswDma_PktInfoQ cqPktInfoQ;
    bool useDefaultFlow = gCpswMainAppObj.useDefaultRxFlow;
    Cpsw_Type cpswType = gCpswMainAppObj.cpswType;

    CpswAppUtils_assert(gCpswMainAppObj.mcmCmdIf[cpswType].hMboxCmd != NULL);
    CpswAppUtils_assert(gCpswMainAppObj.mcmCmdIf[cpswType].hMboxResponse != NULL);

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

    CpswMcm_coreDetach(&gCpswMainAppObj.mcmCmdIf[cpswType], releaseInfo->coreId, releaseInfo->coreKey);
    CpswMcm_releaseHandleInfo(&gCpswMainAppObj.mcmCmdIf[cpswType]);
}

/* Functions called from Config server library based on selection from GUI */
void CpswApp_startSwInterVlan(char *recvBuff,
                              char *sendBuff)
{
    CpswCfgServer_InterVlanConfig *pInterVlanCfg;
    int32_t status = CPSW_SOK;

    if (recvBuff != NULL)
    {
        pInterVlanCfg = (CpswCfgServer_InterVlanConfig *)recvBuff;
        status = CpswApp_addSwIVlanClasifierEntries(pInterVlanCfg);
        CpswAppUtils_assert(CPSW_SOK == status);
    }
}

void CpswApp_startHwInterVlan(char *recvBuff,
                              char *sendBuff)
{
    CpswCfgServer_InterVlanConfig *pInterVlanCfg;

    if (recvBuff != NULL)
    {
        pInterVlanCfg = (CpswCfgServer_InterVlanConfig *)recvBuff;

        CpswApp_hwInterVlanRouting(gCpswMainAppObj.cpswType,
                                   pInterVlanCfg);
    }
}

void stackInitHook(void *hCfg)
{
    int rc;

    /* increase stack size */
    rc = 16384;
    CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_TASKSTKBOOT,
                CFG_ADDMODE_UNIQUE, sizeof(uint32_t), (uint8_t *)&rc, 0);
    AddWebFiles();
}

void stackDeleteHook(void *hCfg)
{
    RemoveWebFiles();
}

int32_t CpswApp_setAleMulticastEntry(uint8_t macAddr[CPSW_MAC_ADDR_LEN],
                                     uint32_t vlanId,
                                     uint32_t numIgnBits,
                                     uint32_t portMask)
{
    int32_t status;
    Cpsw_Handle hCpsw = Cpsw_getHandle(gCpswMainAppObj.cpswType);
    Cpsw_IoctlPrms prms;
    CpswAle_AddEntryOutArgs setMcastOutArgs;
    CpswAle_SetMcastEntryInArgs setMcastInArgs;

    memcpy(&setMcastInArgs.addr.addr[0], macAddr,
           sizeof(setMcastInArgs.addr.addr));
    setMcastInArgs.addr.vlanId = vlanId;

    setMcastInArgs.info.superFlag  = false;
    setMcastInArgs.info.fwdState   = CPSW_ALE_FWDSTLVL_FORWARDING;
    setMcastInArgs.info.portMask   = portMask;
    setMcastInArgs.info.numIgnBits = numIgnBits;

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setMcastInArgs, &setMcastOutArgs);

    status = Cpsw_ioctl(hCpsw, CpswAppSoc_getCoreId(), CPSW_ALE_IOCTL_ADD_MULTICAST,
                        &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("CpswApp_setAleMulticastEntry() failed CPSW_ALE_IOCTL_ADD_MULTICAST: %d\n",
                           status);
    }

    return status;
}

void IpAddrHookFxn(uint32_t IPAddr,
                   uint32_t IfIdx,
                   uint32_t fAdd)
{
    volatile uint32_t ipAddrHex = 0U;
    uint8_t bCastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    char ipAddr[20];
    int32_t status;

    ipAddrHex = ntohl(IPAddr);
    snprintf(ipAddr, 17, "%d.%d.%d.%d\n",
             (uint8_t)(ipAddrHex >> 24) & 0xFF,
             (uint8_t)(ipAddrHex >> 16) & 0xFF,
             (uint8_t)(ipAddrHex >> 8) & 0xFF,
             (uint8_t)ipAddrHex & 0xFF);

    CpswAppUtils_print("\nCPSW NIMU application, IP address I/F 1: %s\n\r", ipAddr);

    /* Assign functions that are to be called based on actions in GUI.
     * These cannot be dynamically pushed to function pointer array, as the
     * index is used in GUI as command.
     */
    cpswCfgServer_fxn_table[9] = &CpswApp_startSwInterVlan;
    cpswCfgServer_fxn_table[10] = &CpswApp_startHwInterVlan;

    /* Start Configuration server */
    status = CpswCfgServer_init(gCpswMainAppObj.cpswType);
    CpswAppUtils_assert(CPSW_SOK == status);

    /* Add ALE entry for broadcast mac address. Note this is needed as the broadcast
     * is disabled via unknownRegMcastFloodMask and other flags in ALE init config.
     * In EthFw we need broadcast to handle ARP entries for clients
     */
    CpswApp_setAleMulticastEntry(&bCastAddr[0U],
                                 0U, /* vlanId */
                                 0U, /* numIgnBits */
                                 CPSW_ALE_ALL_PORTS_MASK);
    CpswApp_swInterVlanRouting(gCpswMainAppObj.cpswType);
}

void netOpenHook(void)
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
    hSock = DaemonNew(SOCK_STREAM, 0, 1002, dtask_tcp_datasrv,
                      OS_TASKPRINORM, OS_TASKSTKNORM, 0, 3);
    hOob = DaemonNew(SOCK_STREAMNC, 0, 999, dtask_tcp_oobsrv,
                     OS_TASKPRINORM, OS_TASKSTKNORM, 0, 3);
#endif
}

void netCloseHook(void)
{
#ifdef ENABLE_NDKSERVERS
    DaemonFree(hOob);
    DaemonFree(hNull);
    DaemonFree(hData);
    DaemonFree(hEchoUdp);
    DaemonFree(hEcho);
    DaemonFree(hSock);
#endif
}

void ServiceReportHook(uint32_t Item, uint32_t Status, uint32_t Report, void * h)
{
    if( (Item == CFGITEM_SERVICE_DHCPCLIENT) && ((Report & 0xFF) == POLLOUT))
    {
        CI_SERVICE_DHCPC dhcpc;
        int status;

        CpswAppUtils_print("DHCP client timed out. Retrying..... \n");

        /* By default, DHCP client service timeouts after three minutes and the
         * service gets terminated. So we have to restart DHCP client service after
         * timeout happens by adding a DHCP client service entry*/
        memset(&dhcpc, 0U, sizeof(dhcpc));
        dhcpc.cisargs.Mode   = CIS_FLG_IFIDXVALID;
        dhcpc.cisargs.IfIdx  = CIS_FLG_IFIDXVALID;
        dhcpc.cisargs.pCbSrv = &ServiceReportHook;
        status = CfgAddEntry(0, CFGTAG_SERVICE, CFGITEM_SERVICE_DHCPCLIENT, 0,
                             sizeof(dhcpc), (unsigned char *)&dhcpc, 0);
        CpswAppUtils_assert(status >= 0);
    }
}

int main(void)
{
    Task_Handle task;
    Task_Params ipc_taskParams;
    uint32_t host_id = CpswAppSoc_getCoreId();
    /* Set ccsHaltFlag to 1 for halting core for CCS connection */
    volatile uint32_t ccsHaltFlag = 0U;

    while (ccsHaltFlag)
    {
        ;
    }

    CpswAppBoardUtils_initEthFw();

    CpswAppUtils_enableClocks(gCpswMainAppObj.cpswType);

    CpswAppUtils_print("=======================================================\n");
    CpswAppUtils_print("           CPSW Ethernet Firmware Demo             \n");
    CpswAppUtils_print("=======================================================\n");

    app_ethrdev_srv_print_ethfw_device_data(host_id);

    Task_Params_init(&ipc_taskParams);
    ipc_taskParams.priority = 2;
    ipc_taskParams.stack = &g_ipcStackBuf[0];
    ipc_taskParams.stackSize = IPC_TASK_STACKSIZE;
    task = Task_create(ipc_init, &ipc_taskParams, NULL);

    if (NULL == task)
    {
        BIOS_exit(0);
    }

    BIOS_start();    /* does not return */

    return(0);
}





#define APP_DATE_OFFSET_MONTH  (0)
#define APP_DATE_OFFSET_DATE   (4)
#define APP_DATE_OFFSET_YEAR   (7)

static void  CpswApp_getEthfwDeviceData(uint32_t host_id,
                                        struct rpmsg_kdrv_ethswitch_device_data *eth_dev_data)
{
    /* __DATE__ is a string constant that contains eleven characters and
     * looks like "Feb 12 1996". If the day of the month is less than
     * 10, it is padded with a space on the left
     */
    char *date = __DATE__;

    eth_dev_data->fw_ver.major = RPMSG_KDRV_TP_ETHSWITCH_VERSION_MAJOR;
    eth_dev_data->fw_ver.minor = RPMSG_KDRV_TP_ETHSWITCH_VERSION_MINOR;
    eth_dev_data->fw_ver.rev = RPMSG_KDRV_TP_ETHSWITCH_VERSION_REVISION;
    memcpy(eth_dev_data->fw_ver.month, &date[APP_DATE_OFFSET_MONTH], sizeof(eth_dev_data->fw_ver.month));
    memcpy(eth_dev_data->fw_ver.date, &date[APP_DATE_OFFSET_DATE], sizeof(eth_dev_data->fw_ver.date));
    memcpy(eth_dev_data->fw_ver.year, &date[APP_DATE_OFFSET_YEAR], sizeof(eth_dev_data->fw_ver.year));
    /* RPMSG_KDRV_TP_ETHSWITCH_VERSION_LAST_COMMIT is defined by the build system */
    memcpy(eth_dev_data->fw_ver.commit_hash, RPMSG_KDRV_TP_ETHSWITCH_VERSION_LAST_COMMIT, sizeof(eth_dev_data->fw_ver.commit_hash));

    /* Enable permission for all ETHDEV remote commands without consideration of cores.
     * This should be changed based on trusted cores
     */
    eth_dev_data->permission_flags = ((1 << RPMSG_KDRV_TP_ETHSWITCH_MAX) - 1);
    eth_dev_data->uart_connected = true;
    eth_dev_data->uart_id = CPSW_UTILS_MCU2_0_UART_INSTANCE;
}

static void CpswApp_proxyServerGetMcmCmdIfCb(Cpsw_Type cpswType, CpswMcm_CmdIf **pMcmCmdIfHandle)
{
    int32_t status;

    if (gCpswMainAppObj.mcmCmdIf[cpswType].hMboxCmd == NULL)
    {
        status = CpswApp_init(cpswType);

        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Failed to open CPSW: %d\n", status);
        }

        CpswAppUtils_assert(status == CPSW_SOK);
        CpswMcm_getCmdIf(cpswType, &gCpswMainAppObj.mcmCmdIf[cpswType]);
    }

    CpswAppUtils_assert(gCpswMainAppObj.mcmCmdIf[cpswType].hMboxCmd != NULL);
    CpswAppUtils_assert(gCpswMainAppObj.mcmCmdIf[cpswType].hMboxResponse != NULL);
    *pMcmCmdIfHandle = &gCpswMainAppObj.mcmCmdIf[cpswType];
}

static void CpswApp_printProfileInfo(appProfileAvgLoadInfo *avgProfileInfo)
{
    uint32_t i;

    CpswAppUtils_print("\n********\n");
    CpswAppUtils_print("CPU Load:%d\n", avgProfileInfo->cpuLoad);
    CpswAppUtils_print("Packet Processing Count:%d\n", avgProfileInfo->packetCount);
    CpswAppUtils_print("ISR:%d\n", avgProfileInfo->isr);
    CpswAppUtils_print("SWI:%d\n", avgProfileInfo->swi);
    CpswAppUtils_print("Total task count:%d\n", avgProfileInfo->totalTaskCount);
    CpswAppUtils_print("Active task count:%d\n", avgProfileInfo->activeTaskCount);
    for (i = 0; i < avgProfileInfo->activeTaskCount; i++)
    {
        CpswAppUtils_print("TASK:%s:%d\n", 
                           avgProfileInfo->tskLoad[i].tskName,
                           avgProfileInfo->tskLoad[i].load);
    }
    CpswAppUtils_print("********\n");
}


static void CpswApp_proxyProfileInfoNotifyHandler(uint32_t host_id,
                                                  Cpsw_Handle hCpsw,
                                                  Cpsw_Type cpswType,
                                                  uint32_t core_key,
                                                  enum rpmsg_kdrv_ethswitch_client_notify_type notifyid,
                                                  uint8_t *notify_info,
                                                  uint32_t notify_info_len)
{

    appProfileAvgLoadInfo *avgProfileInfo = (appProfileAvgLoadInfo *)notify_info;

    CpswAppUtils_assert(Cpsw_getHandle(cpswType) == hCpsw);
    CpswAppUtils_assert(notifyid == RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_CUSTOM);
    CpswAppUtils_assert(notify_info_len == sizeof(appProfileAvgLoadInfo));
    CpswApp_printProfileInfo(avgProfileInfo);
}

static int32_t CpswApp_proxyServerInit(void)
{
    CpswProxyServer_Config_t cfg;
    int32_t status;

    memset(&cfg, 0, sizeof(cfg));
    cfg.getMcmCmdIfCb = &CpswApp_proxyServerGetMcmCmdIfCb;
    cfg.initEthfwDeviceDataCb = &CpswApp_getEthfwDeviceData;
    cfg.notifyCb = &CpswApp_proxyProfileInfoNotifyHandler;
    cfg.rpmsgEndPointId = REMOTE_DEVICE_ENDPT;

    cfg.numRemoteCores = 2;
    cfg.remoteCoreCfg[0].remoteCoreId = IPC_MCU2_1;
    snprintf(cfg.remoteCoreCfg[0].serverName, ETHREMOTECFG_SERVER_MAX_NAME_LEN, ETHREMOTEDEVICE_DEVICE_NAME_MCU_2_1);
    cfg.remoteCoreCfg[1].remoteCoreId = IPC_MPU1_0;
    snprintf(cfg.remoteCoreCfg[1].serverName, ETHREMOTECFG_SERVER_MAX_NAME_LEN, ETHREMOTEDEVICE_DEVICE_NAME_MPU_1_0);

    cfg.autosarEthDriverRemoteCoreId = IPC_MCU2_1;
    cfg.autosarEthDeviceEndPointId = AUTOSAR_ETHDRIVER_DEVICE_ENDPT;
    status = CpswProxyServer_init(&cfg);
    CpswAppUtils_assert(status == CPSW_SOK);
    return CPSW_SOK;
}

static void  app_ethrdev_srv_print_ethfw_device_data(uint32_t host_id)
{
    struct rpmsg_kdrv_ethswitch_device_data eth_dev_data;
    char *tf[] = {"false", "true"};

    CpswApp_getEthfwDeviceData(host_id, &eth_dev_data);

    CpswAppUtils_print("ETHFW Version:%2d.%2d.%2d\n",
                  eth_dev_data.fw_ver.major,
                  eth_dev_data.fw_ver.minor,
                  eth_dev_data.fw_ver.rev);
    CpswAppUtils_print("ETHFW Build Date (YYYY/MMM/DD):%c%c%c%c/%c%c%c/%c%c\n",
                  eth_dev_data.fw_ver.year[0], eth_dev_data.fw_ver.year[1], eth_dev_data.fw_ver.year[2], eth_dev_data.fw_ver.year[3],
                  eth_dev_data.fw_ver.month[0], eth_dev_data.fw_ver.month[1], eth_dev_data.fw_ver.month[2],
                  eth_dev_data.fw_ver.date[0], eth_dev_data.fw_ver.date[1]);
    CpswAppUtils_print("ETHFW Commit SHA:%c%c%c%c%c%c%c%c\n",
                  eth_dev_data.fw_ver.commit_hash[0],
                  eth_dev_data.fw_ver.commit_hash[1],
                  eth_dev_data.fw_ver.commit_hash[2],
                  eth_dev_data.fw_ver.commit_hash[3],
                  eth_dev_data.fw_ver.commit_hash[4],
                  eth_dev_data.fw_ver.commit_hash[5],
                  eth_dev_data.fw_ver.commit_hash[6],
                  eth_dev_data.fw_ver.commit_hash[7]);
    CpswAppUtils_print("ETHFW PermissionFlag:0x%x, UART Connected:%s,UART Id:%d",
                  eth_dev_data.permission_flags,
                  tf[eth_dev_data.uart_connected],
                  eth_dev_data.uart_id);
}

