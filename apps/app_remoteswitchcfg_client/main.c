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
#define ETHFWTRACE_MOD_ID 0x701
#define ETHFWTRACE_MOD_NAME "RTOS-App"

#if defined(__KLOCWORK__)
#include <stdlib.h>
#endif

#include <ti/csl/cslr_gtc.h>
#include <ti/osal/osal.h>

#include <ethremotecfg/client/include/cpsw_proxy.h>
#include <utils/ethfw_common/include/ethfw_trace.h>

#include <ti/drv/enet/lwipif/inc/lwipif2enet_appif.h>
#include <ti/drv/enet/lwipif/inc/lwip2lwipif.h>

#include <ti/drv/udma/udma.h>
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/enet/examples/utils/include/enet_apputils.h>
#include <ti/drv/enet/examples/utils/include/enet_appmemutils_cfg.h>
#include <ti/drv/enet/examples/utils/include/enet_appmemutils.h>

#include <ti/drv/ipc/ipc.h>
#include <apps/ipc_cfg/app_ipc_rsctable.h>

/* lwIP core includes */
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/api.h"

#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/dhcp.h"

/* lwIP netif includes */
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include "netif/bridgeif.h"

#if defined(ETHAPP_ENABLE_IPERF_SERVER)
#include "examples/lwiperf/lwiperf_example.h"
#endif

#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
#include <ti/drv/enet/lwipific/inc/netif_ic.h>
#include <ti/drv/enet/lwipific/inc/lwip2enet_ic.h>
#include <ti/drv/enet/lwipific/inc/lwip2lwipif_ic.h>
#endif

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

/*! Heartbeat poll period for CPSW proxy. */
#define CPSW_REMOTE_APP_POLL_PERIOD_MS        (2000U)

/*! CPSW proxy command response timeout. */
#define CPSW_REMOTE_APP_CMD_TIMEOUT_MS        (1000U)

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

#if defined(SAFERTOS)
#define ETHAPP_LWIP_TASK_STACKSIZE              (16U * 1024U)
#define ETHAPP_LWIP_TASK_STACKALIGN             ETHAPP_LWIP_TASK_STACKSIZE
#define ETHAPP_IPC_TASK_STACKALIGN              IPC_TASK_STACKSIZE
#else
#define ETHAPP_LWIP_TASK_STACKSIZE              (16U * 1024U)
#define ETHAPP_LWIP_TASK_STACKALIGN             (32U)
#define ETHAPP_IPC_TASK_STACKALIGN              (8192U)
#endif

#define ETHAPP_HWRECOVERY_TASK_PRI              (2U)

/* Mailbox used to pass virtual netifs to be closed for recovery */
#define ETHAPP_VIRTNETIF_MBX_MSGSIZE            (sizeof(CpswRemoteApp_HwRecoveryMsg))
#define ETHAPP_VIRTNETIF_MBX_MSGCOUNT           (4U)
#if defined(SAFERTOS)
#define ETHAPP_VIRTNETIF_MBX_SIZE               ((ETHAPP_VIRTNETIF_MBX_MSGSIZE * \
                                                  ETHAPP_VIRTNETIF_MBX_MSGCOUNT) + \
                                                 safertosapiQUEUE_OVERHEAD_BYTES)
#else
#define ETHAPP_VIRTNETIF_MBX_SIZE               (ETHAPP_VIRTNETIF_MBX_MSGSIZE * \
                                                 ETHAPP_VIRTNETIF_MBX_MSGCOUNT)
#endif

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
#define ETHAPP_LWIP_USE_DHCP                    (1)
#if !ETHAPP_LWIP_USE_DHCP
#define ETHFW_CLIENT_IPADDR_SWITCH_PORT(addr)   IP4_ADDR((addr), 192,168,1,201)
#define ETHFW_CLIENT_GW_SWITCH_PORT(addr)       IP4_ADDR((addr), 192,168,1,1)
#define ETHFW_CLIENT_NETMASK_SWITCH_PORT(addr)  IP4_ADDR((addr), 255,255,255,0)

#define ETHFW_CLIENT_IPADDR_MAC_PORT(addr)      IP4_ADDR((addr), 192,168,2,201)
#define ETHFW_CLIENT_GW_MAC_PORT(addr)          IP4_ADDR((addr), 192,168,2,1)
#define ETHFW_CLIENT_NETMASK_MAC_PORT(addr)     IP4_ADDR((addr), 255,255,255,0)
#endif

#if defined(ETHFW_MONITOR_SUPPORT)
static uint8_t gEthAppRecoveryStackBuf[ETHAPP_LWIP_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__((aligned(ETHAPP_LWIP_TASK_STACKSIZE)));
#endif

static uint8_t gEthAppLwipStackBuf[ETHAPP_LWIP_TASK_STACKSIZE]
__attribute__ ((section(".bss:taskStackSection")))
__attribute__((aligned(ETHAPP_LWIP_TASK_STACKALIGN)));

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

#if defined(SAFERTOS)
static sys_sem_t gEthApp_lwipMainSemObj;
#endif

static uint8_t sysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section("ipc_data_buffer"), aligned(8)));
static uint8_t gCntrlBuf[CPSW_REMOTE_APP_IPC_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned(8)));

static uint8_t g_vringMemBuf[IPC_VRING_MEM_SIZE] __attribute__ ((section(".bss:ipc_vring_mem"), aligned(8192)));

#if defined (ETHFW_RTOS_MCU3_0)
static uint32_t selfProcId = IPC_MCU3_0;
static uint32_t gRemoteProc[] =
{
#if defined(SOC_J721E)
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0,
    IPC_MCU2_1, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2,
    IPC_C7X_1,
#elif defined(SOC_J784S4)
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0,
    IPC_MCU2_1, IPC_MCU3_1, IPC_MCU4_0, IPC_MCU4_1,
    IPC_C7X_1,  IPC_C7X_2,  IPC_C7X_3,  IPC_C7X_4,
#endif
};
#else
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
#endif

static uint32_t gNumRemoteProc = sizeof(gRemoteProc) / sizeof(uint32_t);

#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
static struct netif netif_ic;

static uint32_t netif_ic_state[IC_ETH_MAX_VIRTUAL_IF] =
{
    IC_ETH_IF_MCU2_0_MCU2_1,
    IC_ETH_IF_MCU2_1_MCU2_0,
    IC_ETH_IF_MCU2_0_A72,
    IC_ETH_IF_MCU2_0_MCU3_0,
    IC_ETH_IF_MCU3_0_MCU2_0
};

