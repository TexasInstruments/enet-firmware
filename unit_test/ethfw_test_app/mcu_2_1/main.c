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

/* EthFwTrace id for this module, must be unique within ETHFW */
#define ETHFWTRACE_MOD_ID 0x801
#define ETHFWTRACE_MOD_NAME "Client-Test-App"

#include <stdio.h>
#include <stdint.h>

#if defined(__KLOCWORK__)
#include <stdlib.h>
#endif

/* OSAL */
#include <ti/osal/osal.h>
#include <ti/osal/SemaphoreP.h>
#include <ti/osal/TaskP.h>
#include <ti/osal/MailboxP.h>

#include <ti/csl/cslr_gtc.h>

#include <ethremotecfg/client/include/cpsw_proxy.h>
#include <utils/ethfw_common/include/ethfw_trace.h>

#include <apps/ipc_cfg/app_ipc_rsctable.h>
#include <ti/drv/ipc/ipc.h>
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/enet/include/dma/udma/enet_udma.h>
#include <ti/drv/enet/include/core/enet_dma.h>

#include <ti/drv/enet/examples/utils/include/enet_ethutils.h>
#include <ti/drv/enet/examples/utils/include/enet_appmemutils_cfg.h>
#include <ti/drv/enet/examples/utils/include/enet_appmemutils.h>

#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>

#include <unity.h>
#include <ethfw_test_cases.h>

#include <ti/drv/enet/examples/utils/include/enet_apputils.h>

#define System_printf printf
#define System_vprintf vprintf

#if defined(ENABLE_MAC_ONLY_PORTS)
#define CPSW_REMOTE_APP_REMOTE_NETIF_MAX      (2U)
#else
#define CPSW_REMOTE_APP_REMOTE_NETIF_MAX      (1U)
#endif

#define CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US (1000U)
#define CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL   (30U)
#define CPSW_REMOTE_APP_CPTS_HW_PUSH_NUM      (2U)

#define VQ_TIMEOUT              (100)
#define VQ_BUF_SIZE             (2048)

#if defined(SOC_J721E)
#define IPC_VRING_MEM_SIZE                    (32U * 1024U * 1024U)
#elif defined(SOC_J7200)
#define IPC_VRING_MEM_SIZE                    (8U * 1024U * 1024U)
#elif defined(SOC_J784S4)
#define IPC_VRING_MEM_SIZE                    (48U * 1024U * 1024U)
#else
#error "Unsupported device"
#endif

#define CPSW_REMOTE_APP_IPC_RPC_MSG_SIZE      (496U + 32U)
#define CPSW_REMOTE_APP_IPC_NUM_RPMSG_BUFS    (256U)
#define CPSW_REMOTE_APP_IPC_RPMSG_OBJ_SIZE    (256U)
#define CPSW_REMOTE_APP_IPC_DATA_SIZE         (CPSW_REMOTE_APP_IPC_RPC_MSG_SIZE * \
                                               CPSW_REMOTE_APP_IPC_NUM_RPMSG_BUFS + \
                                               CPSW_REMOTE_APP_IPC_RPMSG_OBJ_SIZE)

#define ETHAPP_IPC_TASK_STACKALIGN              (8192U)

static uint8_t g_initTaskStackBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(ETHAPP_IPC_TASK_STACKALIGN)))
;

static uint8_t g_vdevMonStackBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(ETHAPP_IPC_TASK_STACKALIGN)))
;

static uint8_t ctrlTaskBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(ETHAPP_IPC_TASK_STACKALIGN)))
;

static uint8_t sysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section("ipc_data_buffer"), aligned(8)));
static uint8_t gCntrlBuf[CPSW_REMOTE_APP_IPC_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned(8)));

static uint8_t g_vringMemBuf[IPC_VRING_MEM_SIZE] __attribute__ ((section(".bss:ipc_vring_mem"), aligned(8192)));

static uint32_t selfProcId = IPC_MCU2_1;
static uint32_t gRemoteProc[] =
{
#if defined(SOC_J721E)
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0,
    IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2,
    IPC_C7X_1,
#elif defined(SOC_J7200)
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0,
#elif defined(SOC_J784S4)
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0,
    IPC_MCU3_0, IPC_MCU3_1, IPC_MCU4_0, IPC_MCU4_1,
    IPC_C7X_1,  IPC_C7X_2,  IPC_C7X_3,  IPC_C7X_4,
#endif
};
static uint32_t gNumRemoteProc = sizeof(gRemoteProc) / sizeof(uint32_t);



typedef struct CpswRemoteApp_Obj_s
{
    /* UDMA LLD object */
    struct Udma_DrvObj udmaDrvObj;

    /* UDMA LLD handle */
    Udma_DrvHandle hUdmaDrv;

    /* Enet peripheral type */
    Enet_Type enetType;

    /* Enet peripheral instance id */
    uint32_t instId;

    /* Enet LLD DMA handle */
    EnetDma_Handle hEnetDma;

    /* Core id used for Enet LLD APIs */
    uint32_t coreId;

    /* Whether to use default flow or not */
    bool useDefaultRxFlow;

    /* Whether to use extended attach remote command or not */
    bool useExtAttach;
} CpswRemoteApp_Obj;

