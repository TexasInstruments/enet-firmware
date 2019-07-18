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

#include <apps/ipc_cfg/app_ipc_rsctable.h>

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
#include <ti/drv/cpsw/nimucpsw/nimu_ndk.h>
#include <ti/drv/cpsw/nimucpsw/ndk2cpsw_appif.h>

/* NDK headers */
#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/ndk/inc/socket.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/tools/servers.h>
#include <ti/ndk/inc/tools/console.h>

#include "webpage.h"


#define IPC_RPMESSAGE_OBJ_SIZE  256
#define VQ_BUF_SIZE             2048
#define REMOTE_DEVICE_ENDPT     26
#define RPMSG_DATA_SIZE         (256*512 + IPC_RPMESSAGE_OBJ_SIZE)
#define VRING_BASE_ADDRESS      0xBA000000
#define VRING_BUFFER_SIZE       0x02000000

static uint8_t g_monitorStackBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));
static uint8_t g_rdevStackBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));
static uint8_t g_ipcStackBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));
static uint8_t g_vdevMonStackBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));
static uint8_t g_mainStackBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));
static uint8_t ctrlTaskBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(8192)));

static uint8_t  sysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section ("ipc_data_buffer"), aligned (8)));
static uint8_t  gCntrlBuf[RPMSG_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned (8)));

static SemaphoreP_Handle g_rdev_init_wait_sem;
static SemaphoreP_Handle g_ipc_init_wait_sem;
static SemaphoreP_Handle g_rdev_start_sem;

static uint32_t selfProcId = IPC_MCU2_0;
static uint32_t gRemoteProc[] =
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
};
static uint32_t gNumRemoteProc = sizeof(gRemoteProc)/sizeof(uint32_t);
static rdevEthSwitchServerCbFxn_t appRdevEthSwitchServerCbFxnTbl;

/* Test application stack size */
#define APP_TSK_STACK_MAIN              (10U * 1024U)

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
    CpswMcm_Handle hMcm[CPSW_COUNT];

    /* UDMA driver handle */
    Udma_DrvHandle hUdmaDrv;

    /* Host port MAC address */
    uint8_t hostMacAddr[ETH_MAC_ADDR_LEN];
} CpswMain_AppObj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static void CpswApp_setAleConfig(CpswAle_Config *aleConfig);
static void CpswApp_initLinkArgs(Cpsw_OpenPortLinkInArgs *linkArgs,
                                 Cpsw_MacPort macPort);
static int32_t CpswApp_init(Cpsw_Type cpswType);
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
    .masterPort = CPSW_MAC_PORT_1,
#endif
    .macPorts = gCpswMainAppMacPorts,
    .numMacPorts = CPSWAPPUTILS_ARRAY_SIZE(gCpswMainAppMacPorts),
};


void appLogPrintf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    System_vprintf(format, args);
    CpswAppUtils_print(format, args);
    va_end(args);
}

static void rpmsg_vdevMonitorFxn(UArg arg0, UArg arg1)
{
    int32_t status;

    /* Wait for Linux VDev ready... */
    while(!Ipc_isRemoteReady(IPC_MPU1_0))
    {
        Task_sleep(10);
    }

    /* Create the VRing now ... */
    status = Ipc_lateVirtioCreate(IPC_MPU1_0);
    if(status != IPC_SOK)
    {
        CpswAppUtils_print("%s: Ipc_lateVirtioCreate failed\n", __func__);
        return;
    }

    status = RPMessage_lateInit(IPC_MPU1_0);
    if(status != IPC_SOK)
    {
        CpswAppUtils_print("%s: RPMessage_lateInit failed\n", __func__);
        return;
    }

    status = appRemoteDeviceLateAnnounce(IPC_MPU1_0);
    if(status != IPC_SOK)
    {
        CpswAppUtils_print("%s: RPMessage_announce() failed\n", __func__);
    }
}

static Void monitorAndUnlockRdev(UArg a0, UArg a1)
{
    SemaphoreP_pend(g_ipc_init_wait_sem, SemaphoreP_WAIT_FOREVER);
    SemaphoreP_pend(g_rdev_init_wait_sem, SemaphoreP_WAIT_FOREVER);
    SemaphoreP_post(g_rdev_start_sem);

}

static Void ipc_init(UArg a0, UArg a1)
{
    Task_Params       params;
    uint32_t          numProc = gNumRemoteProc;
    Ipc_VirtIoParams  vqParam;

    /* Step1 : Initialize the multiproc */
    Ipc_mpSetConfig(selfProcId, numProc, &gRemoteProc[0]);

    CpswAppUtils_print("IPC_echo_test (core : %s) .....\r\n",
            Ipc_mpGetSelfName());


    Ipc_loadResourceTable(appGetIpcResourceTable());

    /* Step2 : Initialize Virtio */
    vqParam.vqObjBaseAddr = (void*)&sysVqBuf[0];
    vqParam.vqBufSize     = numProc * Ipc_getVqObjMemoryRequiredPerCore();
    vqParam.vringBaseAddr = (void*)VRING_BASE_ADDRESS;
    vqParam.vringBufSize  = VRING_BUFFER_SIZE;
    vqParam.timeoutCnt    = 100;  /* Wait for counts */
    Ipc_initVirtIO(&vqParam);

    /* Step 3: Initialize RPMessage */
    RPMessage_Params cntrlParam;

    /* Initialize the param */
    RPMessageParams_init(&cntrlParam);

    /* Set memory for HeapMemory for control task */
    cntrlParam.buf         = &gCntrlBuf[0];
    cntrlParam.bufSize     = RPMSG_DATA_SIZE;
    cntrlParam.stackBuffer = &ctrlTaskBuf[0];
    cntrlParam.stackSize   = IPC_TASK_STACKSIZE;
    RPMessage_init(&cntrlParam);

    SemaphoreP_post(g_ipc_init_wait_sem);

    Task_Params_init(&params);
    params.priority = 3;
    params.stackSize = IPC_TASK_STACKSIZE;
    params.stack = &g_vdevMonStackBuf[0];
    params.stackSize = IPC_TASK_STACKSIZE;
    Task_create(rpmsg_vdevMonitorFxn, &params, NULL);
}