struct netif netif_bridge;
bridgeif_initdata_t bridge_initdata;

#define ETHAPP_LWIP_BRIDGE_MAX_PORTS (4)
#define ETHAPP_LWIP_BRIDGE_MAX_DYNAMIC_ENTRIES (32)
#define ETHAPP_LWIP_BRIDGE_MAX_STATIC_ENTRIES (8)
#endif /* ETHAPP_ENABLE_INTERCORE_ETH */

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

    /* lwIP network interface */
    struct netif netif;

    /* DHCP network interface */
    struct dhcp dhcpNetif;

    /* Whether this netif should be set as the default netif */
    bool isDfltNetif;

    /* Time synchronization object */
    CpswRemoteApp_SyncTimerObj syncTimerObj;

    /* CPTS HWPUSH number used for remote time synchronization. This example
     * app only supports it on one interface, should be set to CPSW_CPTS_HWPUSH_INVALID
     * on all other interfaces */
    CpswCpts_HwPush hwPushNum;

    /* Whether CPTS sync timer has been initialized */
    bool syncTimerInitDone;
} CpswRemoteApp_VirtNetif;

typedef struct CpswRemoteApp_HwRecoveryMsg_s
{
    /* Virtual network interface data */
    CpswRemoteApp_VirtNetif *virtNetif;

    /* Notify type */
    uint32_t notifyType;
}CpswRemoteApp_HwRecoveryMsg;

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

    /* EthFw server status */
    EthRemoteCfg_ServerStatus hbStatus;

    /* Virtual network interface data */
    CpswRemoteApp_VirtNetif virtNetif[CPSW_REMOTE_APP_REMOTE_NETIF_MAX];

#if defined(ETHFW_MONITOR_SUPPORT)
    /* Mailbox of virtual netifs to be closed during CPSW recovery */
    MailboxP_Handle hMbx;

    /* Mailbox buffer for storing all the command messages */
    uint8_t mbxBuf[ETHAPP_VIRTNETIF_MBX_SIZE] __attribute__ ((aligned(32)));
#endif
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

#if (CPSW_REMOTE_APP_REMOTE_NETIF_MAX >= 2)  && defined(ENABLE_MAC_ONLY_PORTS)
/* Link status on these ports will be used to determine link up on virtual MAC port */
static Enet_MacPort gRemoteApp_virtualMacPorts[] =
{
    ENET_MAC_PORT_4,
};
#endif

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
    .virtNetif        =
    {
        {
            .hCpswProxy  = NULL,
            .virtPort    = ETHREMOTECFG_SWITCH_PORT_1,
            .macPorts    = gRemoteAppMacPorts,
            .numMacPorts = ENET_ARRAYSIZE(gRemoteAppMacPorts),
            .hwPushNum   = CPSW_CPTS_HWPUSH_2,
            .syncTimerInitDone = BFALSE,
            .isDfltNetif = BTRUE,
        },
#if (CPSW_REMOTE_APP_REMOTE_NETIF_MAX >= 2) && defined(ENABLE_MAC_ONLY_PORTS)
        {
            .hCpswProxy  = NULL,
            .virtPort    = ETHREMOTECFG_MAC_PORT_4,
            .macPorts    = gRemoteApp_virtualMacPorts,
            .numMacPorts = ENET_ARRAYSIZE(gRemoteApp_virtualMacPorts),
            .hwPushNum   = CPSW_CPTS_HWPUSH_INVALID,
            .syncTimerInitDone = BFALSE,
            .isDfltNetif = BFALSE,
        },
#endif
    },
};

static void EthApp_waitForDebugger(void);

static uint64_t CpswRemoteApp_virtToPhysFxn(const void *virtAddr,
                                            void *appData);

static void *CpswRemoteApp_physToVirtFxn(uint64_t phyAddr,
                                         void *appData);

static int32_t CpswRemoteApp_openUdma(void);

static int32_t CpswRemoteApp_openEnet(void);

static void CpswRemoteApp_openCpswProxy(CpswRemoteApp_VirtNetif *virtNetif);

char *VerStr = "LWIP CPSW Example";

static void EthApp_lwipMain(void *a0,
                            void *a1);

static void EthApp_initLwip(void *arg);

static void EthApp_initNetif(CpswRemoteApp_VirtNetif *virtNetif);

static void EthApp_virtNetifStatusCb(struct netif *netif);

static void EthApp_lwipNetifStatusCb(struct netif *netif);

#if defined(ETHFW_MONITOR_SUPPORT)
static void EthApp_openDma(struct netif *netif);

static void EthApp_closeDma(struct netif *netif);

static void CpswRemoteApp_hwRecoveryTask(void *a0,
                                         void *a1);

static void CpswRemoteApp_hwRecoveryNotify(uint32_t notifyType,
                                           void *notifyArg,
                                           void *cbArg);
#endif

static void CpswRemoteApp_calcSyncTimeParams(uint32_t notifyType,
                                             void *notifyArg,
                                             void *cbArg);


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

static inline Enet_MacPort CpswRemoteApp_getMacPort(EthRemoteCfg_VirtPort portId)
{
    Enet_MacPort macPort = ENET_MAC_PORT_INV;

    if (CpswProxy_isMacPort(portId))
    {
        macPort = ENET_MACPORT_DENORM(portId - ETHREMOTECFG_MAC_PORT_1);
    }

    return macPort;
}

