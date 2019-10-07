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

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Mailbox.h>

#include <ti/osal/SemaphoreP.h>

#include <ti/drv/ipc/ipc.h>

#include <ethremotecfg/protocol/rpmsg-kdrv-transport-ethswitch.h>
#include <client-rtos/remote-device.h>
#include <ethremotecfg/client/include/ethremotecfg_client.h>

#include <apps/ipc_cfg/app_ipc_rsctable.h>
#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/cpsw/nimucpsw/nimu_ndk.h>
#include <ti/drv/cpsw/nimucpsw/ndk2cpsw_appif.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appsoc.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpswapp_ethutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils_cfg.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils.h>

/* NDK headers */
#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/ndk/inc/socket.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/tools/servers.h>
#include <ti/ndk/inc/tools/console.h>

#include "webpage.h"

#define CPSW_REMOTE_APP_PHY_POLLING_INTERVAL (100)
#define CPSW_REMOTE_APP_PACKET_POLL_PERIOD_MS (1U)

#define IPC_RPMESSAGE_OBJ_SIZE  (256)
#define VQ_TIMEOUT              (100)
#define VQ_BUF_SIZE             (2048)
#define REMOTE_DEVICE_ENDPT     (26)
#define RPMSG_DATA_SIZE         (256*512 + IPC_RPMESSAGE_OBJ_SIZE)

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

static uint8_t  sysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section ("ipc_data_buffer"), aligned (8)));
static uint8_t  gCntrlBuf[RPMSG_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned (8)));

static uint8_t g_vringMemBuf[IPC_VRING_MEM_SIZE] __attribute__ ((section (".bss:ipc_vring_mem"), aligned (8192)));

static SemaphoreP_Handle gIpcInitWaitSem;
static SemaphoreP_Handle gRdevStartSem;

static uint32_t selfProcId = IPC_MCU2_1;
static uint32_t gRemoteProc[] =
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
};
static uint32_t gNumRemoteProc = sizeof(gRemoteProc)/sizeof(uint32_t);

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

typedef struct CpswRemoteApp_Obj_s
{
   Mailbox_Handle hCmdMbx;
   Mailbox_Handle hResponseMbx;
   Cpsw_Handle    hCpsw;
   uint32_t       coreKey;
   uint8_t        macAddr[CPSW_MAC_ADDR_LEN];
   uint8_t        ipv4Addr[CPSW_ALE_IPV4ADDR_NUM_OCTETS];
   CpswDma_Handle hDma;
   bool           useDefaultRxFlow;
   bool           useExtAttach;
   Cpsw_MacPort  *macPorts;
   uint32_t      numMacPorts;
} CpswRemoteApp_Obj;

static Cpsw_MacPort gRemoteAppMacPorts[] = {
    CPSW_MAC_PORT_2,
};

CpswRemoteApp_Obj gRemoteAppObj  =
{
    .hCmdMbx          = NULL,
    .hResponseMbx     = NULL,
    .hCpsw            = NULL,
    .coreKey          = CPSW_RM_INVALIDCORE,
    .hDma             = NULL,
    .useDefaultRxFlow = false,
    .useExtAttach     = true,
    .numMacPorts      = CPSW_UTILS_ARRAYSIZE(gRemoteAppMacPorts),
    .macPorts         = gRemoteAppMacPorts,
};

static void CpswRemoteApp_registerIPV4Addr(Mailbox_Handle hCmdMbx,
                                         Mailbox_Handle hResponseMbx,
                                         Cpsw_Handle hCpsw,
                                         uint32_t coreKey,
                                         uint8_t  *macAddr,
                                         uint8_t  *ipv4Addr);

static void CpswRemoteApp_unregisterIPV4Addr(Mailbox_Handle hCmdMbx,
                                             Mailbox_Handle hResponseMbx,
                                             Cpsw_Handle hCpsw,
                                             uint32_t coreKey,
                                             uint8_t  *ipv4Addr);

static bool CpswRemoteApp_isPhyLinked(Mailbox_Handle hCmdMbx,
                                      Mailbox_Handle hResponseMbx,
                                      Cpsw_Handle hCpsw,
                                      uint32_t coreKey,
                                      Cpsw_MacPort portNum);

char *VerStr = "NIMU CPSW Example";


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
    gRemoteAppObj.ipv4Addr[0] = (uint8_t)(ipAddrHex>>24) & 0xFF;
    gRemoteAppObj.ipv4Addr[1] = (uint8_t)(ipAddrHex>>16) & 0xFF;
    gRemoteAppObj.ipv4Addr[2] = (uint8_t)(ipAddrHex>>8)  & 0xFF;
    gRemoteAppObj.ipv4Addr[3] = (uint8_t)(ipAddrHex      & 0xFF);
    snprintf(ipAddr, 17, "%d.%d.%d.%d\n",
                         gRemoteAppObj.ipv4Addr[0],
                         gRemoteAppObj.ipv4Addr[1],
                         gRemoteAppObj.ipv4Addr[2],
                         gRemoteAppObj.ipv4Addr[3]);

    CpswRemoteApp_registerIPV4Addr(gRemoteAppObj.hCmdMbx, 
                                 gRemoteAppObj.hResponseMbx,
                                 gRemoteAppObj.hCpsw, 
                                 gRemoteAppObj.coreKey,
                                 gRemoteAppObj.macAddr,
                                 gRemoteAppObj.ipv4Addr);
                           
    
    System_printf("\nCPSW NIMU application, IP address I/F 1: %s\n\r", ipAddr);

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

void appLogPrintf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    System_vprintf(format, args);
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
        System_printf("%s: Ipc_lateVirtioCreate failed\n", __func__);
    }

    if(status == IPC_SOK)
    {
        status = RPMessage_lateInit(IPC_MPU1_0);
        if(status != IPC_SOK)
        {
            System_printf("%s: RPMessage_lateInit failed\n", __func__);
        }
    }
    return;
}

bool freeInDetach = false;

typedef struct rdevEthSwitchAppAttachReq_s
{
    enum rpmsg_kdrv_ethswitch_cpsw_type cpswType;
} rdevEthSwitchAppAttachReq_t;

typedef struct rdevEthSwitchAppAttachExtReq_s
{
    enum rpmsg_kdrv_ethswitch_cpsw_type cpswType;
} rdevEthSwitchAppAttachExtReq_t;


typedef struct rdevEthSwitchAppAttachRes_s
{
    uint64_t id;
    uint32_t core_key;
    uint32_t rx_mtu;
    uint32_t tx_mtu[RPMSG_KDRV_TP_ETHSWITCH_CPSW_PRIORITY_NUM];
    uint32_t features;
} rdevEthSwitchAppAttachRes_t;

typedef struct rdevEthSwitchAppAttachExtRes_s
{
    uint64_t id;
    uint32_t core_key;
    uint32_t rx_mtu;
    uint32_t tx_mtu[RPMSG_KDRV_TP_ETHSWITCH_CPSW_PRIORITY_NUM];
    uint32_t features;
    uint32_t tx_id;
    uint32_t rx_flow_allocidx;
    uint8_t  mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
} rdevEthSwitchAppAttachExtRes_t;


typedef struct rdevEthSwitchAppAllocReq_s
{
    uint64_t id;
    uint32_t core_key;
} rdevEthSwitchAppAllocReq_t;

typedef struct rdevEthSwitchAppAllocTxRes_s
{
    uint32_t tx_id;
} rdevEthSwitchAppAllocTxRes_t;

typedef struct rdevEthSwitchAppFreeTxReq_s
{
    uint64_t id;
    uint32_t core_key;
    uint32_t tx_id;
} rdevEthSwitchAppFreeTxReq_t;

typedef struct rdevEthSwitchAppAllocRxRes_s
{
    uint32_t rx_flow_allocidx;
} rdevEthSwitchAppAllocRxRes_t;

typedef struct rdevEthSwitchAppFreeRxReq_s
{
    uint64_t id;
    uint32_t core_key;
    uint32_t rx_flow_allocidx;
} rdevEthSwitchAppFreeRxReq_t;


typedef struct rdevEthSwitchAppAllocMacRes_s
{
    uint8_t  mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
} rdevEthSwitchAppAllocMacRes_t;

typedef struct rdevEthSwitchAppFreeMacReq_s
{
    uint64_t id;
    uint32_t core_key;
    uint8_t  mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
} rdevEthSwitchAppFreeMacReq_t;

typedef struct rdevEthSwitchAppRegDefaultReq_s
{
    uint64_t id;
    uint32_t core_key;
    uint32_t rx_default_flow_allocidx;
} rdevEthSwitchAppRegDefaultReq_t;

typedef struct rdevEthSwitchAppUnRegDefaultReq_s
{
    uint64_t id;
    uint32_t core_key;
    uint32_t rx_default_flow_allocidx;
} rdevEthSwitchAppUnRegDefaultReq_t;