static Void remotedev_init(UArg a0, UArg a1)
{
    app_remote_device_init_prm_t remote_dev_init_prm;
    rdevEthSwitchServerInitPrm_t remote_ethswitch_init_prm;
    rdevEthSwitchServerInstPrm_t *inst;

    appRemoteDeviceInitParamsInit(&remote_dev_init_prm);

    remote_dev_init_prm.rpmsg_buf_size = 256;
    remote_dev_init_prm.remote_device_endpt = REMOTE_DEVICE_ENDPT;
    remote_dev_init_prm.wait_sem = g_rdev_start_sem;

    appRemoteDeviceInit(&remote_dev_init_prm);
    CpswAppUtils_print("Remote device (core : mcu2_1) .....\r\n");

    rdevEthSwitchServerInitPrmSetDefault(&remote_ethswitch_init_prm);

    remote_ethswitch_init_prm.rpmsg_buf_size = 256;
    remote_ethswitch_init_prm.num_instances = 2;
    remote_ethswitch_init_prm.cb = appRdevEthSwitchServerCbFxnTbl;

    inst = &remote_ethswitch_init_prm.inst_prm[0];
    inst->host_id = IPC_MCU2_1;
    {
        snprintf((char *)&inst->name[0], ETHREMOTECFG_SERVER_MAX_NAME_LEN, ETHREMOTEDEVICE_DEVICE_NAME_MCU_2_1);
        snprintf((char *)&inst->data[0], ETHREMOTECFG_SERVER_MAX_DATA_LEN, ETHREMOTEDEVICE_DEVICE_DATA_MCU_2_1);
    }

    inst = &remote_ethswitch_init_prm.inst_prm[1];
    inst->host_id = IPC_MPU1_0;
    {
        snprintf((char *)&inst->name[0], ETHREMOTECFG_SERVER_MAX_NAME_LEN, ETHREMOTEDEVICE_DEVICE_NAME_MPU_1_0);
        snprintf((char *)&inst->data[0], ETHREMOTECFG_SERVER_MAX_DATA_LEN, ETHREMOTEDEVICE_DEVICE_DATA_MPU_1_0);
    }

    rdevEthSwitchServerInit(&remote_ethswitch_init_prm);
    CpswAppUtils_print("Remote demo device (core : mcu2_0) .....\r\n");

    SemaphoreP_post(g_rdev_init_wait_sem);
}