static void CpswRemoteApp_initSyncTimer(CpswRemoteApp_VirtNetif *virtNetif)
{
    int32_t status = CPSWPROXY_SOK;

    memset(&virtNetif->syncTimerObj, 0, sizeof(CpswRemoteApp_SyncTimerObj));

    /* Make sure GTC is disabled before configuring timesync router*/
    CSL_REG32_WR(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCR, 0x0U);

    /* Register callback */
    status = CpswProxy_registerNotifyCb(virtNetif->hCpswProxy,
                                        ETHREMOTECFG_NOTIFY_HWPUSH,
                                        CpswRemoteApp_calcSyncTimeParams,
                                        (void *)virtNetif);
    if (status != CPSWPROXY_SOK)
    {
        ETHFWTRACE_ERR(status, "Failed to register to HW push notification");
    }
    else
    {
        /* Configure GTC push event */
        CSL_REG32_WR(CSL_GTC0_GTC_CFG0_BASE + CSL_GTC_CFG0_PUSHEVT,
                     CPSW_REMOTE_APP_GTC_PUSHEVT_BIT_SEL);

        /* Send request to Ethfw to configure TSR */
        CpswProxy_registerRemoteTimer(virtNetif->hCpswProxy,
                                      CSLR_TIMESYNC_INTRTR0_IN_GTC0_GTC_PUSH_EVENT_0,
                                      (uint8_t)virtNetif->hwPushNum);

        /* Enable GTC */
        CSL_REG32_WR(CSL_GTC0_GTC_CFG1_BASE + CSL_GTC_CFG1_CNTCR, 0x1U);

        /* Set sync timer init status */
        virtNetif->syncTimerInitDone = BTRUE;
    }
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


static void CpswRemoteApp_calcSyncTimeParams(uint32_t notifyType,
                                             void *notifyArg,
                                             void *cbArg)
{
    CpswRemoteApp_VirtNetif *virtNetif = (CpswRemoteApp_VirtNetif *)cbArg;
    CpswProxy_HwPushNotifyParams *params = (CpswProxy_HwPushNotifyParams *)notifyArg;
    CpswCpts_HwPush hwPushNum = (CpswCpts_HwPush)params->hwPushNum;
    uint64_t syncTime = params->timestamp;

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
            ETHFWTRACE_INFO("Current synchronized time via HWPUSH_%u in Epoch format: %lld",
                            hwPushNum, synchronizedTime);
        }

        hSyncTimerObj->prevLocalTime = gtcTime;
        hSyncTimerObj->prevCptsTime = syncTime;
    }
}

static void rpmsg_vdevMonitorFxn(void* arg0,
                                 void* arg1)
{
    int32_t status = IPC_SOK;

    /* Wait for Linux VDev ready... */
    while (!Ipc_isRemoteReady(IPC_MPU1_0))
    {
        TaskP_sleep(10);
    }

    /* Create the VRing now ... */
    if (!Ipc_isRemoteVirtioCreated(IPC_MPU1_0))
    {
        status = Ipc_lateVirtioCreate(IPC_MPU1_0);
        ETHFWTRACE_ERR_IF((status != IPC_SOK), status, "Ipc_lateVirtioCreate failed");
    }

    if (status == IPC_SOK)
    {
        status = RPMessage_lateInit(IPC_MPU1_0);
        ETHFWTRACE_ERR_IF((status != IPC_SOK), status, "RPMessage_lateInit failed");
    }

    return;
}

static void CpswRemoteApp_hbStatus(EthRemoteCfg_ServerStatus serverStatus,
                                   void *cbArg)
{
    switch (serverStatus)
    {
        case ETHREMOTECFG_SERVERSTATUS_UNINIT:
            ETHFWTRACE_INFO("Server status: server has not been initialized");
            break;
        case ETHREMOTECFG_SERVERSTATUS_READY:
            ETHFWTRACE_VERBOSE("Server status: server is running normally");
            break;
        case ETHREMOTECFG_SERVERSTATUS_RECOVERY:
            ETHFWTRACE_INFO("Server status: Ethernet switch is under recovery");
            break;
        case ETHREMOTECFG_SERVERSTATUS_BAD:
            ETHFWTRACE_ERR(serverStatus, "Server status: Not responding, possibly in bad/crashed state");
            break;
        default:
            ETHFWTRACE_ERR(serverStatus, "Server status: Unexpected status: %d", (int32_t)serverStatus);
            break;
    }
}

void printDevInfo(EthRemoteCfg_DeviceData *ethDevData)
{
    char *tf[] = {"false", "true"};

    ETHFWTRACE_INFO("ETHFW Version:%2d.%2d.%2d",
                    ethDevData->fwVer.major,
                    ethDevData->fwVer.minor,
                    ethDevData->fwVer.rev);

    ETHFWTRACE_INFO("ETHFW Build Date (YYYY/MMM/DD):%c%c%c%c/%c%c%c/%c%c",
                    ethDevData->fwVer.year[0],
                    ethDevData->fwVer.year[1],
                    ethDevData->fwVer.year[2],
                    ethDevData->fwVer.year[3],
                    ethDevData->fwVer.month[0],
                    ethDevData->fwVer.month[1],
                    ethDevData->fwVer.month[2],
                    ethDevData->fwVer.date[0],
                    ethDevData->fwVer.date[1]);

    ETHFWTRACE_INFO("ETHFW Commit SHA:%c%c%c%c%c%c%c%c",
                    ethDevData->fwVer.commitHash[0],
                    ethDevData->fwVer.commitHash[1],
                    ethDevData->fwVer.commitHash[2],
                    ethDevData->fwVer.commitHash[3],
                    ethDevData->fwVer.commitHash[4],
                    ethDevData->fwVer.commitHash[5],
                    ethDevData->fwVer.commitHash[6],
                    ethDevData->fwVer.commitHash[7]);

    ETHFWTRACE_INFO("ETHFW PermissionFlag:0x%llx, UART Connected:%s,UART Id:%d",
                    ethDevData->permissionFlags,
                    tf[ethDevData->uartConnected],
                    ethDevData->uartId);
}

