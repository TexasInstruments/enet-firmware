/*
 *
 * Copyright (c) 2019-2023 Texas Instruments Incorporated
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
 *  \file main.c
 *
 *  \brief Main file for Ethernet Firmware server application.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* EthFwTrace id for this module, must be unique within ETHFW */
#define ETHFWTRACE_MOD_ID 0x601

#include <stdio.h>
#include <stdint.h>

/* OSAL */
#include <ti/osal/osal.h>
#include <ti/osal/SemaphoreP.h>
#include <ti/osal/TaskP.h>
#include <ti/osal/MailboxP.h>

/* PDK Driver Header files */
#include <ti/drv/ipc/ipc.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/core/enet_mod_hostport.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/enet/include/dma/udma/enet_udma.h>
#include <ti/drv/enet/include/core/enet_dma.h>
#include <ti/drv/enet/examples/utils/include/enet_apputils.h>

/* EthFw header files */
#include <apps/ipc_cfg/app_ipc_rsctable.h>

#if defined(ETHFW_DEMO_SUPPORT)
#include <utils/intervlan/include/eth_hwintervlan.h>
#include <utils/intervlan/include/eth_swintervlan.h>
#endif

#if defined(ETHFW_GPTP_SUPPORT)
/* Timesync header files */
#include <tsn_buildconf/jacinto_buildconf.h>
#include <tsn_gptp/gptpconf/gptpgcfg.h>
#include <tsn_gptp/gptpconf/xl4-extmod-xl4gptp.h>
#include <ethremotecfg/server/include/ethfw_tsn.h>
#endif

/* EthFw utils header files */
#include <utils/remote_service/include/app_remote_service.h>
#include <utils/perf_stats/include/app_perf_stats.h>
#include <utils/ethfw_stats/include/app_ethfw_stats_osal.h>
#include <utils/board/include/ethfw_board_utils.h>

#define System_printf  Ipc_Trace_printf
#define System_vprintf Ipc_Trace_vprintf

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

#include <ti/drv/enet/lwipif/inc/default_netif.h>
#include <ti/drv/enet/lwipif/inc/lwip2lwipif.h>

#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
#include <ti/drv/enet/lwipific/inc/lwip2enet_ic.h>
#include <ti/drv/enet/lwipific/inc/lwip2lwipif_ic.h>
#endif

#include <utils/ethfw_callbacks/include/ethfw_callbacks_lwipif.h>
#include <ethremotecfg/protocol/ethremotecfg.h>
#include <ethremotecfg/server/include/ethfw.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define ETHAPP_OK                               (0)

#define ETHAPP_ERROR                            (-1)

#define ETHAPP_TRACEBUF_FLUSH_PERIOD_IN_MSEC    (500U)

#define IPC_TRACEBUF_SIZE                       (0x80000U)

#define VQ_BUF_SIZE                             (2048U)

#if defined(SOC_J721E)
#define IPC_VRING_MEM_SIZE                      (32U * 1024U * 1024U)
#elif defined(SOC_J7200)
#define IPC_VRING_MEM_SIZE                      (8U * 1024U * 1024U)
#elif defined(SOC_J784S4)
#define IPC_VRING_MEM_SIZE                      (48U * 1024U * 1024U)
#else
#error "Unsupported device"
#endif

#define ETHAPP_IPC_RPC_MSG_SIZE                 (496U + 32U)
#define ETHAPP_IPC_NUM_RPMSG_BUFS               (256U)
#define ETHAPP_IPC_RPMSG_OBJ_SIZE               (256U)
#define ETHAPP_IPC_DATA_SIZE                    (ETHAPP_IPC_RPC_MSG_SIZE * \
                                                 ETHAPP_IPC_NUM_RPMSG_BUFS + \
                                                 ETHAPP_IPC_RPMSG_OBJ_SIZE)

#define ARRAY_SIZE(x)                           (sizeof((x)) / sizeof(x[0U]))

/* Default port mask for shared multicast addresses: all ports in switch mode */
#if defined(SOC_J721E)
#define ETHAPP_DFLT_PORT_MASK                   (CPSW_ALE_HOST_PORT_MASK | \
                                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_2) | \
                                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_3) | \
                                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_5) | \
                                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_6) | \
                                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_7) | \
                                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_8))
#elif defined(SOC_J7200)
#define ETHAPP_DFLT_PORT_MASK                   (CPSW_ALE_HOST_PORT_MASK | \
                                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_2) | \
                                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_3))
#elif defined(SOC_J784S4)
#define ETHAPP_DFLT_PORT_MASK                   (CPSW_ALE_HOST_PORT_MASK | \
                                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_3) | \
                                                 CPSW_ALE_MACPORT_TO_PORTMASK(ENET_MAC_PORT_5))
#endif

/* Default virtual port mask for shared multicast addresses: all virtual switch ports */
#define ETHAPP_DFLT_VIRT_PORT_MASK              (ETHFW_BIT(ETHREMOTECFG_SWITCH_PORT_0) | \
                                                 ETHFW_BIT(ETHREMOTECFG_SWITCH_PORT_1) | \
                                                 ETHFW_BIT(ETHREMOTECFG_SWITCH_PORT_2))

#if defined(SAFERTOS)
#define ETHAPP_LWIP_TASK_STACKSIZE              (16U * 1024U)
#define ETHAPP_TRACEBUF_TASK_STACKSIZE          (16U * 1024U)
#define ETHAPP_INIT_TASK_STACKSIZE              (16U * 1024U)
#define ETHAPP_LWIP_TASK_STACKALIGN             ETHAPP_LWIP_TASK_STACKSIZE
#define ETHAPP_TRACEBUF_TASK_STACKALIGN         ETHAPP_TRACEBUF_TASK_STACKSIZE
#define ETHAPP_INIT_TASK_STACKALIGN             ETHAPP_INIT_TASK_STACKSIZE
#define ETHAPP_IPC_TASK_STACKALIGN              IPC_TASK_STACKSIZE
#else
#define ETHAPP_LWIP_TASK_STACKSIZE              (4U * 1024U)
#define ETHAPP_TRACEBUF_TASK_STACKSIZE          (1U * 1024U)
#define ETHAPP_INIT_TASK_STACKSIZE              (16U * 1024U)
#define ETHAPP_LWIP_TASK_STACKALIGN             (32)
#define ETHAPP_TRACEBUF_TASK_STACKALIGN         (32)
#define ETHAPP_INIT_TASK_STACKALIGN             (32)
#define ETHAPP_IPC_TASK_STACKALIGN              (8192U)
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
#if defined(ETHFW_BOOT_TIME_PROFILING)
#define ETHAPP_LWIP_USE_DHCP            (0)
#else
#define ETHAPP_LWIP_USE_DHCP            (1)
#endif
#if !ETHAPP_LWIP_USE_DHCP
#define ETHFW_SERVER_IPADDR(addr)       IP4_ADDR((addr), 192,168,1,200)
#define ETHFW_SERVER_GW(addr)           IP4_ADDR((addr), 192,168,1,1)
#define ETHFW_SERVER_NETMASK(addr)      IP4_ADDR((addr), 255,255,255,0)
#endif

/* BridgeIf configuration parameters */
#define ETHAPP_LWIP_BRIDGE_MAX_PORTS (4U)
#define ETHAPP_LWIP_BRIDGE_MAX_DYNAMIC_ENTRIES (32U)
#define ETHAPP_LWIP_BRIDGE_MAX_STATIC_ENTRIES (8U)

/* BridgeIf port IDs
 * Used for creating CoreID to Bridge PortId Map
 */
#define ETHAPP_BRIDGEIF_PORT1_ID        (1U)
#define ETHAPP_BRIDGEIF_PORT2_ID        (2U)
#define ETHAPP_BRIDGEIF_CPU_PORT_ID     BRIDGEIF_MAX_PORTS

/* Inter-core netif IDs */
#define ETHAPP_NETIF_IC_MCU2_0_MCU2_1_IDX   (0U)
#define ETHAPP_NETIF_IC_MCU2_0_A72_IDX      (1U)
#define ETHAPP_NETIF_IC_MAX_IDX             (2U)

/* Max length of shared mcast address list */
#define ETHAPP_MAX_SHARED_MCAST_ADDR        (8U)