typedef struct rdevEthSwitchAppRegMacReq_s
{
    uint64_t id;
    uint32_t core_key;
    uint32_t rx_flow_allocidx;
    uint8_t  mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
} rdevEthSwitchAppRegMacReq_t;

typedef struct rdevEthSwitchAppUnRegMacReq_s
{
    uint64_t id;
    uint32_t core_key;
    uint32_t rx_flow_allocidx;
    uint8_t  mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
} rdevEthSwitchAppUnRegMacReq_t;

typedef struct rdevEthSwitchAppRegIPv4Req_s
{
    uint64_t id;
    uint32_t core_key;
    uint8_t  mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
    uint8_t  ipv4Addr[RPMSG_KDRV_TP_ETHSWITCH_IPV4ADDRLEN];
} rdevEthSwitchAppRegIPv4Req_t;

typedef struct rdevEthSwitchAppUnRegIPv4Req_s
{
    uint64_t id;
    uint32_t core_key;
    uint8_t  ipv4Addr[RPMSG_KDRV_TP_ETHSWITCH_IPV4ADDRLEN];
} rdevEthSwitchAppUnRegIPv4Req_t;

typedef struct rdevEthSwitchAppRegIPv6Req_s
{
    uint64_t id;
    uint32_t core_key;
    uint8_t  mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
    uint8_t  ipv6Addr[RPMSG_KDRV_TP_ETHSWITCH_IPV6ADDRLEN];
} rdevEthSwitchAppRegIPv6Req_t;

typedef struct rdevEthSwitchAppUnRegIPv6Req_s
{
    uint64_t id;
    uint32_t core_key;
    uint8_t  ipv6Addr[RPMSG_KDRV_TP_ETHSWITCH_IPV6ADDRLEN];
} rdevEthSwitchAppUnRegIPv6Req_t;

typedef struct rdevEthSwitchAppDetachReq_s
{
    uint64_t id;
    uint32_t core_key;
} rdevEthSwitchAppDetachReq_t;

typedef struct rdevEthSwitchAppRegRdReq_s
{
    uint32_t regRdAddr;
} rdevEthSwitchAppRegRdReq_t;

typedef struct rdevEthSwitchAppRegRdRes_s
{
    uint32_t regRdValue;
} rdevEthSwitchAppRegRdRes_t;

typedef struct rdevEthSwitchAppRegWrReq_s
{
    uint32_t regWrAddr;
    uint32_t regWrValue;
} rdevEthSwitchAppRegWrReq_t;

typedef struct rdevEthSwitchAppRegWrRes_s
{
    uint32_t regPostWrValue;
} rdevEthSwitchAppRegWrRes_t;

typedef struct rdevEthSwitchAppIOCTLReq_s
{
    uint64_t id;
    uint32_t core_key;
    uint32_t cmd;
    void     *inArgs;
    uint32_t inArgsSize;
    void     *outArgs;
    uint32_t outArgsSize;
} rdevEthSwitchAppIOCTLReq_t;

typedef struct rdevEthSwitchAppIOCTLRes_s
{
    uint8_t  *outArgs;
} rdevEthSwitchAppIOCTLRes_t;

typedef struct rdecEthSwitchAppPingReq_s
{
    char     msgText[128];
} rdecEthSwitchAppPingReq_t;

typedef struct rdecEthSwitchAppPingRes_s
{
    char     resp[128];
} rdecEthSwitchAppPingRes_t;

typedef enum rdevEthSwitchAppCmd_tag
{
    ETHSWT_CMD_IOCTL,
    ETHSWT_CMD_REGWR,
    ETHSWT_CMD_REGRD,
    ETHSWT_CMD_ATTACH,
    ETHSWT_CMD_ATTACHEXT,
    ETHSWT_CMD_DETACH,
    ETHSWT_CMD_REGIPV6,
    ETHSWT_CMD_UNREGIPV6,
    ETHSWT_CMD_REGIPV4,
    ETHSWT_CMD_UNREGIPV4,
    ETHSWT_CMD_ALLOCTX,
    ETHSWT_CMD_FREETX,
    ETHSWT_CMD_ALLOCRX,
    ETHSWT_CMD_FREERX,
    ETHSWT_CMD_ALLOCMAC,
    ETHSWT_CMD_FREEMAC,
    ETHSWT_CMD_REGMAC,
    ETHSWT_CMD_UNREGMAC,
    ETHSWT_CMD_REGDEFAULT,
    ETHSWT_CMD_UNREGDEFAULT,
    ETHSWT_CMD_PING,
} rdevEthSwitchAppCmd_e;

typedef struct rdevEthSwitchAppReqMsg_s
{

    rdevEthSwitchAppCmd_e              cmd;
    Mailbox_Handle                     hResponseMbx;
    union {
        rdevEthSwitchAppIOCTLReq_t         ioctl;
        rdevEthSwitchAppRegWrReq_t         regwr;
        rdevEthSwitchAppRegRdReq_t         regrd;
        rdevEthSwitchAppAttachReq_t        attach;
        rdevEthSwitchAppAttachExtReq_t     attachext;
        rdevEthSwitchAppDetachReq_t        detach;
        rdevEthSwitchAppRegIPv6Req_t       regipv6;
        rdevEthSwitchAppUnRegIPv6Req_t     unregipv6;
        rdevEthSwitchAppRegIPv4Req_t       regipv4;
        rdevEthSwitchAppUnRegIPv4Req_t     unregipv4;
        rdevEthSwitchAppRegMacReq_t        regmac;
        rdevEthSwitchAppUnRegMacReq_t      unregmac;
        rdevEthSwitchAppRegDefaultReq_t    regdefault;
        rdevEthSwitchAppUnRegDefaultReq_t  unregdefault;
        rdevEthSwitchAppAllocReq_t         alloc;
        rdevEthSwitchAppFreeTxReq_t        freetx;
        rdevEthSwitchAppFreeRxReq_t        freerx;
        rdevEthSwitchAppFreeMacReq_t       freemac;
        rdecEthSwitchAppPingReq_t          ping;
    } u;
} rdevEthSwitchAppReqMsg_t;

typedef struct rdevEthSwitchAppResMsg_s
{
    int32_t  retVal;
    union {
        rdevEthSwitchAppAttachRes_t        attach;
        rdevEthSwitchAppAttachExtRes_t     attachext;
        rdevEthSwitchAppAllocTxRes_t       tx;
        rdevEthSwitchAppAllocRxRes_t       rx;
        rdevEthSwitchAppAllocMacRes_t      mac;
        rdevEthSwitchAppIOCTLRes_t         ioctl;
        rdevEthSwitchAppRegWrRes_t         regwr;
        rdevEthSwitchAppRegRdRes_t         regrd;
        rdecEthSwitchAppPingRes_t          ping;
    } u;
} rdevEthSwitchAppResMsg_t;

typedef struct rdevEthSwitchAppMsg_s
{
    rdevEthSwitchAppReqMsg_t req;
    rdevEthSwitchAppResMsg_t res;
} rdevEthSwitchAppMsg_t;


#define RDEVETHSWITCHAPP_MSG_COUNT (3)

static void  rdevEthSwitchApp_createMbx(Mailbox_Handle *pMailboxHandle)
{
    Mailbox_Params mbxParams;

    Mailbox_Params_init(&mbxParams);
    *pMailboxHandle =
        Mailbox_create(
            sizeof (rdevEthSwitchAppMsg_t),
            RDEVETHSWITCHAPP_MSG_COUNT,
            &mbxParams,
            NULL);
}

//static 
void  rdevEthSwitchApp_deleteMbx(Mailbox_Handle *pMailboxHandle)
{
    Mailbox_delete(pMailboxHandle);
}

