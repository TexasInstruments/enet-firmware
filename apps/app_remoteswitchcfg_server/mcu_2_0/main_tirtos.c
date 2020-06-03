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
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>

/* PDK Driver Header files */
#include <ti/drv/ipc/ipc.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appboardutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appsoc.h>

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

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define ETHAPP_OK                       (0)

#define ETHAPP_ERROR                    (-1)

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

    /* CPSW instance type */
    Cpsw_Type cpswType;

    /* Ethernet Firmware handle */
    EthFw_Handle hEthFw;

    /* UDMA driver handle */
    Udma_DrvHandle hUdmaDrv;

    /* Semaphore for synchronizing EthFw and NDK initialization */
    Semaphore_Handle hInitSem;

    /* Host MAC address */
    uint8_t hostMacAddr[CPSW_MAC_ADDR_LEN];

    /* Host IP address */
    uint8_t hostIpAddr[CPSW_ALE_IPV4ADDR_NUM_OCTETS];

    /* Handle to PTP stack */
    TimeSyncPtp_Handle hTimeSyncPtp;
} EthAppObj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static void EthApp_waitForDebugger(void);

static void EthApp_initTaskFxn(UArg arg0, UArg arg1);

static void EthApp_initIpcTaskFxn(UArg arg0, UArg arg1);

static int32_t EthApp_initEthFw(void);

static void CpswApp_setPtpConfig(TimeSyncPtp_Config *ptpConfig);

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
    .cpswType = CPSW_9G,
    .hEthFw = NULL,
    .hUdmaDrv = NULL,
};

static EthFw_Port gEthAppPorts[] =
{
    {
        .portNum    = CPSW_MAC_PORT_0,
        .vlanConfig = { .portPri = 0U, .portCfi = 0U, .portVID = 0U },
    },
#if defined(SOC_J721E)
    /* On J721E EVM to use all 8 ports simultaneously, we use below configuration
       RGMII Ports - 1,3,4,8. QSGMII ports - 2,5,6,7 */
    {
        .portNum    = CPSW_MAC_PORT_2, /* RGMII */
        .vlanConfig = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = CPSW_MAC_PORT_3, /* RGMII */
        .vlanConfig = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = CPSW_MAC_PORT_7, /* RGMII */
        .vlanConfig = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
#if defined(ENABLE_QSGMII_PORTS) //kept it disabled for 6.2
    {
        .portNum    = CPSW_MAC_PORT_1, /* QSGMII main */
        .vlanConfig = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = CPSW_MAC_PORT_4, /* QSGMII sub */
        .vlanConfig = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = CPSW_MAC_PORT_5, /* QSGMII sub */
        .vlanConfig = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
    },
    {
        .portNum    = CPSW_MAC_PORT_6, /* QSGMII sub */
        .vlanConfig = { .portPri = 0U, .portCfi = 0U, .portVID = 0U }
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
{
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C66X_1,
    IPC_C66X_2,
    IPC_C7X_1
};

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

    gEthAppObj.coreId = CpswAppSoc_getCoreId();

    /* Board related initialization */
    CpswAppBoardUtils_initEthFw();
    CpswAppUtils_enableClocks(gEthAppObj.cpswType);

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
    CpswAppUtils_print("=======================================================\n");
    CpswAppUtils_print("           CPSW Ethernet Firmware Demo                 \n");
    CpswAppUtils_print("=======================================================\n");

    /* Open UDMA driver */
    gEthAppObj.hUdmaDrv = CpswAppUtils_udmaOpen(gEthAppObj.cpswType, NULL);
    if (gEthAppObj.hUdmaDrv == NULL)
    {
        CpswAppUtils_print("ETHFW: failed to open UDMA driver\n");
        status = ETHAPP_ERROR;
    }

    /* Initialize Ethernet Firmware */
    if (status == ETHAPP_OK)
    {
        status = EthApp_initEthFw();
    }

    /* Create IPC initialization task */
    if (status == CPSW_SOK)
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

    CpswAppUtils_print("IPC_echo_test (core : %s) .....\r\n", Ipc_mpGetSelfName());

    Ipc_init(NULL);

#if !defined(A72_QNX_OS)
    Ipc_loadResourceTable(appGetIpcResourceTable());
#endif

    /* Step 2: Initialize Virtio */
    vqParam.vqObjBaseAddr = (void *)&gEthAppSysVqBuf[0];
    vqParam.vqBufSize = numProc * Ipc_getVqObjMemoryRequiredPerCore();
    vqParam.vringBaseAddr = (void *)gEthAppVringMemBuf;
    vqParam.vringBufSize = sizeof(gEthAppVringMemBuf);
    vqParam.timeoutCnt = 100;     /* Wait for counts */
    Ipc_initVirtIO(&vqParam);

    /* Step 3: Initialize RPMessage */
    /* Initialize the param and set memory for HeapMemory for control task */
    RPMessageParams_init(&cntrlParam);
    cntrlParam.buf = &gEthAppCntrlBuf[0];
    cntrlParam.bufSize = RPMSG_DATA_SIZE;
    cntrlParam.stackBuffer = &gEthAppCtrlTaskBuf[0];
    cntrlParam.stackSize = IPC_TASK_STACKSIZE;
    RPMessage_init(&cntrlParam);

    /* Initialize the Remote Config server (CPSW Proxy Server) */
    status = EthFw_initRemoteConfig(gEthAppObj.hEthFw);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("EthApp_initIpcTask: failed to init EthFw remote config: %d\n", status);
    }

    /* Wait for Linux VDev ready... */
    if (status == CPSW_SOK)
    {
        while (!Ipc_isRemoteReady(IPC_MPU1_0))
        {
            Task_sleep(10);
        }
    }

    /* Create the VRing now ... */
    if (status == CPSW_SOK)
    {
        status = Ipc_lateVirtioCreate(IPC_MPU1_0);
        if (status != IPC_SOK)
        {
            CpswAppUtils_print("EthApp_initIpcTask: Ipc_lateVirtioCreate failed: %d\n", status);
        }
    }

    /* Late init */
    if (status == IPC_SOK)
    {
        status = RPMessage_lateInit(IPC_MPU1_0);
        if (status != IPC_SOK)
        {
            CpswAppUtils_print("EthApp_initIpcTask: RPMessage_lateInit failed: %d\n", status);
        }
    }

    /* Init EthFw services: task/CPU statistics and Ethernet statistics */
    if (status == IPC_SOK)
    {
        status = EthFw_initRemoteServices(gEthAppObj.hEthFw);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("EthApp_initIpcTask: failed to init EthFw remote services: %d\n", status);
        }
    }
}