/* Required size of the MAC address pool (specific to the TI EVM configuration):
 *  1 x MAC address for Ethernet Firmware
 *  2 x MAC address for mpu1_0 virtual switch and MAC-only ports (Linux, 1 for QNX)
 *  2 x MAC address for mcu2_1 virtual switch and MAC-only ports (RTOS)
 *  1 x MAC address for mcu2_1 virtual switch port (AUTOSAR) */
#define ETHAPP_MAC_ADDR_POOL_SIZE               (6U)

#if defined(ETHFW_BOOT_TIME_PROFILING)
/* Time taken for a given boot milestone since app's main() */
#define ETHFW_BOOT_PROFILING_TS_DIFF(id)       (gEthAppObj.bootTs[ETHFW_BOOT_PROFILING_TS_##id] - \
                                                gEthAppObj.bootTs[ETHFW_BOOT_PROFILING_TS_MAIN])
#endif

/* Define it to enable logs in the stats monitoring event callbacks */
#undef ETHAPP_STATSMON_EVT_LOG_EN

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

#if defined(ETHFW_BOOT_TIME_PROFILING)
/* Boot profiling ids */
typedef enum EthApp_BootTsId_e
{
    ETHFW_BOOT_PROFILING_TS_MAIN = 0U,
    ETHFW_BOOT_PROFILING_TS_IPC,
    ETHFW_BOOT_PROFILING_TS_ETH_PORT,
    ETHFW_BOOT_PROFILING_TS_HOST_PORT,
    ETHFW_BOOT_PROFILING_TS_TCPIP,
    ETHFW_BOOT_PROFILING_TS_PTP,
    ETHFW_BOOT_PROFILING_TS_MAX,
} EthApp_BootTsId;
#endif

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

    /* Host MAC address */
    uint8_t hostMacAddr[ENET_MAC_ADDR_LEN];

#if defined(ETHFW_GPTP_SUPPORT)
    /* Semaphore used to indicate when host port MAC address has been allocated */
    SemaphoreP_Handle hHostMacAllocSem;
#endif

    /* IPC trace buffer address */
    uint8_t *traceBufAddr;

    /* IPC trace buffer size */
    uint32_t traceBufSize;

    /* Timestamp of last IPC trace buffer flush */
    uint64_t traceBufLastFlushTicksInUsecs;

    /* DHCP network interface */
    struct dhcp dhcpNetif;

#if defined(ETHFW_BOOT_TIME_PROFILING)
    /* ETHFW boot time */
    uint64_t bootTs[ETHFW_BOOT_PROFILING_TS_MAX];

    /* UART logging enabled */
    bool uartPrintEn;
#endif
} EthAppObj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

void appLogPrintf(const char *format, ...);

static void EthApp_waitForDebugger(void);

static void EthApp_initTaskFxn(void* arg0, void* arg1);

static void EthApp_initIpcTaskFxn(void* arg0, void* arg1);

static int32_t EthApp_boardInit(void);

static int32_t EthApp_initEthFw(void);

static int32_t EthApp_initRemoteServices(void);

#if defined(ETHFW_DEMO_SUPPORT)
static void EthApp_startSwInterVlan(char *recvBuff,
                                    char *sendBuff);

static void EthApp_startHwInterVlan(char *recvBuff,
                                    char *sendBuff);
#endif

static void EthApp_lwipMain(void *a0,
                            void *a1);

static void EthApp_initLwip(void *arg);

static void EthApp_initNetif(void);

static void EthApp_netifStatusCb(struct netif *netif);

#if defined(ETHFW_MONITOR_SUPPORT)
static void EthApp_closeDmaCb(void *arg);

static void EthApp_openDmaCb(void *arg);

static void EthApp_statsMonHostEvtCb(uint32_t evtMask,
                                     const EthFwMon_Stats *monStats,
                                     const CpswStats_HostPort_Ng *stats,
                                     void *arg);

static void EthApp_statsMonMacEvtCb(Enet_MacPort macPort,
                                    uint32_t evtMask,
                                    const EthFwMon_Stats *monStats,
                                    const CpswStats_MacPort_Ng *stats,
                                    void *arg);
#endif

static void EthApp_traceBufFlush(void* arg0, void* arg1);

#if defined(ETHFW_GPTP_SUPPORT)
static void EthApp_configPtpCb(void *arg);

static void EthApp_initPtp(void);
#endif

#if defined(ETHFW_BOOT_TIME_PROFILING)
static void EthApp_setBootTs(EthApp_BootTsId tsId);
#endif

#if defined(ETHAPP_ENABLE_INTERCORE_ETH) || defined(ETHFW_VEPA_SUPPORT)
static void EthApp_filterAddMacSharedCb(const uint8_t *mac_address,
                                        uint16_t vlanId,
                                        uint8_t hostId);

static void EthApp_filterDelMacSharedCb(const uint8_t *mac_address,
                                        uint16_t vlanId,
                                        uint8_t hostId);

/* Array to store coreId to lwip bridge portId map */
static uint8_t gEthApp_lwipBridgePortIdMap[IPC_MAX_PROCS];

/* Must not exceed ETHAPP_MAX_SHARED_MCAST_ADDR entries */
static EthFwMcast_McastCfg gEthApp_sharedMcastCfgTable[] =
{
    {
        /* MCast IP ADDR: 224.0.0.1 */
        .macAddr      = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x01},
        .vlanId       = 0U,
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        /* MCast IP ADDR: 224.0.0.251 */
        .macAddr      = {0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB},
        .vlanId       = 0U,
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        /* MCast IP ADDR: 224.0.0.252 */
        .macAddr      = {0x01, 0x00, 0x5E, 0x00, 0x00, 0xFC},
        .vlanId       = 0U,
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        .macAddr      = {0x33, 0x33, 0x00, 0x00, 0x00, 0x01},
        .vlanId       = 0U,
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        .macAddr      = {0x33, 0x33, 0xFF, 0x1D, 0x92, 0xC2},
        .vlanId       = 0U,
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        .macAddr      = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00},
        .vlanId       = 0U,
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
    {
        .macAddr      = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x03},
        .vlanId       = 0U,
        .portMask     = ETHAPP_DFLT_PORT_MASK,
        .virtPortMask = ETHAPP_DFLT_VIRT_PORT_MASK,
    },
};

static bridgeif_portmask_t gEthApp_bridgePortMask[ARRAY_SIZE(gEthApp_sharedMcastCfgTable)];
#endif

/* List of multicast addresses reserved for EthFw. Currently, this list is populated
 * only with PTP related multicast addresses which are used by the test PTP stack
 * used by EthFw.
 * Note: Must not exceed ETHFW_RSVD_MCAST_LIST_LEN */
static EthFwMcast_RsvdMcast gEthApp_rsvdMcastCfgTable[] =
{
    /* PTP - Peer delay messages */
    {
        .macAddr  = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0E},
        .vlanId   = 0U,
        .portMask = ETHAPP_DFLT_PORT_MASK,
    },
    /* PTP - Non peer delay messages */
    {
        .macAddr  = {0x01, 0x1b, 0x19, 0x00, 0x00, 0x00},
        .vlanId   = 0U,
        .portMask = ETHAPP_DFLT_PORT_MASK,
    },
};

void EthApp_traceBufCacheWb(void);

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */

extern char Ipc_traceBuffer[IPC_TRACEBUF_SIZE];

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

#if defined(SAFERTOS)
static sys_sem_t gEthApp_lwipMainTaskSemObj;
#endif

static EthAppObj gEthAppObj =
{
#if defined(SOC_J721E) || defined(SOC_J784S4)
    .enetType = ENET_CPSW_9G,
    .instId   = 0U,
#elif defined(SOC_J7200)
    .enetType = ENET_CPSW_5G,
    .instId   = 0U,
#endif
    .hEthFw = NULL,
    .hUdmaDrv = NULL,
};