static void requestLoopFn(UArg a0, UArg a1)
{
    uint32_t device_id = (uint32_t)a0;
    Mailbox_Handle hMailbox = (Mailbox_Handle)a1;
    Bool mbxStatus;
    rdevEthSwitchAppMsg_t msg;

    while(TRUE) {
        mbxStatus =
            Mailbox_pend(hMailbox, &msg, BIOS_WAIT_FOREVER);
        assert(mbxStatus == TRUE);
        switch (msg.req.cmd)
        {
            case ETHSWT_CMD_PING:
            {

                memset(msg.res.u.ping.resp, 0, sizeof(msg.res.u.ping.resp));
                System_printf("%s: sending ping request\n", __func__);
                msg.res.retVal = rdevEthSwitchClient_sendping(device_id, msg.req.u.ping.msgText, strlen(msg.req.u.ping.msgText), msg.res.u.ping.resp, sizeof(msg.res.u.ping.resp));
                if (0 == msg.res.retVal)
                {
                    System_printf("%s: respose %s\n", __func__, msg.res.u.ping.resp);
                }
                break;
            }
            case ETHSWT_CMD_ATTACH:
            {
                msg.res.retVal = rdevEthSwitchClient_attach(device_id, msg.req.u.attach.cpswType, &msg.res.u.attach.id, &msg.res.u.attach.core_key, &msg.res.u.attach.rx_mtu, msg.res.u.attach.tx_mtu, CPSW_UTILS_ARRAYSIZE(msg.res.u.attach.tx_mtu),&msg.res.u.attach.features);
                if (0 == msg.res.retVal)
                {
                    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)(msg.res.u.attach.id));
                    System_printf("Function:%s,Handle:%p,CoreKey:%x, RxMtu:%4u, TxMtu:%4u:%4u:%4u:%4u:%4u:%4u:%4u:%4u, TxCsumEnabled:%u\n",__func__,hCpsw, msg.res.u.attach.core_key, msg.res.u.attach.rx_mtu, msg.res.u.attach.tx_mtu[0], msg.res.u.attach.tx_mtu[1],msg.res.u.attach.tx_mtu[2],msg.res.u.attach.tx_mtu[3], msg.res.u.attach.tx_mtu[4], msg.res.u.attach.tx_mtu[5],msg.res.u.attach.tx_mtu[6],msg.res.u.attach.tx_mtu[7],((msg.res.u.attach.features & RPMSG_KDRV_TP_ETHSWITCH_FEATURE_TXCSUM) != 0));
                }
                break;
            }
            case ETHSWT_CMD_ATTACHEXT:
            {
                msg.res.retVal = rdevEthSwitchClient_attachext(device_id, msg.req.u.attachext.cpswType, &msg.res.u.attachext.id, &msg.res.u.attachext.core_key, &msg.res.u.attachext.rx_mtu, msg.res.u.attachext.tx_mtu, CPSW_UTILS_ARRAYSIZE(msg.res.u.attachext.tx_mtu),&msg.res.u.attachext.features ,&msg.res.u.attachext.tx_id,&msg.res.u.attachext.rx_flow_allocidx, msg.res.u.attachext.mac_address, CPSW_UTILS_ARRAYSIZE(msg.res.u.attachext.mac_address));
                if (0 == msg.res.retVal)
                {
                    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)(msg.res.u.attach.id));
                    System_printf("Function:%s,Handle:%p,CoreKey:%x, RxMtu:%4u, TxMtu:%4u:%4u:%4u:%4u:%4u:%4u:%4u:%4u, TxCsumEnabled:%u\n",__func__,hCpsw, msg.res.u.attach.core_key, msg.res.u.attach.rx_mtu, msg.res.u.attach.tx_mtu[0], msg.res.u.attach.tx_mtu[1],msg.res.u.attach.tx_mtu[2],msg.res.u.attach.tx_mtu[3], msg.res.u.attach.tx_mtu[4], msg.res.u.attach.tx_mtu[5],msg.res.u.attach.tx_mtu[6],msg.res.u.attach.tx_mtu[7],((msg.res.u.attach.features & RPMSG_KDRV_TP_ETHSWITCH_FEATURE_TXCSUM) != 0));
                }
                break;
            }
            case ETHSWT_CMD_ALLOCTX:
            {
                msg.res.retVal = rdevEthSwitchClient_alloctx(device_id, msg.req.u.alloc.id, msg.req.u.alloc.core_key, &msg.res.u.tx.tx_id);
                if (0 == msg.res.retVal)
                {
                    System_printf("Function:%s,Txid:%u\n",__func__,msg.res.u.tx.tx_id);
                }
                break;
            }
            case ETHSWT_CMD_ALLOCRX:
            {
                msg.res.retVal = rdevEthSwitchClient_allocrx(device_id, msg.req.u.alloc.id, msg.req.u.alloc.core_key, &msg.res.u.rx.rx_flow_allocidx);
                if (0 == msg.res.retVal)
                {
                    System_printf("Function:%s,RxAllocId:%u\n",__func__,msg.res.u.rx.rx_flow_allocidx);
                }
                break;
            }
            case ETHSWT_CMD_REGDEFAULT:
            {
                msg.res.retVal = rdevEthSwitchClient_registerrxdefault(device_id, msg.req.u.regdefault.id, msg.req.u.regdefault.core_key,  msg.req.u.regdefault.rx_default_flow_allocidx);
                break;
            }
            case ETHSWT_CMD_ALLOCMAC:
            {
                msg.res.retVal = rdevEthSwitchClient_allocmac(device_id, msg.req.u.alloc.id, msg.req.u.alloc.core_key, msg.res.u.mac.mac_address, CPSW_UTILS_ARRAYSIZE(msg.res.u.mac.mac_address));
                if (0 == msg.res.retVal)
                {
                    System_printf("Function:%s,mac_address:%2x:%2x:%2x:%2x:%2x:%2x \n",__func__,msg.res.u.mac.mac_address[0],msg.res.u.mac.mac_address[1],msg.res.u.mac.mac_address[2],msg.res.u.mac.mac_address[3],msg.res.u.mac.mac_address[4],msg.res.u.mac.mac_address[5]);
                }
                break;
            }
            case ETHSWT_CMD_REGMAC:
            {
                msg.res.retVal = rdevEthSwitchClient_registermac(device_id, msg.req.u.regmac.id, msg.req.u.regmac.core_key, msg.req.u.regmac.rx_flow_allocidx, msg.req.u.regmac.mac_address);
                break;
            }
            case ETHSWT_CMD_REGIPV4:
            {
                msg.res.retVal = rdevEthSwitchClient_ipv4macregister(device_id, msg.req.u.regipv4.id, msg.req.u.regipv4.core_key, msg.req.u.regipv4.mac_address, msg.req.u.regipv4.ipv4Addr);
                break;
            }
            case ETHSWT_CMD_REGIPV6:
            {
                msg.res.retVal = rdevEthSwitchClient_ipv6macregister(device_id, msg.req.u.regipv6.id, msg.req.u.regipv6.core_key, msg.req.u.regipv6.mac_address, msg.req.u.regipv6.ipv6Addr);
                break;
            }
            case ETHSWT_CMD_UNREGIPV4:
            {
                msg.res.retVal = rdevEthSwitchClient_ipv4macunregister(device_id, msg.req.u.unregipv4.id, msg.req.u.unregipv4.core_key, msg.req.u.unregipv4.ipv4Addr);
                break;
            }
            case ETHSWT_CMD_IOCTL:
            {
                msg.res.retVal = rdevEthSwitchClient_ioctl(device_id, msg.req.u.ioctl.id, msg.req.u.ioctl.core_key,msg.req.u.ioctl.cmd, msg.req.u.ioctl.inArgs, msg.req.u.ioctl.inArgsSize, msg.req.u.ioctl.outArgs, msg.req.u.ioctl.outArgsSize);
                break;
            }

            case ETHSWT_CMD_UNREGMAC:
            {
                msg.res.retVal = rdevEthSwitchClient_unregistermac(device_id, msg.req.u.unregmac.id, msg.req.u.unregmac.core_key, msg.req.u.unregmac.rx_flow_allocidx, msg.req.u.unregmac.mac_address);
                break;
            }
            case ETHSWT_CMD_UNREGDEFAULT:
            {
                msg.res.retVal = rdevEthSwitchClient_unregisterrxdefault(device_id, msg.req.u.unregdefault.id, msg.req.u.unregdefault.core_key, msg.req.u.unregdefault.rx_default_flow_allocidx);
                break;
            }
            case ETHSWT_CMD_FREEMAC:
            {
                if (freeInDetach == false)
                {
                    msg.res.retVal = rdevEthSwitchClient_freemac(device_id, msg.req.u.freemac.id, msg.req.u.freemac.core_key, msg.req.u.freemac.mac_address);
                }
                break;
            }
            case ETHSWT_CMD_FREETX:
            {
                if (freeInDetach == false)
                {
                    msg.res.retVal = rdevEthSwitchClient_freetx(device_id, msg.req.u.freetx.id, msg.req.u.freetx.core_key, msg.req.u.freetx.tx_id);
                }
                break;
            }
            case ETHSWT_CMD_FREERX:
            {
                if (freeInDetach == false)
                {
                    msg.res.retVal = rdevEthSwitchClient_freerx(device_id, msg.req.u.freerx.id , msg.req.u.freerx.core_key, msg.req.u.freerx.rx_flow_allocidx);
                }
                break;
            }
            case ETHSWT_CMD_DETACH:
            {
                /* Dump stats before detach */
                uint8_t notifyInfo[] = {'d','u','m','p','s','t','a','t','s'};
                System_printf("%s: sending message\n", __func__);
                rdevEthSwitchClient_sendNotify(device_id, msg.req.u.detach.id, msg.req.u.detach.core_key, RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_DUMPSTATS, notifyInfo, sizeof(notifyInfo));
                msg.res.retVal = rdevEthSwitchClient_detach(device_id, msg.req.u.detach.id, msg.req.u.detach.core_key);
                break;
            }
            case ETHSWT_CMD_REGWR:
            {
                msg.res.retVal = rdevEthSwitchClient_regwr(device_id, msg.req.u.regwr.regWrAddr , msg.req.u.regwr.regWrValue, &msg.res.u.regwr.regPostWrValue);
                break;
            }
            case ETHSWT_CMD_REGRD:
            {
                msg.res.retVal = rdevEthSwitchClient_regrd(device_id, msg.req.u.regrd.regRdAddr , &msg.res.u.regrd.regRdValue);
                break;
            }
        }
        assert(msg.req.hResponseMbx != NULL);
        mbxStatus =
            Mailbox_post(msg.req.hResponseMbx, &msg, BIOS_WAIT_FOREVER);
        assert(mbxStatus == TRUE);
    }
}