static void CpswRemoteApp_initTask(void* a0,
                                   void* a1)
{
    TaskP_Params params;
    uint32_t numProc = gNumRemoteProc;
    Ipc_VirtIoParams vqParam;
    Ipc_InitPrms initPrms;
    RPMessage_Params cntrlParam;
    MailboxP_Params mbxParams;
    CpswProxy_initParams initParams;
    int32_t status;
    uint32_t i;

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

    /* Step 4: Create RPMessage monitor task */
    TaskP_Params_init(&params);
    params.priority = 3;
    params.stack = &g_vdevMonStackBuf[0];
    params.stacksize = sizeof(g_vdevMonStackBuf);

    TaskP_create(&rpmsg_vdevMonitorFxn, &params);

    /* Step 5: Start Cpsw Proxy */
    /* Initialize config to default values, this is NOT mandatory */
    CpswProxy_initConfig(&initParams);

    /* Update App specific params. */
    initParams.hbPeriodInMsecs = CPSW_REMOTE_APP_POLL_PERIOD_MS;

    /* Pass 0U for blocking ProxyClient forever for remote command response */
    initParams.cmdTimeoutInMsecs = CPSW_REMOTE_APP_CMD_TIMEOUT_MS;
    initParams.hbNotifyCb.cbFxn = CpswRemoteApp_hbStatus;
 
    CpswProxy_init(&initParams);

    /* Step 5a. Wait for remote_device to be initialized on the server side */
    do
    {
        status = CpswProxy_connect();
    }
    while (status != IPC_SOK);

    /* Step 5b. Open proxy clients for all virtual interfaces */
    for (i = 0U; i < ENET_ARRAYSIZE(gRemoteAppObj.virtNetif); i++)
    {
        CpswRemoteApp_openCpswProxy(&gRemoteAppObj.virtNetif[i]);
    }

    /* Step 6: Initialize lwIP */
    TaskP_Params_init(&params);
    params.priority  = DEFAULT_THREAD_PRIO;
    params.stack     = &gEthAppLwipStackBuf[0];
    params.stacksize = sizeof(gEthAppLwipStackBuf);
    params.name      = "lwIP main loop";
#if defined(SAFERTOS)
    params.userData  = &gEthApp_lwipMainSemObj;
#endif

    TaskP_create(&EthApp_lwipMain, &params);

#if defined(ETHFW_MONITOR_SUPPORT)
    /* Step 7: Create mailbox to pass virtual netifs to close during recovery */
    MailboxP_Params_init(&mbxParams);
    mbxParams.name    = (uint8_t *)"VirtNetif Mbox";
    mbxParams.size    = ETHAPP_VIRTNETIF_MBX_MSGSIZE;
    mbxParams.count   = ETHAPP_VIRTNETIF_MBX_MSGCOUNT;
    mbxParams.buf     = (void *)&gRemoteAppObj.mbxBuf[0U];
    mbxParams.bufsize = sizeof(gRemoteAppObj.mbxBuf);

    gRemoteAppObj.hMbx = MailboxP_create(&mbxParams);

    /* Step 8: Create HW recovery task */
    TaskP_Params_init(&params);
    params.name      = "HW Recovery Task";
    params.priority  = ETHAPP_HWRECOVERY_TASK_PRI;
    params.arg0      = (void *)&gRemoteAppObj;
    params.stack     = &gEthAppRecoveryStackBuf[0];
    params.stacksize = sizeof(gEthAppRecoveryStackBuf);

    TaskP_create(&CpswRemoteApp_hwRecoveryTask, &params);
#endif
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

    task = TaskP_create(&CpswRemoteApp_initTask, &taskParams);

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

    pRxFlowPrms->disableCacheOpsFlag = BFALSE;
    pRxFlowPrms->dmaDescAllocFxn = &EnetMem_allocDmaDesc;
    pRxFlowPrms->dmaDescFreeFxn = &EnetMem_freeDmaDesc;
    pRxFlowPrms->cbArg = cbArg;
    pRxFlowPrms->useGlobalEvt = BTRUE;
    pRxFlowPrms->useProxy = BFALSE;
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
    pTxChPrms->disableCacheOpsFlag = BFALSE;

    pTxChPrms->dmaDescAllocFxn = &EnetMem_allocDmaDesc;
    pTxChPrms->dmaDescFreeFxn = &EnetMem_freeDmaDesc;

    pTxChPrms->cbArg = cbArg;
    pTxChPrms->notifyCb = eventCb;
    pTxChPrms->useGlobalEvt = BTRUE;
}

static void CpswRemoteApp_openCpswProxy(CpswRemoteApp_VirtNetif *virtNetif)
{
    CpswProxy_Config proxyConfig;
    CpswProxy_Handle hProxy;

    proxyConfig.virtPort = virtNetif->virtPort;

    hProxy = CpswProxy_open(&proxyConfig);
    if (hProxy != NULL)
    {
        virtNetif->hCpswProxy = hProxy;

#if defined(ETHFW_MONITOR_SUPPORT)
        CpswProxy_registerNotifyCb(virtNetif->hCpswProxy,
                                   ETHREMOTECFG_NOTIFY_HWERROR,
                                   CpswRemoteApp_hwRecoveryNotify,
                                   (void *)virtNetif);

        CpswProxy_registerNotifyCb(virtNetif->hCpswProxy,
                                   ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE,
                                   CpswRemoteApp_hwRecoveryNotify,
                                   (void *)virtNetif);
#endif
    }
    else
    {
        ETHFWTRACE_ERR(CPSWPROXY_EFAIL, "Failed to open CpswProxy for virtPortId %u",
                       virtNetif->virtPort);
        localAssert(hProxy != NULL);
    }
}

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

#if defined(ETHAPP_ENABLE_IPERF_SERVER)
    /* Enable iperf */
    lwiperf_example_init();
#endif

    sys_sem_signal(initSem);
}