static Enet_MacPort gEthAppPorts[] =
{
#if defined(SOC_J721E)
#if defined(ETHFW_BOOT_TIME_PROFILING)
    ENET_MAC_PORT_2, /* SGMII */
#else
    ENET_MAC_PORT_1, /* RGMII */
    ENET_MAC_PORT_3, /* RGMII */
    ENET_MAC_PORT_4, /* RGMII */
    ENET_MAC_PORT_8, /* RGMII */
#if defined(ENABLE_QSGMII_PORTS)
    ENET_MAC_PORT_2, /* QSGMII main */
    ENET_MAC_PORT_5, /* QSGMII sub */
    ENET_MAC_PORT_6, /* QSGMII sub */
    ENET_MAC_PORT_7, /* QSGMII sub */
#endif
#endif
#endif

#if defined(SOC_J7200)
#if defined(ETHFW_BOOT_TIME_PROFILING)
    ENET_MAC_PORT_1, /* SGMII */
#else
    ENET_MAC_PORT_1, /* QSGMII main */
    ENET_MAC_PORT_2, /* QSGMII sub */
    ENET_MAC_PORT_3, /* QSGMII sub */
    ENET_MAC_PORT_4, /* QSGMII sub */
#endif
#endif

#if defined(SOC_J784S4)
#if defined(ETHFW_BOOT_TIME_PROFILING)
    ENET_MAC_PORT_2, /* SGMII */
#else
    ENET_MAC_PORT_1, /* QSGMII main */
    ENET_MAC_PORT_3, /* QSGMII sub */
    ENET_MAC_PORT_4, /* QSGMII sub */
    ENET_MAC_PORT_5, /* QSGMII sub */
#endif
#endif
};

#if defined(ETHFW_GPTP_SUPPORT)
/* Ethernet ports where gPTP support is enabled, it must be composed of
 * ports in non MAC-only mode */
static Enet_MacPort gEthAppSwitchPorts[]=
{
#if defined(SOC_J721E)
#if defined(ETHFW_BOOT_TIME_PROFILING)
    ENET_MAC_PORT_2, /* SGMII */
#else
    ENET_MAC_PORT_3,
    ENET_MAC_PORT_8,
#if defined(ENABLE_QSGMII_PORTS)
    ENET_MAC_PORT_2,
    ENET_MAC_PORT_5,
#endif
#endif
#endif

#if defined(SOC_J7200)
#if defined(ETHFW_BOOT_TIME_PROFILING)
    ENET_MAC_PORT_1,
#else
    ENET_MAC_PORT_2,
    ENET_MAC_PORT_3,
#endif
#endif

#if defined(SOC_J784S4)
#if defined(ETHFW_BOOT_TIME_PROFILING)
    ENET_MAC_PORT_2,
#else
    ENET_MAC_PORT_3,
    ENET_MAC_PORT_5,
#endif
#endif
};
#endif

/* Custom policers which clients need to provide to add their own policers.
 * Make sure that size of this array is <= ETHFW_UTILS_NUM_CUSTOM_POLICERS */
static CpswAle_SetPolicerEntryInPartitionInArgs gEthApp_customPolicers[ETHFW_UTILS_NUM_CUSTOM_POLICERS] = {};

/* NOTE: 2 virtual ports should not have same tx channel allocated to them */
static EthFw_VirtPortCfg gEthApp_virtPortCfg[] =
{
    {
        .remoteCoreId  = IPC_MPU1_0,
        .portId        = ETHREMOTECFG_SWITCH_PORT_0,
#if defined(ETHFW_EST_DEMO_SUPPORT) || defined(ETHFW_DEMO_SUPPORT)
        .numTxCh       = 1U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_4
                         },
#else
         .numTxCh      = 2U,
         .txCh         = {
                            [0] = ENET_RM_TX_CH_4,
                            [1] = ENET_RM_TX_CH_7
                         },
#endif
        /* Number of rx flow for this virtual port */
        .numRxFlow     = 1U,
        /* To create custom policers on rx flows clients need to give flow information (i.e. numCustomPolicers and customPolicersInArgs)
         * for each allocated flow.
         * Map the customPolicersInArgs with global custom policer's (i.e. gEthApp_customPolicers) array.
         * For example if numRxFlow is 1 and we want to create 1 custom policer to match with 2'nd custom policer in global array do this: 
         * .rxFlowsInfo = {  
         *                  [0] = {
         *                           .numCustomPolicers    = 1U,
         *                           .customPolicersInArgs = {
         *                                                       [0] = &gEthApp_customPolicers[2U],
         *                                                   }
         *                         }
         *               },
         * It is important to note that number of custom policers per rx flow is <= ETHREMOTECFG_POLICER_PERFLOW */
        .numMacAddress = 1U,
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_LINUX) | ETHFW_BIT(ETHREMOTECFG_CLIENTID_QNX),
    },
    {
        .remoteCoreId  = IPC_MCU1_0,
        .portId        = ETHREMOTECFG_SWITCH_PORT_2,
        .numTxCh       = 1U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_5,
                         },
        .numRxFlow     = 1U,
        .numMacAddress = 1U,
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_AUTOSAR),
    },
    {
        /* Virtual switch port for Ethfw, using ETHREMOTECFG_SWITCH_PORT_LAST */
        .remoteCoreId  = IPC_MCU2_0,
        .portId        = ETHREMOTECFG_SWITCH_PORT_LAST,
        /*
         * When running the demo app, we require two Tx channels one for gPTP and one for EST
         * and the allocation happens in order, hence gPTP gets 7th channel first and then EST
         * gets the 6th channel */
#if defined(ETHFW_EST_DEMO_SUPPORT) || defined(ETHFW_DEMO_SUPPORT)
        .numTxCh       = 3U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_0,
                            [1] = ENET_RM_TX_CH_7,
                            [2] = ENET_RM_TX_CH_6
                         },
#else
        .numTxCh       = 2U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_0,
                            [1] = ENET_RM_TX_CH_6
                         },
#endif
        .numRxFlow     = 5U,
        .numMacAddress = 1U,
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_NONE),
    },
    {
        /* SWITCH_PORT_1 is used for both RTOS and Autosar client */
        .remoteCoreId  = IPC_MCU2_1,
        .portId        = ETHREMOTECFG_SWITCH_PORT_1,
        .numTxCh       = 1U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_1
                         },
        .numRxFlow     = 1U,
        .numMacAddress = 1U,
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_AUTOSAR) | ETHFW_BIT(ETHREMOTECFG_CLIENTID_RTOS),
    },
#if defined(ENABLE_MAC_ONLY_PORTS)
    {
        .remoteCoreId  = IPC_MPU1_0,
        .portId        = ETHREMOTECFG_MAC_PORT_1,
        .numTxCh       = 1U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_3
                         },
        .numRxFlow     = 1U,
        .numMacAddress = 1U,
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_LINUX) | ETHFW_BIT(ETHREMOTECFG_CLIENTID_QNX),
    },
    {
        .remoteCoreId  = IPC_MCU2_1,
        .portId        = ETHREMOTECFG_MAC_PORT_4,
        .numTxCh       = 1U,
        .txCh          = {
                            [0] = ENET_RM_TX_CH_2
                         },
        .numRxFlow     = 1U,
        .numMacAddress = 1U,
        .clientIdMask  = ETHFW_BIT(ETHREMOTECFG_CLIENTID_RTOS),
    },
#endif
};

static EthFw_AllocCfg gEthApp_allocCfg[] =
{
        {
            .clientId = ETHREMOTECFG_CLIENTID_AUTOSAR,
            .remoteProcId = IPC_MCU2_1,
            .virtSwitchPortMask = ENET_MACPORT_MASK(ETHREMOTECFG_SWITCH_PORT_1),
#if defined(ENABLE_MAC_ONLY_PORTS)
            .virtMacPortMask = 0,
#endif
        },
        {
            .clientId = ETHREMOTECFG_CLIENTID_AUTOSAR,
            .remoteProcId = IPC_MCU1_0,
            .virtSwitchPortMask = ENET_MACPORT_MASK(ETHREMOTECFG_SWITCH_PORT_2),
#if defined(ENABLE_MAC_ONLY_PORTS)
            .virtMacPortMask = 0,
#endif
        },
        {
            .clientId = ETHREMOTECFG_CLIENTID_RTOS,
            .remoteProcId = IPC_MCU2_1,
            .virtSwitchPortMask = ENET_MACPORT_MASK(ETHREMOTECFG_SWITCH_PORT_1),
#if defined(ENABLE_MAC_ONLY_PORTS)
            .virtMacPortMask = ENET_MACPORT_MASK(ETHREMOTECFG_MAC_PORT_4),
#endif
        },
        {
            .clientId = ETHREMOTECFG_CLIENTID_LINUX,
            .remoteProcId = IPC_MPU1_0,
            .virtSwitchPortMask = ENET_MACPORT_MASK(ETHREMOTECFG_SWITCH_PORT_0),
#if defined(ENABLE_MAC_ONLY_PORTS)
            .virtMacPortMask = ENET_MACPORT_MASK(ETHREMOTECFG_MAC_PORT_1),
#endif
        },
        {
            .clientId = ETHREMOTECFG_CLIENTID_QNX,
            .remoteProcId = IPC_MPU1_0,
            .virtSwitchPortMask = ENET_MACPORT_MASK(ETHREMOTECFG_SWITCH_PORT_0),
#if defined(ENABLE_MAC_ONLY_PORTS)
            .virtMacPortMask = ENET_MACPORT_MASK(ETHREMOTECFG_MAC_PORT_1),
#endif
        },
};

