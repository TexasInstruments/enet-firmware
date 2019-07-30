/*
 *
 * Copyright (c) 2017 Texas Instruments Incorporated
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
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>

#define IPC_RPMESSAGE_OBJ_SIZE  256
#define VQ_BUF_SIZE             2048
#define REMOTE_DEVICE_ENDPT     26
#define RPMSG_DATA_SIZE         (256*512 + IPC_RPMESSAGE_OBJ_SIZE)
#define VRING_BASE_ADDRESS      0xBA000000
#define VRING_BUFFER_SIZE       0x02000000

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

    static SemaphoreP_Handle g_ipc_init_wait_sem;
    static SemaphoreP_Handle g_rdev_start_sem;

    static uint32_t selfProcId = IPC_MCU2_1;
    static uint32_t gRemoteProc[] =
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
};
static uint32_t gNumRemoteProc = sizeof(gRemoteProc)/sizeof(uint32_t);

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
        return;
    }

    status = RPMessage_lateInit(IPC_MPU1_0);
    if(status != IPC_SOK)
    {
        System_printf("%s: RPMessage_lateInit failed\n", __func__);
        return;
    }
}

Mailbox_Handle gMessageTskMailbox = NULL;

enum rdevEthSwitchAppNotifyTskCmds_e
{
    RDEVETHSWITCHAPP_NOTIFYTSKCMD_ATTACH,
    RDEVETHSWITCHAPP_NOTIFYTSKCMD_DETACH,
};

typedef struct rdevEthSwitchAppNotifyTskCmdInfo_s
{
    enum rdevEthSwitchAppNotifyTskCmds_e cmd;
    uint64_t  handle;
    uint32_t  coreKey;
} rdevEthSwitchAppNotifyTskCmdInfo_t;

#define RDEVETHSWITCHAPP_NOTIFYTSKCMD_COUNT (3)

static void  rdevEthSwitchApp_createMbx(Mailbox_Handle *pMailboxHandle)
{
    Mailbox_Params mbxParams;

    Mailbox_Params_init(&mbxParams);
    *pMailboxHandle =
        Mailbox_create(
            sizeof (rdevEthSwitchAppNotifyTskCmdInfo_t),
            RDEVETHSWITCHAPP_NOTIFYTSKCMD_COUNT,
            &mbxParams,
            NULL);
}

static void  rdevEthSwitchApp_deleteMbx(Mailbox_Handle *pMailboxHandle)
{
    Mailbox_delete(pMailboxHandle);
}

static void messageLoopFn(UArg a0, UArg a1)
{
    uint32_t device_id = (uint32_t)a0;
    Mailbox_Handle hMailbox = (Mailbox_Handle)a1;
    rdevEthSwitchAppNotifyTskCmdInfo_t msg;
    bool sendNotify = false;
    uint64_t attachHandle = 0;
    uint32_t coreKey = 0;
    uint32_t count = 0;
    

    while(TRUE) 
    {
        bool mbxStatus;

        CpswAppUtils_assert(hMailbox != NULL);
        mbxStatus =
            Mailbox_pend(hMailbox,
                         &msg,
                         10000);
        sendNotify = false;
        if (mbxStatus == TRUE)
        {
            switch (msg.cmd)
            {
                case RDEVETHSWITCHAPP_NOTIFYTSKCMD_ATTACH:
                {
                    count++;
                    attachHandle = msg.handle;
                    coreKey      = msg.coreKey;
                    if ((count % 10) == 0)
                    {
                        sendNotify   = true;
                    }
                }
                break;
                case RDEVETHSWITCHAPP_NOTIFYTSKCMD_DETACH:
                {
                    attachHandle = 0;
                    coreKey      = 0;
                    sendNotify   = false;
                }
                break;
                default:
                {
                    CpswAppUtils_assert(false);
                }
            }
        }
        if (sendNotify)
        {
            uint8_t notifyInfo[] = {'d','u','m','p','s','t','a','t','s'};
            CpswAppUtils_assert(attachHandle != 0);
            CpswAppUtils_assert(coreKey != 0);
            System_printf("%s: sending message\n", __func__);
            rdevEthSwitchClient_sendNotify(device_id, attachHandle, coreKey, RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_DUMPSTATS, notifyInfo, sizeof(notifyInfo));
        }
    }
}

static void rdevEthSwitchApp_printStatsNonZero(const char *pcString, uint64_t statVal)
{
    if (0U != statVal)
    {
        System_printf(pcString, statVal);
    }
}

static void rdevEthSwitchApp_printStatsWithIdxNonZero(const char *pcString,
                                                  uint32_t idx,
                                                  uint64_t statVal)
{
    if (0U != statVal)
    {
        System_printf(pcString, idx, statVal);
    }
}

CpswStats_MacPort_9g gCpswStats;

static void rdevEthSwitchApp_printMacPortStats9G(CpswStats_MacPort_9g *st)
{
    uint_fast32_t i;

    gCpswStats = *st;
    rdevEthSwitchApp_printStatsNonZero("  rxGoodFrames            = %u\n",(uint32_t)(st->rxGoodFrames));
    rdevEthSwitchApp_printStatsNonZero("  rxBcastFrames            = %u\n",(uint32_t)(st->rxBcastFrames));
    rdevEthSwitchApp_printStatsNonZero("  rxMcastFrames            = %u\n",(uint32_t)(st->rxMcastFrames));
    rdevEthSwitchApp_printStatsNonZero("  rxPauseFrames            = %u\n",(uint32_t)(st->rxPauseFrames));
    rdevEthSwitchApp_printStatsNonZero("  rxCrcErrors            = %u\n",(uint32_t)(st->rxCrcErrors));
    rdevEthSwitchApp_printStatsNonZero("  rxAlignCodeErrors            = %u\n",(uint32_t)(st->rxAlignCodeErrors));
    rdevEthSwitchApp_printStatsNonZero("  rxOversizedFrames            = %u\n",(uint32_t)(st->rxOversizedFrames));
    rdevEthSwitchApp_printStatsNonZero("  rxJabberFrames            = %u\n",(uint32_t)(st->rxJabberFrames));
    rdevEthSwitchApp_printStatsNonZero("  rxUndersizedFrames            = %u\n",(uint32_t)(st->rxUndersizedFrames));
    rdevEthSwitchApp_printStatsNonZero("  rxFragments            = %u\n",(uint32_t)(st->rxFragments));
    rdevEthSwitchApp_printStatsNonZero("  aleDrop            = %u\n",(uint32_t)(st->aleDrop));
    rdevEthSwitchApp_printStatsNonZero("  aleOverrunDrop            = %u\n",(uint32_t)(st->aleOverrunDrop));
    rdevEthSwitchApp_printStatsNonZero("  rxOctets            = %u\n",(uint32_t)(st->rxOctets));
    rdevEthSwitchApp_printStatsNonZero("  txGoodFrames            = %u\n",(uint32_t)(st->txGoodFrames));
    rdevEthSwitchApp_printStatsNonZero("  txBcastFrames            = %u\n",(uint32_t)(st->txBcastFrames));
    rdevEthSwitchApp_printStatsNonZero("  txMcastFrames            = %u\n",(uint32_t)(st->txMcastFrames));
    rdevEthSwitchApp_printStatsNonZero("  txPauseFrames            = %u\n",(uint32_t)(st->txPauseFrames));
    rdevEthSwitchApp_printStatsNonZero("  txDeferredFrames            = %u\n",(uint32_t)(st->txDeferredFrames));
    rdevEthSwitchApp_printStatsNonZero("  txCollisionFrames            = %u\n",(uint32_t)(st->txCollisionFrames));
    rdevEthSwitchApp_printStatsNonZero("  txSingleCollFrames            = %u\n",(uint32_t)(st->txSingleCollFrames));
    rdevEthSwitchApp_printStatsNonZero("  txMultipleCollFrames            = %u\n",(uint32_t)(st->txMultipleCollFrames));
    rdevEthSwitchApp_printStatsNonZero("  txExcessiveCollFrames            = %u\n",(uint32_t)(st->txExcessiveCollFrames));
    rdevEthSwitchApp_printStatsNonZero("  txLateCollFrames            = %u\n",(uint32_t)(st->txLateCollFrames));
    rdevEthSwitchApp_printStatsNonZero("  rxIPGError            = %u\n",(uint32_t)(st->rxIPGError));
    rdevEthSwitchApp_printStatsNonZero("  txCarrierSenseErrors            = %u\n",(uint32_t)(st->txCarrierSenseErrors));
    rdevEthSwitchApp_printStatsNonZero("  txOctets            = %u\n",(uint32_t)(st->txOctets));
    rdevEthSwitchApp_printStatsNonZero("  octetsFrames64            = %u\n",(uint32_t)(st->octetsFrames64));
    rdevEthSwitchApp_printStatsNonZero("  octetsFrames65to127            = %u\n",(uint32_t)(st->octetsFrames65to127));
    rdevEthSwitchApp_printStatsNonZero("  octetsFrames128to255            = %u\n",(uint32_t)(st->octetsFrames128to255));
    rdevEthSwitchApp_printStatsNonZero("  octetsFrames256to511            = %u\n",(uint32_t)(st->octetsFrames256to511));
    rdevEthSwitchApp_printStatsNonZero("  octetsFrames512to1023            = %u\n",(uint32_t)(st->octetsFrames512to1023));
    rdevEthSwitchApp_printStatsNonZero("  octetsFrames1024            = %u\n",(uint32_t)(st->octetsFrames1024));
    rdevEthSwitchApp_printStatsNonZero("  netOctets            = %u\n",(uint32_t)(st->netOctets));
    rdevEthSwitchApp_printStatsNonZero("  rxBottomOfFifoDrop            = %u\n",(uint32_t)(st->rxBottomOfFifoDrop));
    rdevEthSwitchApp_printStatsNonZero("  portMaskDrop            = %u\n",(uint32_t)(st->portMaskDrop));
    rdevEthSwitchApp_printStatsNonZero("  rxTopOfFifoDrop            = %u\n",(uint32_t)(st->rxTopOfFifoDrop));
    rdevEthSwitchApp_printStatsNonZero("  aleRateLimitDrop            = %u\n",(uint32_t)(st->aleRateLimitDrop));
    rdevEthSwitchApp_printStatsNonZero("  aleVidIngressDrop            = %u\n",(uint32_t)(st->aleVidIngressDrop));
    rdevEthSwitchApp_printStatsNonZero("  aleDAEqSADrop            = %u\n",(uint32_t)(st->aleDAEqSADrop));
    rdevEthSwitchApp_printStatsNonZero("  aleBlockDrop            = %u\n",(uint32_t)(st->aleBlockDrop));
    rdevEthSwitchApp_printStatsNonZero("  aleSecureDrop            = %u\n",(uint32_t)(st->aleSecureDrop));
    rdevEthSwitchApp_printStatsNonZero("  aleAuthDrop            = %u\n",(uint32_t)(st->aleAuthDrop));
    rdevEthSwitchApp_printStatsNonZero("  aleUnknownUcast            = %u\n",(uint32_t)(st->aleUnknownUcast));
    rdevEthSwitchApp_printStatsNonZero("  aleUnknownUcastBcnt            = %u\n",(uint32_t)(st->aleUnknownUcastBcnt));
    rdevEthSwitchApp_printStatsNonZero("  aleUnknownMcast            = %u\n",(uint32_t)(st->aleUnknownMcast));
    rdevEthSwitchApp_printStatsNonZero("  aleUnknownMcastBcnt            = %u\n",(uint32_t)(st->aleUnknownMcastBcnt));
    rdevEthSwitchApp_printStatsNonZero("  aleUnknownBcast            = %u\n",(uint32_t)(st->aleUnknownBcast));
    rdevEthSwitchApp_printStatsNonZero("  aleUnknownBcastBcnt            = %u\n",(uint32_t)(st->aleUnknownBcastBcnt));
    rdevEthSwitchApp_printStatsNonZero("  alePolicyMatch            = %u\n",(uint32_t)(st->alePolicyMatch));
    rdevEthSwitchApp_printStatsNonZero("  alePolicyMatchRed            = %u\n",(uint32_t)(st->alePolicyMatchRed));
    rdevEthSwitchApp_printStatsNonZero("  alePolicyMatchYellow            = %u\n",(uint32_t)(st->alePolicyMatchYellow));
    rdevEthSwitchApp_printStatsNonZero("  aleMultSADrop            = %u\n",(uint32_t)(st->aleMultSADrop));
    rdevEthSwitchApp_printStatsNonZero("  aleDualVlanDrop            = %u\n",(uint32_t)(st->aleDualVlanDrop));
    rdevEthSwitchApp_printStatsNonZero("  aleLenErrorDrop            = %u\n",(uint32_t)(st->aleLenErrorDrop));
    rdevEthSwitchApp_printStatsNonZero("  aleIpNextHdrDrop            = %u\n",(uint32_t)(st->aleIpNextHdrDrop));
    rdevEthSwitchApp_printStatsNonZero("  aleIPv4FragDrop            = %u\n",(uint32_t)(st->aleIPv4FragDrop));
    rdevEthSwitchApp_printStatsNonZero("  ietRxAssemblyErr            = %u\n",(uint32_t)(st->ietRxAssemblyErr));
    rdevEthSwitchApp_printStatsNonZero("  ietRxAssemblyOk            = %u\n",(uint32_t)(st->ietRxAssemblyOk));
    rdevEthSwitchApp_printStatsNonZero("  ietRxSmdError            = %u\n",(uint32_t)(st->ietRxSmdError));
    rdevEthSwitchApp_printStatsNonZero("  ietRxFrag            = %u\n",(uint32_t)(st->ietRxFrag));
    rdevEthSwitchApp_printStatsNonZero("  ietTxHold            = %u\n",(uint32_t)(st->ietTxHold));
    rdevEthSwitchApp_printStatsNonZero("  ietTxFrag            = %u\n",(uint32_t)(st->ietTxFrag));
    rdevEthSwitchApp_printStatsNonZero("  txMemProtectError            = %u\n",(uint32_t)(st->txMemProtectError));

    for (i = 0; i < CPSW_UTILS_ARRAYSIZE(st->enetPnTxPri); i++)
    {
        rdevEthSwitchApp_printStatsWithIdxNonZero("  enetPnTxPri[%u]              = %u\n",i,(uint32_t)(st->enetPnTxPri[i]));
    }

    for (i = 0; i < CPSW_UTILS_ARRAYSIZE(st->enetPnTxPriBcnt); i++)
    {
        rdevEthSwitchApp_printStatsWithIdxNonZero("  enetPnTxPriBcnt[%u]          = %u\n",i,(uint32_t)(st->enetPnTxPriBcnt[i]));
    }

    for (i = 0; i < CPSW_UTILS_ARRAYSIZE(st->enetPnTxPriBcnt); i++)
    {
        rdevEthSwitchApp_printStatsWithIdxNonZero("  enetPnTxPriDrop[%u]          = %u\n",i,(uint32_t)(st->enetPnTxPriBcnt[i]));
    }

    for (i = 0; i < CPSW_UTILS_ARRAYSIZE(st->enetPnTxPriBcnt); i++)
    {
        rdevEthSwitchApp_printStatsWithIdxNonZero("  enetPnTxPriDropBcnt[%u]      = %u\n",i,(uint32_t)(st->enetPnTxPriBcnt[i]));
    }
}

uint32_t gRegWrAddr = 0xDEADBEEF;
uint32_t gRegRdAddr = 0x00C0FFEE;
bool freeInDetach = false;
static volatile bool gWaitInLoop = true;
static void requestLoopFn(UArg a0, UArg a1)
{
    uint32_t cnt = 0;
    uint32_t device_id = (uint32_t)a0;
    Mailbox_Handle hMailbox = (Mailbox_Handle)a1;
    uint64_t id;
    uint32_t core_key;
    uint32_t rx_mtu;
    uint32_t tx_mtu[RPMSG_KDRV_TP_ETHSWITCH_CPSW_PRIORITY_NUM];
    uint32_t features;
    uint32_t tx_id;
    uint32_t rx_flow_allocidx, rx_default_flow_allocidx;
    uint8_t  mac_address[RPMSG_KDRV_TP_ETHSWITCH_MACADDRLEN];
    uint8_t  ipv4Addr[] = {172,24,209,155};
    uint8_t  ipv6Addr[] = {0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,0x10};

    int32_t  ret;
    

    while(TRUE) {
        switch (cnt % 20)
        {
            case 0:
            {
                char     msgText[512];
                char     resp[512];

                memset(resp, 0, sizeof(resp));
                snprintf(msgText, sizeof(msgText), "ping-request %d", cnt);
                System_printf("%s: sending ping request\n", __func__);
                ret = rdevEthSwitchClient_sendping(device_id, msgText, strlen(msgText), resp, sizeof(resp));
                if (0 == ret)
                {
                    System_printf("%s: respose %s\n", __func__, resp);
                }
                break;
            }
            case 1:
            {
                ret = rdevEthSwitchClient_attach(device_id, RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_9G, &id, &core_key, &rx_mtu, tx_mtu, CPSW_UTILS_ARRAYSIZE(tx_mtu),&features);
                if (0 == ret)
                {
                    rdevEthSwitchAppNotifyTskCmdInfo_t notifyTskMsg;
                    Bool mbxStatus;

                    System_printf("Function:%s,Handle:%p,CoreKey:%x, RxMtu:%4u, TxMtu:%4u:%4u:%4u:%4u:%4u:%4u:%4u:%4u, TxCsumEnabled:%u\n",__func__,(uintptr_t)(id & 0xFFFFFFFFU), core_key, rx_mtu, tx_mtu[0], tx_mtu[1],tx_mtu[2],tx_mtu[3], tx_mtu[4], tx_mtu[5],tx_mtu[6],tx_mtu[7],((features & RPMSG_KDRV_TP_ETHSWITCH_FEATURE_TXCSUM) != 0));
                    notifyTskMsg.cmd = RDEVETHSWITCHAPP_NOTIFYTSKCMD_ATTACH;
                    notifyTskMsg.handle = id;
                    notifyTskMsg.coreKey = core_key;
                    CpswAppUtils_assert(hMailbox != NULL);
                    mbxStatus =
                        Mailbox_post(hMailbox,
                                     &notifyTskMsg,
                                     BIOS_WAIT_FOREVER);
                    CpswAppUtils_assert(mbxStatus == TRUE);

                }
                break;
            }
            case 2:
            {
                ret = rdevEthSwitchClient_alloctx(device_id, id, core_key, &tx_id);
                if (0 == ret)
                {
                    System_printf("Function:%s,Txid:%u\n",__func__,tx_id);
                }
                break;
            }
            case 3:
            {
                ret = rdevEthSwitchClient_allocrx(device_id, id, core_key, &rx_flow_allocidx);
                if (0 == ret)
                {
                    System_printf("Function:%s,RxAllocId:%u\n",__func__,rx_flow_allocidx);
                }
                break;
            }
            case 4:
            {
                ret = rdevEthSwitchClient_allocrx(device_id, id, core_key, &rx_default_flow_allocidx);
                if (0 == ret)
                {
                    System_printf("Function:%s,RxDefaultAllocId:%u\n",__func__,rx_default_flow_allocidx);
                }
                break;
            }
            case 5:
            {
                ret = rdevEthSwitchClient_registerrxdefault(device_id, id, core_key, rx_default_flow_allocidx);
                break;
            }
            case 6:
            {
                ret = rdevEthSwitchClient_allocmac(device_id, id, core_key, mac_address, CPSW_UTILS_ARRAYSIZE(mac_address));
                if (0 == ret)
                {
                    System_printf("Function:%s,mac_address:%2x:%2x:%2x:%2x:%2x:%2x \n",__func__,mac_address[0],mac_address[1],mac_address[2],mac_address[3],mac_address[4],mac_address[5]);
                }
                break;
            }
            case 7:
            {
                ret = rdevEthSwitchClient_registermac(device_id, id, core_key, rx_flow_allocidx, mac_address);
                break;
            }
            case 8:
            {
                ret = rdevEthSwitchClient_ipv4macregister(device_id, id, core_key, mac_address, ipv4Addr);
                break;
            }
            case 9:
            {
                ret = rdevEthSwitchClient_ipv6macregister(device_id, id, core_key, mac_address, ipv6Addr);
                break;
            }
            case 10:
            {
                ret = rdevEthSwitchClient_ipv4macunregister(device_id, id, core_key, ipv4Addr);
                break;
            }
            case 11:
            {
                CpswStats_GenericMacPortInArgs inArgs;
                CpswStats_PortStats portStats;

                inArgs.portNum = CPSW_MAC_PORT_1;
                ret = rdevEthSwitchClient_ioctl(device_id, id, core_key,CPSW_STATS_IOCTL_GET_MACPORT_STATS, &inArgs, sizeof(inArgs), &portStats, sizeof(portStats));
                if (0 == ret)
                {
                    CpswStats_MacPort_9g *st;

                    st = (CpswStats_MacPort_9g *)&portStats;
                    rdevEthSwitchApp_printMacPortStats9G(st);
                }
                break;
            }

            case 12:
            {
                ret = rdevEthSwitchClient_unregistermac(device_id, id, core_key, rx_flow_allocidx, mac_address);
                break;
            }
            case 13:
            {
                ret = rdevEthSwitchClient_unregisterrxdefault(device_id, id, core_key, rx_default_flow_allocidx);
                break;
            }
            case 14:
            {
                if (freeInDetach == false)
                {
                    ret = rdevEthSwitchClient_freemac(device_id, id, core_key, mac_address);
                }
                break;
            }
            case 15:
            {
                if (freeInDetach == false)
                {
                    ret = rdevEthSwitchClient_freetx(device_id, id, core_key, tx_id);
                }
                break;
            }
            case 16:
            {
                if (freeInDetach == false)
                {
                    ret = rdevEthSwitchClient_freerx(device_id, id, core_key, rx_flow_allocidx);
                }
                break;
            }
            case 17:
            {
                {
                    rdevEthSwitchAppNotifyTskCmdInfo_t notifyTskMsg;
                    Bool mbxStatus;

                    notifyTskMsg.cmd = RDEVETHSWITCHAPP_NOTIFYTSKCMD_DETACH;
                    notifyTskMsg.handle = id;
                    notifyTskMsg.coreKey = core_key;
                    CpswAppUtils_assert(hMailbox != NULL);
                    mbxStatus =
                        Mailbox_post(hMailbox,
                                     &notifyTskMsg,
                                     BIOS_WAIT_FOREVER);
                    CpswAppUtils_assert(mbxStatus == TRUE);

                }

                ret = rdevEthSwitchClient_detach(device_id, id, core_key);
                break;
            }
            case 18:
            {
                uint32_t postWriteVal;
                ret = rdevEthSwitchClient_regwr(device_id, (uint32_t)&gRegWrAddr, 0xBABEFACE, &postWriteVal);
                if (0 == ret)
                {
                    CpswAppUtils_assert(postWriteVal == 0xBABEFACE);
                }
                break;
            }
            case 19:
            {
                uint32_t regRdVal;
                ret = rdevEthSwitchClient_regrd(device_id, (uint32_t)&gRegRdAddr, &regRdVal);
                if (0 == ret)
                {
                    CpswAppUtils_assert(regRdVal == gRegRdAddr);
                }
                break;
            }
        }
        cnt++;
        while (((cnt % 20) > 8) && gWaitInLoop)
        {
            Task_sleep(10);
        }
    }
}

static void startMessageAndRequestLoop(uint32_t device_id)
{
    Task_Params params;

    rdevEthSwitchApp_createMbx(&gMessageTskMailbox);
    
    CpswAppUtils_assert(gMessageTskMailbox != NULL);
    Task_Params_init(&params);
    params.priority = 1;
    params.stackSize = IPC_TASK_STACKSIZE;
    params.stack = &g_messageTaskStack[0];
    params.stackSize = IPC_TASK_STACKSIZE;
    params.arg0 = device_id;
    params.arg1 = (uint32_t)gMessageTskMailbox;
    Task_create(messageLoopFn, &params, NULL);

    Task_Params_init(&params);
    params.priority = 3;
    params.stackSize = IPC_TASK_STACKSIZE;
    params.stack = &g_requestTaskStack[0];
    params.stackSize = IPC_TASK_STACKSIZE;
    params.arg0 = device_id;
    params.arg1 = (uint32_t)gMessageTskMailbox;
    Task_create(requestLoopFn, &params, NULL);
}


static Void monitorAndUnlockRdev(UArg a0, UArg a1)
{
    int32_t ret = 0;
    rdevEthSwitchClientInitPrms_t prm;

    SemaphoreP_pend(g_ipc_init_wait_sem, SemaphoreP_WAIT_FOREVER);
    SemaphoreP_post(g_rdev_start_sem);


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
        System_printf("Registered a device name = %s, data = %s, id = %u, type = %u\n",
                "mcu2_0-ethswitch-0", prm.data, prm.device_id, prm.device_type);
    }

    startMessageAndRequestLoop(prm.device_id);

}

static Void ipc_init(UArg a0, UArg a1)
{
    Task_Params       params;
    uint32_t          numProc = gNumRemoteProc;
    Ipc_VirtIoParams  vqParam;

    /* Step1 : Initialize the multiproc */
    Ipc_mpSetConfig(selfProcId, numProc, &gRemoteProc[0]);

    System_printf("IPC_echo_test (core : %s) .....\r\n",
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

    appRemoteDeviceInitParamsInit(&remote_dev_init_prm);

    remote_dev_init_prm.rpmsg_buf_size = 256;
    remote_dev_init_prm.remote_endpt = REMOTE_DEVICE_ENDPT;
    remote_dev_init_prm.wait_sem = g_rdev_start_sem;
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
    g_ipc_init_wait_sem = SemaphoreP_create(0, &sem_params);

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

int main(void)
{
    Task_Handle task;
    Task_Params taskParams;


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

