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

#include <ti/drv/ipc/ipc.h>
#include <ti/csl/cslr_gtc.h>

#include <ethremotecfg/protocol/rpmsg-kdrv-transport-ethswitch.h>
#include <client-rtos/remote-device.h>
#include <ethremotecfg/client/include/ethremotecfg_client.h>
#include <ethremotecfg/client/include/cpsw_proxy.h>

#include <apps/ipc_cfg/app_ipc_rsctable.h>
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/enet/include/dma/udma/enet_udma.h>
#include <ti/drv/enet/include/core/enet_dma.h>

#include <ti/drv/enet/nimuenet/nimu_ndk.h>
#include <ti/drv/enet/nimuenet/ndk2enet_appif.h>
#include <ti/drv/enet/examples/utils/include/enet_appsoc.h>
#include <ti/drv/enet/examples/utils/include/enet_ethutils.h>
#include <ti/drv/enet/examples/utils/include/enet_appmemutils_cfg.h>
#include <ti/drv/enet/examples/utils/include/enet_appmemutils.h>

/* NDK headers */
#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/ndk/inc/socket.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/tools/servers.h>
#include <ti/ndk/inc/tools/console.h>


#define CPSW_REMOTE_APP_PHY_POLLING_INTERVAL  (100)
#define CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US (1000U)
#define CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL   (30U)
#define CPSW_REMOTE_APP_CPTS_HW_PUSH_NUM      (2U)

#define IPC_RPMESSAGE_OBJ_SIZE  (256)
#define VQ_TIMEOUT              (100)
#define VQ_BUF_SIZE             (2048)
#define REMOTE_DEVICE_ENDPT     (26)
#define RPMSG_DATA_SIZE         (256 * 512 + IPC_RPMESSAGE_OBJ_SIZE)

static uint8_t g_monitorStackBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
;

static uint8_t g_rdevStackBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
;

static uint8_t g_ipcStackBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
;

static uint8_t g_vdevMonStackBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
;

static uint8_t g_mainStackBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
;

static uint8_t ctrlTaskBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
;

static uint8_t g_messageTaskStack[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
;

static uint8_t g_requestTaskStack[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
;

static uint8_t sysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section("ipc_data_buffer"), aligned(8)));
static uint8_t gCntrlBuf[RPMSG_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned(8)));

static uint8_t g_vringMemBuf[IPC_VRING_MEM_SIZE] __attribute__ ((section(".bss:ipc_vring_mem"), aligned(8192)));

static uint32_t selfProcId = IPC_MCU2_1;
static uint32_t gRemoteProc[] =
#if defined(SOC_J721E)
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0,
    IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2,
    IPC_C7X_1
};
#elif defined(SOC_J7200)
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0,
};
#endif
static uint32_t gNumRemoteProc = sizeof(gRemoteProc) / sizeof(uint32_t);

#define ENABLE_NDKSERVERS

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
#endif

typedef struct CpswRemoteApp_SyncTimerObj_s
{
    uint64_t currLocalTime;
    uint64_t prevLocalTime;
    uint64_t currCptsTime;
    uint64_t prevCptsTime;
    double rate;
    double offset;
} CpswRemoteApp_SyncTimerObj;

typedef struct CpswRemoteApp_Obj_s
{
    CpswProxy_Handle hCpswProxy;
    Enet_Handle hEnet;
    uint32_t coreKey;
    uint8_t macAddr[ENET_MAC_ADDR_LEN];
    uint8_t ipv4Addr[ENET_IPv4_ADDR_LEN];
    EnetDma_Handle hDma;
    bool useDefaultRxFlow;
    bool useExtAttach;
    Enet_MacPort *macPorts;
    uint32_t numMacPorts;
    CpswRemoteApp_SyncTimerObj syncTimerObj;
} CpswRemoteApp_Obj;

static Enet_MacPort gRemoteAppMacPorts[] =
{
#if defined (SOC_J721E)
    ENET_MAC_PORT_3,
    ENET_MAC_PORT_4,
#elif defined (SOC_J7200)
#if defined(ENABLE_QSGMII_PORTS)
    ENET_MAC_PORT_1,
#else
    ENET_MAC_PORT_2,
#endif
#endif
};

CpswRemoteApp_Obj gRemoteAppObj =
{
    .hCpswProxy       = NULL,
    .hEnet            = NULL,
    .coreKey          = ENET_RM_INVALIDCORE,
    .hDma             = NULL,
    .useDefaultRxFlow = false,
    .useExtAttach     = true,
    .numMacPorts      = ENET_ARRAYSIZE(gRemoteAppMacPorts),
    .macPorts         = gRemoteAppMacPorts,
};