/* Test VLAN config */
static EthFwVlan_VlanCfg gEthApp_vlanCfg[] =
{
    {
        .vlanId              = 1024U,
        .memberMask          = ETHAPP_DFLT_PORT_MASK,
        .regMcastFloodMask   = ETHAPP_DFLT_PORT_MASK,
        .unregMcastFloodMask = ETHAPP_DFLT_PORT_MASK,
        .virtMemberMask      = ETHAPP_DFLT_VIRT_PORT_MASK,
        .untagMask           = 0U,
    },
};

static uint8_t gEthAppStackBuf[ETHAPP_INIT_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(ETHAPP_INIT_TASK_STACKALIGN)));

static uint8_t gEthAppLwipStackBuf[ETHAPP_LWIP_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__((aligned(ETHAPP_LWIP_TASK_STACKALIGN)));

static uint8_t gEthAppTraceBufFlushBuf[ETHAPP_TRACEBUF_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__((aligned(ETHAPP_TRACEBUF_TASK_STACKALIGN)));

static uint8_t gEthAppIpcInitStackBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(ETHAPP_IPC_TASK_STACKALIGN)));

static uint8_t gEthAppCtrlTaskBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(ETHAPP_IPC_TASK_STACKALIGN)));

static uint8_t gEthAppSysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section("ipc_data_buffer"), aligned(8)));

static uint8_t gEthAppCntrlBuf[ETHAPP_IPC_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned(8)));

static uint8_t gEthAppVringMemBuf[IPC_VRING_MEM_SIZE] __attribute__ ((section(".bss:ipc_vring_mem"), aligned(8192)));

static uint32_t gEthAppRemoteProc[] =
{
#if defined(SOC_J721E)
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_1,
    IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2,
    IPC_C7X_1,
#elif defined(SOC_J7200)
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_1,
#elif defined(SOC_J784S4)
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_1,
    IPC_MCU3_0, IPC_MCU3_1, IPC_MCU4_0, IPC_MCU4_1,
    IPC_C7X_1,  IPC_C7X_2,  IPC_C7X_3,  IPC_C7X_4,
#endif
};

static struct netif netif;
#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
static struct netif netif_ic[ETHAPP_NETIF_IC_MAX_IDX];

static uint32_t netif_ic_state[IC_ETH_MAX_VIRTUAL_IF] =
{
    IC_ETH_IF_MCU2_0_MCU2_1,
    IC_ETH_IF_MCU2_1_MCU2_0,
    IC_ETH_IF_MCU2_0_A72
};

static struct netif netif_bridge;
bridgeif_initdata_t bridge_initdata;
#endif /* ETHAPP_ENABLE_INTERCORE_ETH */

#if defined(ETHFW_VEPA_SUPPORT)
/* Private VLAN ids used in broadcast/multicast packets sent from ETHFW
 * to remote clients using multihost flow */
static uint32_t gEthApp_remoteClientPrivVlanIdMap[ETHREMOTECFG_SWITCH_PORT_LAST+1] =
{
    [ETHREMOTECFG_SWITCH_PORT_0] = 1100U, /* Linux client */
    [ETHREMOTECFG_SWITCH_PORT_1] = 1200U, /* AUTOSAR or RTOS client */
    [ETHREMOTECFG_SWITCH_PORT_2] = 1300U, /* AUTOSAR client */
};
#endif

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int main(void)
{
    TaskP_Handle task;
    TaskP_Params taskParams;

#if defined(ETHFW_BOOT_TIME_PROFILING)
    memset(gEthAppObj.bootTs, 0, sizeof(gEthAppObj.bootTs));
    gEthAppObj.uartPrintEn = BFALSE;
#endif

    OS_init();

    /* Wait for debugger to attach (disabled by default) */
    EthApp_waitForDebugger();

    gEthAppObj.coreId = EnetSoc_getCoreId();

    /* Create initialization task */
    TaskP_Params_init(&taskParams);
    taskParams.priority = 2;
    taskParams.stack = &gEthAppStackBuf[0];
    taskParams.stacksize = sizeof(gEthAppStackBuf);
    taskParams.name = "EthFw Init Task";

    task = TaskP_create(&EthApp_initTaskFxn, &taskParams);
    if (NULL == task)
    {
        OS_stop();
    }

    /* Does not return */
    OS_start();

    return(0);
}

void appLogPrintf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    System_vprintf(format, args);
#if defined(ETHFW_BOOT_TIME_PROFILING)
    if (gEthAppObj.uartPrintEn)
    {
        EnetAppUtils_vprint(format, args);
    }
#else
    EnetAppUtils_vprint(format, args);
#endif
    va_end(args);
}

static void EthApp_ipcPrint(const char *str)
{
    appLogPrintf("%s", str);
    return;
}

static void EthApp_waitForDebugger(void)
{
    /* Set ccsHaltFlag to 1 for halting core for CCS connection */
    volatile uint32_t ccsHaltFlag = 0U;

    while (ccsHaltFlag);
}

static int32_t EthApp_boardInit(void)
{
    uint32_t flags = 0U;
    int32_t status;

#if defined(ETHFW_BOOT_TIME_PROFILING)
    flags = (ETHFW_BOARD_ENET_BRIDGE_ENABLE |
             ETHFW_BOARD_SERDES_CONFIG |
             ETHFW_BOARD_UART_ALLOWED |
             ETHFW_BOARD_I2C_ALLOWED);
#else /* !ETHFW_BOOT_TIME_PROFILING */
#if defined(SOC_J721E)
    flags |= (ETHFW_BOARD_GESI_ENABLE | ETHFW_BOARD_UART_ALLOWED);
#if defined(ETHFW_CCS)
    flags |= (ETHFW_BOARD_SERDES_CONFIG | ETHFW_BOARD_I2C_ALLOWED | ETHFW_BOARD_GPIO_ALLOWED);
#endif
#if defined(ENABLE_QSGMII_PORTS)
    flags |= ETHFW_BOARD_QENET_ENABLE;
#endif
#endif

#if defined(SOC_J7200)
    flags |= (ETHFW_BOARD_QENET_ENABLE | ETHFW_BOARD_SERDES_CONFIG);
#if defined(ETHFW_CCS)
    flags |= (ETHFW_BOARD_UART_ALLOWED | ETHFW_BOARD_I2C_ALLOWED);
#endif
#endif

#if defined(SOC_J784S4)
    flags |= (ETHFW_BOARD_SERDES_CONFIG | ETHFW_BOARD_QENET_ENABLE | ETHFW_BOARD_UART_ALLOWED);
#if defined(ETHFW_CCS)
    flags |= ETHFW_BOARD_I2C_ALLOWED;
#endif
#endif
#endif /* !ETHFW_BOOT_TIME_PROFILING */

    /* Board related initialization */
    status = EthFwBoard_init(flags);

    return status;
}

/* Trace configuration */
static EthFwTrace_Cfg gEthApp_traceCfg =
{
    .print        = appLogPrintf,
    .traceTsFunc  = NULL,
    .extTraceFunc = NULL,
};