static void EthApp_initNetif(CpswRemoteApp_VirtNetif *virtNetif)
{
    struct netif *netif = &virtNetif->netif;
    ip4_addr_t ipaddr, netmask, gw;
#if LWIP_CHECKSUM_CTRL_PER_NETIF
    uint32_t chksumFlags = NETIF_CHECKSUM_ENABLE_ALL;
/* Disable checksum in software if CPSW can do it (i.e. not impacted by errata) */
#if !defined(ETHFW_CPSW_MULTIHOST_CHECKSUM_ERRATA)
    chksumFlags = ~(NETIF_CHECKSUM_GEN_UDP |
                    NETIF_CHECKSUM_GEN_TCP |
                    NETIF_CHECKSUM_CHECK_TCP |
                    NETIF_CHECKSUM_CHECK_UDP);
#endif
#endif
#if ETHAPP_LWIP_USE_DHCP
    err_t err;
#endif

    ip4_addr_set_zero(&gw);
    ip4_addr_set_zero(&ipaddr);
    ip4_addr_set_zero(&netmask);

    if(CpswProxy_isSwitchPort(virtNetif->virtPort))
    {
#if ETHAPP_LWIP_USE_DHCP
        ETHFWTRACE_INFO("Starting lwIP, local interface IP is dhcp-enabled");
#else /* ETHAPP_LWIP_USE_DHCP */
        ETHFW_CLIENT_GW_SWITCH_PORT(&gw);
        ETHFW_CLIENT_IPADDR_SWITCH_PORT(&ipaddr);
        ETHFW_CLIENT_NETMASK_SWITCH_PORT(&netmask);
        ETHFWTRACE_INFO("Starting lwIP, local interface IP is %s", ip4addr_ntoa(&ipaddr));
#endif /* ETHAPP_LWIP_USE_DHCP */

#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
        /* Create Enet LLD ethernet interface */
        netif_add(netif, NULL, NULL, NULL, NULL, LWIPIF_LWIP_init, tcpip_input);
#if LWIP_CHECKSUM_CTRL_PER_NETIF
        NETIF_SET_CHECKSUM_CTRL(netif, chksumFlags);
#endif

#if defined (ETHFW_RTOS_MCU3_0)
        /* Create inter-core virtual ethernet interface: MCU3_0 <-> MCU2_0 */
        netif_add(&netif_ic, NULL, NULL, NULL,
                  (void*)&netif_ic_state[IC_ETH_IF_MCU3_0_MCU2_0],
                  LWIPIF_LWIP_IC_init, tcpip_input);
#else
        /* Create inter-core virtual ethernet interface: MCU2_1 <-> MCU2_0 */
        netif_add(&netif_ic, NULL, NULL, NULL,
                  (void*)&netif_ic_state[IC_ETH_IF_MCU2_1_MCU2_0],
                  LWIPIF_LWIP_IC_init, tcpip_input);
#endif

        /* Create bridge interface */
        bridge_initdata.max_ports = ETHAPP_LWIP_BRIDGE_MAX_PORTS;
        bridge_initdata.max_fdb_dynamic_entries = ETHAPP_LWIP_BRIDGE_MAX_DYNAMIC_ENTRIES;
        bridge_initdata.max_fdb_static_entries = ETHAPP_LWIP_BRIDGE_MAX_STATIC_ENTRIES;
        EnetUtils_copyMacAddr(&bridge_initdata.ethaddr.addr[0U], &virtNetif->macAddr[0U]);

        netif_add(&netif_bridge, &ipaddr, &netmask, &gw, &bridge_initdata, bridgeif_init, netif_input);

        /* Add all network interfaces to the bridge */
        bridgeif_add_port_with_opts(&netif_bridge, netif, BRIDGEIF_PORT_CPSW);
        bridgeif_add_port_with_opts(&netif_bridge, &netif_ic, BRIDGEIF_PORT_VIRTUAL);

        /* Set bridge interface as the default */
        netif_set_default(&netif_bridge);
        netif_set_status_callback(&netif_bridge, EthApp_lwipNetifStatusCb);
#else
        netif_add(netif, &ipaddr, &netmask, &gw, NULL, LWIPIF_LWIP_init, tcpip_input);
#if LWIP_CHECKSUM_CTRL_PER_NETIF
        NETIF_SET_CHECKSUM_CTRL(netif, chksumFlags);
#endif

        if (virtNetif->isDfltNetif)
        {
            netif_set_default(netif);
        }
#endif
        netif_set_status_callback(netif, EthApp_virtNetifStatusCb);

        dhcp_set_struct(netif_default, &virtNetif->dhcpNetif);

        netif_set_up(netif);
#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
        netif_set_up(&netif_ic);
        netif_set_up(&netif_bridge);
#endif

#if ETHAPP_LWIP_USE_DHCP
        err = dhcp_start(netif_default);
        ETHFWTRACE_ERR_IF((err != ERR_OK), err, "Failed to start DHCP");
#endif /* ETHAPP_LWIP_USE_DHCP */
    }
    else
    {
#if ETHAPP_LWIP_USE_DHCP
        ETHFWTRACE_INFO("Starting lwIP, local interface IP is dhcp-enabled");
#else /* ETHAPP_LWIP_USE_DHCP */
        ETHFW_CLIENT_GW_MAC_PORT(&gw);
        ETHFW_CLIENT_IPADDR_MAC_PORT(&ipaddr);
        ETHFW_CLIENT_NETMASK_MAC_PORT(&netmask);
        ETHFWTRACE_INFO("Starting lwIP, local interface IP is %s", ip4addr_ntoa(&ipaddr));
#endif /* ETHAPP_LWIP_USE_DHCP */

        netif_add(netif, &ipaddr, &netmask, &gw, NULL, LWIPIF_LWIP_init, tcpip_input);
#if LWIP_CHECKSUM_CTRL_PER_NETIF
        NETIF_SET_CHECKSUM_CTRL(netif, chksumFlags);
#endif

        if (virtNetif->isDfltNetif)
        {
            netif_set_default(netif);
        }

        netif_set_status_callback(netif, EthApp_virtNetifStatusCb);

        dhcp_set_struct(netif, &virtNetif->dhcpNetif);

        netif_set_up(netif);

#if ETHAPP_LWIP_USE_DHCP
        err = dhcp_start(netif);
        ETHFWTRACE_ERR_IF((err != ERR_OK), err, "Failed to start DHCP");
#endif /* ETHAPP_LWIP_USE_DHCP */
    }
}

static void EthApp_virtNetifStatusCb(struct netif *netif)
{
    CpswRemoteApp_VirtNetif *virtNetif;

    virtNetif = container_of(netif, CpswRemoteApp_VirtNetif, netif);

    if (netif_is_up(netif))
    {
        const ip4_addr_t *ipAddr = netif_ip4_addr(netif);

        ETHFWTRACE_INFO("Added interface '%c%c%d', IP is %s",
                        netif->name[0], netif->name[1], netif->num, ip4addr_ntoa(ipAddr));

        if (ipAddr->addr != 0)
        {
            virtNetif->ipv4Addr[0] = ip4_addr1_val(*ipAddr);
            virtNetif->ipv4Addr[1] = ip4_addr2_val(*ipAddr);
            virtNetif->ipv4Addr[2] = ip4_addr3_val(*ipAddr);
            virtNetif->ipv4Addr[3] = ip4_addr4_val(*ipAddr);

            localAssert(virtNetif->hCpswProxy != NULL);

            if (CpswProxy_isSwitchPort(virtNetif->virtPort))
            {
                CpswProxy_registerIPV4Addr(virtNetif->hCpswProxy,
                                           virtNetif->macAddr,
                                           virtNetif->ipv4Addr);
            }
        }

        /* Init time synchronization */
        if ((virtNetif->hwPushNum != CPSW_CPTS_HWPUSH_INVALID) &&
                (virtNetif->syncTimerInitDone == BFALSE))
        {
            CpswRemoteApp_initSyncTimer(virtNetif);
        }

    }
    else
    {
        ETHFWTRACE_INFO("Removed interface '%c%c%d'", netif->name[0], netif->name[1], netif->num);
    }
}

static void EthApp_lwipNetifStatusCb(struct netif *netif)
{
    if (netif_is_up(netif))
    {
        const ip4_addr_t *ipAddr = netif_ip4_addr(netif);

        ETHFWTRACE_INFO("Added interface '%c%c%d', IP is %s",
                        netif->name[0], netif->name[1], netif->num, ip4addr_ntoa(ipAddr));
    }
    else
    {
        ETHFWTRACE_INFO("Removed interface '%c%c%d'", netif->name[0], netif->name[1], netif->num);
    }
}