static CpswProxy_Handle CpswRemoteApp_initCpswProxy(void);

char *VerStr = "NIMU CPSW Example";

static void CpswRemoteApp_initSyncTimer(void);

static uint64_t CpswRemoteApp_getLocalTime(void);

static uint64_t CpswRemoteApp_getSynchronizedTime(void);

static void CpswRemoteApp_calcSyncTimeParams(CpswCpts_HwPush hwPushNum,
                                             uint64_t syncTime);

// hack for release mode build fix TODO fix this
void localAssert(bool cond)
{
#if defined(__KLOCWORK__)
    if (!cond)
    {
        abort();
    }
#else
    assert(cond);
#endif
}

void stackInitHook(void *hCfg)
{
    int rc;

    /* increase stack size */
    rc = 16384;
    CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_TASKSTKBOOT,
                CFG_ADDMODE_UNIQUE, sizeof(uint32_t), (uint8_t *)&rc, 0);
}

void stackDeleteHook(void *hCfg)
{
}

void IpAddrHookFxn(uint32_t IPAddr,
                   uint32_t IfIdx,
                   uint32_t fAdd)
{
    volatile uint32_t ipAddrHex = 0U;
    char ipAddr[20];

    ipAddrHex = ntohl(IPAddr);
    gRemoteAppObj.ipv4Addr[0] = (uint8_t)(ipAddrHex >> 24) & 0xFF;
    gRemoteAppObj.ipv4Addr[1] = (uint8_t)(ipAddrHex >> 16) & 0xFF;
    gRemoteAppObj.ipv4Addr[2] = (uint8_t)(ipAddrHex >> 8) & 0xFF;
    gRemoteAppObj.ipv4Addr[3] = (uint8_t)(ipAddrHex & 0xFF);
    snprintf(ipAddr, 17, "%d.%d.%d.%d\n",
             gRemoteAppObj.ipv4Addr[0],
             gRemoteAppObj.ipv4Addr[1],
             gRemoteAppObj.ipv4Addr[2],
             gRemoteAppObj.ipv4Addr[3]);

    localAssert((gRemoteAppObj.hCpswProxy != NULL) && (gRemoteAppObj.hEnet != NULL));

    CpswProxy_registerIPV4Addr(gRemoteAppObj.hCpswProxy,
                               gRemoteAppObj.hEnet,
                               gRemoteAppObj.coreKey,
                               gRemoteAppObj.macAddr,
                               gRemoteAppObj.ipv4Addr);

    System_printf("\nCPSW NIMU application, IP address I/F 1: %s\n\r", ipAddr);
    CpswRemoteApp_initSyncTimer();
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
#endif
}