static void EthApp_initTaskFxn(void* arg0, void* arg1)
{
#if defined(ETHFW_GPTP_SUPPORT)
    SemaphoreP_Params semParams;
#endif
    TaskP_Params taskParams;
    int32_t status = ETHAPP_OK;

#if defined(ETHFW_BOOT_TIME_PROFILING)
    /* EthFw's initial timestamp, used as offset for further profiling timestamps */
    EthApp_setBootTs(ETHFW_BOOT_PROFILING_TS_MAIN);
#endif

    /* Initialize EthFw trace utils */
    if (status == ENET_SOK)
    {
        EthFwTrace_init(&gEthApp_traceCfg);
        EthFwTrace_setLevel(ETHFW_TRACE_INFO);
    }

    /* Board initialization: Serdes, GPIOs, pinmux, etc */
    status = EthApp_boardInit();
    if (status != ENET_SOK)
    {
        appLogPrintf("ETHFW: Board initialization failed\n");
    }

    /* Print EthFw banner */
    if (status == ETHAPP_OK)
    {
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
    }

#if defined(ETHFW_GPTP_SUPPORT)
    /* Create semaphore used to synchronize MAC alloc by lwIP which is required and
     * shared with the gPTP stack */
    if (status == ETHAPP_OK)
    {
        SemaphoreP_Params_init(&semParams);
        semParams.mode = SemaphoreP_Mode_BINARY;

        gEthAppObj.hHostMacAllocSem = SemaphoreP_create(0, &semParams);
        if (gEthAppObj.hHostMacAllocSem == NULL)
        {
            appLogPrintf("ETHFW: failed to create hostport MAC addr semaphore\n");
            status = ETHAPP_ERROR;
        }
    }
#endif

    /* Initialize Ethernet Firmware */
    if (status == ETHAPP_OK)
    {
        status = EthApp_initEthFw();
    }

    /* Initialize lwIP */
    if (status == ENET_SOK)
    {
        TaskP_Params_init(&taskParams);
        taskParams.priority  = DEFAULT_THREAD_PRIO;
        taskParams.stack     = &gEthAppLwipStackBuf[0];
        taskParams.stacksize = sizeof(gEthAppLwipStackBuf);
#if defined(SAFERTOS)
        taskParams.userData  = &gEthApp_lwipMainTaskSemObj;
#endif
        taskParams.name      = "lwIP main loop";

        TaskP_create(&EthApp_lwipMain, &taskParams);
    }

    /* Create IPC initialization task */
    if (status == ENET_SOK)
    {
        TaskP_Params_init(&taskParams);
        taskParams.priority = 1;
        taskParams.stack = &gEthAppIpcInitStackBuf[0];
        taskParams.stacksize = sizeof(gEthAppIpcInitStackBuf);
        taskParams.name = "EthFw IPC init Task";

        TaskP_create(&EthApp_initIpcTaskFxn, &taskParams);
    }

#if defined(ETHFW_GPTP_SUPPORT)
    /* Initialize gPTP stack */
    if (status == ENET_SOK)
    {
        EthApp_initPtp();
    }
#endif

}