void appLogPrintf(const char *format, ...);

/* Trace configuration */
static EthFwTrace_Cfg gRemoteApp_traceCfg =
{
    .print        = appLogPrintf,
    .traceTsFunc  = NULL,
    .extTraceFunc = NULL,
};

/* Link status on these ports will be used to determine link up on virtual switch port */
static Enet_MacPort gRemoteAppMacPorts[] =
{
    ENET_MAC_PORT_3,
};

CpswRemoteApp_Obj gRemoteAppObj =
{
    .hUdmaDrv         = NULL,
#if defined(SOC_J721E) || defined(SOC_J784S4)
    .enetType         = ENET_CPSW_9G,
    .instId           = 0U,
#elif defined(SOC_J7200)
    .enetType         = ENET_CPSW_5G,
    .instId           = 0U,
#endif
    .hEnetDma         = NULL,
    .useDefaultRxFlow = BFALSE,
    .useExtAttach     = BFALSE,
};

static void EthApp_waitForDebugger(void);

static uint64_t CpswRemoteApp_virtToPhysFxn(const void *virtAddr,
                                            void *appData);

static void *CpswRemoteApp_physToVirtFxn(uint64_t phyAddr,
                                         void *appData);

static int32_t CpswRemoteApp_openUdma(void);

static int32_t CpswRemoteApp_openEnet(void);

// hack for release mode build fix TODO fix this
void localAssert(bool cond)
{
#if defined(__KLOCWORK__)
    if (!cond)
    {
        abort();
    }
#endif
}

void appLogPrintf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    System_vprintf(format, args);
    va_end(args);
}

static void CpswRemoteApp_ipcPrint(const char *str)
{
    appLogPrintf("%s", str);
    return;
}

void setUp(void)
{

}

void tearDown(void)
{

}

static void CpswRemoteTestApp_initTask(void* a0,
                                       void* a1)
{
    TaskP_Params params;
    uint32_t numProc = gNumRemoteProc;
    Ipc_VirtIoParams vqParam;
    Ipc_InitPrms initPrms;
    RPMessage_Params cntrlParam;
    MailboxP_Params mbxParams;
    CpswProxy_Config proxyConfig;
    CpswProxy_Handle hProxy;
    int32_t status;


    uint32_t i;

    UART_stdioInit(0U);

    /* Initialize ETHFW Trace with INFO log level and higher */
    EthFwTrace_init(&gRemoteApp_traceCfg);
    EthFwTrace_setLevel(ETHFW_TRACE_INFO);

    /* Step1 : Initialize the multiproc */
    status = Ipc_mpSetConfig(selfProcId, numProc, &gRemoteProc[0]);

    /* Initialize params with defaults */
    IpcInitPrms_init(0U, &initPrms);

    initPrms.printFxn = &CpswRemoteApp_ipcPrint;

    status += Ipc_init(&initPrms);
#if !defined(A72_QNX_OS)
    if (status == ENET_SOK)
    {
        Ipc_loadResourceTable(appGetIpcResourceTable());
    }
#else
    ETHFWTRACE_INFO("Skipping Ipc_loadResourceTable for QNX (core : %s)", Ipc_mpGetSelfName());
#endif

    if (status == ENET_SOK)
    {
        /* Step2 : Initialize Virtio */
        vqParam.vqObjBaseAddr = (void *)&sysVqBuf[0];
        vqParam.vqBufSize = numProc * Ipc_getVqObjMemoryRequiredPerCore();
        vqParam.vringBaseAddr = (void *)g_vringMemBuf;
        vqParam.vringBufSize = sizeof(g_vringMemBuf);
        vqParam.timeoutCnt = VQ_TIMEOUT;     /* Wait for counts */
        status = Ipc_initVirtIO(&vqParam);
    }

    if (status == ENET_SOK)
    {
        /* Step 3: Initialize RPMessage */
        /* Initialize the param */
        status = RPMessageParams_init(&cntrlParam);
    }

    if (status == ENET_SOK)
    {
        /* Set memory for HeapMemory for control task */
        cntrlParam.buf = &gCntrlBuf[0];
        cntrlParam.bufSize = CPSW_REMOTE_APP_IPC_DATA_SIZE;
        cntrlParam.stackBuffer = &ctrlTaskBuf[0];
        cntrlParam.stackSize = sizeof(ctrlTaskBuf);
        status = RPMessage_init(&cntrlParam);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "ETHFW RPMessage_init failed");
    }


    /* Step 5: Start Cpsw Proxy */
    CpswProxy_init();

    /* Step 5a. Wait for remote_device to be initialized on the server side */
    do
    {
        status = CpswProxy_connect();
    }
    while (status != IPC_SOK);
    
    proxyConfig.virtPort = ETHREMOTECFG_SWITCH_PORT_1;

    hProxy = CpswProxy_open(&proxyConfig);
    localAssert(hProxy != NULL);

    setUp();

    EthFwUT_testConnection((void *)hProxy);

    tearDown();

    // UNITY_BEGIN();

    // RUN_TEST(EthFwUT_attachCmdPosTest,  0, (void *)hProxy);

    // RUN_TEST(EthFwUT_attachCmdNegTest,  0, (void *)hProxy);

    // UNITY_END();

}

