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
#include <xdc/runtime/Timestamp.h>
#include <xdc/runtime/Types.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>

/* PDK Driver Header files */
#include <ti/drv/ipc/ipc.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/core/enet_mod_hostport.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/enet/include/dma/udma/enet_udma.h>
#include <ti/drv/enet/include/core/enet_dma.h>
#include <ti/drv/enet/examples/utils/include/enet_apputils.h>
#include <ti/drv/enet/examples/utils/include/enet_appboardutils.h>
#include <ti/drv/enet/examples/utils/include/enet_appsoc.h>

/* EthFw header files */
#include <apps/ipc_cfg/app_ipc_rsctable.h>
#include <utils/intervlan/include/eth_hwintervlan.h>
#include <utils/intervlan/include/eth_swintervlan.h>
#include <utils/ethfw_callbacks/include/ethfw_callbacks_nimu.h>
#include <utils/ethfw_callbacks/include/ethfw_callbacks_ndk.h>
#include <ethfw/ethfw.h>

/* Timesync header files */
#include <ti/transport/timeSync/v2/include/timeSync.h>
#include <ti/transport/timeSync/v2/protocol/ptp/include/timeSync_ptp.h>

/* EthFw utils header files */
#include <utils/remote_service/include/app_remote_service.h>
#include <utils/perf_stats/include/app_perf_stats.h>
#include <utils/ethfw_stats/include/app_ethfw_stats_sysbios.h>

/* NS headers */
#include <ti/ndk/slnetif/slnetifndk.h>
#include <ti/net/slnet.h>
#include <ti/net/slnetif.h>
#include <ti/net/slnetutils.h>

/* HTTP webpage server header files */
#include "webdata/webpage.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define ETHAPP_OK                       (0)

#define ETHAPP_ERROR                    (-1)

#define ETHAPP_TRACEBUF_FLUSH_PERIOD    (5ULL)

#define IPC_TRACEBUF_SIZE               (0x80000U)

#define VQ_BUF_SIZE                     (2048U)

#define IPC_RPMESSAGE_OBJ_SIZE          (256U)

#define RPMSG_DATA_SIZE                 ((256U * 512U) + IPC_RPMESSAGE_OBJ_SIZE)

#define ARRAY_SIZE(x)                   (sizeof((x)) / sizeof(x[0U]))

/* Define A72_QNX_OS if A72 is running Qnx. Qnx doesn't load resource table. */
/* #define A72_QNX_OS */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct
{
    /* Core Id */
    uint32_t coreId;

    /* Enet instance type */
    Enet_Type enetType;

    /* Enet instance id */
    uint32_t instId;

    /* Ethernet Firmware handle */
    EthFw_Handle hEthFw;

    /* UDMA driver handle */
    Udma_DrvHandle hUdmaDrv;

    /* Semaphore for synchronizing EthFw and NDK initialization */
    Semaphore_Handle hInitSem;

    /* Host MAC address */
    uint8_t hostMacAddr[ENET_MAC_ADDR_LEN];

    /* Host IP address */
    uint32_t hostIpAddr;

    /* IPC trace buffer address */
    uint8_t *traceBufAddr;

    /* IPC trace buffer size */
    uint32_t traceBufSize;

    /* Timestamp of last IPC trace buffer flush */
    uint64_t traceBufLastFlushTicks;
} EthAppObj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static void EthApp_waitForDebugger(void);

static void EthApp_initTaskFxn(UArg arg0, UArg arg1);

static void EthApp_initIpcTaskFxn(UArg arg0, UArg arg1);

static int32_t EthApp_initEthFw(void);

static int32_t EthApp_initRemoteServices(void);

static void EthApp_startSwInterVlan(char *recvBuff,
                                    char *sendBuff);

static void EthApp_startHwInterVlan(char *recvBuff,
                                    char *sendBuff);

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