static void EthApp_initIpcTaskFxn(void* arg0, void* arg1)
{
    uint32_t selfProcId = IPC_MCU2_0;
    uint32_t numProc = ARRAY_SIZE(gEthAppRemoteProc);
    Ipc_VirtIoParams vqParam;
    Ipc_InitPrms initPrms;
    int32_t status;
    TaskP_Params taskParams;
    RPMessage_Params cntrlParam;

    /* Step 1: Initialize the multiproc */
    Ipc_mpSetConfig(selfProcId, numProc, &gEthAppRemoteProc[0]);

    /* Task to flush IPC traceBuf */
    TaskP_Params_init(&taskParams);
    taskParams.priority  = 0;
    taskParams.stack     = &gEthAppTraceBufFlushBuf[0];
    taskParams.stacksize = sizeof(gEthAppTraceBufFlushBuf);
    taskParams.name      = "IPC tracebuf flush";

    TaskP_create(&EthApp_traceBufFlush, &taskParams);

    /* Initialize params with defaults */
    IpcInitPrms_init(0U, &initPrms);

    initPrms.printFxn = &EthApp_ipcPrint;

    status = Ipc_init(&initPrms);

#if !defined(A72_QNX_OS)
    if (status == ENET_SOK)
    {
        status = Ipc_loadResourceTable(appGetIpcResourceTable());
    }
#else
    appLogPrintf("Skipping Ipc_loadResourceTable for QNX (core : %s) .....\r\n", Ipc_mpGetSelfName());
#endif

    if (status == ENET_SOK)
    {
        /* Trace buffer */
        gEthAppObj.traceBufAddr = (uint8_t *)Ipc_traceBuffer;
        gEthAppObj.traceBufSize = IPC_TRACEBUF_SIZE;
        gEthAppObj.traceBufLastFlushTicksInUsecs = 0ULL;
    }

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
        cntrlParam.bufSize = ETHAPP_IPC_DATA_SIZE;
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

#if defined(ETHFW_BOOT_TIME_PROFILING)
    EthApp_setBootTs(ETHFW_BOOT_PROFILING_TS_IPC);
#endif

    /* Wait for Linux VDev ready... */
    if (status == ENET_SOK)
    {
        while (!Ipc_isRemoteReady(IPC_MPU1_0))
        {
            TaskP_sleep(10);
        }
    }

    /* Create the VRing now ... */
    if (status == ENET_SOK)
    {
        /* Create virtio if one hasn't been created already */
        if(!Ipc_isRemoteVirtioCreated(IPC_MPU1_0))
        {
            status = Ipc_lateVirtioCreate(IPC_MPU1_0);
            if (status != IPC_SOK)
            {
                appLogPrintf("EthApp_initIpcTask: Ipc_lateVirtioCreate failed: %d\n", status);
            }
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
}

#if defined(ETHFW_BOOT_TIME_PROFILING)
static void EthApp_portLinkStatusChangeCb(Enet_MacPort macPort,
                                          bool isLinkUp,
                                          void *appArg)
{
    EthApp_setBootTs(ETHFW_BOOT_PROFILING_TS_ETH_PORT);
}
#endif

static int32_t EthApp_initEthFw(void)
{
    EthFw_Version ver;
    EthFw_Config ethFwCfg;
    Cpsw_Cfg *cpswCfg = &ethFwCfg.cpswCfg;
    EnetUdma_Cfg dmaCfg;
    EnetRm_MacAddressPool *pool = &cpswCfg->resCfg.macList;
    EthFwMcast_SharedMcastCfg *sharedMcastCfg = &ethFwCfg.mcastCfg.sharedMcastCfg;
    EthFwMcast_RsvdMcastCfg *rsvdMcastCfg = &ethFwCfg.mcastCfg.rsvdMcastCfg;
    uint32_t poolSize;
    int32_t status = ETHAPP_OK;
    uint32_t i;

    /* Init EthFw config params */
    EthFw_initConfigParams(gEthAppObj.enetType, &ethFwCfg);

    /* Set UDMA handle to Enet LLD config */
    dmaCfg.hUdmaDrv = gEthAppObj.hUdmaDrv;
    dmaCfg.rxChInitPrms.dmaPriority = UDMA_DEFAULT_RX_CH_DMA_PRIORITY;
    cpswCfg->dmaCfg = (void *)&dmaCfg;

    /* Populate MAC address pool */
    poolSize = EnetUtils_min(ENET_ARRAYSIZE(pool->macAddress), ETHAPP_MAC_ADDR_POOL_SIZE);
    pool->numMacAddress = EthFwBoard_getMacAddrPool(pool->macAddress, poolSize);

    /* Set hardware port configuration parameters */
    ethFwCfg.ports    = &gEthAppPorts[0];
    ethFwCfg.numPorts = ARRAY_SIZE(gEthAppPorts);
    ethFwCfg.setPortCfg = EthFwBoard_setPortCfg;

    /* Set virtual port configuration parameters */
    ethFwCfg.virtPortCfg  = &gEthApp_virtPortCfg[0];
    ethFwCfg.numVirtPorts = ARRAY_SIZE(gEthApp_virtPortCfg);
    ethFwCfg.isStaticTxChanAllocated = BTRUE;

    /* Set static VLAN configuration parameters */
    ethFwCfg.vlanCfg  = &gEthApp_vlanCfg[0];
    ethFwCfg.numVlans = ARRAY_SIZE(gEthApp_vlanCfg);

    /* CPTS_RFT_CLK is sourced from MAIN_SYSCLK0 (500MHz) */
    cpswCfg->cptsCfg.cptsRftClkFreq = CPSW_CPTS_RFTCLK_FREQ_500MHZ;

    /* Set remote client object parameters */
    ethFwCfg.allocCfg = &gEthApp_allocCfg[0];
    ethFwCfg.numAlloc = ARRAY_SIZE(gEthApp_allocCfg);

#if defined(ETHFW_MONITOR_SUPPORT)
    /* Save the Lwip Dma parametrers */
    ethFwCfg.monitorCfg.lwipDmaCbArg      = (void *)&netif;
    ethFwCfg.monitorCfg.openLwipDmaCb     = EthApp_openDmaCb;
    ethFwCfg.monitorCfg.closeLwipDmaCb    = EthApp_closeDmaCb;
    ethFwCfg.monitorCfg.statsMonHostEvtCb = EthApp_statsMonHostEvtCb;
    ethFwCfg.monitorCfg.statsMonMacEvtCb  = EthApp_statsMonMacEvtCb;
    ethFwCfg.monitorCfg.statsMonCbArg     = NULL;
#endif

#if defined(ETHFW_GPTP_SUPPORT)
    /* gPTP stack config parameters */
    ethFwCfg.configPtpCb    = EthApp_configPtpCb;
    ethFwCfg.configPtpCbArg = NULL;
#endif

#if defined(ETHFW_BOOT_TIME_PROFILING)
    /* Link-up timestamp */
    cpswCfg->portLinkStatusChangeCb    = &EthApp_portLinkStatusChangeCb;
    cpswCfg->portLinkStatusChangeCbArg = &gEthAppObj;
#endif

#if defined(ETHFW_DEMO_SUPPORT)
    /* Overwrite config params with those for hardware interVLAN */
    EthHwInterVlan_setOpenPrms(&ethFwCfg.cpswCfg);
#endif

#if defined(ETHAPP_ENABLE_INTERCORE_ETH) || defined(ETHFW_VEPA_SUPPORT)
    if (ARRAY_SIZE(gEthApp_sharedMcastCfgTable) > ETHAPP_MAX_SHARED_MCAST_ADDR)
    {
        appLogPrintf("ETHFW error: No. of shared mcast addr cannot exceed %d\n",
                    ETHAPP_MAX_SHARED_MCAST_ADDR);
        status = ETHAPP_ERROR;
    }
    else
    {
        sharedMcastCfg->mcastCfg = &gEthApp_sharedMcastCfgTable[0U];
        sharedMcastCfg->numMcast = ARRAY_SIZE(gEthApp_sharedMcastCfgTable);
        sharedMcastCfg->filterAddMacSharedCb = EthApp_filterAddMacSharedCb;
        sharedMcastCfg->filterDelMacSharedCb = EthApp_filterDelMacSharedCb;
    }
#endif

    if (status == ETHAPP_OK)
    {
        if (ARRAY_SIZE(gEthApp_rsvdMcastCfgTable) > ETHFW_RSVD_MCAST_LIST_LEN)
        {
            appLogPrintf("ETHFW error: No. of rsvd mcast addr cannot exceed %d\n",
                         ETHFW_RSVD_MCAST_LIST_LEN);
            status = ETHAPP_ERROR;
        }
        else
        {
            rsvdMcastCfg->mcastCfg = &gEthApp_rsvdMcastCfgTable[0U];
            rsvdMcastCfg->numMcast = ARRAY_SIZE(gEthApp_rsvdMcastCfgTable);
        }
    }

#if defined(ETHFW_VEPA_SUPPORT)
    memcpy(ethFwCfg.vepaCfg.privVlanId,
           gEthApp_remoteClientPrivVlanIdMap,
           sizeof(gEthApp_remoteClientPrivVlanIdMap));
#endif

    /* Initialize the EthFw */
    if (status == ETHAPP_OK)
    {
        gEthAppObj.hEthFw = EthFw_init(gEthAppObj.enetType, &ethFwCfg);
        if (gEthAppObj.hEthFw == NULL)
        {
            appLogPrintf("ETHFW: failed to initialize the firmware\n");
            status = ETHAPP_ERROR;
        }
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

bool EthFwCallbacks_isPortLinked(struct netif *netif,
                                 void *handleArg)
{
    bool linked = BFALSE;
    Enet_Handle hEnet = (Enet_Handle)handleArg;
    uint32_t i;

    /* Report port linked as long as any port owned by EthFw is up */
    for (i = 0U; (i < ARRAY_SIZE(gEthAppPorts)) && !linked; i++)
    {
        linked = EnetAppUtils_isPortLinkUp(hEnet,
                                           gEthAppObj.coreId,
                                           gEthAppPorts[i]);
    }

    return linked;
}

void LwipifEnetAppCb_getHandle(LwipifEnetAppIf_GetHandleInArgs *inArgs,
                               LwipifEnetAppIf_GetHandleOutArgs *outArgs)
{
    EthFwCallbacks_lwipifCpswGetHandle(inArgs, outArgs);

    /* Save host port MAC address */
    EnetUtils_copyMacAddr(&gEthAppObj.hostMacAddr[0U],
                          &outArgs->rxInfo[0U].macAddr[0U]);

#if defined(ETHFW_BOOT_TIME_PROFILING)
    EthApp_setBootTs(ETHFW_BOOT_PROFILING_TS_HOST_PORT);
#endif
#if defined(ETHFW_GPTP_SUPPORT)
    SemaphoreP_post(gEthAppObj.hHostMacAllocSem);
#endif
}

void LwipifEnetAppCb_releaseHandle(LwipifEnetAppIf_ReleaseHandleInfo *releaseInfo)
{
    EthFwCallbacks_lwipifCpswReleaseHandle(releaseInfo);
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

    LWIP_ASSERT("arg != NULL", arg != NULL);
    initSem = (sys_sem_t*)arg;

    /* init randomizer again (seed per thread) */
    srand((unsigned int)sys_now()/1000);

    /* init network interfaces */
    EthApp_initNetif();

#if defined(ETHAPP_ENABLE_IPERF_SERVER)
    /* Enable iperf */
    lwiperf_example_init();
#endif

    sys_sem_signal(initSem);
}

static void EthApp_initNetif(void)
{
    ip4_addr_t ipaddr, netmask, gw;
#if LWIP_CHECKSUM_CTRL_PER_NETIF
    uint32_t chksumFlags = NETIF_CHECKSUM_ENABLE_ALL;
/* Disable checksum in software if CPSW can do it (i.e. not impacted by errata) */
#if !defined(ETHFW_CPSW_MULTIHOST_CHECKSUM_ERRATA)
    chksumFlags &= ~(NETIF_CHECKSUM_GEN_UDP |
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

#if ETHAPP_LWIP_USE_DHCP
    appLogPrintf("Starting lwIP, local interface IP is dhcp-enabled\n");
#else /* ETHAPP_LWIP_USE_DHCP */
    ETHFW_SERVER_GW(&gw);
    ETHFW_SERVER_IPADDR(&ipaddr);
    ETHFW_SERVER_NETMASK(&netmask);
    appLogPrintf("Starting lwIP, local interface IP is %s\n", ip4addr_ntoa(&ipaddr));
#endif /* ETHAPP_LWIP_USE_DHCP */

#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
    /* Create Enet LLD ethernet interface */
    netif_add(&netif, NULL, NULL, NULL, NULL, LWIPIF_LWIP_init, tcpip_input);
#if LWIP_CHECKSUM_CTRL_PER_NETIF
    NETIF_SET_CHECKSUM_CTRL(&netif, chksumFlags);
#endif

    /* Create inter-core virtual ethernet interface: MCU2_0 <-> MCU2_1 */
    netif_add(&netif_ic[ETHAPP_NETIF_IC_MCU2_0_MCU2_1_IDX], NULL, NULL, NULL,
              (void*)&netif_ic_state[IC_ETH_IF_MCU2_0_MCU2_1],
              LWIPIF_LWIP_IC_init, tcpip_input);

    /* Create inter-core virtual ethernet interface: MCU2_0 <-> A72 */
    netif_add(&netif_ic[ETHAPP_NETIF_IC_MCU2_0_A72_IDX], NULL, NULL, NULL,
              (void*)&netif_ic_state[IC_ETH_IF_MCU2_0_A72],
              LWIPIF_LWIP_IC_init, tcpip_input);

    /* Create bridge interface */
    bridge_initdata.max_ports = ETHAPP_LWIP_BRIDGE_MAX_PORTS;
    bridge_initdata.max_fdb_dynamic_entries = ETHAPP_LWIP_BRIDGE_MAX_DYNAMIC_ENTRIES;
    bridge_initdata.max_fdb_static_entries = ETHAPP_LWIP_BRIDGE_MAX_STATIC_ENTRIES;
    EnetUtils_copyMacAddr(&bridge_initdata.ethaddr.addr[0U], &gEthAppObj.hostMacAddr[0U]);

    netif_add(&netif_bridge, &ipaddr, &netmask, &gw, &bridge_initdata, bridgeif_init, netif_input);

    /* Add all netifs to the bridge and create coreId to bridge portId map */
    bridgeif_add_port(&netif_bridge, &netif);
    gEthApp_lwipBridgePortIdMap[IPC_MCU2_0] = ETHAPP_BRIDGEIF_CPU_PORT_ID;

    bridgeif_add_port(&netif_bridge, &netif_ic[0]);
    gEthApp_lwipBridgePortIdMap[IPC_MCU2_1] = ETHAPP_BRIDGEIF_PORT1_ID;

    bridgeif_add_port(&netif_bridge, &netif_ic[1]);
    gEthApp_lwipBridgePortIdMap[IPC_MPU1_0] = ETHAPP_BRIDGEIF_PORT2_ID;

    /* Set bridge interface as the default */
    netif_set_default(&netif_bridge);
#else
    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, LWIPIF_LWIP_init, tcpip_input);
    netif_set_default(&netif);
#if LWIP_CHECKSUM_CTRL_PER_NETIF
    NETIF_SET_CHECKSUM_CTRL(&netif, chksumFlags);
#endif
#endif

    netif_set_status_callback(netif_default, EthApp_netifStatusCb);

    dhcp_set_struct(netif_default, &gEthAppObj.dhcpNetif);

#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
    netif_set_up(&netif);
    netif_set_up(&netif_ic[ETHAPP_NETIF_IC_MCU2_0_MCU2_1_IDX]);
    netif_set_up(&netif_ic[ETHAPP_NETIF_IC_MCU2_0_A72_IDX]);
    netif_set_up(&netif_bridge);
#else
    netif_set_up(netif_default);
#endif

#if ETHAPP_LWIP_USE_DHCP
    err = dhcp_start(netif_default);
    if (err != ERR_OK)
    {
        appLogPrintf("Failed to start DHCP: %d\n", err);
    }
#endif /* ETHAPP_LWIP_USE_DHCP */
}

static void EthApp_netifStatusCb(struct netif *netif)
{
    int32_t status;

    if (netif_is_up(netif))
    {
        const ip4_addr_t *ipAddr = netif_ip4_addr(netif);

        appLogPrintf("Added interface '%c%c%d', IP is %s\n",
                     netif->name[0], netif->name[1], netif->num, ip4addr_ntoa(ipAddr));

        if (ipAddr->addr != 0)
        {
#if defined(ETHFW_BOOT_TIME_PROFILING)
            /* Timestamp when EthFw got an IP */
            EthApp_setBootTs(ETHFW_BOOT_PROFILING_TS_TCPIP);
#endif

#if defined(ETHFW_DEMO_SUPPORT)
            /* Assign functions that are to be called based on actions in GUI.
             * These cannot be dynamically pushed to function pointer array, as the
             * index is used in GUI as command */
            EnetCfgServer_fxn_table[9] = &EthApp_startSwInterVlan;
            EnetCfgServer_fxn_table[10] = &EthApp_startHwInterVlan;

            /* Start Configuration server */
            status = EnetCfgServer_init(gEthAppObj.enetType, gEthAppObj.instId);
            EnetAppUtils_assert(ENET_SOK == status);

            /* Start the software-based interVLAN routing */
            EthSwInterVlan_setupRouting(gEthAppObj.enetType, ETH_SWINTERVLAN_TASK_PRI);
#endif
        }
    }
    else
    {
        appLogPrintf("Removed interface '%c%c%d'\n", netif->name[0], netif->name[1], netif->num);
    }
}

#if defined(ETHFW_MONITOR_SUPPORT)
static void EthApp_closeDmaCb(void *arg)
{
    struct netif *netif = (struct netif *)arg;

    /* Issue link down to Lwip stack and close the Lwip DMA channels */
    sys_lock_tcpip_core();
    netif_set_link_down(netif);
    sys_unlock_tcpip_core();

    LWIPIF_LWIP_closeDma(netif);
}

static void EthApp_openDmaCb(void *arg)
{
    struct netif *netif = (struct netif *)arg;

    /* Open the lwip dma channels and issue link up to Lwip stack */
    LWIPIF_LWIP_openDma(netif);

    sys_lock_tcpip_core();
    netif_set_link_up(netif);
    sys_unlock_tcpip_core();
}

static void EthApp_statsMonHostEvtCb(uint32_t evtMask,
                                     const EthFwMon_Stats *monStats,
                                     const CpswStats_HostPort_Ng *stats,
                                     void *arg)
{
#if defined(ETHAPP_STATSMON_EVT_LOG_EN)
    appLogPrintf("Host port stats monitor (%x): rxBottomOfFifoDrop=%llu rxTopOfFifoDrop=%llu "
                 "txPriDrop=%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
                 evtMask,
                 monStats->rxBottomOfFifoDrop,
                 monStats->rxTopOfFifoDrop,
                 monStats->txPriDrop[0U],
                 monStats->txPriDrop[1U],
                 monStats->txPriDrop[2U],
                 monStats->txPriDrop[3U],
                 monStats->txPriDrop[4U],
                 monStats->txPriDrop[5U],
                 monStats->txPriDrop[6U],
                 monStats->txPriDrop[7]);
#endif
}

static void EthApp_statsMonMacEvtCb(Enet_MacPort macPort,
                                    uint32_t evtMask,
                                    const EthFwMon_Stats *monStats,
                                    const CpswStats_MacPort_Ng *stats,
                                    void *arg)
{
#if defined(ETHAPP_STATSMON_EVT_LOG_EN)
    appLogPrintf("MAC port %u stats monitor (%x): rxBottomOfFifoDrop=%llu txBottomOfFifoDrop=%llu "
                 "txPriDrop=%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
                 ENET_MACPORT_ID(macPort),
                 evtMask,
                 monStats->rxBottomOfFifoDrop,
                 monStats->rxTopOfFifoDrop,
                 monStats->txPriDrop[0U],
                 monStats->txPriDrop[1U],
                 monStats->txPriDrop[2U],
                 monStats->txPriDrop[3U],
                 monStats->txPriDrop[4U],
                 monStats->txPriDrop[5U],
                 monStats->txPriDrop[6U],
                 monStats->txPriDrop[7]);
#endif
}
#endif

static void EthApp_traceBufFlush(void* arg0, void* arg1)
{
    bool exitTask = BFALSE;

    while (!exitTask)
    {
        TaskP_sleepInMsecs(ETHAPP_TRACEBUF_FLUSH_PERIOD_IN_MSEC);
        EthApp_traceBufCacheWb();
    }
}

/* Functions called from Config server library based on selection from GUI */
#if defined(ETHFW_DEMO_SUPPORT)
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
#endif

/* IPC trace buffer flush */
void EthApp_traceBufCacheWb(void)
{
    uint64_t newticksInUsecs = TimerP_getTimeInUsecs();

    /* Don't keep flusing cache */
    if ((newticksInUsecs - gEthAppObj.traceBufLastFlushTicksInUsecs) >=
        (uint64_t)(ETHAPP_TRACEBUF_FLUSH_PERIOD_IN_MSEC * 1000))
    {
        gEthAppObj.traceBufLastFlushTicksInUsecs = newticksInUsecs;

        /* Flush the cache of the traceBuf buffer */
        if (gEthAppObj.traceBufAddr != NULL)
        {
            CacheP_wb((const void *)gEthAppObj.traceBufAddr,
                      gEthAppObj.traceBufSize);
        }
    }
}

#if defined(ETHAPP_ENABLE_INTERCORE_ETH)
/* Application callback function to handle addition of a shared mcast
 * address in the ALE */
static void EthApp_filterAddMacSharedCb(const uint8_t *mac_address,
                                        uint16_t vlanId,
                                        uint8_t hostId)
{
    uint8_t idx = 0;
    struct eth_addr ethaddr;
    bool matchFound = BFALSE;
    int32_t errVal = 0;

    /* Search the mac_address in the shared mcast addr table */
    for (idx = 0; idx < ARRAY_SIZE(gEthApp_sharedMcastCfgTable); idx++)
    {
        if (EnetUtils_cmpMacAddr(mac_address,
                    &gEthApp_sharedMcastCfgTable[idx].macAddr[0]))
        {
            matchFound = BTRUE;
            /* Read and update stored port mask */
            gEthApp_bridgePortMask[idx] |= ETHFW_BIT(gEthApp_lwipBridgePortIdMap[hostId]);

            /* Update bridge fdb entry for this mac_address */
            EnetUtils_copyMacAddr(&ethaddr.addr[0U], mac_address);

            /* There will be a delay between removing existing FDB entry
             * and adding the updated one. During this time, multicast
             * packets will be flodded to all the bridge ports
             */
            bridgeif_fdb_remove(&netif_bridge, &ethaddr);

            errVal = bridgeif_fdb_add(&netif_bridge,
                                      &ethaddr,
                                      gEthApp_bridgePortMask[idx]);

            if (errVal)
            {
                appLogPrintf("addMacSharedCb: bridgeif_fdb_add failed (%d)\n", errVal);
            }

            /* The array should have unique mcast addresses,
             * so no other match is expected
             */
            break;
        }
    }

    if (!matchFound)
    {
        appLogPrintf("addMacSharedCb: Address not found\n");
    }
}

/* Application callback function to handle deletion of a shared mcast
 * address from the ALE */
static void EthApp_filterDelMacSharedCb(const uint8_t *mac_address,
                                        uint16_t vlanId,
                                        uint8_t hostId)
{
    uint8_t idx = 0;
    bridgeif_portmask_t bridgePortMask;
    struct eth_addr ethaddr;
    bool matchFound = BFALSE;
    int32_t errVal = 0;

    /* Search the mac_address in the shared mcast addr table */
    for (idx = 0; idx < ARRAY_SIZE(gEthApp_sharedMcastCfgTable); idx++)
    {
        if (EnetUtils_cmpMacAddr(mac_address,
                    &gEthApp_sharedMcastCfgTable[idx].macAddr[0]))
        {
            matchFound = BTRUE;
            /* Read and update stored port mask */
            gEthApp_bridgePortMask[idx] &= ~ETHFW_BIT(gEthApp_lwipBridgePortIdMap[hostId]);

            /* Update bridge fdb entry for this mac_address */
            EnetUtils_copyMacAddr(&ethaddr.addr[0U], mac_address);

            /* There will be a delay between removing existing FDB entry
             * and adding the updated one. During this time, multicast
             * packets will be flodded to all the bridge ports
             */
            bridgeif_fdb_remove(&netif_bridge, &ethaddr);

            if (gEthApp_bridgePortMask[idx])
            {
                errVal = bridgeif_fdb_add(&netif_bridge,
                                          &ethaddr,
                                          gEthApp_bridgePortMask[idx]);
            }

            if (errVal)
            {
                appLogPrintf("delMacSharedCb: bridgeif_fdb_add failed (%d)\n", errVal);
            }

            /* The array should have unique mcast addresses,
             * so no other match is expected
             */
            break;
        }
    }

    if (!matchFound)
    {
        appLogPrintf("delMacSharedCb: Address not found\n");
    }
}
#endif

#if defined(ETHFW_VEPA_SUPPORT)
/* Application callback function to handle addition of a shared mcast
 * address in the ALE */
static void EthApp_filterAddMacSharedCb(const uint8_t *macAddr,
                                        uint16_t vlanId,
                                        uint8_t hostId)
{
    bool found = BFALSE;
    uint32_t i = 0U;

    /* Search the MAC address in the shared multicast address table */
    for (i = 0U; i < ARRAY_SIZE(gEthApp_sharedMcastCfgTable); i++)
    {
        if (EnetUtils_cmpMacAddr(macAddr, &gEthApp_sharedMcastCfgTable[i].macAddr[0]))
        {
            found = BTRUE;
            break;
        }
    }

    if (!found)
    {
        appLogPrintf("Unexpected shared multicast %02x:%02x:%02x:%02x:%02x:%02x\n",
                     macAddr[0], macAddr[1], macAddr[2],
                     macAddr[3], macAddr[4], macAddr[5]);
    }
}

/* Application callback function to handle deletion of a shared mcast
 * address from the ALE */
static void EthApp_filterDelMacSharedCb(const uint8_t *macAddr,
                                        uint16_t vlanId,
                                        uint8_t hostId)
{
    bool found = BFALSE;
    uint32_t i = 0U;

    /* Search the MAC address in the shared multicast address table */
    for (i = 0U; i < ARRAY_SIZE(gEthApp_sharedMcastCfgTable); i++)
    {
        if (EnetUtils_cmpMacAddr(macAddr, &gEthApp_sharedMcastCfgTable[i].macAddr[0]))
        {
            found = BTRUE;
            break;
        }
    }

    if (!found)
    {
        appLogPrintf("Unexpected shared multicast %02x:%02x:%02x:%02x:%02x:%02x\n",
                     macAddr[0], macAddr[1], macAddr[2],
                     macAddr[3], macAddr[4], macAddr[5]);
    }
}
#endif

#if defined(ETHFW_GPTP_SUPPORT)
static void EthApp_configPtpCb(void *arg)
{
    int32_t useHwPhase = 1;

    /* Apply phase adjustment directly to the HW */
    gptpgcfg_set_item(0, XL4_EXTMOD_XL4GPTP_USE_HW_PHASE_ADJUSTMENT, 
                      YDBI_CONFIG, &useHwPhase, sizeof(useHwPhase));
}

static void EthApp_initPtp(void)
{
    uint32_t portMask = 0U;
    uint8_t i;

    /* MAC port used for PTP */
#if defined(ETHFW_BOOT_TIME_PROFILING)
    portMask = ENET_MACPORT_MASK(gEthAppPorts[0U]);
#else
    for (i = 0U; i < ENET_ARRAYSIZE(gEthAppSwitchPorts); i++)
    {
        portMask |= ENET_MACPORT_MASK(gEthAppSwitchPorts[i]);
    }
#endif

    /* Wait for host port MAC address to be allocated during lwIP getHandle */
    SemaphoreP_pend(gEthAppObj.hHostMacAllocSem, SemaphoreP_WAIT_FOREVER);

    EthFwTsn_initTimeSyncPtp(&gEthAppObj.hostMacAddr[0U], portMask);

#if defined(ETHFW_BOOT_TIME_PROFILING)
    /* Timestamp when gPTP stack is initialized */
    EthApp_setBootTs(ETHFW_BOOT_PROFILING_TS_PTP);
#endif
}
#endif

#if defined(ETHFW_BOOT_TIME_PROFILING)
static void EthApp_setBootTs(EthApp_BootTsId tsId)
{
    bool done = BTRUE;
    uint64_t val;
    uint32_t i;

    val = TimerP_getTimeInUsecs();

    /* Save the profiling timestamp */
    switch (tsId)
    {
        case ETHFW_BOOT_PROFILING_TS_MAIN:
        case ETHFW_BOOT_PROFILING_TS_IPC:
        case ETHFW_BOOT_PROFILING_TS_HOST_PORT:
        case ETHFW_BOOT_PROFILING_TS_PTP:
            gEthAppObj.bootTs[tsId] = val;
            break;

        case ETHFW_BOOT_PROFILING_TS_ETH_PORT:
        case ETHFW_BOOT_PROFILING_TS_TCPIP:
            /* Save first time this event is hit (i.e. new IP, multiple Ethernet ports) */
            if (gEthAppObj.bootTs[tsId] == 0U)
            {
                gEthAppObj.bootTs[tsId] = val;
            }

        default:
            break;
    }

    /* Check if all boot profiling timestamps have been set */
    for (i = 0U; i < ETHFW_BOOT_PROFILING_TS_MAX; i++)
    {
        done &= (gEthAppObj.bootTs[i] != 0);
    }

    if (done)
    {
        /* Re-enable UART prints (which add boot time overhead) once all timestamps are captured */
        gEthAppObj.uartPrintEn = BTRUE;

        /* Print all time diffs wrt app's main() once we have hit all boot milestones */
        EnetAppUtils_print("IPC server  = %llu usecs\n", ETHFW_BOOT_PROFILING_TS_DIFF(IPC));
        EnetAppUtils_print("Eth link-up = %llu usecs\n", ETHFW_BOOT_PROFILING_TS_DIFF(ETH_PORT));
        EnetAppUtils_print("Host port   = %llu usecs\n", ETHFW_BOOT_PROFILING_TS_DIFF(HOST_PORT));
        EnetAppUtils_print("Got IP addr = %llu usecs\n", ETHFW_BOOT_PROFILING_TS_DIFF(TCPIP));
        EnetAppUtils_print("PTP stack   = %llu usecs\n", ETHFW_BOOT_PROFILING_TS_DIFF(PTP));
    }
}
#endif