void ServiceReportHook(uint32_t Item, uint32_t Status, uint32_t Report, void * h)
{
    if( (Item == CFGITEM_SERVICE_DHCPCLIENT) && ((Report & 0xFF) == POLLOUT))
    {
        CI_SERVICE_DHCPC dhcpc;
        int status;

        System_printf("DHCP client timed out. Retrying..... \n");

        /* By default, DHCP client service timeouts after three minutes and the
         * service gets terminated. So we have to restart DHCP client service after
         * timeout happens by adding a DHCP client service entry*/
        memset(&dhcpc, 0U, sizeof(dhcpc));
        dhcpc.cisargs.Mode   = CIS_FLG_IFIDXVALID;
        dhcpc.cisargs.IfIdx  = CIS_FLG_IFIDXVALID;
        dhcpc.cisargs.pCbSrv = &ServiceReportHook;
        status = CfgAddEntry(0, CFGTAG_SERVICE, CFGITEM_SERVICE_DHCPCLIENT, 0,
                             sizeof(dhcpc), (unsigned char *)&dhcpc, 0);
        localAssert(status >= 0);
    }
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

static void CpswRemoteApp_initSyncTimer(void)
{
    int32_t status;

    memset(&gRemoteAppObj.syncTimerObj, 0, sizeof(CpswRemoteApp_SyncTimerObj));

    /* Make sure GTC is disabled before configuring timesync router*/
    CSL_REG32_WR(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCR, 0x0U);

    /* Register callback */
    status = CpswProxy_registerHwPushNotifyCb(gRemoteAppObj.hCpswProxy,
                                              CpswRemoteApp_calcSyncTimeParams);
    if (status == ENET_EALREADYOPEN)
    {
        System_printf("CpswProxy_registerHwPushNotifyCb(): Callback is registered already\n");
    }

    /* Configure GTC push event */
    CSL_REG32_WR(CSL_GTC0_GTC_CFG0_BASE + CSL_GTC_CFG0_PUSHEVT,
                 CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL);

    /* Send request to Ethfw to configure TSR */
    CpswProxy_registerRemoteTimer(gRemoteAppObj.hCpswProxy,
                                  gRemoteAppObj.hEnet,
                                  gRemoteAppObj.coreKey,
                                  CSLR_TIMESYNC_INTRTR0_IN_GTC0_GTC_PUSH_EVENT_0,
                                  CPSW_REMOTE_APP_CPTS_HW_PUSH_NUM);

    /* Enable GTC */
    CSL_REG32_WR(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCR, 0x1U);
}

static uint64_t CpswRemoteApp_getLocalTime(void)
{
    uint32_t gtcTimeLo = 0U, gtcTimeHi = 0U;
    uint64_t gtcTime = 0U;

    gtcTimeLo = *(uint32_t *)(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCV_LO);
    gtcTimeHi = *(uint32_t *)(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCV_HI);
    gtcTime = (((uint64_t)(gtcTimeHi) << 32U) |
                (uint64_t)(gtcTimeLo));

    return gtcTime;
}

static uint64_t CpswRemoteApp_getSynchronizedTime(void)
{
    uint64_t gtcTime = 0U, synchronizedTime = 0U;
    CpswRemoteApp_SyncTimerObj *hSyncTimerObj = (CpswRemoteApp_SyncTimerObj*)&gRemoteAppObj.syncTimerObj;

    /* Get GTC time */
    gtcTime = CpswRemoteApp_getLocalTime();

    /* Compute synchronized time from GTC time */
    synchronizedTime = (uint64_t)((hSyncTimerObj->rate * (double)gtcTime) + hSyncTimerObj->offset);

    return synchronizedTime;
}

static void CpswRemoteApp_calcSyncTimeParams(CpswCpts_HwPush hwPushNum, uint64_t syncTime)
{
    if (hwPushNum == CPSW_REMOTE_APP_CPTS_HW_PUSH_NUM)
    {
        uint64_t gtcTime = 0U;
        uint64_t synchronizedTime = 0U;
        CpswRemoteApp_SyncTimerObj *hSyncTimerObj = (CpswRemoteApp_SyncTimerObj*)&gRemoteAppObj.syncTimerObj;
        double temp1, temp2;

        if(hSyncTimerObj->prevLocalTime == 0U)
        {
            /* Disable GTC */
            CSL_REG32_WR(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCR, 0x0U);

            /* Set GTC time */
            gtcTime = 1U << CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL;
            CSL_REG32_WR(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCV_LO, gtcTime & 0xFFFFFFFF);
            CSL_REG32_WR(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCV_HI, gtcTime >> 32U);

            /* Re-enable GTC */
            CSL_REG32_WR(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCR, 0x1U);
        }
        else
        {
            /* Increment GTC time used for computation based on selected bit for event */
            gtcTime = hSyncTimerObj->prevLocalTime + (1U << CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL);
        }

        if ((hSyncTimerObj->prevLocalTime != 0U) &&
            (hSyncTimerObj->prevCptsTime != 0U))
        {
            /* Logic:
             *  T1, T2 - Previous & Current CPTS time
             *  t1, t2 - Previous & Current local GTC time
             *  rate = (T2-T1) / (t2-t1)
             *  offset = (t2T1 - t1T2) / (t2-t1)
             *  temp1 = t2 * (T1 / (t2-t1))
             *  temp2 = t1 * (T2 / (t2-t1))
             *  offset = temp1 - temp2
             */
            temp1 = (double)gtcTime *
                    ((double)hSyncTimerObj->prevCptsTime / (double)(gtcTime - hSyncTimerObj->prevLocalTime));
            temp2 = (double)hSyncTimerObj->prevLocalTime *
                    ((double)syncTime / (double)(gtcTime - hSyncTimerObj->prevLocalTime));

            hSyncTimerObj->rate = (double)((double)(syncTime - hSyncTimerObj->prevCptsTime) /
                                          (double)(gtcTime - hSyncTimerObj->prevLocalTime));
            hSyncTimerObj->offset = temp1 - temp2;

            synchronizedTime = CpswRemoteApp_getSynchronizedTime();
            System_printf("Current Synchronized time in Epoch format: %lld\n",synchronizedTime);
        }

        hSyncTimerObj->prevLocalTime = gtcTime;
        hSyncTimerObj->prevCptsTime = syncTime;
    }
}

static void rpmsg_vdevMonitorFxn(void* arg0,
                                 void* arg1)
{
    int32_t status;

    /* Wait for Linux VDev ready... */
    while (!Ipc_isRemoteReady(IPC_MPU1_0))
    {
        TaskP_sleep(10);
    }

    /* Create the VRing now ... */
    status = Ipc_lateVirtioCreate(IPC_MPU1_0);
    if (status != IPC_SOK)
    {
        System_printf("%s: Ipc_lateVirtioCreate failed\n", __func__);
    }

    if (status == IPC_SOK)
    {
        status = RPMessage_lateInit(IPC_MPU1_0);
        if (status != IPC_SOK)
        {
            System_printf("%s: RPMessage_lateInit failed\n", __func__);
        }
    }

    return;
}

static void printDevInfo(struct rpmsg_kdrv_ethswitch_device_data *ethDevData)
{
    char *tf[] = {"false", "true"};

    System_printf("ETHFW Version:%2d.%2d.%2d\n",
                  ethDevData->fw_ver.major,
                  ethDevData->fw_ver.minor,
                  ethDevData->fw_ver.rev);
    System_printf("ETHFW Build Date (YYYY/MMM/DD):%c%c%c%c/%c%c%c/%c%c\n",
                  ethDevData->fw_ver.year[0], ethDevData->fw_ver.year[1], ethDevData->fw_ver.year[2], ethDevData->fw_ver.year[3],
                  ethDevData->fw_ver.month[0], ethDevData->fw_ver.month[1], ethDevData->fw_ver.month[2],
                  ethDevData->fw_ver.date[0], ethDevData->fw_ver.date[1]);
    System_printf("ETHFW Commit SHA:%c%c%c%c%c%c%c%c\n",
                  ethDevData->fw_ver.commit_hash[0],
                  ethDevData->fw_ver.commit_hash[1],
                  ethDevData->fw_ver.commit_hash[2],
                  ethDevData->fw_ver.commit_hash[3],
                  ethDevData->fw_ver.commit_hash[4],
                  ethDevData->fw_ver.commit_hash[5],
                  ethDevData->fw_ver.commit_hash[6],
                  ethDevData->fw_ver.commit_hash[7]);
    System_printf("ETHFW PermissionFlag:0x%x, UART Connected:%s,UART Id:%d",
                  ethDevData->permission_flags,
                  tf[ethDevData->uart_connected],
                  ethDevData->uart_id);
}

static void CpswRemoteApp_initIpc(void* a0,
                     void* a1)
{
    TaskP_Params params;
    uint32_t numProc = gNumRemoteProc;
    Ipc_VirtIoParams vqParam;
    Ipc_InitPrms initPrms;
    RPMessage_Params cntrlParam;
    int32_t status;

    /* Step1 : Initialize the multiproc */
    status = Ipc_mpSetConfig(selfProcId, numProc, &gRemoteProc[0]);

    System_printf("IPC_echo_test (core : %s) .....\r\n",
                  Ipc_mpGetSelfName());

    /* Initialize params with defaults */
    IpcInitPrms_init(0U, &initPrms);

    initPrms.printFxn = &CpswRemoteApp_ipcPrint;

    status += Ipc_init(&initPrms);
    if (status == ENET_SOK)
    {
        Ipc_loadResourceTable(appGetIpcResourceTable());
    }

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
        cntrlParam.bufSize = RPMSG_DATA_SIZE;
        cntrlParam.stackBuffer = &ctrlTaskBuf[0];
        cntrlParam.stackSize = sizeof(ctrlTaskBuf);
        status = RPMessage_init(&cntrlParam);
    }

    if (status != ENET_SOK)
    {
        System_printf("ETHFW RPMessage_init failed\n");
    }

    /* Step 4: Create RPMessage monitor task */
    TaskP_Params_init(&params);
    params.priority = 3;
    params.stack = &g_vdevMonStackBuf[0];
    params.stacksize = sizeof(g_vdevMonStackBuf);
    TaskP_create(rpmsg_vdevMonitorFxn, &params);

    /* Step 5: Start Cpsw Proxy */
    localAssert(gRemoteAppObj.hCpswProxy != NULL);
    CpswProxy_start(gRemoteAppObj.hCpswProxy);
}