static void startMessageAndRequestLoop(uint32_t device_id)
{
    Task_Params params;


    assert(gRemoteAppObj.hCmdMbx != NULL);
    assert(gRemoteAppObj.hResponseMbx != NULL);

    Task_Params_init(&params);
    params.priority = 3;
    params.stack = &g_requestTaskStack[0];
    params.stackSize = sizeof(g_requestTaskStack);
    params.arg0 = device_id;
    params.arg1 = (uint32_t)gRemoteAppObj.hCmdMbx;
    Task_create(requestLoopFn, &params, NULL);
}

static Void printDevInfo(struct rpmsg_kdrv_ethswitch_device_data *ethDevData)
{
    char *tf[] = {"false", "true"};

    System_printf("ETHFW Version:%2d.%2d.%2d\n",
                  ethDevData->fw_ver.major, 
                  ethDevData->fw_ver.minor, 
                  ethDevData->fw_ver.rev);
    System_printf("ETHFW Build Date (YYYY/MMM/DD):%c%c%c%c/%c%c%c/%c%c\n",
                  ethDevData->fw_ver.year[0],ethDevData->fw_ver.year[1],ethDevData->fw_ver.year[2],ethDevData->fw_ver.year[3],
                  ethDevData->fw_ver.month[0],ethDevData->fw_ver.month[1],ethDevData->fw_ver.month[2],
                  ethDevData->fw_ver.date[0],ethDevData->fw_ver.date[1]);
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


static Void monitorAndUnlockRdev(UArg a0, UArg a1)
{
    int32_t ret = 0;
    rdevEthSwitchClientInitPrms_t prm;

    SemaphoreP_pend(gIpcInitWaitSem, SemaphoreP_WAIT_FOREVER);
    SemaphoreP_post(gRdevStartSem);


    sprintf(prm.device_name, ETHREMOTEDEVICE_DEVICE_NAME_MCU_2_1);
    prm.cbHandler = rdevEthSwitchClient_printText;
    while(TRUE) {
        ret = rdevEthSwitchClient_connect(&prm);
        if(ret != 0)
            System_printf("error in device query\n");

        if(ret != 0 || (ret == 0 && prm.device_id != APP_REMOTE_DEVICE_DEVICE_ID_EAGAIN))
            break;
        if (ret == 0 && (prm.device_id == APP_REMOTE_DEVICE_DEVICE_ID_EAGAIN))
        {
            Task_sleep(10);
        }
    }

    if(ret == 0) {
        System_printf("Registered a device name = %s, id = %u, type = %u\n",
                "mcu2_0-ethswitch-0", prm.device_id, prm.device_type);
        printDevInfo(&prm.eth_device_data);
    }

    startMessageAndRequestLoop(prm.device_id);

}

static Void ipc_init(UArg a0, UArg a1)
{
    Task_Params       params;
    uint32_t          numProc = gNumRemoteProc;
    Ipc_VirtIoParams  vqParam;
    RPMessage_Params cntrlParam;

    /* Step1 : Initialize the multiproc */
    Ipc_mpSetConfig(selfProcId, numProc, &gRemoteProc[0]);

    System_printf("IPC_echo_test (core : %s) .....\r\n",
            Ipc_mpGetSelfName());

    Ipc_init(NULL);
    Ipc_loadResourceTable(appGetIpcResourceTable());

    /* Step2 : Initialize Virtio */
    vqParam.vqObjBaseAddr = (void*)&sysVqBuf[0];
    vqParam.vqBufSize     = numProc * Ipc_getVqObjMemoryRequiredPerCore();
    vqParam.vringBaseAddr = (void*)g_vringMemBuf;
    vqParam.vringBufSize  = sizeof(g_vringMemBuf);
    vqParam.timeoutCnt    = VQ_TIMEOUT;  /* Wait for counts */
    Ipc_initVirtIO(&vqParam);

    /* Step 3: Initialize RPMessage */
    /* Initialize the param */
    RPMessageParams_init(&cntrlParam);

    /* Set memory for HeapMemory for control task */
    cntrlParam.buf         = &gCntrlBuf[0];
    cntrlParam.bufSize     = RPMSG_DATA_SIZE;
    cntrlParam.stackBuffer = &ctrlTaskBuf[0];
    cntrlParam.stackSize   = sizeof(ctrlTaskBuf);
    RPMessage_init(&cntrlParam);

    SemaphoreP_post(gIpcInitWaitSem);

    /* Step 4: Create RPMessage monitor task */

    Task_Params_init(&params);
    params.priority = 3;
    params.stack = &g_vdevMonStackBuf[0];
    params.stackSize = sizeof(g_vdevMonStackBuf);
    Task_create(rpmsg_vdevMonitorFxn, &params, NULL);
}

static Void remotedev_init(UArg a0, UArg a1)
{
    app_remote_device_init_prm_t remote_dev_init_prm;

    appRemoteDeviceInitParamsInit(&remote_dev_init_prm);

    remote_dev_init_prm.rpmsg_buf_size = 256;
    remote_dev_init_prm.remote_endpt = REMOTE_DEVICE_ENDPT;
    remote_dev_init_prm.wait_sem = gRdevStartSem;
    remote_dev_init_prm.num_cores = 1;
    remote_dev_init_prm.cores[0] = IPC_MCU2_0;

    appRemoteDeviceInit(&remote_dev_init_prm);
    System_printf("Remote device (core : mcu2_1) .....\r\n");

}

static Void taskFxn(UArg a0, UArg a1)
{

    Task_Params ipc_taskParams;
    Task_Params rdev_taskParams;
    Task_Params monitor_taskParams;
    SemaphoreP_Params sem_params;

    SemaphoreP_Params_init(&sem_params);
    sem_params.mode = SemaphoreP_Mode_BINARY;
    gIpcInitWaitSem = SemaphoreP_create(0, &sem_params);

    SemaphoreP_Params_init(&sem_params);
    sem_params.mode = SemaphoreP_Mode_BINARY;
    gRdevStartSem = SemaphoreP_create(0, &sem_params);

    Task_Params_init(&ipc_taskParams);
    ipc_taskParams.priority = 2;
    ipc_taskParams.stack = &g_ipcStackBuf[0];
    ipc_taskParams.stackSize = sizeof(g_ipcStackBuf);
    Task_create(ipc_init, &ipc_taskParams, NULL);

    Task_Params_init(&rdev_taskParams);
    rdev_taskParams.priority = 2;
    rdev_taskParams.stack = &g_rdevStackBuf[0];
    rdev_taskParams.stackSize = sizeof(g_rdevStackBuf);
    Task_create(remotedev_init, &rdev_taskParams, NULL);

    Task_Params_init(&monitor_taskParams);
    monitor_taskParams.priority = 2;
    monitor_taskParams.stack = &g_monitorStackBuf[0];
    monitor_taskParams.stackSize = sizeof(g_monitorStackBuf);
    Task_create(monitorAndUnlockRdev, &monitor_taskParams, NULL);
}

int main(void)
{
    Task_Handle task;
    Task_Params taskParams;


    /* Initialize the task params */
    Task_Params_init(&taskParams);
    /* Set the task priority higher than the default priority (1) */
    taskParams.priority = 2;
    taskParams.stack = &g_mainStackBuf[0];
    taskParams.stackSize = sizeof(g_mainStackBuf);

    task = Task_create(taskFxn, &taskParams, NULL);
    if(NULL == task)
    {
        BIOS_exit(0);
    }
    rdevEthSwitchApp_createMbx(&gRemoteAppObj.hCmdMbx);
    rdevEthSwitchApp_createMbx(&gRemoteAppObj.hResponseMbx);
    BIOS_start();    /* does not return */

    return(0);
}

static bool CpswRemoteApp_IsAllPhyLinked(Cpsw_Handle hCpsw)
{
    uint32_t i;
    static bool     isPhyLinked = false;
    static uint32_t pollingInterVal = 0;

    if ((isPhyLinked == false) || ((pollingInterVal % CPSW_REMOTE_APP_PHY_POLLING_INTERVAL) == 0))
    {
        for (i = 0; i < gRemoteAppObj.numMacPorts; i++)
        {
            if (0 == i)
            {
                isPhyLinked = CpswRemoteApp_isPhyLinked(gRemoteAppObj.hCmdMbx, gRemoteAppObj.hResponseMbx,  hCpsw, gRemoteAppObj.coreKey,  gRemoteAppObj.macPorts[i]);
            }
            else
            {
                isPhyLinked = (isPhyLinked && CpswRemoteApp_isPhyLinked(gRemoteAppObj.hCmdMbx, gRemoteAppObj.hResponseMbx, hCpsw, gRemoteAppObj.coreKey,  gRemoteAppObj.macPorts[i]));
            }
        }
    }
    pollingInterVal = (pollingInterVal + 1) % CPSW_REMOTE_APP_PHY_POLLING_INTERVAL;
    return isPhyLinked;
}

struct Udma_DrvObj udmaDrvObj;

static Udma_DrvHandle CpswRemoteApp_udmaOpen(void)
{
    Udma_InitPrms initPrms;
    Udma_DrvHandle hUdmaDrv;
    int32_t retVal;
    uint32_t instId;

    hUdmaDrv = &udmaDrvObj;
    memset(hUdmaDrv, 0U, sizeof (*hUdmaDrv));

    instId = UDMA_INST_ID_MAIN_0;

    /* Initialize the UDMA driver based on NAVSS instance */
    UdmaInitPrms_init(instId, &initPrms);
    initPrms.printFxn = (Udma_PrintFxn)&System_printf;
    retVal = Udma_init(hUdmaDrv, &initPrms);

    /* assert if UDMA failed to open */
    assert(UDMA_SOK == retVal);

    return hUdmaDrv;
}

static void CpswRemoteApp_sendCmd(Mailbox_Handle hCmdMbx,
                                  Mailbox_Handle hResponseMbx,
                                  rdevEthSwitchAppCmd_e cmd,
                                  rdevEthSwitchAppMsg_t *msg)
{
    Bool mbxStatus;

    assert(hCmdMbx != NULL);
    assert(hResponseMbx != NULL);

    msg->req.cmd = cmd;
    msg->req.hResponseMbx = hResponseMbx;
    mbxStatus = Mailbox_post(hCmdMbx, msg, BIOS_WAIT_FOREVER);
    assert(mbxStatus == TRUE);
    mbxStatus = Mailbox_pend(hResponseMbx, msg, BIOS_WAIT_FOREVER);
    assert(mbxStatus == TRUE);
    assert(msg->res.retVal == CPSW_SOK);
}

static void CpswRemoteApp_setRxFlowPrms(CpswDma_OpenRxFlowPrms *pRxFlowPrms,
                                        uint32_t rxStartFlowIdx,
                                        uint32_t rxFlowIdx,
                                        Udma_DrvHandle  hUdmaDrv,
                                        uint32_t numRxPackets,
                                        void     *cbArg,
                                        CpswDma_PktNotifyCb eventCb,
                                        uint32_t rxFlowMtu)
{
    pRxFlowPrms->startIdx               = rxStartFlowIdx;
    pRxFlowPrms->flowIdx                = rxFlowIdx;

    pRxFlowPrms->hUdmaDrv               = hUdmaDrv;

    pRxFlowPrms->ringMemAllocFxn        = &CpswAppMemUtils_allocRingMemFxn;
    pRxFlowPrms->ringMemFreeFxn         = &CpswAppMemUtils_freeRingMemFxn;

    pRxFlowPrms->notifyCb               = eventCb;

    pRxFlowPrms->numRxPkts              = numRxPackets;

    pRxFlowPrms->disableCacheOpsFlag    = false;
    pRxFlowPrms->dmaDescAllocFxn        = &CpswAppMemUtils_allocDmaDescFxn;
    pRxFlowPrms->dmaDescFreeFxn         = &CpswAppMemUtils_freeDmaDescFxn;
    pRxFlowPrms->hCbArg                 = cbArg;
    pRxFlowPrms->useProxy               = false;
    pRxFlowPrms->rxFlowMtu              = rxFlowMtu;
}


static void CpswRemoteApp_addHostPortEntry(Mailbox_Handle hCmdMbx,
                                           Mailbox_Handle hResponseMbx,
                                           Cpsw_Handle hCpsw,
                                           uint32_t coreKey,
                                           uint8_t *macAddr)
{
    rdevEthSwitchAppMsg_t msg;
    CpswAle_AddEntryOutArgs       setUcastOutArgs;
    CpswAle_SetUcastEntryInArgs   setUcastInArgs;

    memset(&setUcastInArgs, 0 , sizeof(setUcastInArgs));
    memcpy(&setUcastInArgs.addr.addr[0U], macAddr, sizeof (setUcastInArgs.addr.addr));
    setUcastInArgs.addr.vlanId  = 0U;
    setUcastInArgs.info.portNum = CPSW_ALE_HOST_PORT_NUM;
    setUcastInArgs.info.blocked = false;
    setUcastInArgs.info.secure  = true;
    setUcastInArgs.info.super   = false;
    setUcastInArgs.info.ageable = false;

    msg.req.u.ioctl.cmd = CPSW_ALE_IOCTL_ADD_UNICAST;
    msg.req.u.ioctl.core_key = coreKey;
    msg.req.u.ioctl.id       = (uint64_t) hCpsw;
    msg.req.u.ioctl.inArgsSize = sizeof(setUcastInArgs);
    msg.req.u.ioctl.inArgs     = &setUcastInArgs;
    msg.req.u.ioctl.outArgs     = &setUcastOutArgs;
    msg.req.u.ioctl.outArgsSize = sizeof(setUcastOutArgs);

    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_IOCTL, &msg);
}