int main(void)
{
    TaskP_Handle task;
    TaskP_Params taskParams;
    int32_t status;

    OS_init();

    /* Wait for debugger to attach (disabled by default) */
    EthApp_waitForDebugger();

    /* Init UDMA LLD based on NAVSS instance */
    status = CpswRemoteApp_openUdma();
    if (status != UDMA_SOK)
    {
        ETHFWTRACE_ERR(status, "Failed to open UDMA LLD");
        localAssert(status == UDMA_SOK);
    }

    /* Init Enet LLD and open Enet DMA */
    status = CpswRemoteApp_openEnet();
    if (status != ENET_SOK)
    {
        ETHFWTRACE_ERR(status, "Failed to open Enet LLD");
        localAssert(status == ENET_SOK);
    }

    TaskP_Params_init(&taskParams);
    taskParams.priority = 2;
    taskParams.stack = &g_initTaskStackBuf[0];
    taskParams.stacksize = sizeof(g_initTaskStackBuf);

    task = TaskP_create(&CpswRemoteTestApp_initTask, &taskParams);

    if (NULL == task)
    {
        OS_stop();
    }

    OS_start();    /* does not return */

    return(0);
}

static void EthApp_waitForDebugger(void)
{
    /* Set ccsHaltFlag to 1 for halting core for CCS connection */
    volatile uint32_t ccsHaltFlag = 0U;

    while (ccsHaltFlag);
}

static int32_t CpswRemoteApp_openUdma(void)
{
    Udma_InitPrms initPrms;
    Udma_DrvHandle hUdmaDrv = &gRemoteAppObj.udmaDrvObj;
    uint32_t instId = UDMA_INST_ID_MAIN_0;
    int32_t status;

    memset(hUdmaDrv, 0, sizeof(*hUdmaDrv));

    /* Initialize the UDMA driver based on NAVSS instance */
    UdmaInitPrms_init(instId, &initPrms);
    initPrms.printFxn = (Udma_PrintFxn) &System_printf;

    status = Udma_init(hUdmaDrv, &initPrms);
    if (status != UDMA_SOK)
    {
        ETHFWTRACE_ERR(status, "Failed init UDMA driver");
        hUdmaDrv = NULL;
    }

    gRemoteAppObj.hUdmaDrv = hUdmaDrv;

    return status;
}

static uint64_t CpswRemoteApp_virtToPhysFxn(const void *virtAddr,
                                            void *appData)
{
    return ((uint64_t)virtAddr);
}

static void *CpswRemoteApp_physToVirtFxn(uint64_t phyAddr,
                                         void *appData)
{
#if defined(__aarch64__)
    uint64_t temp = phyAddr;
#else
    /* R5 is 32-bit machine, need to truncate to avoid void * typecast error */
    uint32_t temp = (uint32_t)phyAddr;
#endif

    return ((void *)temp);
}

static int32_t CpswRemoteApp_openEnet(void)
{
    EnetOsal_Cfg osalPrms;
    EnetUtils_Cfg utilsPrms;
    EnetDma_initCfg dmaCfg;
    EnetDma_Handle hEnetDma;
    int32_t status = ENET_SOK;

    /* Init Enet LLD (OSAL, utils) */
    utilsPrms.print      = (Enet_Print)System_printf;
    utilsPrms.physToVirt = &CpswRemoteApp_physToVirtFxn;
    utilsPrms.virtToPhys = &CpswRemoteApp_virtToPhysFxn;
    Enet_initOsalCfg(&osalPrms);
    Enet_init(&osalPrms, &utilsPrms);

    /* Initialize Enet memutils */
    status = EnetMem_init();
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to init memutils");

    /* Initialize data path of Enet LLD */
    if (status == ENET_SOK)
    {
        EnetUdma_initDataPathParams(&dmaCfg);
        dmaCfg.hUdmaDrv = gRemoteAppObj.hUdmaDrv;

        hEnetDma = EnetUdma_initDataPath(gRemoteAppObj.enetType,
                                         gRemoteAppObj.instId,
                                         &dmaCfg);
        if (hEnetDma == NULL)
        {
            status = ENET_EFAIL;
            ETHFWTRACE_ERR(status, "Failed to init Enet LLD data path");
            EnetMem_deInit();
        }
        else
        {
            gRemoteAppObj.hEnetDma = hEnetDma;
            gRemoteAppObj.coreId   = EnetSoc_getCoreId();
        }
    }

    return status;
}