static void CpswRemoteApp_openLwipRxCh(CpswProxy_Handle hProxy,
                                       Udma_DrvHandle hUdmaDrv,
                                       bool useDefaultFlow,
                                       uint32_t rxFlowStartIdx,
                                       uint32_t rxFlowIdx,
                                       uint8_t *macAddress,
                                       LwipifEnetAppIf_RxConfig *rxConfig,
                                       LwipifEnetAppIf_RxHandleInfo *rxHandleInfo,
                                       uint32_t rxFlowMtu,
                                       bool isSwitchPort)
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

    if (useDefaultFlow && isSwitchPort)
    {
        CpswProxy_registerDefaultRxFlow(hProxy,
                                        rxHandleInfo->rxFlowStartIdx,
                                        rxHandleInfo->rxFlowIdx);
    }
    else
    {
        CpswProxy_registerDstMacRxFlow(hProxy,
                                       rxHandleInfo->rxFlowStartIdx,
                                       rxHandleInfo->rxFlowIdx,
                                       rxHandleInfo->macAddr);
    }
}

static void CpswRemoteApp_closeLwipRxCh(CpswProxy_Handle hProxy,
                                        Udma_DrvHandle hUdmaDrv,
                                        bool useDefaultFlow,
                                        uint8_t *ipV4Addr,
                                        LwipifEnetAppIf_RxHandleInfo *rxHandleInfo,
                                        void *freeFxnArg,
                                        LwipifEnetAppIf_FreePktCbFxn freeFxn,
                                        bool isSwitchPort)
{
    EnetDma_PktQ fqPktInfoQ;
    EnetDma_PktQ cqPktInfoQ;
    int32_t status;

    EnetQueue_initQ(&fqPktInfoQ);
    EnetQueue_initQ(&cqPktInfoQ);

    EnetDma_disableRxEvent(rxHandleInfo->hRxFlow);

    if (isSwitchPort)
    {
        CpswProxy_unregisterIPV4Addr(hProxy, ipV4Addr);
    }

    if (useDefaultFlow && isSwitchPort)
    {
        CpswProxy_unregisterDefaultRxFlow(hProxy,
                                          rxHandleInfo->rxFlowStartIdx,
                                          rxHandleInfo->rxFlowIdx);
    }
    else
    {
        CpswProxy_unregisterDstMacRxFlow(hProxy,
                                         rxHandleInfo->rxFlowStartIdx,
                                         rxHandleInfo->rxFlowIdx,
                                         rxHandleInfo->macAddr);
    }

    status = EnetDma_closeRxCh(rxHandleInfo->hRxFlow,
                               &fqPktInfoQ,
                               &cqPktInfoQ);
    localAssert(status == ENET_SOK);
    CpswProxy_freeMac(hProxy,
                      rxHandleInfo->macAddr);
    CpswProxy_freeRxFlow(hProxy,
                         rxHandleInfo->rxFlowStartIdx,
                         rxHandleInfo->rxFlowIdx);
    freeFxn(freeFxnArg, &fqPktInfoQ, &cqPktInfoQ);
}

static void CpswRemoteApp_openLwipTxCh(Udma_DrvHandle hUdmaDrv,
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
                                        Udma_DrvHandle hUdmaDrv,
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
                       txHandleInfo->txChNum);
    freeFxn(freeFxnArg, &fqPktInfoQ, &cqPktInfoQ);
}

static bool LwipifEnetAppCb_isPortLinked(struct netif *netif,
                                         void *handleArg)
{
    bool isLinked = BFALSE;

    isLinked = (isLinked || CpswProxy_isPhyLinked((CpswProxy_Handle)handleArg));

    return isLinked;
}

void LwipifEnetAppCb_getHandle(LwipifEnetAppIf_GetHandleInArgs *inArgs,
                               LwipifEnetAppIf_GetHandleOutArgs *outArgs)
{
    CpswRemoteApp_VirtNetif *virtNetif;
    uint32_t txPSILId;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;
    bool isSwitchPort;
    uint32_t numTxCh;
    uint32_t numRxFlow;
    uint32_t relChPriority = 0U;
    uint32_t flowIdx = 0U;
    uint32_t i = 0U;

    virtNetif = container_of(inArgs->netif, CpswRemoteApp_VirtNetif, netif);
    localAssert(virtNetif->hCpswProxy != NULL);

    isSwitchPort = CpswProxy_isSwitchPort(virtNetif->virtPort);

    outArgs->handleArg = (void *)(virtNetif->hCpswProxy);
    outArgs->coreId = gRemoteAppObj.coreId;
    outArgs->hUdmaDrv = gRemoteAppObj.hUdmaDrv;
    outArgs->print = (Enet_Print) & printf;
    outArgs->isPortLinkedFxn = &LwipifEnetAppCb_isPortLinked;
    outArgs->txCsumOffloadEn = BTRUE;
    outArgs->rxCsumOffloadEn = BTRUE;
    outArgs->rxInfo[0U].disableEvent = BTRUE;
    outArgs->timerPeriodUs = CPSW_REMOTE_APP_PACKET_POLL_PERIOD_US;
    outArgs->txInfo.disableEvent = BTRUE;
    outArgs->txInfo.txPortNum = CpswRemoteApp_getMacPort(virtNetif->virtPort);

    if (gRemoteAppObj.useExtAttach)
    {
        CpswProxy_attachExtended(virtNetif->hCpswProxy,
                                 virtNetif->virtPort,
                                 &outArgs->hostPortRxMtu,
                                 &outArgs->txMtu[0U],
                                 &txPSILId,
                                 &rxStartFlowId,
                                 &rxFlowIdOffset,
                                 virtNetif->macAddr);
    }
    else
    {
        CpswProxy_attach(virtNetif->hCpswProxy,
                         virtNetif->virtPort,
                         &outArgs->hostPortRxMtu,
                         &outArgs->txMtu[0U],
                         &numTxCh,
                         &numRxFlow);
        ETHFWTRACE_ERR_IF((numTxCh == 0U), ETHFW_EFAIL, "Number of tx channel allocated cannot be 0U");
        ETHFWTRACE_ERR_IF((numRxFlow == 0U), ETHFW_EFAIL, "Number of rx flow allocated cannot be 0U");
        /* To demonstrate the QoS functionality we should allocate multiple channels
         * But Out of box we are only giving 1 tx channel for each virtual port for RTOS client
         * Suppose 3 channels are allocated (update ethfw server main.c virtual port config for the same),
         * to allocate first channel with low priority, second channel with medium
         * and third channel with highest priority give relative channel priority to
         * be 0U, 1U and 2U in order respectively.
         * It is important that the numTxCh allocated should be > 1 to test the functionality.
         * relChPriority range from 0U to numTxCh - 1U */
        CpswProxy_allocTxCh(virtNetif->hCpswProxy,
                            &txPSILId,
                            relChPriority);
        /* As we have not added multiple rx flow support for QoS on RTOS client side.
         * For now we are allocating only 1 rx flow (i.e. default flow).
         * But we have tested rx QoS by manually calling multiple allocRxFlow
         * i.e. with default flow and extended flow as well */
        CpswProxy_allocRxFlow(virtNetif->hCpswProxy,
                              &rxStartFlowId,
                              &rxFlowIdOffset,
                              flowIdx);
        CpswProxy_allocMac(virtNetif->hCpswProxy,
                           virtNetif->macAddr);
    }

    /* Update the outArgs->txMtu[] with the value of outArgs->txMtu[0U] returned by CpswProxy_attach()/CpswProxy_attachExtended() */
    for (i = 0U; i < ETHFW_ARRAYSIZE(outArgs->txMtu); i++)
    {
        outArgs->txMtu[i] = outArgs->txMtu[0U];
    }

    CpswRemoteApp_openLwipTxCh(outArgs->hUdmaDrv,
                               txPSILId,
                               &inArgs->txCfg,
                               &outArgs->txInfo);

    CpswRemoteApp_openLwipRxCh(virtNetif->hCpswProxy,
                               outArgs->hUdmaDrv,
                               gRemoteAppObj.useDefaultRxFlow,
                               rxStartFlowId,
                               rxFlowIdOffset,
                               virtNetif->macAddr,
                               &inArgs->rxCfg[0U],
                               &outArgs->rxInfo[0U],
                               outArgs->hostPortRxMtu,
                               isSwitchPort);
}