static EthAppObj gEthAppObj =
{
#if defined(SOC_J721E)
    .enetType = ENET_CPSW_9G,
    .instId   = 0U,
#elif defined(SOC_J7200)
    .enetType = ENET_CPSW_5G,
    .instId   = 0U,
#endif
    .hEthFw = NULL,
    .hUdmaDrv = NULL,
};

static EthFw_Port gEthAppPorts[] =
{
#if defined(SOC_J721E)
    /* On J721E EVM to use all 8 ports simultaneously, we use below configuration
       RGMII Ports - 1,3,4,8. QSGMII ports - 2,5,6,7 */
    {
        .portNum    = ENET_MAC_PORT_1,
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U },
    },
    {
        .portNum    = ENET_MAC_PORT_3, /* RGMII */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = ENET_MAC_PORT_4, /* RGMII */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = ENET_MAC_PORT_8, /* RGMII */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
#if defined(ENABLE_QSGMII_PORTS) //kept it disabled for 6.2
    {
        .portNum    = ENET_MAC_PORT_2, /* QSGMII main */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = ENET_MAC_PORT_5, /* QSGMII sub */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = ENET_MAC_PORT_6, /* QSGMII sub */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = ENET_MAC_PORT_7, /* QSGMII sub */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
#endif
#endif
#if defined(SOC_J7200)
#if defined(ENABLE_QSGMII_PORTS)
    /* On J7200 to use all 4 ports simultaneously, we use below configuration
     * QSGMII ports - 0, 1, 2, 3 */
    {
        .portNum    = ENET_MAC_PORT_1, /* QSGMII main */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = ENET_MAC_PORT_2, /* QSGMII sub */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = ENET_MAC_PORT_3, /* QSGMII sub */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = ENET_MAC_PORT_4, /* QSGMII sub */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
#else
    /* For internal testing only - Alternatively, a single RGMII port
     * configuration via GESI board is also available */
    {
        .portNum    = ENET_MAC_PORT_2, /* RGMII */
        .vlanCfg = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
#endif
#endif
};

static uint8_t gEthAppStackBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));

static uint8_t gEthAppIpcInitStackBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));

static uint8_t gEthAppCtrlTaskBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));

static uint8_t gEthAppSysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section("ipc_data_buffer"), aligned(8)));

static uint8_t gEthAppCntrlBuf[RPMSG_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned(8)));

static uint8_t gEthAppVringMemBuf[IPC_VRING_MEM_SIZE] __attribute__ ((section(".bss:ipc_vring_mem"), aligned(8192)));

static uint32_t gEthAppRemoteProc[] =
#if defined(SOC_J721E)
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_1,
    IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2,
    IPC_C7X_1
};
#elif defined(SOC_J7200)
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_1,
};
#endif

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int main(void)
{
    Task_Handle task;
    Task_Params taskParams;
    Semaphore_Params semParams;

    /* Wait for debugger to attach (disabled by default) */
    EthApp_waitForDebugger();

    gEthAppObj.coreId = EnetSoc_getCoreId();

    /* Board related initialization */
    EnetBoard_initEthFw();
    EnetAppUtils_enableClocks(gEthAppObj.enetType, gEthAppObj.instId);

    /* Create semaphore used to synchronize EthFw and NDK init.
     * EthFw opens the CPSW driver which is required by NDK during NIMU
     * initialization, hence EthFw init must complete first.
     * Currently, there is no control over NDK initialization time and its
     * task runs right away after BIOS_start() hence causing a race
     * condition with EthFw init */
    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    gEthAppObj.hInitSem = Semaphore_create(0, &semParams, NULL);

    /* Create initialization task */
    Task_Params_init(&taskParams);
    taskParams.priority = 2;
    taskParams.stack = &gEthAppStackBuf[0];
    taskParams.stackSize = sizeof(gEthAppStackBuf);
    taskParams.instance->name = "EthFw Init Task";

    task = Task_create(EthApp_initTaskFxn, &taskParams, NULL);
    if (NULL == task)
    {
        BIOS_exit(0);
    }

    /* Does not return */
    BIOS_start();

    return(0);
}