static Void taskFxn(UArg a0, UArg a1)
{

    Task_Params ipc_taskParams;
    Task_Params rdev_taskParams;
    Task_Params monitor_taskParams;
    SemaphoreP_Params sem_params;

    /* Set ccsHaltFlag to 1 for halting core for CCS connection */
    volatile uint32_t ccsHaltFlag = 0U;
    while(ccsHaltFlag);

    CpswAppUtils_print("=======================================================\n");
    CpswAppUtils_print ("           CPSW L2 Switching APP          \n");
    CpswAppUtils_print("=======================================================\n");

    SemaphoreP_Params_init(&sem_params);
    sem_params.mode = SemaphoreP_Mode_BINARY;
    g_ipc_init_wait_sem = SemaphoreP_create(0, &sem_params);

    SemaphoreP_Params_init(&sem_params);
    sem_params.mode = SemaphoreP_Mode_BINARY;
    g_rdev_init_wait_sem = SemaphoreP_create(0, &sem_params);

    SemaphoreP_Params_init(&sem_params);
    sem_params.mode = SemaphoreP_Mode_BINARY;
    g_rdev_start_sem = SemaphoreP_create(0, &sem_params);

    Task_Params_init(&ipc_taskParams);
    ipc_taskParams.priority = 2;
    ipc_taskParams.stack = &g_ipcStackBuf[0];
    ipc_taskParams.stackSize = IPC_TASK_STACKSIZE;
    Task_create(ipc_init, &ipc_taskParams, NULL);

    Task_Params_init(&rdev_taskParams);
    rdev_taskParams.priority = 2;
    rdev_taskParams.stack = &g_rdevStackBuf[0];
    rdev_taskParams.stackSize = IPC_TASK_STACKSIZE;
    Task_create(remotedev_init, &rdev_taskParams, NULL);

    Task_Params_init(&monitor_taskParams);
    monitor_taskParams.priority = 2;
    monitor_taskParams.stack = &g_monitorStackBuf[0];
    monitor_taskParams.stackSize = IPC_TASK_STACKSIZE;
    Task_create(monitorAndUnlockRdev, &monitor_taskParams, NULL);
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

static int32_t CpswApp_init(Cpsw_Type cpswType)
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
    cpswCfg.resourcePartitionConfig.isDefaultRmPartition = true;
    cpswCfg.resourcePartitionConfig.rmPartitionPrms      = NULL;

    CpswApp_setAleConfig(&cpswCfg.aleConfig);

    cpswCfg.dmaConfig.rxChInitPrms.dmaPriority = UDMA_DEFAULT_RX_CH_DMA_PRIORITY;

    /* Open UDMA */
    gCpswMainAppObj.hUdmaDrv = CpswAppUtils_udmaOpen(cpswType);
    cpswCfg.dmaConfig.hUdmaDrv = gCpswMainAppObj.hUdmaDrv;

    cpswMcmCfg.pCpswCfg     = &cpswCfg;
    cpswMcmCfg.cpswType     = cpswType;
    cpswMcmCfg.setMacConfig = CpswApp_initLinkArgs;
    cpswMcmCfg.numMacPorts  = gCpswMainAppObj.numMacPorts;
    cpswMcmCfg.periodicTaskPeriod = CPSW_PHY_FSM_TICK_PERIOD_MS; /* msecs */

    memcpy(&cpswMcmCfg.macPortList[0U],
           gCpswMainAppObj.macPorts,
           gCpswMainAppObj.numMacPorts);

    /* First MAC port in the array gives the host address */
    status = CpswSoc_getMacAddr(cpswType,
                       gCpswMainAppObj.macPorts[0],
                       &cpswCfg.aleConfig.macAddr[0]);

    memcpy(&gCpswMainAppObj.hostMacAddr[0U],
           &cpswCfg.aleConfig.macAddr[0],
           ETH_MAC_ADDR_LEN);

    CpswAppUtils_print("Host MAC address: ");
    CpswAppUtils_printMacAddr(&gCpswMainAppObj.hostMacAddr[0U]);

    gCpswMainAppObj.hMcm[cpswType] = CpswMcm_init(&cpswMcmCfg);
    CpswAppUtils_assert (NULL != gCpswMainAppObj.hMcm[cpswType]);

    return status;
}

void CpswApp_deInit(void)
{
    CpswAppUtils_udmaclose(gCpswMainAppObj.hUdmaDrv);

    memset(&gCpswMainAppObj, 0U, sizeof(CpswMain_AppObj));
}

void CpswAppIf_getHandles(CpswAppIf_HandleInfo *pAppIfHandleInfo)
{
    int32_t status;
    CpswMcm_HandleInfo handleInfo;

    if (gCpswMainAppObj.hMcm[gCpswMainAppObj.cpswType] == NULL)
    {
        status = CpswApp_init(gCpswMainAppObj.cpswType);
        pAppIfHandleInfo->isDefaultFlow = true;

        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Failed to open CPSW: %d\n", status);
            CpswAppUtils_assert(status == CPSW_SOK);
        }
    }
    else
    {
        pAppIfHandleInfo->isDefaultFlow = false;
    }

    CpswMcm_getHandle(gCpswMainAppObj.hMcm[gCpswMainAppObj.cpswType], &handleInfo);

    pAppIfHandleInfo->hCpsw         = handleInfo.hCpsw;
    pAppIfHandleInfo->hUdmaDrv      = handleInfo.hUdmaDrv;
    pAppIfHandleInfo->coreId        = handleInfo.coreId;
    pAppIfHandleInfo->coreKey       = handleInfo.coreKey;
    pAppIfHandleInfo->hostPortRxMtu = handleInfo.hostPortRxMtu;
    memcpy (&pAppIfHandleInfo->txMtu[0U], &handleInfo.txMtu[0U],
                            CPSW_UTILS_ARRAYSIZE(pAppIfHandleInfo->txMtu));

    pAppIfHandleInfo->printFxnCb    = &CpswAppUtils_print;
}


void CpswAppIf_releaseHandles(void)
{
    CpswMcm_releaseHandle(gCpswMainAppObj.hMcm[gCpswMainAppObj.cpswType]);
}