static int32_t EthApp_initEthFw(void)
{
    EthFw_Version ver;
    EthFw_Config ethFwCfg;
    uint32_t i;
    int32_t status = ETHAPP_OK;

    /* Set EthFw config params */
    EthFw_initConfigParams(gEthAppObj.cpswType, &ethFwCfg);
    ethFwCfg.cpswConfig.dmaConfig.hUdmaDrv = gEthAppObj.hUdmaDrv;
    ethFwCfg.ports = &gEthAppPorts[0];
    ethFwCfg.numPorts = ARRAY_SIZE(gEthAppPorts);

    /* Overwrite config params with those for hardware interVLAN */
    EthHwInterVlan_setOpenPrms(&ethFwCfg.cpswConfig);

    for (i = 0U; i < ethFwCfg.numPorts; i++)
    {
        EthHwInterVlan_setVlanConfig(&ethFwCfg.ports[i].vlanConfig,
                                     ethFwCfg.ports[i].portNum);
    }

    /* Initialize the EthFw */
    gEthAppObj.hEthFw = EthFw_init(gEthAppObj.cpswType, &ethFwCfg);
    if (gEthAppObj.hEthFw == NULL)
    {
        CpswAppUtils_print("ETHFW: failed to initialize the firmware\n");
        status = ETHAPP_ERROR;
    }

    /* Get and print EthFw version */
    if (status == ETHAPP_OK)
    {
        EthFw_getVersion(gEthAppObj.hEthFw, &ver);
        CpswAppUtils_print("\nETHFW Version   : %d.%02d.%02d\n", ver.major, ver.minor, ver.rev);
        CpswAppUtils_print("ETHFW Build Date: %s %s, %s\n", ver.month, ver.date, ver.year);
        CpswAppUtils_print("ETHFW Build Time: %s:%s:%s\n", ver.hour, ver.min, ver.sec);
        CpswAppUtils_print("ETHFW Commit SHA: %s\n\n", ver.commitHash);
    }

    /* Post semaphore so that NDK/NIMU can continue with their initialization */
    Semaphore_post(gEthAppObj.hInitSem);

    return status;
}

/* PTP related functions */

static void CpswApp_setPtpConfig(TimeSyncPtp_Config *ptpConfig)
{
    ptpConfig->socConfig.socVersion = TIMESYNC_SOC_J721E;
    ptpConfig->socConfig.ipVersion  = TIMESYNC_IP_VER_CPSW_9G;
    ptpConfig->vlanCfg.vlanType     = TIMESYNC_VLAN_TYPE_NONE;
    ptpConfig->deviceMode           = TIMESYNC_ORDINARY_CLOCK;
    ptpConfig->portMask            |= CPSW_SET_BIT(2U);

    memcpy(&ptpConfig->ifMacID[0U],
           &gEthAppObj.hostMacAddr[0U],
           CPSW_MAC_ADDR_LEN);

    memcpy(&ptpConfig->ipAddr[0U],
           &gEthAppObj.hostIpAddr[0U],
           CPSW_ALE_IPV4ADDR_NUM_OCTETS);
}