void appLogPrintf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    System_vprintf(format, args);
    EnetAppUtils_vprint(format, args);
    va_end(args);
}

static void EthApp_waitForDebugger(void)
{
    /* Set ccsHaltFlag to 1 for halting core for CCS connection */
    volatile uint32_t ccsHaltFlag = 0U;

    while (ccsHaltFlag);
}

static void EthApp_initTaskFxn(UArg arg0, UArg arg1)
{
    Task_Params taskParams;
    int32_t status = ETHAPP_OK;

    /* Print EthFw banner */
    appLogPrintf("=======================================================\n");
    appLogPrintf("            CPSW Ethernet Firmware                     \n");
    appLogPrintf("=======================================================\n");

    /* Open UDMA driver */
    gEthAppObj.hUdmaDrv = EnetAppUtils_udmaOpen(gEthAppObj.enetType, NULL);
    if (gEthAppObj.hUdmaDrv == NULL)
    {
        appLogPrintf("ETHFW: failed to open UDMA driver\n");
        status = ETHAPP_ERROR;
    }

    /* Initialize Ethernet Firmware */
    if (status == ETHAPP_OK)
    {
        status = EthApp_initEthFw();
    }

    /* Create IPC initialization task */
    if (status == ENET_SOK)
    {
        Task_Params_init(&taskParams);
        taskParams.priority = 1;
        taskParams.stack = &gEthAppIpcInitStackBuf[0];
        taskParams.stackSize = sizeof(gEthAppIpcInitStackBuf);
        taskParams.instance->name = "EthFw IPC init Task";

        Task_create(EthApp_initIpcTaskFxn, &taskParams, NULL);
    }
}

static void EthApp_initIpcTaskFxn(UArg arg0, UArg arg1)
{
    uint32_t selfProcId = IPC_MCU2_0;
    uint32_t numProc = ARRAY_SIZE(gEthAppRemoteProc);
    Ipc_VirtIoParams vqParam;
    RPMessage_Params cntrlParam;
    int32_t status;

    /* Step 1: Initialize the multiproc */
    Ipc_mpSetConfig(selfProcId, numProc, &gEthAppRemoteProc[0]);

    appLogPrintf("IPC_echo_test (core : %s) .....\r\n", Ipc_mpGetSelfName());

    status = Ipc_init(NULL);

#if !defined(A72_QNX_OS)
    if (status == ENET_SOK)
    {
        status = Ipc_loadResourceTable(appGetIpcResourceTable());
    }
#endif

    if (status == ENET_SOK)
    {
        /* Step 2: Initialize Virtio */
        vqParam.vqObjBaseAddr = (void *)&gEthAppSysVqBuf[0];
        vqParam.vqBufSize = numProc * Ipc_getVqObjMemoryRequiredPerCore();
        vqParam.vringBaseAddr = (void *)gEthAppVringMemBuf;
        vqParam.vringBufSize = sizeof(gEthAppVringMemBuf);
        vqParam.timeoutCnt = 100;     /* Wait for counts */
        status = Ipc_initVirtIO(&vqParam);
    }

    if (status == ENET_SOK)
    {
        /* Step 3: Initialize RPMessage */
        /* Initialize the param and set memory for HeapMemory for control task */
        RPMessageParams_init(&cntrlParam);
        cntrlParam.buf = &gEthAppCntrlBuf[0];
        cntrlParam.bufSize = RPMSG_DATA_SIZE;
        cntrlParam.stackBuffer = &gEthAppCtrlTaskBuf[0];
        cntrlParam.stackSize = IPC_TASK_STACKSIZE;
        status = RPMessage_init(&cntrlParam);
    }

    /* Initialize the Remote Config server (CPSW Proxy Server) */
    status = EthFw_initRemoteConfig(gEthAppObj.hEthFw);
    if (status != ENET_SOK)
    {
        appLogPrintf("EthApp_initIpcTask: failed to init EthFw remote config: %d\n", status);
    }

    /* Wait for Linux VDev ready... */
    if (status == ENET_SOK)
    {
        while (!Ipc_isRemoteReady(IPC_MPU1_0))
        {
            Task_sleep(10);
        }
    }

    /* Create the VRing now ... */
    if (status == ENET_SOK)
    {
        status = Ipc_lateVirtioCreate(IPC_MPU1_0);
        if (status != IPC_SOK)
        {
            appLogPrintf("EthApp_initIpcTask: Ipc_lateVirtioCreate failed: %d\n", status);
        }
    }

    /* Late init */
    if (status == IPC_SOK)
    {
        status = RPMessage_lateInit(IPC_MPU1_0);
        if (status != IPC_SOK)
        {
            appLogPrintf("EthApp_initIpcTask: RPMessage_lateInit failed: %d\n", status);
        }
    }

    /* Late announcement of server's endpoint to MPU */
    if (status == IPC_SOK)
    {
        status = EthFw_lateAnnounce(gEthAppObj.hEthFw, IPC_MPU1_0);
        if (status != ENET_SOK)
        {
            appLogPrintf("EthApp_initIpcTask: late announcement failed: %d\n", status);
        }
    }

    /* Init EthFw services: task/CPU statistics and Ethernet statistics */
    if (status == IPC_SOK)
    {
        status = EthApp_initRemoteServices();
        if (status != ENET_SOK)
        {
            appLogPrintf("EthApp_initIpcTask: failed to init EthFw remote services: %d\n", status);
        }
    }

    /* Trace buffer */
    gEthAppObj.traceBufAddr = Ipc_getResourceTraceBufPtr();
    gEthAppObj.traceBufSize = IPC_TRACEBUF_SIZE;
    gEthAppObj.traceBufLastFlushTicks = 0ULL;
}