int main(void)
{
    TaskP_Handle task;
    TaskP_Params ipc_taskParams;

    /* Set ccsHaltFlag to 1 for halting core for CCS connection */
    volatile uint32_t ccsHaltFlag = 0U;

    while (ccsHaltFlag)
    {
        ;
    }

    TaskP_Params_init(&ipc_taskParams);
    ipc_taskParams.priority = 2;
    ipc_taskParams.stack = &g_ipcStackBuf[0];
    ipc_taskParams.stacksize = sizeof(g_ipcStackBuf);
    task = TaskP_create(CpswRemoteApp_initIpc, &ipc_taskParams);

    if (NULL == task)
    {
        OS_stop();
    }

    gRemoteAppObj.hCpswProxy = CpswRemoteApp_initCpswProxy();
    OS_start();    /* does not return */

    return(0);
}

static bool CpswRemoteApp_isAllPortLinked(Enet_Handle hEnet)
{
    uint32_t i;
    static bool isPhyLinked = false;
    static uint32_t pollingInterVal = 0;

    if ((isPhyLinked == false) || ((pollingInterVal % CPSW_REMOTE_APP_PHY_POLLING_INTERVAL) == 0))
    {
        for (i = 0; i < gRemoteAppObj.numMacPorts; i++)
        {
            isPhyLinked = (isPhyLinked ||
                            CpswProxy_isPhyLinked(gRemoteAppObj.hCpswProxy, hEnet, gRemoteAppObj.coreKey, gRemoteAppObj.macPorts[i]));
        }
    }

    pollingInterVal = (pollingInterVal + 1) % CPSW_REMOTE_APP_PHY_POLLING_INTERVAL;
    return isPhyLinked;
}

