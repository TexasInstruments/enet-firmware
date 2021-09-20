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

#if defined (SYSBIOS)
#include <ti/drv/enet/nimuenet/nimu_ndk.h>
#include <ti/drv/enet/nimuenet/ndk2enet_appif.h>
#elif defined (FREERTOS)
#include <ti/drv/enet/lwipif/inc/lwipif2enet_appif.h>
#include <ti/drv/enet/lwipif/inc/lwip2lwipif.h>
#endif
#include <ti/drv/enet/examples/utils/include/enet_appsoc.h>
#include <ti/drv/enet/examples/utils/include/enet_ethutils.h>
#include <ti/drv/enet/examples/utils/include/enet_appmemutils_cfg.h>
#include <ti/drv/enet/examples/utils/include/enet_appmemutils.h>

#if defined (SYSBIOS)
/* NDK headers */
#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/ndk/inc/socket.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/tools/servers.h>
#include <ti/ndk/inc/tools/console.h>
#else /* FREERTOS */
/* lwIP core includes */
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/api.h"

#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/dhcp.h"

/* lwIP netif includes */
#include "lwip/etharp.h"
#include "netif/ethernet.h"

#include <ti/drv/enet/lwipif/inc/default_netif.h>
#endif

#if defined (FREERTOS)
#define System_printf printf
#define System_vprintf vprintf
#endif

#if defined(FREERTOS)
#define CPSW_REMOTE_APP_REMOTE_NETIF_MAX      (2U)
#else
#define CPSW_REMOTE_APP_REMOTE_NETIF_MAX      (1U)
#endif

#define CPSW_REMOTE_APP_MASTER_CORE_ID        (IPC_MCU2_0)
#define CPSW_REMOTE_APP_MASTER_ENDPT          (26)

#define CPSW_REMOTE_APP_PHY_POLLING_INTERVAL  (100)
#define CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US (1000U)
#define CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL   (30U)
#define CPSW_REMOTE_APP_CPTS_HW_PUSH_NUM      (2U)

#define IPC_RPMESSAGE_OBJ_SIZE  (256)
#define VQ_TIMEOUT              (100)
#define VQ_BUF_SIZE             (2048)
#define RPMSG_DATA_SIZE         (256 * 512 + IPC_RPMESSAGE_OBJ_SIZE)

#if defined (FREERTOS)
#define ETHAPP_LWIP_TASK_STACKSIZE      (4U * 1024U)

/* lwIP features that EthFw relies on */
#ifndef LWIP_IPV4
#error "LWIP_IPV4 is not enabled"
#endif
#ifndef LWIP_NETIF_STATUS_CALLBACK
#error "LWIP_NETIF_STATUS_CALLBACK is not enabled"
#endif
#ifndef LWIP_NETIF_LINK_CALLBACK
#error "LWIP_NETIF_LINK_CALLBACK is not enabled"
#endif

/* DHCP or static IP */
#define ETHAPP_LWIP_USE_DHCP            (1)
#if !ETHAPP_LWIP_USE_DHCP
#define ETHFW_CLIENT_IPADDR(addr)       IP4_ADDR((addr), 192,168,1,201)
#define ETHFW_CLIENT_GW(addr)           IP4_ADDR((addr), 192,168,1,1)
#define ETHFW_CLIENT_NETMASK(addr)      IP4_ADDR((addr), 255,255,255,0)
#endif
#endif