static int32_t EthApp_initEthFw(void)
{
    EthFw_Version ver;
    EthFw_Config ethFwCfg;
    EnetUdma_Cfg dmaCfg;
    int32_t status = ETHAPP_OK;

    /* Set EthFw config params */
    EthFw_initConfigParams(gEthAppObj.enetType, &ethFwCfg);
    dmaCfg.hUdmaDrv                 = gEthAppObj.hUdmaDrv;
    dmaCfg.rxChInitPrms.dmaPriority = UDMA_DEFAULT_RX_CH_DMA_PRIORITY;
    ethFwCfg.cpswCfg.dmaCfg         = (void *)&dmaCfg;
    ethFwCfg.ports                  = &gEthAppPorts[0];
    ethFwCfg.numPorts = ARRAY_SIZE(gEthAppPorts);

    uint32_t i;
    /* Overwrite config params with those for hardware interVLAN */
    EthHwInterVlan_setOpenPrms(&ethFwCfg.cpswCfg);

    for (i = 0U; i < ethFwCfg.numPorts; i++)
    {
        EthHwInterVlan_setVlanConfig(&ethFwCfg.ports[i].vlanCfg,
                                     ethFwCfg.ports[i].portNum);
    }

    /* Initialize the EthFw */
    gEthAppObj.hEthFw = EthFw_init(gEthAppObj.enetType, &ethFwCfg);
    if (gEthAppObj.hEthFw == NULL)
    {
        appLogPrintf("ETHFW: failed to initialize the firmware\n");
        status = ETHAPP_ERROR;
    }

    /* Get and print EthFw version */
    if (status == ETHAPP_OK)
    {
        EthFw_getVersion(gEthAppObj.hEthFw, &ver);
        appLogPrintf("\nETHFW Version   : %d.%02d.%02d\n", ver.major, ver.minor, ver.rev);
        appLogPrintf("ETHFW Build Date: %s %s, %s\n", ver.month, ver.date, ver.year);
        appLogPrintf("ETHFW Build Time: %s:%s:%s\n", ver.hour, ver.min, ver.sec);
        appLogPrintf("ETHFW Commit SHA: %s\n\n", ver.commitHash);
    }

    /* Post semaphore so that NDK/NIMU can continue with their initialization */
    Semaphore_post(gEthAppObj.hInitSem);

    return status;
}