void LwipifEnetAppCb_releaseHandle(LwipifEnetAppIf_ReleaseHandleInfo *releaseInfo)
{
    CpswRemoteApp_VirtNetif *virtNetif;
    bool isSwitchPort;

    virtNetif = container_of(releaseInfo->netif, CpswRemoteApp_VirtNetif, netif);
    localAssert(virtNetif->hCpswProxy != NULL);

    isSwitchPort = CpswProxy_isSwitchPort(virtNetif->virtPort);

    CpswRemoteApp_closeLwipTxCh(virtNetif->hCpswProxy,
                                releaseInfo->hUdmaDrv,
                                &releaseInfo->txInfo,
                                releaseInfo->txFreePkt.cbArg,
                                releaseInfo->txFreePkt.cb);

    CpswRemoteApp_closeLwipRxCh(virtNetif->hCpswProxy,
                                releaseInfo->hUdmaDrv,
                                gRemoteAppObj.useDefaultRxFlow,
                                virtNetif->ipv4Addr,
                                &releaseInfo->rxInfo[0U],
                                releaseInfo->rxFreePkt[0U].cbArg,
                                releaseInfo->rxFreePkt[0U].cb,
                                isSwitchPort);

    CpswProxy_detach(virtNetif->hCpswProxy);
}

#if defined(ETHFW_MONITOR_SUPPORT)
void LwipifEnetAppCb_openDma(LwipifEnetAppIf_GetHandleInArgs *inArgs,
                             LwipifEnetAppIf_GetHandleOutArgs *outArgs)
{
    EnetUdma_OpenTxChPrms cpswTxChCfg;
    EnetUdma_OpenRxFlowPrms cpswRxFlowCfg;
    LwipifEnetAppIf_RxHandleInfo *rxInfo;
    LwipifEnetAppIf_RxConfig *rxCfg;
    CpswRemoteApp_VirtNetif *virtNetif;

    virtNetif = container_of(inArgs->netif, CpswRemoteApp_VirtNetif, netif);
    localAssert(virtNetif->hCpswProxy != NULL);

    /* Set configuration parameters */
    EnetDma_initTxChParams(&cpswTxChCfg);
    EnetAppUtils_setCommonTxChPrms(&cpswTxChCfg);
    cpswTxChCfg.hUdmaDrv  = outArgs->hUdmaDrv;
    cpswTxChCfg.useProxy  = BTRUE;
    cpswTxChCfg.numTxPkts = inArgs->txCfg.numPackets;
    cpswTxChCfg.cbArg     = inArgs->txCfg.cbArg;
    cpswTxChCfg.notifyCb  = inArgs->txCfg.notifyCb;
    cpswTxChCfg.chNum     = outArgs->txInfo.txChNum;

    outArgs->txInfo.hTxChannel = EnetDma_openTxCh(gRemoteAppObj.hEnetDma, &cpswTxChCfg);
    localAssert(NULL != outArgs->txInfo.hTxChannel);

    rxInfo = &outArgs->rxInfo[0U];
    rxCfg = &inArgs->rxCfg[0U];

    EnetDma_initRxChParams(&cpswRxFlowCfg);
    EnetAppUtils_setCommonRxFlowPrms(&cpswRxFlowCfg);

    cpswRxFlowCfg.notifyCb  = rxCfg->notifyCb;
    cpswRxFlowCfg.numRxPkts = rxCfg->numPackets;
    cpswRxFlowCfg.hUdmaDrv  = outArgs->hUdmaDrv;;
    cpswRxFlowCfg.cbArg     = rxCfg->cbArg;
    cpswRxFlowCfg.useGlobalEvt = BTRUE;
    cpswRxFlowCfg.useProxy  = BFALSE;
    cpswRxFlowCfg.rxFlowMtu = outArgs->hostPortRxMtu;
    cpswRxFlowCfg.startIdx  = rxInfo->rxFlowStartIdx;
    cpswRxFlowCfg.flowIdx   = rxInfo->rxFlowIdx;

    outArgs->rxInfo[0U].hRxFlow = EnetDma_openRxCh(gRemoteAppObj.hEnetDma, &cpswRxFlowCfg);
    localAssert(outArgs->rxInfo[0U].hRxFlow != NULL);

    CpswProxy_registerDstMacRxFlow(virtNetif->hCpswProxy,
                                   rxInfo->rxFlowStartIdx,
                                   rxInfo->rxFlowIdx,
                                   rxInfo->macAddr);
}