static void CpswRemoteApp_delAddrEntry(Mailbox_Handle hCmdMbx,
                                       Mailbox_Handle hResponseMbx,
                                       Cpsw_Handle hCpsw,
                                       uint32_t coreKey,
                                       uint8_t *macAddr)
{
    rdevEthSwitchAppMsg_t msg;
    CpswAle_MacAddrInfo           addrInfo;

    memset(&addrInfo, 0 , sizeof(addrInfo));
    memcpy(&addrInfo.addr[0U], macAddr, sizeof (addrInfo.addr));
    addrInfo.vlanId = 0U;

    msg.req.u.ioctl.cmd = CPSW_ALE_IOCTL_REMOVE_ADDR;
    msg.req.u.ioctl.core_key = coreKey;
    msg.req.u.ioctl.id       = (uint64_t) hCpsw;
    msg.req.u.ioctl.inArgsSize = sizeof(addrInfo);
    msg.req.u.ioctl.inArgs     = &addrInfo;
    msg.req.u.ioctl.outArgs     = NULL;
    msg.req.u.ioctl.outArgsSize = 0;

    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_IOCTL, &msg);
}

static void CpswRemoteApp_getRxStartFlowIdx(Mailbox_Handle hCmdMbx,
                                            Mailbox_Handle hResponseMbx,
                                            Cpsw_Handle hCpsw,
                                            uint32_t coreKey,
                                            uint32_t *startFlowIdx)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.ioctl.cmd = CPSW_HOSTPORT_GET_FLOW_ID_OFFSET;
    msg.req.u.ioctl.core_key = coreKey;
    msg.req.u.ioctl.id       = (uint64_t) hCpsw;
    msg.req.u.ioctl.inArgsSize = 0;
    msg.req.u.ioctl.inArgs     = NULL;
    msg.req.u.ioctl.outArgs     = startFlowIdx;
    msg.req.u.ioctl.outArgsSize = sizeof(*startFlowIdx);

    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_IOCTL, &msg);

}

static void CpswRemoteApp_allocRxFlow(Mailbox_Handle hCmdMbx,
                                      Mailbox_Handle hResponseMbx,
                                      Cpsw_Handle    hCpsw,
                                      uint32_t       coreKey,
                                      uint32_t       *rxFlowStartIdx,
                                      uint32_t       *rxFlowIdx)
{
    rdevEthSwitchAppMsg_t msg;
    uint32_t absRxFlowIdx;

    msg.req.u.alloc.id = (uint64_t)hCpsw;
    msg.req.u.alloc.core_key = coreKey;
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_ALLOCRX, &msg);
    absRxFlowIdx = msg.res.u.rx.rx_flow_allocidx;
    CpswRemoteApp_getRxStartFlowIdx(hCmdMbx, hResponseMbx, hCpsw, coreKey, rxFlowStartIdx);
    assert((absRxFlowIdx >= *rxFlowStartIdx) && (absRxFlowIdx < (*rxFlowStartIdx + CPSW_DMA_MAX_RX_FLOW)));
    *rxFlowIdx = (absRxFlowIdx - *rxFlowStartIdx);
}

static void CpswRemoteApp_allocMac(Mailbox_Handle hCmdMbx,
                                   Mailbox_Handle hResponseMbx,
                                   Cpsw_Handle hCpsw,
                                   uint32_t coreKey,
                                   uint8_t *macAddress)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.alloc.id       = (uint64_t)hCpsw;
    msg.req.u.alloc.core_key = coreKey;
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_ALLOCMAC, &msg);
    memcpy(macAddress, msg.res.u.mac.mac_address, sizeof(msg.res.u.mac.mac_address));
}