static int32_t EthApp_initRemoteServices(void)
{
    int32_t status;
    app_remote_service_init_prms_t remoteServicePrms;

    appRemoteServiceInitSetDefault(&remoteServicePrms);
    status = appRemoteServiceInit(&remoteServicePrms);
    if (status != ENET_SOK)
    {
        appLogPrintf("Remote service init failed: %d !!!\n", status);
    }

    if (status == ENET_SOK)
    {
        status = appPerfStatsInit();
        if (status != ENET_SOK)
        {
            appLogPrintf("Perf stats init failed: %d !!!\n", status);
        }
    }

    if (status == ENET_SOK)
    {
        status = appPerfStatsRemoteServiceInit();
        if (status != ENET_SOK)
        {
            appLogPrintf("Perf stats remote service init failed: %d !!!\n", status);
        }
    }

    if (status == ENET_SOK)
    {
        status = appEthfwStatsInit(gEthAppObj.enetType, gEthAppObj.instId);
        if (status != ENET_SOK)
        {
            appLogPrintf("Ethfw stats init failed: %d !!!\n", status);
        }
    }

    if (status == ENET_SOK)
    {
        status = appEthfwStatsRemoteServiceInit();
        if (status != ENET_SOK)
        {
            appLogPrintf("Ethfw stats remote service init failed: %d !!!\n", status);
        }
    }

    return status;
}

/* NIMU callbacks (exact name required) */

bool EthFwCallbacks_isPortLinked(Enet_Handle hEnet)
{
    bool linked = false;
    uint32_t i;

    /* Report port linked as long as any port owned by EthFw is up */
    for (i = 0U; (i < ARRAY_SIZE(gEthAppPorts)) && !linked; i++)
    {
        linked = EnetAppUtils_isPortLinkUp(hEnet,
                                           gEthAppObj.coreId,
                                           gEthAppPorts[i].portNum);
    }

    return linked;
}

void NimuEnetAppCb_getHandle(NimuEnetAppIf_GetHandleInArgs *inArgs,
                             NimuEnetAppIf_GetHandleOutArgs *outArgs)
{
    /* Wait for EthFw to be initialized */
    Semaphore_pend(gEthAppObj.hInitSem, BIOS_WAIT_FOREVER);

    EthFwCallbacks_nimuCpswGetHandle(inArgs, outArgs);

    /* Save host port MAC address */
    memcpy(&gEthAppObj.hostMacAddr[0U],
           &outArgs->rxInfo.macAddr[0U],
           ENET_MAC_ADDR_LEN);
}

void NimuEnetAppCb_releaseHandle(NimuEnetAppIf_ReleaseHandleInfo *releaseInfo)
{
    EthFwCallbacks_nimuCpswReleaseHandle(releaseInfo);
}

/* This generated function must be called after the network stack(s) are
 * initialized.
 */
int32_t ti_net_SlNet_initConfig()
{
    int32_t status;

    status = SlNetIf_init(0);

    if (status == ENET_SOK)
    {
        status = SlNetSock_init(0);
    }

    if (status == ENET_SOK)
    {
        SlNetUtil_init(0);
    }

    /* add CONFIG_SLNET_0 interface */
    if (status == ENET_SOK)
    {
        status = SlNetIf_add(SLNETIF_ID_2,
                             "eth0",
                             (const SlNetIf_Config_t *)&SlNetIfConfigNDK,
                             5);
    }

    return status;
}

/* NDK hooks */