Cpsw_MacPort CpswAppIf_getMacPortNum(void)
{
    return (Cpsw_MacPort)gCpswMainAppObj.masterPort;
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
    char ipAddr[20];

    ipAddrHex = ntohl(IPAddr);
    snprintf(ipAddr, 17, "%d.%d.%d.%d\n",
             (uint8_t)(ipAddrHex>>24)&0xFF,
             (uint8_t)(ipAddrHex>>16)&0xFF,
             (uint8_t)(ipAddrHex>>8)&0xFF,
             (uint8_t)ipAddrHex&0xFF);

    CpswAppUtils_print("\nCPSW NIMU application, IP address I/F 1: %s\n\r", ipAddr);

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


int main(void)
{
    Task_Handle task;
    Task_Params taskParams;


    CpswAppBoardUtils_init();

    CpswAppUtils_enableClocks(gCpswMainAppObj.cpswType,
                              MAC_CONN_TYPE_RGMII_FORCE_1000_FULL);
    /* Initialize the task params */
    Task_Params_init(&taskParams);
    /* Set the task priority higher than the default priority (1) */
    taskParams.priority = 2;
    taskParams.stack = &g_mainStackBuf[0];
    taskParams.stackSize = IPC_TASK_STACKSIZE;

    task = Task_create(taskFxn, &taskParams, NULL);
    if(NULL == task)
    {
        BIOS_exit(0);
    }
    BIOS_start();    /* does not return */

    return(0);
}


Cpsw_Type gCpswType;

static int32_t app_ethrdev_srv_cb_attach_handler (uint32_t host_id,uint8_t cpsw_type, struct rpmsg_kdrv_ethswitch_attach_response *resp)
{
    uint32_t i;
    CpswMcm_HandleInfo handleInfo;
    Cpsw_IoctlPrms        prms;
    Cpsw_AttachCoreOutArgs attachCoreOutArgs;
    int32_t status;

    if (cpsw_type == RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_2G)
    {
        gCpswType = CPSW_2G;
    }
    else
    {
        CpswAppUtils_assert(cpsw_type == RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_9G);
        gCpswType = CPSW_9G;
    }
    CpswAppUtils_print("Function:%s,HostId:%u,CpswType:%u\n",__func__,host_id, gCpswType);
    if (gCpswMainAppObj.hMcm[gCpswType] == NULL)
    {
        status = CpswApp_init(gCpswType);

        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Failed to open CPSW: %d\n", status);
            CpswAppUtils_assert(status == CPSW_SOK);
        }
    }
    CpswMcm_getHandle(gCpswMainAppObj.hMcm[gCpswType], &handleInfo);
   
    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &host_id, &attachCoreOutArgs);
    status = Cpsw_ioctl(handleInfo.hCpsw,
                        host_id,
                        CPSW_IOCTL_ATTACH_CORE,
                        &prms);

    CpswAppUtils_assert(status == CPSW_SOK);

    resp->id = (uint64_t)(handleInfo.hCpsw);
    resp->core_key = attachCoreOutArgs.coreKey;
    resp->rx_mtu = attachCoreOutArgs.rxMtu;
    CPSW_UTILS_COMPILETIME_ASSERT(CPSW_UTILS_ARRAYSIZE(resp->tx_mtu) == 
                                  CPSW_UTILS_ARRAYSIZE(attachCoreOutArgs.txMtu));
    for (i = 0; i < CPSW_UTILS_ARRAYSIZE(resp->tx_mtu) ;i++)
    {
        resp->tx_mtu[i] = attachCoreOutArgs.txMtu[i];
    }
    resp->features = 0;
    resp->info.status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    return RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
}

static int32_t app_ethrdev_srv_cb_alloc_tx_handler (uint32_t host_id,uint64_t handle,  uint32_t core_key, struct rpmsg_kdrv_ethswitch_alloc_tx_response * resp)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x\n",__func__,host_id, hCpsw, core_key);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &core_key, &resp->tx_cpsw_psil_dst_id);
    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_RM_IOCTL_ALLOC_TX_CH_PEERID,
                        &prms);

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    return status;
}

static int32_t app_ethrdev_srv_cb_alloc_rx_handler (uint32_t host_id,uint64_t handle,  uint32_t core_key, struct rpmsg_kdrv_ethswitch_alloc_rx_response * resp)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    CpswRm_AllocRxFlowOutArgs allocRxFlowOutArgs;

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x\n",__func__,host_id, hCpsw, core_key);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &core_key, &allocRxFlowOutArgs);
    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_RM_IOCTL_ALLOC_RX_FLOW,
                        &prms);

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        resp->start_idx = allocRxFlowOutArgs.startIdx;
        resp->alloc_flow_idx = allocRxFlowOutArgs.freeFlowIdx;
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }
    return status;
}

static int32_t app_ethrdev_srv_cb_alloc_rx_default_handler (uint32_t host_id,uint64_t handle,  uint32_t core_key, struct rpmsg_kdrv_ethswitch_alloc_rx_default_response * resp)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    CpswRm_AllocRxFlowOutArgs allocRxFlowOutArgs;

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x\n",__func__,host_id, hCpsw, core_key);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &core_key, &allocRxFlowOutArgs);
    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_IOCTL_ALLOC_RX_DEFAULT_FLOW,
                        &prms);

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        resp->start_idx = allocRxFlowOutArgs.startIdx;
        resp->alloc_flow_idx = allocRxFlowOutArgs.freeFlowIdx;
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }
    return status;
}

static int32_t app_ethrdev_srv_cb_alloc_mac_handler (uint32_t host_id,uint64_t handle,  uint32_t core_key,struct rpmsg_kdrv_ethswitch_alloc_mac_response * resp)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    CpswRm_AllocMacAddrOutArgs allocMacAddrOutArgs;


    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x\n",__func__,host_id, hCpsw, core_key);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &core_key, &allocMacAddrOutArgs);

    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_RM_IOCTL_ALLOC_MAC_ADDR,
                        &prms);

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        CPSW_UTILS_COMPILETIME_ASSERT(sizeof(resp->mac_address) == sizeof(allocMacAddrOutArgs.macAddr));
        memcpy(resp->mac_address, allocMacAddrOutArgs.macAddr, sizeof(resp->mac_address));
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }
    return status;
}