static struct Udma_DrvObj udmaDrvObj;

static Udma_DrvHandle CpswRemoteApp_udmaOpen(void)
{
    Udma_InitPrms initPrms;
    Udma_DrvHandle hUdmaDrv;
    int32_t retVal;
    uint32_t instId;

    hUdmaDrv = &udmaDrvObj;
    memset(hUdmaDrv, 0U, sizeof(*hUdmaDrv));

    instId = UDMA_INST_ID_MAIN_0;

    /* Initialize the UDMA driver based on NAVSS instance */
    UdmaInitPrms_init(instId, &initPrms);
    initPrms.printFxn = (Udma_PrintFxn) & System_printf;
    retVal = Udma_init(hUdmaDrv, &initPrms);

    /* localAssert if UDMA failed to open */
    localAssert(UDMA_SOK == retVal);

    return hUdmaDrv;
}

static void CpswRemoteApp_setRxFlowPrms(EnetUdma_OpenRxFlowPrms *pRxFlowPrms,
                                      uint32_t rxStartFlowIdx,
                                      uint32_t rxFlowIdx,
                                      Udma_DrvHandle hUdmaDrv,
                                      uint32_t numRxPackets,
                                      void *cbArg,
                                      EnetDma_PktNotifyCb eventCb,
                                      uint32_t rxFlowMtu)
{
    pRxFlowPrms->startIdx = rxStartFlowIdx;
    pRxFlowPrms->flowIdx = rxFlowIdx;

    pRxFlowPrms->hUdmaDrv = hUdmaDrv;

    pRxFlowPrms->ringMemAllocFxn = &EnetMem_allocRingMem;
    pRxFlowPrms->ringMemFreeFxn = &EnetMem_freeRingMem;

    pRxFlowPrms->notifyCb = eventCb;

    pRxFlowPrms->numRxPkts = numRxPackets;

    pRxFlowPrms->disableCacheOpsFlag = false;
    pRxFlowPrms->dmaDescAllocFxn = &EnetMem_allocDmaDesc;
    pRxFlowPrms->dmaDescFreeFxn = &EnetMem_freeDmaDesc;
    pRxFlowPrms->cbArg = cbArg;
    pRxFlowPrms->useProxy = false;
    pRxFlowPrms->rxFlowMtu = rxFlowMtu;
}

static void CpswRemoteApp_openNDKRxCh(CpswProxy_Handle hProxy,
                                    Enet_Handle hEnet,
                                    Udma_DrvHandle hUdmaDrv,
                                    uint32_t coreKey,
                                    bool useDefaultFlow,
                                    uint32_t rxFlowStartIdx,
                                    uint32_t rxFlowIdx,
                                    uint8_t *macAddress,
                                    NimuEnetAppIf_RxConfig *rxConfig,
                                    NimuEnetAppIf_RxHandleInfo *rxHandleInfo,
                                    uint32_t rxFlowMtu)
{
    EnetUdma_OpenRxFlowPrms cpswRxFlowCfg;

    rxHandleInfo->rxFlowStartIdx = rxFlowStartIdx;
    rxHandleInfo->rxFlowIdx = rxFlowIdx;
    ENET_UTILS_ARRAY_COPY(rxHandleInfo->macAddr, macAddress);

    EnetDma_initRxChParams(&cpswRxFlowCfg);

    CpswRemoteApp_setRxFlowPrms(&cpswRxFlowCfg,
                              rxHandleInfo->rxFlowStartIdx,
                              rxHandleInfo->rxFlowIdx,
                              hUdmaDrv,
                              rxConfig->numPackets,
                              rxConfig->cbArg,
                              rxConfig->notifyCb,
                              rxFlowMtu);

    rxHandleInfo->hRxFlow = EnetDma_openRxCh(gRemoteAppObj.hDma, &cpswRxFlowCfg);
    localAssert(rxHandleInfo->hRxFlow != NULL);

    CpswProxy_addHostPortEntry(hProxy, hEnet, coreKey, rxHandleInfo->macAddr);
    if (useDefaultFlow)
    {
        CpswProxy_registerDefaultRxFlow(hProxy,
                                        hEnet,
                                        coreKey,
                                        rxHandleInfo->rxFlowStartIdx,
                                        rxHandleInfo->rxFlowIdx);
    }
    else
    {
        CpswProxy_registerDstMacRxFlow(hProxy,
                                       hEnet,
                                       coreKey,
                                      rxHandleInfo->rxFlowStartIdx,
                                       rxHandleInfo->rxFlowIdx,
                                       rxHandleInfo->macAddr);
    }
}