void LwipifEnetAppCb_closeDma(LwipifEnetAppIf_ReleaseHandleInfo *releaseInfo)
{
    EnetDma_PktQ fqPktInfoQ;
    EnetDma_PktQ cqPktInfoQ;
    int32_t status = ENET_SOK;
    CpswRemoteApp_VirtNetif *virtNetif;

    virtNetif = container_of(releaseInfo->netif, CpswRemoteApp_VirtNetif, netif);
    localAssert(virtNetif->hCpswProxy != NULL);

    EnetQueue_initQ(&fqPktInfoQ);
    EnetQueue_initQ(&cqPktInfoQ);

    localAssert(NULL != releaseInfo->txInfo.hTxChannel);
    localAssert(NULL != releaseInfo->rxInfo[0U].hRxFlow);

    EnetDma_disableTxEvent(releaseInfo->txInfo.hTxChannel);
    status = EnetDma_closeTxCh(releaseInfo->txInfo.hTxChannel, &fqPktInfoQ, &cqPktInfoQ);
    releaseInfo->txInfo.hTxChannel = NULL;

    releaseInfo->txFreePkt.cb(releaseInfo->txFreePkt.cbArg, &fqPktInfoQ, &cqPktInfoQ);

    EnetQueue_initQ(&fqPktInfoQ);
    EnetQueue_initQ(&cqPktInfoQ);

    EnetDma_disableRxEvent(releaseInfo->rxInfo[0U].hRxFlow);

    CpswProxy_unregisterDstMacRxFlow(virtNetif->hCpswProxy,
                                     releaseInfo->rxInfo[0U].rxFlowStartIdx,
                                     releaseInfo->rxInfo[0U].rxFlowIdx,
                                     releaseInfo->rxInfo[0U].macAddr);

    status = EnetDma_closeRxCh(releaseInfo->rxInfo[0U].hRxFlow,
                               &fqPktInfoQ,
                               &cqPktInfoQ);

    /* Put some delay for flow to close by Sciclient.
     * Temp change will be removed when gPTP_stop issue is resolved. */
    TaskP_sleep(200U);
    releaseInfo->rxInfo[0U].hRxFlow = NULL;

    releaseInfo->rxFreePkt[0U].cb(releaseInfo->rxFreePkt[0U].cbArg, &fqPktInfoQ, &cqPktInfoQ);

}

static void EthApp_openDma(struct netif *netif)
{
    /* Open the lwIP DMA channels and issue link up to lwIP stack */
    LWIPIF_LWIP_openDma(netif);

    sys_lock_tcpip_core();
    netif_set_link_up(netif);
    sys_unlock_tcpip_core();
}

static void EthApp_closeDma(struct netif *netif)
{
    /* Issue link down to lwIP stack and close the lwIP DMA channels */
    sys_lock_tcpip_core();
    netif_set_link_down(netif);
    sys_unlock_tcpip_core();

    /* Close the lwIP DMA channels */
    LWIPIF_LWIP_closeDma(netif);
}

static void CpswRemoteApp_hwRecoveryNotify(uint32_t notifyType,
                                           void *notifyArg,
                                           void *cbArg)
{
    CpswRemoteApp_VirtNetif *virtNetif = (CpswRemoteApp_VirtNetif *)cbArg;
    CpswRemoteApp_HwRecoveryMsg msgMbx;
    bool isMacPort;
    uint32_t portNum;

    if (virtNetif != NULL)
    {
        isMacPort = CpswProxy_isMacPort(virtNetif->virtPort);
        portNum = CpswProxy_getPortNum(virtNetif->virtPort);

        if (notifyType == ETHREMOTECFG_NOTIFY_HWERROR)
        {
            msgMbx.virtNetif = virtNetif;
            msgMbx.notifyType = notifyType;
            ETHFWTRACE_INFO("Virtual %s port %u: HWERROR notify received",
                            isMacPort ? "MAC" : "switch", portNum);
        MailboxP_post(gRemoteAppObj.hMbx,  (void *)&msgMbx, MailboxP_WAIT_FOREVER);
        }
        else if (notifyType == ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE)
        {
            msgMbx.virtNetif = virtNetif;
            msgMbx.notifyType = notifyType;
            ETHFWTRACE_INFO("Virtual %s port %u: HWRECOVERY_COMPLETE notify received",
                    isMacPort ? "MAC" : "switch", portNum);
            MailboxP_post(gRemoteAppObj.hMbx, (void *)&msgMbx, MailboxP_WAIT_FOREVER);
        }
        else
        {
            ETHFWTRACE_ERR(CPSWPROXY_EUNEXPECTED, "Unexpected HW recovery notify Type");
        }
    }
    else
    {
        ETHFWTRACE_ERR(CPSWPROXY_EUNEXPECTED, "Unexpected HW recovery notify callback argument");
    }
}

static void CpswRemoteApp_hwRecoveryTask(void *a0,
                                         void *a1)
{
    CpswRemoteApp_HwRecoveryMsg msgMbx;
    volatile bool exitTask = BFALSE;
    bool isMacPort;
    uint32_t portNum;

    while (!exitTask)
    {
        MailboxP_pend(gRemoteAppObj.hMbx,  (void *)&msgMbx, MailboxP_WAIT_FOREVER);

        isMacPort = CpswProxy_isMacPort(msgMbx.virtNetif->virtPort);
        portNum = CpswProxy_getPortNum(msgMbx.virtNetif->virtPort);

        if (msgMbx.notifyType == ETHREMOTECFG_NOTIFY_HWERROR)
        {
            ETHFWTRACE_INFO("Virtual %s port %u: Initiating DMA channel teardown",
                            isMacPort ? "MAC" : "switch", portNum);
            EthApp_closeDma(&msgMbx.virtNetif->netif);

            /* Send teardown completion */
            ETHFWTRACE_INFO("Virtual %s port %u: DMA teardown complete, notify ETHFW",
                            isMacPort ? "MAC" : "switch", portNum);
            CpswProxy_teardownCompletion(msgMbx.virtNetif->hCpswProxy);
        }
        else if (msgMbx.notifyType == ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE)
        {
            ETHFWTRACE_INFO("Virtual %s port %u: Opening back DMA channel",
                            isMacPort ? "MAC" : "switch", portNum);
            EthApp_openDma(&msgMbx.virtNetif->netif);
        }
        else
        {
            ETHFWTRACE_ERR(CPSWPROXY_EUNEXPECTED, "Unexpected HW recovery notify Type");
        }
    }
}
#endif