static void CpswRemoteApp_registerDefaultRxFlow(Mailbox_Handle hCmdMbx,
                                                Mailbox_Handle hResponseMbx,
                                                Cpsw_Handle hCpsw,
                                                uint32_t coreKey,
                                                uint32_t rxFlowStartIdx,
                                                uint32_t freeRxFlowIdx)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.regdefault.id       = (uint64_t)hCpsw;
    msg.req.u.regdefault.core_key = coreKey;
    msg.req.u.regdefault.rx_default_flow_allocidx = (rxFlowStartIdx + freeRxFlowIdx);
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_REGDEFAULT, &msg);
}

static void CpswRemoteApp_unregisterDefaultRxFlow(Mailbox_Handle hCmdMbx,
                                                  Mailbox_Handle hResponseMbx,
                                                  Cpsw_Handle hCpsw,
                                                  uint32_t coreKey,
                                                  uint32_t rxFlowStartIdx,
                                                  uint32_t freeRxFlowIdx)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.unregdefault.id       = (uint64_t)hCpsw;
    msg.req.u.unregdefault.core_key = coreKey;
    msg.req.u.unregdefault.rx_default_flow_allocidx = (rxFlowStartIdx + freeRxFlowIdx);
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_UNREGDEFAULT, &msg);
}

static void CpswRemoteApp_registerDstMacRxFlow(Mailbox_Handle hCmdMbx,
                                               Mailbox_Handle hResponseMbx,
                                               Cpsw_Handle hCpsw,
                                               uint32_t coreKey,
                                               uint32_t rxFlowStartIdx,
                                               uint32_t freeRxFlowIdx,
                                               uint8_t  *macAddress)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.regmac.id       = (uint64_t)hCpsw;
    msg.req.u.regmac.core_key = coreKey;
    msg.req.u.regmac.rx_flow_allocidx = (rxFlowStartIdx + freeRxFlowIdx);
    memcpy(msg.req.u.regmac.mac_address, macAddress, sizeof(msg.req.u.regmac.mac_address));
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_REGMAC, &msg);
}

static void CpswRemoteApp_unregisterDstMacRxFlow(Mailbox_Handle hCmdMbx,
                                                 Mailbox_Handle hResponseMbx,
                                                 Cpsw_Handle hCpsw,
                                                 uint32_t coreKey,
                                                 uint32_t rxFlowStartIdx,
                                                 uint32_t freeRxFlowIdx,
                                                 uint8_t  *macAddress)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.unregmac.id       = (uint64_t)hCpsw;
    msg.req.u.unregmac.core_key = coreKey;
    msg.req.u.unregmac.rx_flow_allocidx = (rxFlowStartIdx + freeRxFlowIdx);
    memcpy(msg.req.u.unregmac.mac_address, macAddress, sizeof(msg.req.u.unregmac.mac_address));
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_UNREGMAC, &msg);
}

static void CpswRemoteApp_openNDKRxCh(Mailbox_Handle hCmdMbx,
                                      Mailbox_Handle hResponseMbx,
                                      Cpsw_Handle hCpsw,
                                      Udma_DrvHandle  hUdmaDrv,
                                      uint32_t coreKey,
                                      bool     useDefaultFlow,
                                      uint32_t rxFlowStartIdx,
                                      uint32_t rxFlowIdx,
                                      uint8_t  *macAddress,
                                      NimuCpswAppIf_RxConfig *rxConfig,
                                      NimuCpswAppIf_RxHandleInfo *rxHandleInfo,
                                      uint32_t rxFlowMtu)
{
    CpswDma_OpenRxFlowPrms cpswRxFlowCfg;

    rxHandleInfo->rxFlowStartIdx = rxFlowStartIdx;
    rxHandleInfo->rxFlowIdx      = rxFlowIdx;
    CPSW_UTILS_ARRAY_COPY(rxHandleInfo->macAddr, macAddress);

    CpswDma_initRxFlowParams(&cpswRxFlowCfg);

    CpswRemoteApp_setRxFlowPrms(&cpswRxFlowCfg, 
                                rxHandleInfo->rxFlowStartIdx,
                                rxHandleInfo->rxFlowIdx,
                                hUdmaDrv,
                                rxConfig->numPackets,
                                rxConfig->cbArg,
                                rxConfig->notifyCb,
                                rxFlowMtu);
    rxHandleInfo->hRxFlow = CpswDma_openRxFlow(&cpswRxFlowCfg);

    assert(rxHandleInfo->hRxFlow != NULL);

    CpswRemoteApp_addHostPortEntry(hCmdMbx, hResponseMbx, hCpsw, coreKey, rxHandleInfo->macAddr);
    if (useDefaultFlow)
    {
        CpswRemoteApp_registerDefaultRxFlow(hCmdMbx, 
                                            hResponseMbx,
                                            hCpsw,
                                            coreKey,
                                            rxHandleInfo->rxFlowStartIdx,
                                            rxHandleInfo->rxFlowIdx);
    }
    else
    {
        CpswRemoteApp_registerDstMacRxFlow(hCmdMbx, 
                                           hResponseMbx,
                                           hCpsw,
                                           coreKey,
                                           rxHandleInfo->rxFlowStartIdx,
                                           rxHandleInfo->rxFlowIdx,
                                           rxHandleInfo->macAddr);
    }
}


void CpswRemoteApp_freeMac(Mailbox_Handle hCmdMbx,
                           Mailbox_Handle hResponseMbx,
                           Cpsw_Handle hCpsw,
                           uint32_t coreKey,
                           uint8_t *macAddress)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.freemac.id       = (uint64_t)hCpsw;
    msg.req.u.freemac.core_key = coreKey;
    memcpy(msg.req.u.freemac.mac_address, macAddress, sizeof(msg.req.u.freemac.mac_address));
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_FREEMAC, &msg);
}

void CpswRemoteApp_freeRxFlow(Mailbox_Handle hCmdMbx,
                                 Mailbox_Handle hResponseMbx,
                                 Cpsw_Handle hCpsw,
                                 uint32_t coreKey,
                                 uint32_t rxFlowIdx)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.freerx.id       = (uint64_t)hCpsw;
    msg.req.u.freerx.core_key = coreKey;
    msg.req.u.freerx.rx_flow_allocidx = rxFlowIdx;

    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_FREERX, &msg);
}


static void CpswRemoteApp_closeNDKRxCh(Mailbox_Handle hCmdMbx,
                                       Mailbox_Handle hResponseMbx,
                                       Cpsw_Handle hCpsw,
                                       Udma_DrvHandle  hUdmaDrv,
                                       uint32_t coreKey,
                                       bool     useDefaultFlow,
                                       uint8_t  *ipV4Addr,
                                       NimuCpswAppIf_RxHandleInfo *rxHandleInfo,
                                       void *freeFxnArg,
                                       NimuCpswAppIf_FreePktCbFxn freeFxn)
{
    CpswDma_PktInfoQ fqPktInfoQ;
    CpswDma_PktInfoQ cqPktInfoQ;
    int32_t status;

    CpswUtils_initQ(&fqPktInfoQ);
    CpswUtils_initQ(&cqPktInfoQ);

    CpswDma_disableRxEvent(rxHandleInfo->hRxFlow);
    
    CpswRemoteApp_unregisterIPV4Addr(hCmdMbx,
                                     hResponseMbx,
                                     hCpsw,
                                     coreKey,
                                     ipV4Addr);
    if (useDefaultFlow)
    {
        CpswRemoteApp_unregisterDefaultRxFlow(hCmdMbx,
                                              hResponseMbx,
                                              hCpsw,
                                              coreKey,
                                              rxHandleInfo->rxFlowStartIdx,
                                              rxHandleInfo->rxFlowIdx);
    }
    else
    {
        CpswRemoteApp_unregisterDstMacRxFlow(hCmdMbx,
                                             hResponseMbx,
                                             hCpsw,
                                             coreKey,
                                             rxHandleInfo->rxFlowStartIdx,
                                             rxHandleInfo->rxFlowIdx,
                                             rxHandleInfo->macAddr);
    }
    CpswRemoteApp_delAddrEntry(hCmdMbx, hResponseMbx,hCpsw, coreKey, rxHandleInfo->macAddr);
    status = CpswDma_closeRxFlow(rxHandleInfo->hRxFlow,
                                 &fqPktInfoQ,
                                 &cqPktInfoQ);
    assert(status == CPSW_SOK);
    CpswRemoteApp_freeMac(hCmdMbx, 
                          hResponseMbx,
                          hCpsw,
                          coreKey,
                          rxHandleInfo->macAddr);
    CpswRemoteApp_freeRxFlow(hCmdMbx, 
                             hResponseMbx,
                             hCpsw,
                             coreKey,
                             rxHandleInfo->rxFlowIdx);
    freeFxn(freeFxnArg, &fqPktInfoQ, &cqPktInfoQ);
}