static void CpswRemoteApp_closeNDKRxCh(CpswProxy_Handle hProxy,
                                     Enet_Handle hEnet,
                                     Udma_DrvHandle hUdmaDrv,
                                     uint32_t coreKey,
                                     bool useDefaultFlow,
                                     uint8_t *ipV4Addr,
                                     NimuEnetAppIf_RxHandleInfo *rxHandleInfo,
                                     void *freeFxnArg,
                                     NimuEnetAppIf_FreePktCbFxn freeFxn)
{
    EnetDma_PktQ fqPktInfoQ;
    EnetDma_PktQ cqPktInfoQ;
    int32_t status;

    EnetQueue_initQ(&fqPktInfoQ);
    EnetQueue_initQ(&cqPktInfoQ);

    EnetDma_disableRxEvent(rxHandleInfo->hRxFlow);

    CpswProxy_unregisterIPV4Addr(hProxy,
                                hEnet,
                                coreKey,
                                ipV4Addr);
    if (useDefaultFlow)
    {
        CpswProxy_unregisterDefaultRxFlow(hProxy,
                                      hEnet,
                                      coreKey,
                                      rxHandleInfo->rxFlowStartIdx,
                                      rxHandleInfo->rxFlowIdx);
    }
    else
    {
        CpswProxy_unregisterDstMacRxFlow(hProxy,
                                     hEnet,
                                     coreKey,
                                     rxHandleInfo->rxFlowStartIdx,
                                     rxHandleInfo->rxFlowIdx,
                                       rxHandleInfo->macAddr);
    }

    CpswProxy_delAddrEntry(hProxy, hEnet, coreKey, rxHandleInfo->macAddr);
    status = EnetDma_closeRxCh(rxHandleInfo->hRxFlow,
                               &fqPktInfoQ,
                               &cqPktInfoQ);
    localAssert(status == ENET_SOK);
    CpswProxy_freeMac(hProxy,
                      hEnet,
                      coreKey,
                      rxHandleInfo->macAddr);
    CpswProxy_freeRxFlow(hProxy,
                         hEnet,
                         coreKey,
                         rxHandleInfo->rxFlowStartIdx,
                         rxHandleInfo->rxFlowIdx);
    freeFxn(freeFxnArg, &fqPktInfoQ, &cqPktInfoQ);
}

static void CpswRemoteApp_setTxChPrms(EnetUdma_OpenTxChPrms *pTxChPrms,
                                    uint32_t txChNum,
                                    Udma_DrvHandle hUdmaDrv,
                                    uint32_t numTxPackets,
                                    void *cbArg,
                                    EnetDma_PktNotifyCb eventCb)
{
    pTxChPrms->chNum = txChNum;
    pTxChPrms->hUdmaDrv = hUdmaDrv;

    pTxChPrms->ringMemAllocFxn = &EnetMem_allocRingMem;
    pTxChPrms->ringMemFreeFxn = &EnetMem_freeRingMem;

    pTxChPrms->numTxPkts = numTxPackets;
    pTxChPrms->disableCacheOpsFlag = false;

    pTxChPrms->dmaDescAllocFxn = &EnetMem_allocDmaDesc;
    pTxChPrms->dmaDescFreeFxn = &EnetMem_freeDmaDesc;

    pTxChPrms->cbArg = cbArg;

    pTxChPrms->notifyCb = eventCb;
}