static int32_t app_ethrdev_srv_cb_register_mac_handler (uint32_t host_id,uint64_t handle,  uint32_t core_key, u8 *mac_address, uint32_t flow_idx)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    CpswAle_SetPolicerEntryOutArgs  setPolicerOutArgs;
    CpswAle_SetPolicerEntryInArgs   setPolicerInArgs;

    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, MacAddress:%x:%x:%x:%x:%x:%x, FlowIdx:%u\n",
                       __func__,
                       host_id, 
                       hCpsw, 
                       core_key, 
                       mac_address[0],
                       mac_address[1],
                       mac_address[2],
                       mac_address[3],
                       mac_address[4],
                       mac_address[5],
                       flow_idx);

    memset(&setPolicerInArgs,0,sizeof(setPolicerInArgs));

    setPolicerInArgs.policerMatch.policerMatchEnableMask   = CPSW_ALE_POLICER_MATCH_MACDST;
    memcpy(&setPolicerInArgs.policerMatch.dstMacAddr.addr.addr[0U], mac_address,
        sizeof (setPolicerInArgs.policerMatch.dstMacAddr.addr.addr));
    setPolicerInArgs.policerMatch.dstMacAddr.addr.vlanId = 0;
    setPolicerInArgs.policerMatch.dstMacAddr.egressPortNum = CPSW_ALE_HOST_PORT_NUM;
    setPolicerInArgs.threadIdEnable                        = TRUE;
    setPolicerInArgs.threadId                              = flow_idx;
    setPolicerInArgs.peakRateInBitsPerSec                  = 0;
    setPolicerInArgs.commitRateInBitsPerSec                = 0;

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setPolicerInArgs, &setPolicerOutArgs);

    status = Cpsw_ioctl(hCpsw,host_id, CPSW_ALE_IOCTL_SET_POLICER,
                        &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
            "CpswApp_setPolicyEntry() failed CPSW_ALE_IOCTL_SET_POLICER: %d\n",
            status);
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    
    }
    return status;

}

static int32_t app_ethrdev_srv_cb_unregister_mac_handler (uint32_t host_id,uint64_t handle,  uint32_t core_key, u8 *mac_address, uint32_t flow_idx)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    CpswAle_DelPolicerEntryInArgs delPolicerInArgs;

    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, MacAddress:%x:%x:%x:%x:%x:%x, FlowIdx:%u\n",
                       __func__,
                       host_id, 
                       hCpsw, 
                       core_key, 
                       mac_address[0],
                       mac_address[1],
                       mac_address[2],
                       mac_address[3],
                       mac_address[4],
                       mac_address[5],
                       flow_idx);

    memset(&delPolicerInArgs,0,sizeof(delPolicerInArgs));

    delPolicerInArgs.policerMatch.policerMatchEnableMask = CPSW_ALE_POLICER_MATCH_MACDST;
    memcpy(&delPolicerInArgs.policerMatch.dstMacAddr.addr.addr[0U], mac_address,
        sizeof (delPolicerInArgs.policerMatch.dstMacAddr.addr.addr));
    delPolicerInArgs.policerMatch.dstMacAddr.addr.vlanId = 0;
    delPolicerInArgs.policerMatch.dstMacAddr.egressPortNum = CPSW_ALE_HOST_PORT_NUM;
    delPolicerInArgs.delAleEntryMask = CPSW_ALE_POLICER_TABLEENTRY_DELETE_MACDST;

    
    CPSW_IOCTL_SET_IN_ARGS(&prms, &delPolicerInArgs);

    status = Cpsw_ioctl(hCpsw,host_id, CPSW_ALE_IOCTL_DEL_POLICER,
                        &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
            "CpswApp_delPolicyEntry() failed CPSW_ALE_IOCTL_DEL_POLICER: %d\n",
            status);
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    
    }
    return status;
}

static int32_t app_ethrdev_srv_cb_unregister_rx_default_handler (uint32_t host_id,uint64_t handle,  uint32_t core_key, uint32_t flow_idx)
{
    int32_t status = CPSW_SOK;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    CpswAle_SetDefaultThreadConfigInArgs setDefaultThreadConfigInArgs;

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, FlowId:%x\n",__func__,host_id, hCpsw, core_key, flow_idx);

    CPSW_IOCTL_SET_OUT_ARGS(&prms, &setDefaultThreadConfigInArgs);

    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_ALE_IOCTL_GET_DEFAULT_THREADCFG,
                        &prms);

    CpswAppUtils_assert(status == CPSW_SOK);
    setDefaultThreadConfigInArgs.defaultThreadEnable         = TRUE;
    /* TODO: Reserved thread should be queried and set . This is a hack to workaround RM bugs */
    setDefaultThreadConfigInArgs.threadId                    = 1;
    CPSW_IOCTL_SET_IN_ARGS(&prms, &setDefaultThreadConfigInArgs);
    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_ALE_IOCTL_SET_DEFAULT_THREADCFG,
                        &prms);
    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    
    }

    return status;
}


static int32_t app_ethrdev_srv_cb_free_tx_handler(uint32_t host_id,uint64_t handle,  uint32_t core_key, uint32_t tx_cpsw_psil_dst_id)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    CpswRm_FreeTxChInArgs freeTxInArgs;

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, TxId:%x\n",__func__,host_id, hCpsw, core_key, tx_cpsw_psil_dst_id);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

    freeTxInArgs.txChNum = tx_cpsw_psil_dst_id;
    freeTxInArgs.coreKey = core_key;
    CPSW_IOCTL_SET_IN_ARGS(&prms, &freeTxInArgs);
    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_RM_IOCTL_FREE_TX_CH_PEERID,
                        &prms);

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    
    }

    return status;
}