static void CpswRemoteApp_setTxChPrms(CpswDma_OpenTxChPrms *pTxChPrms,
                                      uint32_t txChNum,
                                      Udma_DrvHandle  hUdmaDrv,
                                      uint32_t numTxPackets,
                                      void     *cbArg,
                                      CpswDma_PktNotifyCb eventCb)
{
    pTxChPrms->chNum               = txChNum;
    pTxChPrms->hUdmaDrv            = hUdmaDrv;

    pTxChPrms->ringMemAllocFxn     = &CpswAppMemUtils_allocRingMemFxn;
    pTxChPrms->ringMemFreeFxn      = &CpswAppMemUtils_freeRingMemFxn;

    pTxChPrms->numTxPkts           = numTxPackets;
    pTxChPrms->disableCacheOpsFlag = false;

    pTxChPrms->dmaDescAllocFxn     = &CpswAppMemUtils_allocDmaDescFxn;
    pTxChPrms->dmaDescFreeFxn      = &CpswAppMemUtils_freeDmaDescFxn;

    pTxChPrms->hCbArg        = cbArg;

    pTxChPrms->notifyCb = eventCb;

}

void CpswRemoteApp_allocTxCh(Mailbox_Handle hCmdMbx,
                             Mailbox_Handle hResponseMbx,
                             Cpsw_Handle hCpsw,
                             uint32_t coreKey,
                             uint32_t *txPSILThreadId)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.alloc.id = (uint64_t)hCpsw;
    msg.req.u.alloc.core_key = coreKey;
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_ALLOCTX, &msg);
    *txPSILThreadId = msg.res.u.tx.tx_id;
}

static void CpswRemoteApp_openNDKTxCh(Mailbox_Handle hCmdMbx,
                                      Mailbox_Handle hResponseMbx,
                                      Cpsw_Handle hCpsw,
                                      Udma_DrvHandle  hUdmaDrv,
                                      uint32_t coreKey,
                                      uint32_t txPSILId,
                                      NimuCpswAppIf_TxConfig *txConfig,
                                      NimuCpswAppIf_TxHandleInfo *txHandleInfo)
{
    CpswDma_OpenTxChPrms   cpswTxChCfg;


    txHandleInfo->txChNum = txPSILId;
    /* Set configuration parameters */
    CpswDma_initTxChParams(&cpswTxChCfg);
    CpswRemoteApp_setTxChPrms(&cpswTxChCfg,
                              txHandleInfo->txChNum,
                              hUdmaDrv,
                              txConfig->numPackets,
                              txConfig->cbArg,
                              txConfig->notifyCb);
    txHandleInfo->hTxChannel = CpswDma_openTxCh(&cpswTxChCfg);
    assert (NULL != txHandleInfo->hTxChannel);
}

static void CpswRemoteApp_freeTxCh(Mailbox_Handle hCmdMbx,
                                   Mailbox_Handle hResponseMbx,
                                   Cpsw_Handle hCpsw,
                                   uint32_t coreKey,
                                   uint32_t txChNum)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.cmd = ETHSWT_CMD_FREETX;
    msg.req.u.freetx.id    = (uint64_t)hCpsw;
    msg.req.u.freetx.core_key = coreKey;
    msg.req.u.freetx.tx_id    = txChNum;
    
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_FREETX, &msg);
}

static void CpswRemoteApp_closeNDKTxCh(Mailbox_Handle hCmdMbx,
                                       Mailbox_Handle hResponseMbx,
                                       Cpsw_Handle hCpsw,
                                       Udma_DrvHandle  hUdmaDrv,
                                       uint32_t coreKey,
                                       NimuCpswAppIf_TxHandleInfo *txHandleInfo,
                                       void *freeFxnArg,
                                       NimuCpswAppIf_FreePktCbFxn freeFxn)
{
    CpswDma_PktInfoQ fqPktInfoQ;
    CpswDma_PktInfoQ cqPktInfoQ;
    int32_t          status;

    CpswUtils_initQ(&fqPktInfoQ);
    CpswUtils_initQ(&cqPktInfoQ);

    CpswDma_disableTxEvent(txHandleInfo->hTxChannel);
    status = CpswDma_closeTxCh(txHandleInfo->hTxChannel, &fqPktInfoQ, &cqPktInfoQ);
    assert (CPSW_SOK == status);

    CpswRemoteApp_freeTxCh(hCmdMbx,
                           hResponseMbx,
                           hCpsw,
                           coreKey,
                           txHandleInfo->txChNum);
    freeFxn(freeFxnArg, &fqPktInfoQ, &cqPktInfoQ);
}

static void CpswRemoteApp_attach(Mailbox_Handle hCmdMbx,
                                 Mailbox_Handle hResponseMbx,
                                 enum rpmsg_kdrv_ethswitch_cpsw_type cpswType,
                                 Cpsw_Handle *pCpswHandle,
                                 uint32_t *coreKey,
                                 uint32_t *rxMtu,
                                 uint32_t *txMtu)
{
    rdevEthSwitchAppMsg_t msg;
    uint32_t i;

    msg.req.u.attach.cpswType = cpswType;
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_ATTACH, &msg);
    *pCpswHandle = (Cpsw_Handle) ((uintptr_t)(msg.res.u.attach.id));
    *coreKey = msg.res.u.attach.core_key;
    *rxMtu = msg.res.u.attach.rx_mtu;
    for (i = 0; i < CPSW_UTILS_ARRAYSIZE(msg.res.u.attach.tx_mtu); i++)
    {
        txMtu[i] = msg.res.u.attach.tx_mtu[i];
    }
}

static void CpswRemoteApp_attachExtended(Mailbox_Handle hCmdMbx,
                                         Mailbox_Handle hResponseMbx,
                                         enum rpmsg_kdrv_ethswitch_cpsw_type cpswType,
                                         Cpsw_Handle *pCpswHandle,
                                         uint32_t *coreKey,
                                         uint32_t *rxMtu,
                                         uint32_t *txMtu,
                                         uint32_t *txPSILThreadId,
                                         uint32_t *rxFlowStartIdx,
                                         uint32_t *rxFlowIdx,
                                         uint8_t  *macAddress)
{
    rdevEthSwitchAppMsg_t msg;
    uint32_t i;
    uint32_t absRxFlowIdx;

    msg.req.u.attach.cpswType = cpswType;
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_ATTACHEXT, &msg);
    *pCpswHandle = (Cpsw_Handle) ((uintptr_t)(msg.res.u.attachext.id));
    *coreKey = msg.res.u.attachext.core_key;
    *rxMtu = msg.res.u.attachext.rx_mtu;
    for (i = 0; i < CPSW_UTILS_ARRAYSIZE(msg.res.u.attachext.tx_mtu); i++)
    {
        txMtu[i] = msg.res.u.attachext.tx_mtu[i];
    }
    *txPSILThreadId = msg.res.u.attachext.tx_id;
    absRxFlowIdx = msg.res.u.attachext.rx_flow_allocidx;
    CpswRemoteApp_getRxStartFlowIdx(hCmdMbx, 
                                    hResponseMbx, 
                                    *pCpswHandle, 
                                    *coreKey, 
                                    rxFlowStartIdx);
    assert((absRxFlowIdx >= *rxFlowStartIdx) && (absRxFlowIdx < (*rxFlowStartIdx + CPSW_DMA_MAX_RX_FLOW)));
    *rxFlowIdx = (absRxFlowIdx - *rxFlowStartIdx);
    memcpy(macAddress, msg.res.u.attachext.mac_address, sizeof(msg.res.u.attachext.mac_address));
}


static void CpswRemoteApp_detach(Mailbox_Handle hCmdMbx,
                                 Mailbox_Handle hResponseMbx,
                                 Cpsw_Handle hCpsw,
                                 uint32_t coreKey)

{
    rdevEthSwitchAppMsg_t msg;

    msg.req.cmd = ETHSWT_CMD_DETACH;
    msg.req.u.detach.id = (uint64_t)hCpsw;
    msg.req.u.detach.core_key = coreKey;

    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_DETACH, &msg);
}

static void CpswRemoteApp_registerIPV4Addr(Mailbox_Handle hCmdMbx,
                                         Mailbox_Handle hResponseMbx,
                                         Cpsw_Handle hCpsw,
                                         uint32_t coreKey,
                                         uint8_t  *macAddr,
                                         uint8_t  *ipv4Addr)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.regipv4.id       = (uint64_t)hCpsw;
    msg.req.u.regipv4.core_key = coreKey;
    CPSW_UTILS_ARRAY_COPY(msg.req.u.regipv4.mac_address, macAddr);
    memcpy(msg.req.u.regipv4.ipv4Addr, ipv4Addr,sizeof(msg.req.u.regipv4.ipv4Addr));
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_REGIPV4, &msg);
}