#if defined(FREERTOS)
static uint8_t gEthAppLwipStackBuf[ETHAPP_LWIP_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__((aligned(32)));
#endif

static uint8_t g_monitorStackBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
;

static uint8_t g_rdevStackBuf[IPC_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
;

static uint8_t g_initTaskStackBuf[IPC_TASK_STACKSIZE]
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

#if defined (SYSBIOS)
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
#endif /* defined (SYSBIOS) */

typedef struct CpswRemoteApp_SyncTimerObj_s
{
    uint64_t currLocalTime;
    uint64_t prevLocalTime;
    uint64_t currCptsTime;
    uint64_t prevCptsTime;
    double rate;
    double offset;
} CpswRemoteApp_SyncTimerObj;

typedef struct CpswRemoteApp_VirtNetif_s
{
    /* CpswProxy handle used to communicate with EthFw */
    CpswProxy_Handle hCpswProxy;

    /* Virtual port id */
    EthRemoteCfg_VirtPort virtPort;

    /* MAC ports used by this client app */
    Enet_MacPort *macPorts;

    /* Number of MAC ports */
    uint32_t numMacPorts;

    /* MAC address */
    uint8_t macAddr[ENET_MAC_ADDR_LEN];

    /* IPv4 address */
    uint8_t ipv4Addr[ENET_IPv4_ADDR_LEN];

#if defined(FREERTOS)
    /* lwIP network interface */
    struct netif netif;

    /* DHCP network interface */
    struct dhcp dhcpNetif;

    /* Whether this netif should be set as the default netif */
    bool isDfltNetif;
#endif

    /* Time synchronization object */
    CpswRemoteApp_SyncTimerObj syncTimerObj;

    /* CPTS HWPUSH number used for remote time synchronization. This example
     * app only supports it on one interface, should be set to CPSW_CPTS_HWPUSH_INVALID
     * on all other interfaces */
    CpswCpts_HwPush hwPushNum;
} CpswRemoteApp_VirtNetif;

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

    /* Enet LLD handle */
    Enet_Handle hEnet;

    /* Enet LLD DMA handle */
    EnetDma_Handle hEnetDma;

    /* Core id used for Enet LLD APIs */
    uint32_t coreId;

    /* coreKey used for Enet LLD APIs */
    uint32_t coreKey;

    /* Whether to use default flow or not */
    bool useDefaultRxFlow;

    /* Whether to use extended attach remote command or not */
    bool useExtAttach;

    /* Virtual network interface data */
    CpswRemoteApp_VirtNetif virtNetif[CPSW_REMOTE_APP_REMOTE_NETIF_MAX];

#if defined(SYSBIOS)
    /* Semaphore for synchronizing EthFw and NDK initialization */
    SemaphoreP_Handle hInitSem;
#endif
} CpswRemoteApp_Obj;

/* Link status on these ports will be used to determine link up on virtual switch port */
static Enet_MacPort gRemoteAppMacPorts[] =
{
    ENET_MAC_PORT_3,
};

#if (CPSW_REMOTE_APP_REMOTE_NETIF_MAX >= 2)
/* Link status on these ports will be used to determine link up on virtual MAC port */
static Enet_MacPort gRemoteApp_virtualMacPorts[] =
{
    ENET_MAC_PORT_4,
};
#endif

CpswRemoteApp_Obj gRemoteAppObj =
{
    .hUdmaDrv         = NULL,
#if defined(SOC_J721E)
    .enetType         = ENET_CPSW_9G,
    .instId           = 0U,
#elif defined(SOC_J7200)
    .enetType         = ENET_CPSW_5G,
    .instId           = 0U,
#endif
    .hEnet            = NULL,
    .hEnetDma         = NULL,
    .coreKey          = ENET_RM_INVALIDCORE,
    .useDefaultRxFlow = false,
    .useExtAttach     = true,
    .virtNetif        =
    {
        {
            .hCpswProxy  = NULL,
            .virtPort    = ETHREMOTECFG_SWITCH_PORT_1,
            .macPorts    = gRemoteAppMacPorts,
            .numMacPorts = ENET_ARRAYSIZE(gRemoteAppMacPorts),
            .hwPushNum   = CPSW_CPTS_HWPUSH_2,
#if defined (FREERTOS)
            .isDfltNetif = true,
#endif
        },
#if (CPSW_REMOTE_APP_REMOTE_NETIF_MAX >= 2)
        {
            .hCpswProxy  = NULL,
            .virtPort    = ETHREMOTECFG_MAC_PORT_4,
            .macPorts    = gRemoteApp_virtualMacPorts,
            .numMacPorts = ENET_ARRAYSIZE(gRemoteApp_virtualMacPorts),
            .hwPushNum   = CPSW_CPTS_HWPUSH_INVALID,
#if defined (FREERTOS)
            .isDfltNetif = false,
#endif
        },
#endif
    },
};

static uint64_t CpswRemoteApp_virtToPhysFxn(const void *virtAddr,
                                            void *appData);

static void *CpswRemoteApp_physToVirtFxn(uint64_t phyAddr,
                                         void *appData);

static int32_t CpswRemoteApp_openUdma(void);

static int32_t CpswRemoteApp_openEnet(void);

static void CpswRemoteApp_openCpswProxy(CpswRemoteApp_VirtNetif *virtNetif);

#if defined (SYSBIOS)
char *VerStr = "NIMU CPSW Example";
#elif defined (FREERTOS)
char *VerStr = "LWIP CPSW Example";
#endif

static void CpswRemoteApp_initSyncTimer(CpswRemoteApp_VirtNetif *virtNetif);

static uint64_t CpswRemoteApp_getLocalTime(void);

static uint64_t CpswRemoteApp_getSynchronizedTime(CpswRemoteApp_SyncTimerObj *hSyncTimerObj);

static void CpswRemoteApp_calcSyncTimeParams(CpswCpts_HwPush hwPushNum,
                                             uint64_t syncTime,
                                             void *cbArg);

#if defined (FREERTOS)
static void EthApp_lwipMain(void *a0,
                            void *a1);

static void EthApp_initLwip(void *arg);

static void EthApp_initNetif(CpswRemoteApp_VirtNetif *virtNetif);

static void EthApp_netifStatusCb(struct netif *netif);
#endif

// hack for release mode build fix TODO fix this
void localAssert(bool cond)
{
#if defined(__KLOCWORK__)
    if (!cond)
    {
        abort();
    }
#else
#if defined (SYSBIOS)
    assert(cond);
#elif defined (FREERTOS)
    // TODO: Need to add support
#endif
#endif
}

#if defined (SYSBIOS)
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
    CpswRemoteApp_VirtNetif *virtNetif = &gRemoteAppObj.virtNetif[0];
    volatile uint32_t ipAddrHex = 0U;
    char ipAddr[20];

    ipAddrHex = NDK_ntohl(IPAddr);
    virtNetif->ipv4Addr[0] = (uint8_t)(ipAddrHex >> 24) & 0xFF;
    virtNetif->ipv4Addr[1] = (uint8_t)(ipAddrHex >> 16) & 0xFF;
    virtNetif->ipv4Addr[2] = (uint8_t)(ipAddrHex >> 8) & 0xFF;
    virtNetif->ipv4Addr[3] = (uint8_t)(ipAddrHex & 0xFF);
    snprintf(ipAddr, 17, "%d.%d.%d.%d\n",
             virtNetif->ipv4Addr[0],
             virtNetif->ipv4Addr[1],
             virtNetif->ipv4Addr[2],
             virtNetif->ipv4Addr[3]);

    localAssert((virtNetif->hCpswProxy != NULL) && (gRemoteAppObj.hEnet != NULL));

    CpswProxy_registerIPV4Addr(virtNetif->hCpswProxy,
                               gRemoteAppObj.hEnet,
                               gRemoteAppObj.coreKey,
                               virtNetif->macAddr,
                               virtNetif->ipv4Addr);

    System_printf("\nCPSW NIMU application, IP address I/F 1: %s\n\r", ipAddr);
    CpswRemoteApp_initSyncTimer(virtNetif);
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
#endif  /*defined (SYSBIOS) */

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

static void CpswRemoteApp_initSyncTimer(CpswRemoteApp_VirtNetif *virtNetif)
{
    int32_t status;

    memset(&virtNetif->syncTimerObj, 0, sizeof(CpswRemoteApp_SyncTimerObj));

    /* Make sure GTC is disabled before configuring timesync router*/
    CSL_REG32_WR(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCR, 0x0U);

    /* Register callback */
    status = CpswProxy_registerHwPushNotifyCb(CpswRemoteApp_calcSyncTimeParams,
                                              (void *)virtNetif);
    if (status == ENET_EALREADYOPEN)
    {
        System_printf("CpswProxy_registerHwPushNotifyCb(): Callback is registered already\n");
    }

    /* Configure GTC push event */
    CSL_REG32_WR(CSL_GTC0_GTC_CFG0_BASE + CSL_GTC_CFG0_PUSHEVT,
                 CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL);

    /* Send request to Ethfw to configure TSR */
    CpswProxy_registerRemoteTimer(virtNetif->hCpswProxy,
                                  gRemoteAppObj.hEnet,
                                  gRemoteAppObj.coreKey,
                                  CSLR_TIMESYNC_INTRTR0_IN_GTC0_GTC_PUSH_EVENT_0,
                                  (uint8_t)virtNetif->hwPushNum);

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

static uint64_t CpswRemoteApp_getSynchronizedTime(CpswRemoteApp_SyncTimerObj *hSyncTimerObj)
{
    uint64_t gtcTime = 0U, synchronizedTime = 0U;

    /* Get GTC time */
    gtcTime = CpswRemoteApp_getLocalTime();

    /* Compute synchronized time from GTC time */
    synchronizedTime = (uint64_t)((hSyncTimerObj->rate * (double)gtcTime) + hSyncTimerObj->offset);

    return synchronizedTime;
}

static void CpswRemoteApp_calcSyncTimeParams(CpswCpts_HwPush hwPushNum,
                                             uint64_t syncTime,
                                             void *cbArg)
{
    CpswRemoteApp_VirtNetif *virtNetif = (CpswRemoteApp_VirtNetif *)cbArg;

    if (hwPushNum == virtNetif->hwPushNum)
    {
        uint64_t gtcTime = 0U;
        uint64_t synchronizedTime = 0U;
        CpswRemoteApp_SyncTimerObj *hSyncTimerObj = &virtNetif->syncTimerObj;
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

            synchronizedTime = CpswRemoteApp_getSynchronizedTime(hSyncTimerObj);
            System_printf("Current Synchronized time via HWPUSH_%u in Epoch format: %lld\n",
                          hwPushNum, synchronizedTime);
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
    System_printf("ETHFW PermissionFlag:0x%x, UART Connected:%s,UART Id:%d\n",
                  ethDevData->permission_flags,
                  tf[ethDevData->uart_connected],
                  ethDevData->uart_id);
}

static void CpswRemoteApp_initTask(void* a0,
                                   void* a1)
{
    TaskP_Params params;
    uint32_t numProc = gNumRemoteProc;
    Ipc_VirtIoParams vqParam;
    Ipc_InitPrms initPrms;
    RPMessage_Params cntrlParam;
    int32_t status;
    uint32_t i;

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
    CpswProxy_init(CPSW_REMOTE_APP_MASTER_CORE_ID,
                   CPSW_REMOTE_APP_MASTER_ENDPT);

    for (i = 0U; i < ENET_ARRAYSIZE(gRemoteAppObj.virtNetif); i++)
    {
        CpswRemoteApp_openCpswProxy(&gRemoteAppObj.virtNetif[i]);
    }

#if defined(FREERTOS)
    /* Step 6: Initialize lwIP */
    TaskP_Params_init(&params);
    params.priority  = DEFAULT_THREAD_PRIO;
    params.stack     = &gEthAppLwipStackBuf[0];
    params.stacksize = sizeof(gEthAppLwipStackBuf);
    params.name      = "lwIP main loop";

    TaskP_create(EthApp_lwipMain, &params);
#else
    /* Step 6: Post semaphore so that NDK/NIMU can continue with their initialization */
    SemaphoreP_post(gRemoteAppObj.hInitSem);
#endif
}

int main(void)
{
    TaskP_Handle task;
    TaskP_Params taskParams;
#if defined (SYSBIOS)
    SemaphoreP_Params semParams;
#endif
    int32_t status;

    /* Set ccsHaltFlag to 1 for halting core for CCS connection */
    volatile uint32_t ccsHaltFlag = 0U;

    while (ccsHaltFlag)
    {
        ;
    }

#if defined(SYSBIOS)
    /* Currently, there is no control over NDK initialization time and its
     * task runs right away after OS_start() which can cause a race condition
     * if NDK/NIMU is started before connection with EthFw is complete.
     * A semaphore is created to synchronize connection with EthFw and NDK
     * initialization.  Essentially, NIMU's getHandle() would block until
     * we signal that the CpswProxy initialization is done. */
    SemaphoreP_Params_init(&semParams);
    semParams.mode = SemaphoreP_Mode_BINARY;
    gRemoteAppObj.hInitSem = SemaphoreP_create(0, &semParams);
#endif

    /* Init UDMA LLD based on NAVSS instance */
    status = CpswRemoteApp_openUdma();
    if (status != UDMA_SOK)
    {
        appLogPrintf("main() failed to open UDMA LLD: %d\n", status);
        localAssert(status == UDMA_SOK);
    }

    /* Init Enet LLD and open Enet DMA */
    status = CpswRemoteApp_openEnet();
    if (status != ENET_SOK)
    {
        appLogPrintf("main() failed to open Enet LLD: %d\n", status);
        localAssert(status == ENET_SOK);
    }

    TaskP_Params_init(&taskParams);
    taskParams.priority = 2;
    taskParams.stack = &g_initTaskStackBuf[0];
    taskParams.stacksize = sizeof(g_initTaskStackBuf);
    task = TaskP_create(CpswRemoteApp_initTask, &taskParams);

    if (NULL == task)
    {
        OS_stop();
    }

    OS_start();    /* does not return */

    return(0);
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
        appLogPrintf("CpswRemoteApp_openUdma() failed init UDMA driver: %d\n", status);
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
    if (status != ENET_SOK)
    {
        appLogPrintf("CpswRemoteApp_openEnet() failed to init memutils: %d\n", status);
    }

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
            appLogPrintf("CpswRemoteApp_openEnet() failed to init Enet LLD data path\n");
            EnetMem_deInit();
            status = ENET_EFAIL;
        }
        else
        {
            gRemoteAppObj.hEnetDma = hEnetDma;
            gRemoteAppObj.coreId   = EnetSoc_getCoreId();
        }
    }

    return status;
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
    pRxFlowPrms->useGlobalEvt = true;
    pRxFlowPrms->useProxy = false;
    pRxFlowPrms->rxFlowMtu = rxFlowMtu;
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
    pTxChPrms->useGlobalEvt = true;
}

static void CpswRemoteApp_openCpswProxy(CpswRemoteApp_VirtNetif *virtNetif)
{
     CpswProxy_Config proxyConfig;
     CpswProxy_Handle hProxy;

     proxyConfig.virtPort = virtNetif->virtPort;
     proxyConfig.deviceDataNotifyCb = &printDevInfo;

     hProxy = CpswProxy_open(&proxyConfig);
     if (hProxy != NULL)
     {
         virtNetif->hCpswProxy = hProxy;
     }
     else
     {
         System_printf("Failed to open CpswProxy for virtPortId %u\n", virtNetif->virtPort);
         localAssert(hProxy != NULL);
     }
}

#if defined (FREERTOS)
static void EthApp_lwipMain(void *a0,
                            void *a1)
{
    err_t err;
    sys_sem_t initSem;

    /* initialize lwIP stack and network interfaces */
    err = sys_sem_new(&initSem, 0);
    LWIP_ASSERT("failed to create initSem", err == ERR_OK);
    LWIP_UNUSED_ARG(err);

    tcpip_init(EthApp_initLwip, &initSem);

    /* we have to wait for initialization to finish before
     * calling update_adapter()! */
    sys_sem_wait(&initSem);
    sys_sem_free(&initSem);

#if (LWIP_SOCKET || LWIP_NETCONN) && LWIP_NETCONN_SEM_PER_THREAD
    netconn_thread_init();
#endif
}

static void EthApp_initLwip(void *arg)
{
    sys_sem_t *initSem;
    uint32_t i;

    LWIP_ASSERT("arg != NULL", arg != NULL);
    initSem = (sys_sem_t*)arg;

    /* init randomizer again (seed per thread) */
    srand((unsigned int)sys_now()/1000);

    /* init network interfaces */
    for (i = 0U; i < ENET_ARRAYSIZE(gRemoteAppObj.virtNetif); i++)
    {
        EthApp_initNetif(&gRemoteAppObj.virtNetif[i]);
    }

    sys_sem_signal(initSem);
}

static void EthApp_initNetif(CpswRemoteApp_VirtNetif *virtNetif)
{
    struct netif *netif = &virtNetif->netif;
    ip4_addr_t ipaddr, netmask, gw;
#if ETHAPP_LWIP_USE_DHCP
    err_t err;
#endif

    ip4_addr_set_zero(&gw);
    ip4_addr_set_zero(&ipaddr);
    ip4_addr_set_zero(&netmask);

#if ETHAPP_LWIP_USE_DHCP
    appLogPrintf("Starting lwIP, local interface IP is dhcp-enabled\n");
#else /* ETHAPP_LWIP_USE_DHCP */
    ETHFW_CLIENT_GW(&gw);
    ETHFW_CLIENT_IPADDR(&ipaddr);
    ETHFW_CLIENT_NETMASK(&netmask);
    appLogPrintf("Starting lwIP, local interface IP is %s\n", ip4addr_ntoa(&ipaddr));
#endif /* ETHAPP_LWIP_USE_DHCP */

    netif_add(netif, &ipaddr, &netmask, &gw, NULL, LWIPIF_LWIP_init, tcpip_input);

    if (virtNetif->isDfltNetif)
    {
        netif_set_default(netif);
    }

    netif_set_status_callback(netif, EthApp_netifStatusCb);

    dhcp_set_struct(netif, &virtNetif->dhcpNetif);

    netif_set_up(netif);

#if ETHAPP_LWIP_USE_DHCP
    err = dhcp_start(netif);
    if (err != ERR_OK)
    {
        appLogPrintf("Failed to start DHCP: %d\n", err);
    }
#endif /* ETHAPP_LWIP_USE_DHCP */
}

static void EthApp_netifStatusCb(struct netif *netif)
{
    CpswRemoteApp_VirtNetif *virtNetif;

    virtNetif = container_of(netif, CpswRemoteApp_VirtNetif, netif);

    if (netif_is_up(netif))
    {
        const ip4_addr_t *ipAddr = netif_ip4_addr(netif);

        appLogPrintf("Added interface '%c%c%d', IP is %s\n",
                     netif->name[0], netif->name[1], netif->num, ip4addr_ntoa(ipAddr));

        if (ipAddr->addr != 0)
        {
            virtNetif->ipv4Addr[0] = ip4_addr1_val(*ipAddr);
            virtNetif->ipv4Addr[1] = ip4_addr2_val(*ipAddr);
            virtNetif->ipv4Addr[2] = ip4_addr3_val(*ipAddr);
            virtNetif->ipv4Addr[3] = ip4_addr4_val(*ipAddr);

            localAssert(virtNetif->hCpswProxy != NULL);
            localAssert(gRemoteAppObj.hEnet != NULL);

            if (EthRemoteCfg_isSwitchPort(virtNetif->virtPort))
            {
                CpswProxy_registerIPV4Addr(virtNetif->hCpswProxy,
                                           gRemoteAppObj.hEnet,
                                           gRemoteAppObj.coreKey,
                                           virtNetif->macAddr,
                                           virtNetif->ipv4Addr);
            }

            /* Init time synchronization */
            if (virtNetif->hwPushNum != CPSW_CPTS_HWPUSH_INVALID)
            {
                CpswRemoteApp_initSyncTimer(virtNetif);
            }
        }
    }
    else
    {
        appLogPrintf("Removed interface '%c%c%d'\n", netif->name[0], netif->name[1], netif->num);
    }
}

static void CpswRemoteApp_openLwipRxCh(CpswProxy_Handle hProxy,
                                       Enet_Handle hEnet,
                                       Udma_DrvHandle hUdmaDrv,
                                       uint32_t coreKey,
                                       bool useDefaultFlow,
                                       uint32_t rxFlowStartIdx,
                                       uint32_t rxFlowIdx,
                                       uint8_t *macAddress,
                                       LwipifEnetAppIf_RxConfig *rxConfig,
                                       LwipifEnetAppIf_RxHandleInfo *rxHandleInfo,
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

    rxHandleInfo->hRxFlow = EnetDma_openRxCh(gRemoteAppObj.hEnetDma, &cpswRxFlowCfg);
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

static void CpswRemoteApp_closeLwipRxCh(CpswProxy_Handle hProxy,
                                        Enet_Handle hEnet,
                                        Udma_DrvHandle hUdmaDrv,
                                        uint32_t coreKey,
                                        bool useDefaultFlow,
                                        uint8_t *ipV4Addr,
                                        LwipifEnetAppIf_RxHandleInfo *rxHandleInfo,
                                        void *freeFxnArg,
                                        LwipifEnetAppIf_FreePktCbFxn freeFxn)
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

static void CpswRemoteApp_openLwipTxCh(Udma_DrvHandle hUdmaDrv,
                                       uint32_t coreKey,
                                       uint32_t txPSILId,
                                       LwipifEnetAppIf_TxConfig *txConfig,
                                       LwipifEnetAppIf_TxHandleInfo *txHandleInfo)
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

    txHandleInfo->hTxChannel = EnetDma_openTxCh(gRemoteAppObj.hEnetDma, &cpswTxChCfg);
    localAssert(NULL != txHandleInfo->hTxChannel);
}

static void CpswRemoteApp_closeLwipTxCh(CpswProxy_Handle hProxy,
                                        Enet_Handle hEnet,
                                        Udma_DrvHandle hUdmaDrv,
                                        uint32_t coreKey,
                                        LwipifEnetAppIf_TxHandleInfo *txHandleInfo,
                                        void *freeFxnArg,
                                        LwipifEnetAppIf_FreePktCbFxn freeFxn)
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

static bool LwipifEnetAppCb_isPortLinked(struct netif *netif,
                                         Enet_Handle hEnet)
{
    CpswRemoteApp_VirtNetif *virtNetif;
    static bool isPhyLinked = false;
    static uint32_t pollingInterVal = 0;
    uint32_t i;

    virtNetif = container_of(netif, CpswRemoteApp_VirtNetif, netif);
    localAssert(virtNetif->hCpswProxy != NULL);

    if ((isPhyLinked == false) || ((pollingInterVal % CPSW_REMOTE_APP_PHY_POLLING_INTERVAL) == 0))
    {
        for (i = 0U; i < virtNetif->numMacPorts; i++)
        {
            isPhyLinked = (isPhyLinked ||
                           CpswProxy_isPhyLinked(virtNetif->hCpswProxy,
                                                 hEnet,
                                                 gRemoteAppObj.coreKey,
                                                 virtNetif->macPorts[i]));
        }
    }

    pollingInterVal = (pollingInterVal + 1) % CPSW_REMOTE_APP_PHY_POLLING_INTERVAL;

    return isPhyLinked;
}

void LwipifEnetAppCb_getHandle(LwipifEnetAppIf_GetHandleInArgs *inArgs,
                               LwipifEnetAppIf_GetHandleOutArgs *outArgs)
{
    CpswRemoteApp_VirtNetif *virtNetif;
    uint32_t txPSILId;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;

    virtNetif = container_of(inArgs->netif, CpswRemoteApp_VirtNetif, netif);
    localAssert(virtNetif->hCpswProxy != NULL);

    outArgs->coreId = gRemoteAppObj.coreId;
    outArgs->hUdmaDrv = gRemoteAppObj.hUdmaDrv;
    outArgs->print = (Enet_Print) & printf;
    outArgs->isPortLinkedFxn = &LwipifEnetAppCb_isPortLinked;
    outArgs->rxInfo[0U].disableEvent = true;
    outArgs->timerPeriodUs = CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US;
    outArgs->txInfo.disableEvent = true;

    if (gRemoteAppObj.useExtAttach)
    {
        CpswProxy_attachExtended(virtNetif->hCpswProxy,
                                 gRemoteAppObj.enetType,
                                 &outArgs->hEnet,
                                 &outArgs->coreKey,
                                 &outArgs->hostPortRxMtu,
                                 outArgs->txMtu,
                                 &txPSILId,
                                 &rxStartFlowId,
                                 &rxFlowIdOffset,
                                 virtNetif->macAddr,
                                 &outArgs->txInfo.txPortNum);
    }
    else
    {
        CpswProxy_attach(virtNetif->hCpswProxy,
                         gRemoteAppObj.enetType,
                         &outArgs->hEnet,
                         &outArgs->coreKey,
                         &outArgs->hostPortRxMtu,
                         outArgs->txMtu,
                         &outArgs->txInfo.txPortNum);
        CpswProxy_allocTxCh(virtNetif->hCpswProxy,
                            outArgs->hEnet,
                            outArgs->coreKey,
                            &txPSILId);
        CpswProxy_allocRxFlow(virtNetif->hCpswProxy,
                              outArgs->hEnet,
                              outArgs->coreKey,
                              &rxStartFlowId,
                              &rxFlowIdOffset);
        CpswProxy_allocMac(virtNetif->hCpswProxy,
                           outArgs->hEnet,
                           outArgs->coreKey,
                           virtNetif->macAddr);
    }

    CpswRemoteApp_openLwipTxCh(outArgs->hUdmaDrv,
                               outArgs->coreKey,
                               txPSILId,
                               &inArgs->txCfg,
                               &outArgs->txInfo);

    CpswRemoteApp_openLwipRxCh(virtNetif->hCpswProxy,
                               outArgs->hEnet,
                               outArgs->hUdmaDrv,
                               outArgs->coreKey,
                               gRemoteAppObj.useDefaultRxFlow,
                               rxStartFlowId,
                               rxFlowIdOffset,
                               virtNetif->macAddr,
                               &inArgs->rxCfg[0U],
                               &outArgs->rxInfo[0U],
                               outArgs->hostPortRxMtu);
    gRemoteAppObj.coreKey = outArgs->coreKey;
    gRemoteAppObj.hEnet = outArgs->hEnet;
}

void LwipifEnetAppCb_releaseHandle(LwipifEnetAppIf_ReleaseHandleInfo *releaseInfo)
{
    CpswRemoteApp_VirtNetif *virtNetif;

    virtNetif = container_of(releaseInfo->netif, CpswRemoteApp_VirtNetif, netif);
    localAssert(virtNetif->hCpswProxy != NULL);

    CpswRemoteApp_closeLwipTxCh(virtNetif->hCpswProxy,
                                releaseInfo->hEnet,
                                releaseInfo->hUdmaDrv,
                                releaseInfo->coreKey,
                                &releaseInfo->txInfo,
                                releaseInfo->txFreePkt.cbArg,
                                releaseInfo->txFreePkt.cb);
    CpswRemoteApp_closeLwipRxCh(virtNetif->hCpswProxy,
                                releaseInfo->hEnet,
                                releaseInfo->hUdmaDrv,
                                releaseInfo->coreKey,
                                gRemoteAppObj.useDefaultRxFlow,
                                virtNetif->ipv4Addr,
                                &releaseInfo->rxInfo[0U],
                                releaseInfo->rxFreePkt[0U].cbArg,
                                releaseInfo->rxFreePkt[0U].cb);

    CpswProxy_detach(virtNetif->hCpswProxy, releaseInfo->hEnet, releaseInfo->coreKey);
}
#else /* !FREERTOS */
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

    rxHandleInfo->hRxFlow = EnetDma_openRxCh(gRemoteAppObj.hEnetDma, &cpswRxFlowCfg);
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

    txHandleInfo->hTxChannel = EnetDma_openTxCh(gRemoteAppObj.hEnetDma, &cpswTxChCfg);
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

static bool NimuEnetAppCb_isPortLinked(Enet_Handle hEnet)
{
    CpswRemoteApp_VirtNetif *virtNetif = &gRemoteAppObj.virtNetif[0];
    static bool isPhyLinked = false;
    static uint32_t pollingInterVal = 0;
    uint32_t i;

    localAssert(virtNetif->hCpswProxy != NULL);

    if ((isPhyLinked == false) || ((pollingInterVal % CPSW_REMOTE_APP_PHY_POLLING_INTERVAL) == 0))
    {
        for (i = 0; i < virtNetif->numMacPorts; i++)
        {
            isPhyLinked = (isPhyLinked ||
                            CpswProxy_isPhyLinked(virtNetif->hCpswProxy,
                                                  hEnet,
                                                  gRemoteAppObj.coreKey,
                                                  virtNetif->macPorts[i]));
        }
    }

    pollingInterVal = (pollingInterVal + 1) % CPSW_REMOTE_APP_PHY_POLLING_INTERVAL;
    return isPhyLinked;
}

void NimuEnetAppCb_getHandle(NimuEnetAppIf_GetHandleInArgs *inArgs,
                             NimuEnetAppIf_GetHandleOutArgs *outArgs)
{
    CpswRemoteApp_VirtNetif *virtNetif = &gRemoteAppObj.virtNetif[0];
    uint32_t txPSILId;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;
    Enet_MacPort macPort;

    /* Wait for EthFw connection to be established */
    SemaphoreP_pend(gRemoteAppObj.hInitSem, SemaphoreP_WAIT_FOREVER);

    localAssert(virtNetif->hCpswProxy != NULL);

    outArgs->coreId = gRemoteAppObj.coreId;
    outArgs->hUdmaDrv = gRemoteAppObj.hUdmaDrv;
    outArgs->print = (Enet_Print) & ConPrintf;
    outArgs->isPortLinkedFxn = &NimuEnetAppCb_isPortLinked;
    outArgs->isRingMonUsed = false;
    outArgs->timerPeriodUs = CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US;
    /* Let NIMU use optimized processing where TX packets are relinquished in next
     * TX submit call */
    outArgs->disableTxEvent = true;

    if (gRemoteAppObj.useExtAttach)
    {
        CpswProxy_attachExtended(virtNetif->hCpswProxy,
                                 gRemoteAppObj.enetType,
                                 &outArgs->hEnet,
                                 &outArgs->coreKey,
                                 &outArgs->hostPortRxMtu,
                                 outArgs->txMtu,
                                 &txPSILId,
                                 &rxStartFlowId,
                                 &rxFlowIdOffset,
                                 virtNetif->macAddr,
                                 &macPort);
    }
    else
    {
        CpswProxy_attach(virtNetif->hCpswProxy,
                         gRemoteAppObj.enetType,
                         &outArgs->hEnet,
                         &outArgs->coreKey,
                         &outArgs->hostPortRxMtu,
                         outArgs->txMtu,
                         &macPort);
        CpswProxy_allocTxCh(virtNetif->hCpswProxy,
                            outArgs->hEnet,
                            outArgs->coreKey,
                            &txPSILId);
        CpswProxy_allocRxFlow(virtNetif->hCpswProxy,
                              outArgs->hEnet,
                              outArgs->coreKey,
                              &rxStartFlowId,
                              &rxFlowIdOffset);
        CpswProxy_allocMac(virtNetif->hCpswProxy,
                           outArgs->hEnet,
                           outArgs->coreKey,
                           virtNetif->macAddr);
    }

    CpswRemoteApp_openNDKTxCh(outArgs->hUdmaDrv,
                             outArgs->coreKey,
                             txPSILId,
                             &inArgs->txCfg,
                             &outArgs->txInfo);

    CpswRemoteApp_openNDKRxCh(virtNetif->hCpswProxy,
                             outArgs->hEnet,
                             outArgs->hUdmaDrv,
                             outArgs->coreKey,
                             gRemoteAppObj.useDefaultRxFlow,
                             rxStartFlowId,
                             rxFlowIdOffset,
                             virtNetif->macAddr,
                             &inArgs->rxCfg,
                             &outArgs->rxInfo,
                             outArgs->hostPortRxMtu);
    gRemoteAppObj.coreKey = outArgs->coreKey;
    gRemoteAppObj.hEnet = outArgs->hEnet;
}

void NimuEnetAppCb_releaseHandle(NimuEnetAppIf_ReleaseHandleInfo *releaseInfo)
{
    CpswRemoteApp_VirtNetif *virtNetif = &gRemoteAppObj.virtNetif[0];

    localAssert(virtNetif->hCpswProxy != NULL);

    CpswRemoteApp_closeNDKTxCh(virtNetif->hCpswProxy,
                               releaseInfo->hEnet,
                               releaseInfo->hUdmaDrv,
                               releaseInfo->coreKey,
                               &releaseInfo->txInfo,
                               releaseInfo->freePktCbArg,
                               releaseInfo->txFreePktCb);

    CpswRemoteApp_closeNDKRxCh(virtNetif->hCpswProxy,
                               releaseInfo->hEnet,
                               releaseInfo->hUdmaDrv,
                               releaseInfo->coreKey,
                               gRemoteAppObj.useDefaultRxFlow,
                               virtNetif->ipv4Addr,
                               &releaseInfo->rxInfo,
                               releaseInfo->freePktCbArg,
                               releaseInfo->rxFreePktCb);

    CpswProxy_detach(virtNetif->hCpswProxy, releaseInfo->hEnet, releaseInfo->coreKey);
}
#endif /* !FREERTOS */