static int32_t app_ethrdev_srv_cb_free_rx_handler(uint32_t host_id,uint64_t handle,  uint32_t core_key, uint32_t alloc_flow_idx)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    CpswRm_FreeRxFlowInArgs freeRxInArgs;

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, RxId:%x\n",__func__,host_id, hCpsw, core_key, alloc_flow_idx);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

    freeRxInArgs.flowIdx = alloc_flow_idx;
    freeRxInArgs.coreKey  = core_key;
    CPSW_IOCTL_SET_IN_ARGS(&prms, &freeRxInArgs);
    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_RM_IOCTL_FREE_RX_FLOW,
                        &prms);

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    
    }

    return status;
}



static int32_t app_ethrdev_srv_cb_free_mac_handler(uint32_t host_id,uint64_t handle,  uint32_t core_key,  u8 *mac_address)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    CpswRm_FreeMacAddrInArgs freeMacInArgs;

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, MacAddress:%x:%x:%x:%x:%x:%x\n",
                       __func__,
                       host_id, 
                       hCpsw, 
                       core_key, 
                       mac_address[0],
                       mac_address[1],
                       mac_address[2],
                       mac_address[3],
                       mac_address[4],
                       mac_address[5]);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

    freeMacInArgs.coreKey = core_key;
    CPSW_UTILS_ARRAY_COPY(freeMacInArgs.macAddr,mac_address);
    CPSW_IOCTL_SET_IN_ARGS(&prms, &freeMacInArgs);
    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_RM_IOCTL_FREE_MAC_ADDR,
                        &prms);

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    
    }

    return status;
}



static int32_t app_ethrdev_srv_cb_detach_handler(uint32_t host_id,uint64_t handle,  uint32_t core_key)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
 
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x\n",__func__,host_id, hCpsw, core_key);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

    CPSW_IOCTL_SET_IN_ARGS(&prms, &core_key);
    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_IOCTL_DETACH_CORE,
                        &prms);

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    
    }

    return status;
}

static void app_showStats(Cpsw_Handle hCpsw, Cpsw_Type cpswType, uint32_t coreId)
{
    Cpsw_IoctlPrms      prms;
    CpswStats_GenericMacPortInArgs inArgs;
    CpswStats_PortStats portStats;
    int32_t status = CPSW_SOK;
    uint32_t i;

    CPSW_IOCTL_SET_OUT_ARGS(&prms, &portStats);
    status =
        Cpsw_ioctl(hCpsw,coreId, CPSW_STATS_IOCTL_GET_HOSTPORT_STATS,
                   &prms);
    if (status == CPSW_SOK)
    {
        CpswAppUtils_print("\n Port 0 Statistics\n");
        CpswAppUtils_print("-----------------------------------------\n");
        switch (cpswType)
        {
            case CPSW_2G:
            {
                CpswStats_HostPort_2g *st;

                st = (CpswStats_HostPort_2g *)&portStats;
                CpswAppUtils_printHostPortStats2G(st);
                break;
            }
            case CPSW_9G:
            {
                CpswStats_HostPort_9g *st;

                st = (CpswStats_HostPort_9g *)&portStats;
                CpswAppUtils_printHostPortStats9G(st);
                break;
            }
        }
        CpswAppUtils_print("\n");
    }
    else
    {
        CpswAppUtils_print(
            "CpswTestCommon_showStats() failed to get host stats: %d\n",
            status);
    }

    if (status == CPSW_SOK)
    {
        for (i = 0,inArgs.portNum = CPSW_MAC_PORT_FIRST ;i < Cpsw_getMacPortMax(cpswType) ; i++,inArgs.portNum++)
        {
            CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &portStats);
            status =
                Cpsw_ioctl(hCpsw,coreId, CPSW_STATS_IOCTL_GET_MACPORT_STATS,
                           &prms);
            if (status == CPSW_SOK)
            {
                CpswAppUtils_print("\n External Port %d Statistics\n",CPSW_NORMALIZE_MACPORT(inArgs.portNum));
                CpswAppUtils_print("-----------------------------------------\n");
                switch (cpswType)
                {
                    case CPSW_2G:
                    {
                        CpswStats_MacPort_2g *st;

                        st = (CpswStats_MacPort_2g *)&portStats;
                        CpswAppUtils_printMacPortStats2G(st);
                        break;
                    }
                    case CPSW_9G:
                    {
                        CpswStats_MacPort_9g *st;

                        st = (CpswStats_MacPort_9g *)&portStats;
                        CpswAppUtils_printMacPortStats9G(st);
                        break;
                    }
                }
                CpswAppUtils_print("\n");
            }
            else
            {
                CpswAppUtils_print(
                    "CpswTestCommon_showStats() failed to get MAC stats: %d\n",
                    status);
            }
        }
    }
}