static void CpswRemoteApp_unregisterIPV4Addr(Mailbox_Handle hCmdMbx,
                                             Mailbox_Handle hResponseMbx,
                                             Cpsw_Handle hCpsw,
                                             uint32_t coreKey,
                                             uint8_t  *ipv4Addr)
{
    rdevEthSwitchAppMsg_t msg;

    msg.req.u.unregipv4.id       = (uint64_t)hCpsw;
    msg.req.u.unregipv4.core_key = coreKey;
    memcpy(msg.req.u.unregipv4.ipv4Addr, ipv4Addr,sizeof(msg.req.u.unregipv4.ipv4Addr));
    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_UNREGIPV4, &msg);
}

static uint64_t CpswRemoteApp_virtToPhyFxn(const void *virtAddr,
                                   void       *appData)
{
    return ((uint64_t) virtAddr);
}

static void *CpswRemoteApp_phyToVirtFxn(uint64_t phyAddr,
                                void    *appData)
{
#if defined (__aarch64__)
    uint64_t temp = phyAddr;
#else
    /* R5 is 32-bit machine, need to truncate to avoid void * typecast error */
    uint32_t temp = (uint32_t) phyAddr;
#endif
    return ((void *) temp);
}


static CpswDma_Handle CpswRemoteApp_initCpswDma(Cpsw_Type cpswType, Udma_DrvHandle hUdmaDrv)
{
    CpswDma_DataPathConfig dmaConfig;
    CpswDma_Handle cpswDmaHandle = NULL;

    CpswDma_initDataPathParams(&dmaConfig);
    dmaConfig.hUdmaDrv = hUdmaDrv;
    cpswDmaHandle = CpswDma_initDataPath(cpswType, &dmaConfig);
    return cpswDmaHandle;
}

static int32_t CpswRemoteApp_deinitCpswDma(CpswDma_Handle cpswDmaHandle)
{
    int32_t status;

    status = CpswDma_deInitDataPath(cpswDmaHandle);
    return status;
}


static enum rpmsg_kdrv_ethswitch_cpsw_type CpswRemoteApp_getRdevCpswType(Cpsw_Type cpswType)
{
    enum rpmsg_kdrv_ethswitch_cpsw_type rdevCpswType;

    switch(cpswType)
    {
        case CPSW_2G:
            rdevCpswType = RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_2G;
            break;
    
        case CPSW_9G:
            rdevCpswType = RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_9G;
            break;
    
    }
    return rdevCpswType;
}

void NimuCpswAppCb_getHandle(NimuCpswAppIf_GetHandleInArgs *inArgs,
                             NimuCpswAppIf_GetHandleOutArgs *outArgs)
{
    int32_t status;
    uint32_t coreId = CpswAppSoc_getCoreId();
    CpswOsal_Prms  osalPrms;
    CpswUtils_Prms utilsPrms;
    Cpsw_Type cpswType = CPSW_9G;
    enum rpmsg_kdrv_ethswitch_cpsw_type rdevCpswType;
    uint32_t txPSILId;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;

    assert(gRemoteAppObj.hCmdMbx != NULL);
    assert(gRemoteAppObj.hResponseMbx != NULL);
    
    rdevCpswType = CpswRemoteApp_getRdevCpswType(cpswType);

    /* Initialize CPSW driver with default OSAL, utils */
    utilsPrms.printFxn     = (Cpsw_PrintFxnCb) System_printf;
    utilsPrms.traceFxn     = (Cpsw_TraceFxnCb) System_printf;
    utilsPrms.phyToVirtFxn = &CpswRemoteApp_phyToVirtFxn;
    utilsPrms.virtToPhyFxn = &CpswRemoteApp_virtToPhyFxn;

    Cpsw_initOsalPrms(&osalPrms);

    Cpsw_init(cpswType, &osalPrms, &utilsPrms);

    status = CpswAppMemUtils_init();
    assert(status == CPSW_SOK);
    outArgs->coreId = coreId;
    outArgs->hUdmaDrv = CpswRemoteApp_udmaOpen();
    outArgs->printFxnCb = (Cpsw_PrintFxnCb)&ConPrintf;
    outArgs->isPhyLinkedFxn = &CpswRemoteApp_IsAllPhyLinked;
    outArgs->isRingMonUsed = false;
    outArgs->clkPeriodMs   = CPSW_REMOTE_APP_PACKET_POLL_PERIOD_MS;

    gRemoteAppObj.hDma    = CpswRemoteApp_initCpswDma(cpswType, outArgs->hUdmaDrv);

    if (gRemoteAppObj.useExtAttach)
    {
        CpswRemoteApp_attachExtended(gRemoteAppObj.hCmdMbx, 
                                     gRemoteAppObj.hResponseMbx, 
                                     rdevCpswType,
                                     &outArgs->hCpsw,
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
        CpswRemoteApp_attach(gRemoteAppObj.hCmdMbx, 
                             gRemoteAppObj.hResponseMbx, 
                             rdevCpswType,
                             &outArgs->hCpsw,
                             &outArgs->coreKey,
                             &outArgs->hostPortRxMtu,
                             outArgs->txMtu);
        CpswRemoteApp_allocTxCh(gRemoteAppObj.hCmdMbx, 
                                gRemoteAppObj.hResponseMbx,
                                outArgs->hCpsw, 
                                outArgs->coreKey, 
                                &txPSILId);
        CpswRemoteApp_allocRxFlow(gRemoteAppObj.hCmdMbx,
                                  gRemoteAppObj.hResponseMbx,
                                  outArgs->hCpsw, 
                                  outArgs->coreKey, 
                                  &rxStartFlowId,
                                  &rxFlowIdOffset);
        CpswRemoteApp_allocMac(gRemoteAppObj.hCmdMbx, 
                               gRemoteAppObj.hResponseMbx,
                               outArgs->hCpsw,
                               outArgs->coreKey,
                               gRemoteAppObj.macAddr);
    }
    CpswRemoteApp_openNDKTxCh(gRemoteAppObj.hCmdMbx, 
                              gRemoteAppObj.hResponseMbx,
                              outArgs->hCpsw, 
                              outArgs->hUdmaDrv, 
                              outArgs->coreKey, 
                              txPSILId,
                              &inArgs->txCfg,
                              &outArgs->txInfo);

    CpswRemoteApp_openNDKRxCh(gRemoteAppObj.hCmdMbx, 
                              gRemoteAppObj.hResponseMbx,
                              outArgs->hCpsw, 
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
    gRemoteAppObj.hCpsw   = outArgs->hCpsw;
}

void NimuCpswAppCb_releaseHandle(NimuCpswAppIf_ReleaseHandleInfo *releaseInfo)
{
    assert(gRemoteAppObj.hCmdMbx != NULL); 
    assert(gRemoteAppObj.hResponseMbx != NULL);

    CpswRemoteApp_closeNDKTxCh(gRemoteAppObj.hCmdMbx, 
                               gRemoteAppObj.hResponseMbx,
                               releaseInfo->hCpsw, 
                               releaseInfo->hUdmaDrv, 
                               releaseInfo->coreKey, 
                               &releaseInfo->txInfo,
                               releaseInfo->freePktCbArg,
                               releaseInfo->txFreePktCb);
    CpswRemoteApp_closeNDKRxCh(gRemoteAppObj.hCmdMbx, 
                               gRemoteAppObj.hResponseMbx,
                               releaseInfo->hCpsw, 
                               releaseInfo->hUdmaDrv, 
                               releaseInfo->coreKey, 
                               gRemoteAppObj.useDefaultRxFlow,
                               gRemoteAppObj.ipv4Addr,
                               &releaseInfo->rxInfo,
                               releaseInfo->freePktCbArg,
                               releaseInfo->rxFreePktCb);

    CpswRemoteApp_detach(gRemoteAppObj.hCmdMbx, gRemoteAppObj.hResponseMbx,releaseInfo->hCpsw, releaseInfo->coreKey);
    CpswRemoteApp_deinitCpswDma(gRemoteAppObj.hDma);
}

static bool CpswRemoteApp_isPhyLinked(Mailbox_Handle hCmdMbx,
                                      Mailbox_Handle hResponseMbx,
                                      Cpsw_Handle hCpsw,
                                      uint32_t coreKey,
                                      Cpsw_MacPort portNum)
{
    rdevEthSwitchAppMsg_t msg;
    Cpsw_GenericPortLinkInArgs inArgs;
    bool isLinked;

    memset(&inArgs, 0 , sizeof(inArgs));
    inArgs.portNum = portNum;


    msg.req.u.ioctl.cmd = CPSW_IOCTL_IS_PORT_LINK_UP;
    msg.req.u.ioctl.core_key = coreKey;
    msg.req.u.ioctl.id       = (uint64_t) hCpsw;
    msg.req.u.ioctl.inArgsSize = sizeof(inArgs);
    msg.req.u.ioctl.inArgs     = &inArgs;
    msg.req.u.ioctl.outArgs     = &isLinked;
    msg.req.u.ioctl.outArgsSize = sizeof(isLinked);

    CpswRemoteApp_sendCmd(hCmdMbx, hResponseMbx, ETHSWT_CMD_IOCTL, &msg);
    return isLinked;
}