void EthApp_ipAddrHookFxn(uint32_t IPAddr,
                          uint32_t IfIdx,
                          uint32_t fAdd)
{
    volatile uint32_t ipAddrHex = 0U;
    Enet_MacPort macPort = ENET_MAC_PORT_1;
    int32_t status;

    /* Use default/generic hook function */
    EthFwCallbacks_ipAddrHookFxn(IPAddr, IfIdx, fAdd);

    /* initialize SlNet interface(s) */
    status = ti_net_SlNet_initConfig();
    if (status < ENET_SOK)
    {
        appLogPrintf("Failed to initialize SlNet interface(s) - status (%d)\n", status);
    }

    /* Save host port IP address */
    ipAddrHex = ntohl(IPAddr);
    gEthAppObj.hostIpAddr = ipAddrHex;

    /* MAC port used for PTP */
#if defined(SOC_J721E)
    macPort = ENET_MAC_PORT_3;
#elif defined(SOC_J7200)
#if defined(ENABLE_QSGMII_PORTS)
    macPort = ENET_MAC_PORT_1;
#else
    macPort = ENET_MAC_PORT_2;
#endif
#endif

    /* Initialize and enable PTP stack */
    EthFw_initTimeSyncPtp(gEthAppObj.hostIpAddr,
                          &gEthAppObj.hostMacAddr[0U],
                          ENET_BIT(ENET_MACPORT_NORM(macPort)));

    /* Assign functions that are to be called based on actions in GUI.
     * These cannot be dynamically pushed to function pointer array, as the
     * index is used in GUI as command */
    EnetCfgServer_fxn_table[9] = &EthApp_startSwInterVlan;
    EnetCfgServer_fxn_table[10] = &EthApp_startHwInterVlan;

    /* Start Configuration server */
    status = EnetCfgServer_init(gEthAppObj.enetType);
    EnetAppUtils_assert(ENET_SOK == status);

    /* Start the software-based interVLAN routing */
    EthSwInterVlan_setupRouting(gEthAppObj.enetType,
                                ETH_SWINTERVLAN_TASK_PRI);

    AddWebFiles();
}

/* Functions called from Config server library based on selection from GUI */

static void EthApp_startSwInterVlan(char *recvBuff,
                                    char *sendBuff)
{
    EnetCfgServer_InterVlanConfig *pInterVlanCfg;
    int32_t status;

    if (recvBuff != NULL)
    {
        pInterVlanCfg = (EnetCfgServer_InterVlanConfig *)recvBuff;
        status = EthSwInterVlan_addClassifierEntries(pInterVlanCfg);
        EnetAppUtils_assert(ENET_SOK == status);
    }
}

static void EthApp_startHwInterVlan(char *recvBuff,
                                    char *sendBuff)
{
    EnetCfgServer_InterVlanConfig *pInterVlanCfg;

    if (recvBuff != NULL)
    {
        pInterVlanCfg = (EnetCfgServer_InterVlanConfig *)recvBuff;
        EthHwInterVlan_setupRouting(gEthAppObj.enetType, pInterVlanCfg);
    }
}

/* IPC trace buffer flush */

void EthApp_traceBufCacheWb(void)
{
    Types_Timestamp64 ts;
    uint64_t newticks;

    Timestamp_get64(&ts);
    newticks = ((uint64_t)ts.hi << 32U) | ts.lo;

    /* Don't keep flusing cache */
    if ((newticks - gEthAppObj.traceBufLastFlushTicks) >=
        (uint64_t)ETHAPP_TRACEBUF_FLUSH_PERIOD)
    {
        gEthAppObj.traceBufLastFlushTicks = newticks;

        /* Flush the cache of the SysMin buffer only */
        if (gEthAppObj.traceBufAddr != NULL)
        {
            CacheP_wb((const void *)gEthAppObj.traceBufAddr,
                      gEthAppObj.traceBufSize);
        }
    }
}