static int32_t app_ethrdev_srv_cb_ioctl_handler(uint32_t host_id,uint64_t handle,  uint32_t core_key, u32 cmd, const u8 *inargs, u32 inargs_len, u8 *outargs, uint32_t outargs_len)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms    prms;

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, Cmd:%x,InArgsLen:%u, OutArgsLen:%u \n",__func__,host_id, hCpsw, core_key,cmd,inargs_len, outargs_len);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

    prms.inArgsSize = inargs_len;
    prms.outArgsSize = outargs_len;
    prms.inArgs = inargs;
    prms.outArgs = outargs;
    if (prms.inArgsSize == 0)
    {
        prms.inArgs = NULL;
    }
    if (prms.outArgsSize == 0)
    {
        prms.outArgs = NULL;
    }

    status = Cpsw_ioctl(hCpsw, host_id, cmd, &prms);

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    
    }

    return status;
}


static int32_t app_ethrdev_srv_cb_regwr_handler(uint32_t host_id, uint32_t regaddr, uint32_t regval,uint32_t *pRegval)
{
    CpswAppUtils_print("Function:%s,HostId:%u, RegAddr:%p, RegVal:%x \n",__func__,host_id, regaddr, regval);
    
    CSL_REG32_WR(regaddr, regval);
    
    *pRegval = CSL_REG32_RD(regaddr);


    return RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;

}


static int32_t app_ethrdev_srv_cb_regrd_handler(uint32_t host_id, uint32_t regaddr, uint32_t *pRegval)
{
    CpswAppUtils_print("Function:%s,HostId:%u, RegAddr:%p \n",__func__,host_id, regaddr);

    *pRegval = CSL_REG32_RD(regaddr);

    return RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
}

static void app_print_lli_entry(uint32_t entryIdx,LLI_INFO *entry)
{
    char   str[40];
    
    NtIPN2Str( entry->IPAddr, str );
    CpswAppUtils_print("%d ", entryIdx);
    CpswAppUtils_print("        %-15s  ",str);
    CpswAppUtils_print("  %02X:%02X:%02X:%02X:%02X:%02X",
                       entry->MacAddr[0], entry->MacAddr[1], entry->MacAddr[2],
                       entry->MacAddr[3], entry->MacAddr[4], entry->MacAddr[5]);
    CpswAppUtils_print("\n");

}

static void app_dump_lli_table(LLI_INFO* llitable, uint32_t numEntries)
{
    LLI_INFO *entry;
    uint32_t entryIdx;

    CpswAppUtils_print("\n================LLI Table entries=========== \n");
    CpswAppUtils_print("\nNumber of Static ARP Entries: %d \n", numEntries);
    CpswAppUtils_print("\nSNo.      IP Address         MAC Address  \n");
    CpswAppUtils_print("------    -------------      --------------- \n");

    entry = (LLI_INFO *)list_get_head ((NDK_LIST_NODE**)&llitable);
    entryIdx = 0;
    while (entry != NULL)
    {
        app_print_lli_entry(entryIdx, entry);
        /* Get the next LLI Entry. */
        entry = (LLI_INFO *)list_get_next ((NDK_LIST_NODE*)entry);
        entryIdx++;
    }
}

static int32_t app_ethrdev_srv_cb_register_ipv4_mac_handler(uint32_t host_id,uint64_t handle,  uint32_t core_key, uint8_t *mac_address, uint8_t *ipv4_addr)
{
    uint32_t          numEntries;
    int32_t status;
    uint32_t ipaddr = ((uint32_t)ipv4_addr[0] << 24U) | ((uint32_t)ipv4_addr[1] << 16U) | ((uint32_t)ipv4_addr[2] << 8U) | ((uint32_t)ipv4_addr[3] << 0U);
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    LLI_INFO *llitable = NULL;

    ipaddr    = htonl(ipaddr);
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, MacAddress:%x:%x:%x:%x:%x:%x IPv4Addr:%d.%d.%d.%d\n",
                       __func__,
                       host_id, 
                       hCpsw, 
                       core_key, 
                       mac_address[0],
                       mac_address[1],
                       mac_address[2],
                       mac_address[3],
                       mac_address[4],
                       mac_address[5],
                       ipv4_addr[0],
                       ipv4_addr[1],
                       ipv4_addr[2],
                       ipv4_addr[3]);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

    ConCmdRoute(1,"print",NULL,NULL,NULL);
    status =  LLIAddStaticEntry(ipaddr,
                                mac_address );
    if (status != 0)
    {
        status =  LLIAddStaticEntry(ipaddr,
                                    mac_address );
    }
    if (status != 0)
    {
        CpswAppUtils_print("Failed to add Static ARP Entry \n");
    }

    LLIGetStaticARPTable(&numEntries,
                         &llitable );

    app_dump_lli_table(llitable, numEntries);
    LLIFreeStaticARPTable(llitable);
    if (status != 0)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    
    }

    return status;
}