/* NIMU callbacks (exact name required) */

bool EthFwCallbacks_isPortLinked(Cpsw_Handle hCpsw)
{
    bool linked = false;
    uint32_t i;

    /* Report port linked as long as any port owned by EthFw is up */
    for (i = 0U; (i < ARRAY_SIZE(gEthAppPorts)) && !linked; i++)
    {
        linked = CpswAppUtils_isPortLinkUp(hCpsw,
                                           gEthAppObj.coreId,
                                           gEthAppPorts[i].portNum);
    }

    return linked;
}

void NimuCpswAppCb_getHandle(NimuCpswAppIf_GetHandleInArgs *inArgs,
                             NimuCpswAppIf_GetHandleOutArgs *outArgs)
{
    /* Wait for EthFw to be initialized */
    Semaphore_pend(gEthAppObj.hInitSem, BIOS_WAIT_FOREVER);

    EthFwCallbacks_nimuCpswGetHandle(inArgs, outArgs);

    /* Save host port MAC address */
    memcpy(&gEthAppObj.hostMacAddr[0U],
           &outArgs->rxInfo.macAddr[0U],
           CPSW_MAC_ADDR_LEN);
}

void NimuCpswAppCb_releaseHandle(NimuCpswAppIf_ReleaseHandleInfo *releaseInfo)
{
    EthFwCallbacks_nimuCpswReleaseHandle(releaseInfo);
}

/* NDK hooks */

void EthApp_ipAddrHookFxn(uint32_t IPAddr,
                          uint32_t IfIdx,
                          uint32_t fAdd)
{
    volatile uint32_t ipAddrHex = 0U;
    TimeSyncPtp_Config ptpConfig;
    int32_t status;

    /* Use default/generic hook function */
    EthFwCallbacks_ipAddrHookFxn(IPAddr, IfIdx, fAdd);

    /* Save host port IP address */
    ipAddrHex = ntohl(IPAddr);
    memcpy(&gEthAppObj.hostIpAddr[0U],
           (uint8_t *)&ipAddrHex,
           CPSW_ALE_IPV4ADDR_NUM_OCTETS);

    /* Initialize and enable PTP stack */
    TimeSyncPtp_setDefaultPtpConfig(&ptpConfig);
    CpswApp_setPtpConfig(&ptpConfig);
    gEthAppObj.hTimeSyncPtp = TimeSyncPtp_init(&ptpConfig);
    CpswAppUtils_assert(gEthAppObj.hTimeSyncPtp != NULL);
    TimeSyncPtp_enable(gEthAppObj.hTimeSyncPtp);

    /* Assign functions that are to be called based on actions in GUI.
     * These cannot be dynamically pushed to function pointer array, as the
     * index is used in GUI as command */
    cpswCfgServer_fxn_table[9] = &EthApp_startSwInterVlan;
    cpswCfgServer_fxn_table[10] = &EthApp_startHwInterVlan;

    /* Start Configuration server */
    status = CpswCfgServer_init(gEthAppObj.cpswType);
    CpswAppUtils_assert(CPSW_SOK == status);

    /* Start the software-based interVLAN routing */
    EthSwInterVlan_setupRouting(gEthAppObj.cpswType,
                                ETH_SWINTERVLAN_TASK_PRI);
}

/* Functions called from Config server library based on selection from GUI */

static void EthApp_startSwInterVlan(char *recvBuff,
                                    char *sendBuff)
{
    CpswCfgServer_InterVlanConfig *pInterVlanCfg;
    int32_t status;

    if (recvBuff != NULL)
    {
        pInterVlanCfg = (CpswCfgServer_InterVlanConfig *)recvBuff;
        status = EthSwInterVlan_addClassifierEntries(pInterVlanCfg);
        CpswAppUtils_assert(CPSW_SOK == status);
    }
}

static void EthApp_startHwInterVlan(char *recvBuff,
                                    char *sendBuff)
{
    CpswCfgServer_InterVlanConfig *pInterVlanCfg;

    if (recvBuff != NULL)
    {
        pInterVlanCfg = (CpswCfgServer_InterVlanConfig *)recvBuff;
        EthHwInterVlan_setupRouting(gEthAppObj.cpswType, pInterVlanCfg);
    }
}