static void CpswRemoteApp_openNDKTxCh(Udma_DrvHandle hUdmaDrv,
                                    uint32_t coreKey,
                                    uint32_t txPSILId,
                                    NimuEnetAppIf_TxConfig *txConfig,
                                    NimuEnetAppIf_TxHandleInfo *txHandleInfo)
{
    EnetUdma_OpenTxChPrms cpswTxChCfg;

    txHandleInfo->txChNum = txPSILId;

    /* Set configuration parameters */
    EnetDma_initTxChParams(&cpswTxChCfg);

    CpswRemoteApp_setTxChPrms(&cpswTxChCfg,
                              txHandleInfo->txChNum,
                              hUdmaDrv,
                              txConfig->numPackets,
                              txConfig->cbArg,
                              txConfig->notifyCb);

    txHandleInfo->hTxChannel = EnetDma_openTxCh(gRemoteAppObj.hDma, &cpswTxChCfg);
    localAssert(NULL != txHandleInfo->hTxChannel);
}

static void CpswRemoteApp_closeNDKTxCh(CpswProxy_Handle hProxy,
                                     Enet_Handle hEnet,
                                     Udma_DrvHandle hUdmaDrv,
                                     uint32_t coreKey,
                                     NimuEnetAppIf_TxHandleInfo *txHandleInfo,
                                     void *freeFxnArg,
                                     NimuEnetAppIf_FreePktCbFxn freeFxn)
{
    EnetDma_PktQ fqPktInfoQ;
    EnetDma_PktQ cqPktInfoQ;
    int32_t status;

    EnetQueue_initQ(&fqPktInfoQ);
    EnetQueue_initQ(&cqPktInfoQ);

    EnetDma_disableTxEvent(txHandleInfo->hTxChannel);
    status = EnetDma_closeTxCh(txHandleInfo->hTxChannel, &fqPktInfoQ, &cqPktInfoQ);
    localAssert(ENET_SOK == status);

    CpswProxy_freeTxCh(hProxy,
                       hEnet,
                       coreKey,
                       txHandleInfo->txChNum);
    freeFxn(freeFxnArg, &fqPktInfoQ, &cqPktInfoQ);
}

static uint64_t CpswRemoteApp_virtToPhyFxn(const void *virtAddr,
                                         void *appData)
{
    return((uint64_t)virtAddr);
}

static void *CpswRemoteApp_phyToVirtFxn(uint64_t phyAddr,
                                      void *appData)
{
#if defined(__aarch64__)
    uint64_t temp = phyAddr;
#else
    /* R5 is 32-bit machine, need to truncate to avoid void * typecast error */
    uint32_t temp = (uint32_t)phyAddr;
#endif
    return((void *)temp);
}

static EnetDma_Handle CpswRemoteApp_initCpswDma(Enet_Type enetType,
                                              Udma_DrvHandle hUdmaDrv)
{
    EnetDma_initCfg dmaCfg;
     EnetDma_Handle cpswDmaHandle = NULL;

    EnetUdma_initDataPathParams(&dmaCfg);
    dmaCfg.hUdmaDrv = hUdmaDrv;
    cpswDmaHandle = EnetUdma_initDataPath(enetType,
                                          0 /* instId */,
                                          &dmaCfg);
    return cpswDmaHandle;
}

static int32_t CpswRemoteApp_deinitCpswDma(EnetDma_Handle cpswDmaHandle)
{
    int32_t status;

    status = EnetUdma_deInitDataPath(cpswDmaHandle);
    return status;
}

static CpswProxy_Handle CpswRemoteApp_initCpswProxy(void)
{
     CpswProxy_Config proxyConfig;
     CpswProxy_Handle hProxy;

     strncpy(proxyConfig.device_name, ETHREMOTEDEVICE_DEVICE_NAME_MCU_2_1, (sizeof(proxyConfig.device_name) - 1));
     proxyConfig.device_name[(sizeof(proxyConfig.device_name) - 1)] = 0;
     proxyConfig.deviceDataNotifyCb = &printDevInfo;
     proxyConfig.masterCoreId = IPC_MCU2_0;
     proxyConfig.rpmsgEndPointId = REMOTE_DEVICE_ENDPT;

     hProxy = CpswProxy_init(&proxyConfig);
     localAssert(hProxy != NULL);

     return hProxy;
}