static int32_t app_ethrdev_srv_cb_unregister_ipv4_mac_handler(uint32_t host_id,uint64_t handle,  uint32_t core_key, uint8_t *ipv4_addr)
{
    uint32_t          numEntries;
    int32_t status;
    uint32_t ipaddr = ((uint32_t)ipv4_addr[0] << 24U) | ((uint32_t)ipv4_addr[1] << 16U) | ((uint32_t)ipv4_addr[2] << 8U) | ((uint32_t)ipv4_addr[3] << 0U);
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    LLI_INFO *llitable = NULL;

    ipaddr    = htonl(ipaddr);
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x,IPv4Addr:%x:%x:%x:%x\n",
                       __func__,
                       host_id, 
                       hCpsw, 
                       core_key, 
                       ipv4_addr[0],
                       ipv4_addr[1],
                       ipv4_addr[2],
                       ipv4_addr[3]);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));


    status =  LLIRemoveStaticEntry(ipaddr);
    if (status != 0)
    {
        CpswAppUtils_print("Failed to add Static ARP Entry \n");
    }
    LLIGetStaticARPTable(&numEntries,
                         &llitable );

    app_dump_lli_table(llitable, numEntries);
    LLIFreeStaticARPTable(llitable);
    if (status != 0)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    
    }

    return status;
}


static int32_t app_ethrdev_srv_cb_register_ipv6_mac_handler(uint32_t host_id,uint64_t handle,  uint32_t core_key, uint8_t *mac_address, uint8_t *ipv6_addr)
{
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, MacAddress:%x:%x:%x:%x:%x:%x IPv6Addr:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n",
                       __func__,
                       host_id, 
                       handle, 
                       core_key, 
                       mac_address[0],
                       mac_address[1],
                       mac_address[2],
                       mac_address[3],
                       mac_address[4],
                       mac_address[5],
                       ipv6_addr[0],
                       ipv6_addr[1],
                       ipv6_addr[2],
                       ipv6_addr[3],
                       ipv6_addr[4],
                       ipv6_addr[5],
                       ipv6_addr[6],
                       ipv6_addr[7],
                       ipv6_addr[8],
                       ipv6_addr[9],
                       ipv6_addr[10],
                       ipv6_addr[11],
                       ipv6_addr[12],
                       ipv6_addr[13],
                       ipv6_addr[14],
                       ipv6_addr[15]);

    return RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
}

static void app_ethrdev_srv_cb_client_notify_handler(uint32_t host_id,uint64_t handle,  uint32_t core_key, enum rpmsg_kdrv_ethswitch_client_notify_type notifyid, uint8_t *notify_info, uint32_t notify_info_len)
{
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms    prms;
#define STRINGIFY(x) #x
#define XSTRINGIFY(x) STRINGIFY(x)
    char * notify_type_str[] = {XSTRINGIFY(RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_DUMPSTATS)};

    CpswAppUtils_assert(notifyid < CPSW_UTILS_ARRAYSIZE(notify_type_str));
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x,NotifyId:%s,NotifyLen\n",__func__, host_id, core_key, hCpsw, notify_type_str[notifyid], notify_info_len);

    switch (notifyid)
    {
        case  RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_DUMPSTATS:
        {
            int32_t status;

            CpswAppUtils_assert(hCpsw == Cpsw_getHandle(gCpswType));

            CPSW_IOCTL_SET_NO_ARGS(&prms);
            status = Cpsw_ioctl(hCpsw,host_id, CPSW_ALE_IOCTL_DUMP_TABLE,
                       &prms);
            CpswAppUtils_assert(status == CPSW_SOK);
            
            CPSW_IOCTL_SET_NO_ARGS(&prms);
            status = Cpsw_ioctl(hCpsw,host_id, CPSW_ALE_IOCTL_DUMP_POLICER_ENTRIES,
                       &prms);

            CpswAppUtils_assert(status == CPSW_SOK);
            
            app_showStats(hCpsw, gCpswType, host_id);
            break;
        }
        default:
            /* unhandled notify.do nothing */
            break;
    }
}


static rdevEthSwitchServerCbFxn_t appRdevEthSwitchServerCbFxnTbl = 
{
    .attach_handler = app_ethrdev_srv_cb_attach_handler,
    .alloc_tx_handler = app_ethrdev_srv_cb_alloc_tx_handler,
    .alloc_rx_handler = app_ethrdev_srv_cb_alloc_rx_handler,
    .alloc_rx_default_handler = app_ethrdev_srv_cb_alloc_rx_default_handler,
    .alloc_mac_handler = app_ethrdev_srv_cb_alloc_mac_handler,
    .register_mac_handler = app_ethrdev_srv_cb_register_mac_handler,
    .unregister_mac_handler = app_ethrdev_srv_cb_unregister_mac_handler,
    .unregister_rx_default_handler = app_ethrdev_srv_cb_unregister_rx_default_handler,
    .free_tx_handler = app_ethrdev_srv_cb_free_tx_handler,
    .free_rx_handler = app_ethrdev_srv_cb_free_rx_handler,
    .free_mac_handler = app_ethrdev_srv_cb_free_mac_handler,
    .detach_handler = app_ethrdev_srv_cb_detach_handler,
    .ioctl_handler = app_ethrdev_srv_cb_ioctl_handler,
    .regwr_handler = app_ethrdev_srv_cb_regwr_handler,
    .regrd_handler = app_ethrdev_srv_cb_regrd_handler,
    .ipv4_register_mac_handler = app_ethrdev_srv_cb_register_ipv4_mac_handler,
    .ipv6_register_mac_handler = app_ethrdev_srv_cb_register_ipv6_mac_handler,
    .ipv4_unregister_mac_handler = app_ethrdev_srv_cb_unregister_ipv4_mac_handler,
    .client_notify_handler = app_ethrdev_srv_cb_client_notify_handler,
};