void NimuEnetAppCb_getHandle(NimuEnetAppIf_GetHandleInArgs *inArgs,
                             NimuEnetAppIf_GetHandleOutArgs *outArgs)
{
     int32_t status;
    uint32_t coreId = EnetSoc_getCoreId();
    EnetOsal_Cfg osalPrms;
    EnetUtils_Cfg utilsPrms;
#if defined(SOC_J721E)
    Enet_Type enetType = ENET_CPSW_9G;
#elif defined(SOC_J7200)
    Enet_Type enetType = ENET_CPSW_5G;
#endif
    uint32_t txPSILId;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;

    localAssert(gRemoteAppObj.hCpswProxy != NULL);

    /* Initialize CPSW driver with default OSAL, utils */
    utilsPrms.print = (Enet_Print)System_printf;
    utilsPrms.physToVirt = &CpswRemoteApp_phyToVirtFxn;
    utilsPrms.virtToPhys = &CpswRemoteApp_virtToPhyFxn;

    Enet_initOsalCfg(&osalPrms);

    Enet_init(&osalPrms, &utilsPrms);

    status = EnetMem_init();
    localAssert(status == ENET_SOK);
    outArgs->coreId = coreId;
    outArgs->hUdmaDrv = CpswRemoteApp_udmaOpen();
    outArgs->print = (Enet_Print) & ConPrintf;
    outArgs->isPortLinkedFxn = &CpswRemoteApp_isAllPortLinked;
    outArgs->isRingMonUsed = false;
    outArgs->timerPeriodUs = CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US;
    /* Let NIMU use optimized processing where TX packets are relinquished in next
     * TX submit call */
    outArgs->disableTxEvent = true;

    gRemoteAppObj.hDma = CpswRemoteApp_initCpswDma(enetType, outArgs->hUdmaDrv);

    if (gRemoteAppObj.useExtAttach)
    {
        CpswProxy_attachExtended(gRemoteAppObj.hCpswProxy,
                                 enetType,
                                 &outArgs->hEnet,
                                 &outArgs->coreKey,
                                 &outArgs->hostPortRxMtu,
                                 outArgs->txMtu,
                                 &txPSILId,
                                 &rxStartFlowId,
                                 &rxFlowIdOffset,
                                 gRemoteAppObj.macAddr);
    }
    else
    {
        CpswProxy_attach(gRemoteAppObj.hCpswProxy,
                         enetType,
                         &outArgs->hEnet,
                         &outArgs->coreKey,
                         &outArgs->hostPortRxMtu,
                         outArgs->txMtu);
        CpswProxy_allocTxCh(gRemoteAppObj.hCpswProxy,
                            outArgs->hEnet,
                            outArgs->coreKey,
                            &txPSILId);
        CpswProxy_allocRxFlow(gRemoteAppObj.hCpswProxy,
                              outArgs->hEnet,
                              outArgs->coreKey,
                              &rxStartFlowId,
                              &rxFlowIdOffset);
        CpswProxy_allocMac(gRemoteAppObj.hCpswProxy,
                           outArgs->hEnet,
                           outArgs->coreKey,
                           gRemoteAppObj.macAddr);
    }

    CpswRemoteApp_openNDKTxCh(outArgs->hUdmaDrv,
                             outArgs->coreKey,
                             txPSILId,
                             &inArgs->txCfg,
                             &outArgs->txInfo);

    CpswRemoteApp_openNDKRxCh(gRemoteAppObj.hCpswProxy,
                             outArgs->hEnet,
                             outArgs->hUdmaDrv,
                             outArgs->coreKey,
                             gRemoteAppObj.useDefaultRxFlow,
                             rxStartFlowId,
                             rxFlowIdOffset,
                             gRemoteAppObj.macAddr,
                             &inArgs->rxCfg,
                             &outArgs->rxInfo,
                             outArgs->hostPortRxMtu);
    gRemoteAppObj.coreKey = outArgs->coreKey;
    gRemoteAppObj.hEnet = outArgs->hEnet;
}

void NimuEnetAppCb_releaseHandle(NimuEnetAppIf_ReleaseHandleInfo *releaseInfo)
{
    localAssert(gRemoteAppObj.hCpswProxy != NULL);

    CpswRemoteApp_closeNDKTxCh(gRemoteAppObj.hCpswProxy,
                               releaseInfo->hEnet,
                               releaseInfo->hUdmaDrv,
                               releaseInfo->coreKey,
                               &releaseInfo->txInfo,
                               releaseInfo->freePktCbArg,
                               releaseInfo->txFreePktCb);
    CpswRemoteApp_closeNDKRxCh(gRemoteAppObj.hCpswProxy,
                               releaseInfo->hEnet,
                               releaseInfo->hUdmaDrv,
                               releaseInfo->coreKey,
                               gRemoteAppObj.useDefaultRxFlow,
                               gRemoteAppObj.ipv4Addr,
                               &releaseInfo->rxInfo,
                               releaseInfo->freePktCbArg,
                               releaseInfo->rxFreePktCb);

    CpswProxy_detach(gRemoteAppObj.hCpswProxy, releaseInfo->hEnet, releaseInfo->coreKey);
    CpswRemoteApp_deinitCpswDma(gRemoteAppObj.hDma);
}
