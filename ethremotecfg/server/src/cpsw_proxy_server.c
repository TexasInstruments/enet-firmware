/*
 *
 * Copyright (c) 2020 Texas Instruments Incorporated
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

#include <ti/osal/osal.h>
#include <ti/osal/SemaphoreP.h>
#include <ti/osal/TaskP.h>
#include <ti/osal/EventP.h>

#include <ti/drv/ipc/ipc.h>
#include <server-rtos/remote-device.h>
#include <ethremotecfg/server/include/ethremotecfg_server.h>
#include <ethremotecfg/server/include/cpsw_proxy_server.h>

#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_mcm.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appsoc.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apprm.h>

/* NDK headers */
#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/ndk/inc/socket.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/tools/servers.h>
#include <ti/ndk/inc/tools/console.h>

#include <ethremotecfg/protocol/Eth_Rpc.h>
#include <ethremotecfg/protocol/cpsw_remote_notify_service.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define CPSWPROXY_RDEV_MSGSIZE                          (256U)

#define CPSWPROXY_CPSW9G_HWPUSH_BASE                    (26U)

#define CPSWPROXY_CPTS_HWPUSH_EVENTS_OR_MASK            (0xFFU)

#define CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_NAME           ("ASRETHDEVICE")
/**< Task name */
#define CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_PRIORITY       (2U)
/**< Task priority */
#define CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_STACK          (0x4000)
/**< Stack required for the task */

#define CPSWPROXY_AUTOSAR_ETHDRIVER_MSG_SIZE            (512U)

#define CPSWPROXY_AUTOSAR_ETHDRIVER_NUM_RPMSG_BUFS      (256U)

#define CPSWPROXY_AUTOSAR_ETHDRIVER_RPMSG_OBJ_SIZE      (256U)

#define CPSWPROXY_AUTOSAR_ETHDRIVER_DATA_SIZE           (CPSWPROXY_AUTOSAR_ETHDRIVER_MSG_SIZE * \
                                                         CPSWPROXY_AUTOSAR_ETHDRIVER_NUM_RPMSG_BUFS + \
                                                         CPSWPROXY_AUTOSAR_ETHDRIVER_RPMSG_OBJ_SIZE)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */
typedef struct CpswProxyServer_HandleEntry_s
{
    Cpsw_Type   cpswType;
    Cpsw_Handle cpswHandle;
    CpswMcm_CmdIf  *hMcmCmdIf;
} CpswProxyServer_HandleEntry;

typedef struct CpswProxyServer_HandleTable_s
{
    uint32_t numEntries;
    CpswProxyServer_HandleEntry entries[CPSW_COUNT];
} CpswProxyServer_HandleTable;

typedef struct CpswProxyServer_EthDriverObj_s
{
    Task_Handle                  hAutosarEthTsk;
    RPMessage_Handle             hAutosarEthRpMsgEp;
    uint32_t                     dstProc;
    uint32_t                     localEp;
    uint32_t                     remoteEp;
} CpswProxyServer_EthDriverObj;

typedef struct CpswProxyServer_NotifyServiceObj_s
{
    Cpsw_Type                    notifyServiceCpswType;
    Task_Handle                  hNotifyServiceTsk;
    EventP_Handle                hHwPushNotifyServiceEvent;
    uint32_t                     hwPushNotifyEventId[CPSW_CPTS_HW_PUSH_COUNT_MAX];
    RPMessage_Handle             hNotifyServicRpMsgEp;
    uint32_t                     dstProc;
    uint32_t                     localEp;
    uint32_t                     remoteEp;
} CpswProxyServer_NotifyServiceObj;

typedef struct CpswProxyServer_Obj_s
{
    bool                        initDone;
    CpswProxyServer_HandleTable handleTbl;
    CpswProxyServer_InitEthfwDeviceDataCb initEthfwDeviceDataCb;
    CpswProxyServer_GetMcmCmdIfCb         getMcmCmdIfCb;
    CpswProxyServer_NotifyCb              notifyCb;
    SemaphoreP_Handle                     rdevStartSem;
    CpswProxyServer_EthDriverObj          ethDrvObj;
    CpswProxyServer_NotifyServiceObj      notifyServiceObj;
} CpswProxyServer_Obj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */
static int32_t CpswProxyServer_initAutosarEthDeviceEp(CpswProxyServer_Obj * hProxyServer,
                                                      CpswProxyServer_Config_t * cfg);
static Void CpswProxyServer_autosarEthDriverTaskFxn(UArg arg0, UArg arg1);

static int32_t CpswProxyServer_initNotifyServiceEp(CpswProxyServer_Obj * hProxyServer,
                                                      CpswProxyServer_Config_t * cfg);

static void CpswProxyServer_hwPushNotifyFxn(void *arg, CpswCpts_HwPush hwPushNum);

static Void CpswProxyServer_notifyServiceTaskFxn(UArg arg0, UArg arg1);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/**< Buffer to store received messages. 256 messages of 512 bytes +
        space for book-keeping */
static uint8_t g_CpswProxyServerAutosarRpmsgBuf[CPSWPROXY_AUTOSAR_ETHDRIVER_DATA_SIZE]  __attribute__ ((aligned(8192)));

/**< Buffer to store received messages. 256 messages of 512 bytes +
        space for book-keeping */
static uint8_t g_CpswProxyServerNotifyServiceRpmsgBuf[CPSW_REMOTE_NOTIFY_SERVICE_DATA_SIZE]  __attribute__ ((aligned(8192)));

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */
static int32_t CpswProxyServer_getMcmCmdIf(const CpswProxyServer_HandleTable *tbl, Cpsw_Type key, CpswMcm_CmdIf  **hMcmCmdIf)
{
    int32_t i;
    int32_t retVal;

    CpswAppUtils_assert(tbl->numEntries < CPSW_UTILS_ARRAYSIZE(tbl->entries));

    for (i = 0; i < tbl->numEntries; i++)
    {
        if (key == tbl->entries[i].cpswType)
        {
            break;
        }
    }
    if (i < tbl->numEntries)
    {
        *hMcmCmdIf = tbl->entries[i].hMcmCmdIf;
        retVal = CPSW_SOK;
    }
    else
    {
        *hMcmCmdIf = NULL;
        retVal = CPSW_EFAIL;
    }
    return retVal;
}

static int32_t CpswProxyServer_getCpswHandle(const CpswProxyServer_HandleTable *tbl, Cpsw_Type key, Cpsw_Handle  *pCpswHandle)
{
    int32_t i;
    int32_t retVal;

    CpswAppUtils_assert(tbl->numEntries < CPSW_UTILS_ARRAYSIZE(tbl->entries));

    for (i = 0; i < tbl->numEntries; i++)
    {
        if (key == tbl->entries[i].cpswType)
        {
            break;
        }
    }
    if (i < tbl->numEntries)
    {
        *pCpswHandle = tbl->entries[i].cpswHandle;
        retVal = CPSW_SOK;
    }
    else
    {
        *pCpswHandle = NULL;
        retVal = CPSW_EFAIL;
    }
    return retVal;
}

static int32_t CpswProxyServer_getCpswType(const CpswProxyServer_HandleTable *tbl, Cpsw_Handle hCpsw, Cpsw_Type *pCpswType)
{
    int32_t i;
    int32_t retVal;

    CpswAppUtils_assert(tbl->numEntries < CPSW_UTILS_ARRAYSIZE(tbl->entries));

    for (i = 0; i < tbl->numEntries; i++)
    {
        if (hCpsw == tbl->entries[i].cpswHandle)
        {
            break;
        }
    }
    if (i < tbl->numEntries)
    {
        *pCpswType = tbl->entries[i].cpswType;
        retVal = CPSW_SOK;
    }
    else
    {
        retVal = CPSW_EFAIL;
    }
    return retVal;
}

static  int32_t CpswProxy_mapRdev2CpswType(enum rpmsg_kdrv_ethswitch_cpsw_type  rdevCpswType, Cpsw_Type * pCpswType)
{
    int32_t retVal = CPSW_SOK;

    switch (rdevCpswType)
    {
        case RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_2G:
            *pCpswType = CPSW_2G;
            break;

        case RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_5G:
            *pCpswType = CPSW_5G;
            break;

        case RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_9G:
            *pCpswType = CPSW_9G;
            break;

        default:
            retVal     = CPSW_EFAIL;
            break;
    }
    return retVal;
}

static  int32_t CpswProxy_mapEthRpc2RdevCpswType(Eth_RpcCpswType ethRpcCpswType, enum rpmsg_kdrv_ethswitch_cpsw_type  *rdevCpswType)
{
    int32_t retVal = CPSW_SOK;

    switch (ethRpcCpswType)
    {
        case ETH_RPC_CPSWTYPE_2G:
            *rdevCpswType = RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_2G;
            break;

        case ETH_RPC_CPSWTYPE_9G:
            *rdevCpswType = RPMSG_KDRV_TP_ETHSWITCH_CPSWTYPE_9G;
            break;

        default:
            retVal     = CPSW_EFAIL;
            break;
    }
    return retVal;
}

static  int32_t CpswProxy_mapEthRpcClientNotify2RdevClientNotifyType(Eth_RpcClientNotifyType ethRpcClientNotifyType, enum rpmsg_kdrv_ethswitch_client_notify_type *rdevNotifyType)
{
    int32_t retVal = CPSW_SOK;

    switch (ethRpcClientNotifyType)
    {
        case ETH_RPC_CLIENTNOTIFY_DUMPSTATS:
            *rdevNotifyType = RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_DUMPSTATS;
            break;

        case ETH_RPC_CLIENTNOTIFY_CUSTOM:
            *rdevNotifyType = RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_CUSTOM;
            break;

        default:
            retVal     = CPSW_EFAIL;
            break;
    }
    return retVal;
}

static CpswProxyServer_Obj * CpswProxyServer_getHandle(void)
{
    static CpswProxyServer_Obj gProxyServerObj =
    {
        .handleTbl = {.numEntries = 0},
        .initEthfwDeviceDataCb = NULL,
        .getMcmCmdIfCb         = NULL,
        .rdevStartSem          = NULL,
        .initDone              = FALSE
    };

    return (&gProxyServerObj);
}


static void CpswProxyServer_addHandleEntry(CpswProxyServer_HandleTable *tbl, Cpsw_Handle hCpsw, Cpsw_Type cpswType, CpswMcm_CmdIf *hMcmCmdIf)
{
    int32_t status;
    Cpsw_Handle hCpswLocal;

    status = CpswProxyServer_getCpswHandle(tbl, cpswType, &hCpswLocal);

    if (status == CPSW_SOK)
    {
        CpswMcm_CmdIf *hMcmCmdIfLocal;

        CpswAppUtils_assert(hCpswLocal == hCpsw);
        CpswProxyServer_getMcmCmdIf(tbl, cpswType, &hMcmCmdIfLocal);
        CpswAppUtils_assert(hMcmCmdIfLocal == hMcmCmdIf);
    }
    else
    {
        CpswAppUtils_assert(tbl->numEntries < CPSW_UTILS_ARRAYSIZE(tbl->entries));
        tbl->entries[tbl->numEntries].cpswHandle = hCpsw;
        tbl->entries[tbl->numEntries].cpswType   = cpswType;
        tbl->entries[tbl->numEntries].hMcmCmdIf  = hMcmCmdIf;
        tbl->numEntries++;
    }
}


static int32_t CpswProxyServer_attachHandlerCb(uint32_t host_id,
                                               uint8_t cpsw_type,
                                               uint64_t *pId,
                                               uint32_t *pCoreKey,
                                               uint32_t *pRxMtu,
                                               uint32_t *pTxMtu,
                                               uint32_t txMtuArraySize,
                                               uint32_t *pFeatures)
{
    int32_t status;
    int32_t retVal;
    CpswMcm_HandleInfo handleInfo;
    Cpsw_AttachCoreOutArgs attachInfo;
    Cpsw_IoctlPrms prms;
    bool csumOffloadFlag;
    Cpsw_Type cpswType;
    CpswProxyServer_Obj * hProxyServer;
    CpswMcm_CmdIf *hMcmCmdIf;
    enum rpmsg_kdrv_ethswitch_cpsw_type  rdevCpswType = (enum rpmsg_kdrv_ethswitch_cpsw_type)cpsw_type;

    status = CpswProxy_mapRdev2CpswType(rdevCpswType, &cpswType);
    if (CPSW_SOK == status)
    {
        CpswAppUtils_print("Function:%s,HostId:%u,CpswType:%u\n", __func__, host_id, cpswType);

        hProxyServer = CpswProxyServer_getHandle();
        CpswAppUtils_assert((hProxyServer != NULL) && (hProxyServer->initDone == true));
        status = CpswProxyServer_getMcmCmdIf(&hProxyServer->handleTbl, cpswType, &hMcmCmdIf);
        if ((status != CPSW_SOK) || (hMcmCmdIf == NULL))
        {
            CpswAppUtils_assert(hProxyServer->getMcmCmdIfCb != NULL);
            hProxyServer->getMcmCmdIfCb(cpswType, &hMcmCmdIf);
            CpswAppUtils_assert(hMcmCmdIf != NULL);
            status = CPSW_SOK;
        }
    }

    if (CPSW_SOK == status)
    {
        CpswAppUtils_assert(hMcmCmdIf->hMboxCmd != NULL);
        CpswAppUtils_assert(hMcmCmdIf->hMboxResponse != NULL);

        CpswMcm_acquireHandleInfo(hMcmCmdIf, &handleInfo);
        CpswMcm_coreAttach(hMcmCmdIf, host_id, &attachInfo);

        *pId = (uint64_t)(handleInfo.hCpsw);
        *pCoreKey = attachInfo.coreKey;
        *pRxMtu = attachInfo.rxMtu;
        CpswAppUtils_assert(txMtuArraySize ==
                            CPSW_UTILS_ARRAYSIZE(attachInfo.txMtu));
        memcpy(pTxMtu, attachInfo.txMtu, sizeof(attachInfo.txMtu));
        *pFeatures = 0;
        CPSW_IOCTL_SET_OUT_ARGS(&prms, &csumOffloadFlag);
        status = Cpsw_ioctl(handleInfo.hCpsw,
                            host_id,
                            CPSW_HOSTPORT_IS_CSUM_OFFLOAD_ENABLE,
                            &prms);

        CpswAppUtils_assert(status == CPSW_SOK);

        if (csumOffloadFlag)
        {
            *pFeatures |= RPMSG_KDRV_TP_ETHSWITCH_FEATURE_TXCSUM;
        }
        CpswProxyServer_addHandleEntry(&hProxyServer->handleTbl, handleInfo.hCpsw, cpswType, hMcmCmdIf);
    }
    if (CPSW_SOK == status)
    {
        retVal = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }
    else
    {
        retVal = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    return retVal;
}

static void CpswProxyServer_validateHandle(Cpsw_Handle hCpsw)
{
    int32_t status;
    CpswProxyServer_Obj * hProxyServer;
    Cpsw_Type cpswType;

    hProxyServer = CpswProxyServer_getHandle();
    CpswAppUtils_assert((hProxyServer != NULL) && (hProxyServer->initDone == true));
    status = CpswProxyServer_getCpswType(&hProxyServer->handleTbl, hCpsw, &cpswType);
    CpswAppUtils_assert(CPSW_SOK == status);
    CpswAppUtils_assert(hCpsw == Cpsw_getHandle(cpswType));

}

static int32_t CpswProxyServer_allocTxHandlerCb(uint32_t host_id,
                                                uint64_t handle,
                                                uint32_t core_key,
                                                uint32_t *pTxCpswPsilDstId)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x\n", __func__, host_id, hCpsw, core_key);
    CpswProxyServer_validateHandle(hCpsw);

    status = CpswAppUtils_allocTxCh(hCpsw,
                                    core_key,
                                    host_id,
                                    pTxCpswPsilDstId);

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

static void CpswProxyServer_validateStartIdx(Cpsw_Handle hCpsw,
                                             uint32_t host_id,
                                             uint32_t rxFlowStartId)
{
    uint32_t p0FlowIdOffset;

    p0FlowIdOffset = CpswAppUtils_getStartFlowIdx(hCpsw, host_id);
    CpswAppUtils_assert(rxFlowStartId == p0FlowIdOffset);
}

static int32_t CpswProxyServer_allocRxHandlerCb(uint32_t host_id,
                                                uint64_t handle,
                                                uint32_t core_key,
                                                uint32_t *pAllocFlowIdx)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    uint32_t start_flow_idx, flow_idx_offset;

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x\n", __func__, host_id, hCpsw, core_key);
    CpswProxyServer_validateHandle(hCpsw);

    status = CpswAppUtils_allocRxFlow(hCpsw, core_key, host_id, &start_flow_idx, &flow_idx_offset);

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        CpswProxyServer_validateStartIdx(hCpsw, host_id, start_flow_idx);
        *pAllocFlowIdx = start_flow_idx + flow_idx_offset;
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }

    return status;
}

static int32_t CpswProxyServer_allocMacHandlerCb(uint32_t host_id,
                                                 uint64_t handle,
                                                 uint32_t core_key,
                                                 u8 *mac_address)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x\n", __func__, host_id, hCpsw, core_key);
    CpswProxyServer_validateHandle(hCpsw);

    status = CpswAppUtils_allocMac(hCpsw, core_key, host_id, mac_address);
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

static int32_t CpswProxyServer_registerMacHandlerCb(uint32_t host_id,
                                                    uint64_t handle,
                                                    uint32_t core_key,
                                                    u8 *mac_address,
                                                    uint32_t flow_idx)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    uint32_t start_flow_idx, flow_idx_offset;

    CpswProxyServer_validateHandle(hCpsw);
    CpswAppUtils_absFlowIdx2FlowIdxOffset(hCpsw, host_id, flow_idx, &start_flow_idx, &flow_idx_offset);
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, MacAddress:%x:%x:%x:%x:%x:%x, FlowIdx:%u, FlowIdxOffset:%u\n",
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
                       flow_idx,
                       flow_idx_offset);

    status = CpswAppUtils_registerDstMacRxFlow(hCpsw, core_key, host_id, start_flow_idx, flow_idx_offset, mac_address);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
                           "CpswAppUtils_registerDstMacRxFlow() failed CPSW_ALE_IOCTL_SET_POLICER: %d\n",
                           status);
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }

    return status;
}

static int32_t CpswProxyServer_unregisterMacHandlerCb(uint32_t host_id,
                                                      uint64_t handle,
                                                      uint32_t core_key,
                                                      u8 *mac_address,
                                                      uint32_t flow_idx)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    uint32_t start_flow_idx, flow_idx_offset;

    CpswProxyServer_validateHandle(hCpsw);
    CpswAppUtils_absFlowIdx2FlowIdxOffset(hCpsw, host_id, flow_idx, &start_flow_idx, &flow_idx_offset);
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, MacAddress:%x:%x:%x:%x:%x:%x, FlowIdx:%u, FlowIdOffset:%u\n",
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
                       flow_idx,
                       flow_idx_offset);

    status = CpswAppUtils_unregisterDstMacRxFlow(hCpsw, core_key, host_id, start_flow_idx, flow_idx_offset, mac_address);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
                           "Failed CpswAppUtils_unregisterDstMacRxFlow: %d\n",
                           status);
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }

    return status;
}

static int32_t CpswProxyServer_registerRxDefaultHandlerCb(uint32_t host_id,
                                                          uint64_t handle,
                                                          uint32_t core_key,
                                                          uint32_t flow_idx)
{
    int32_t status = CPSW_SOK;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    uint32_t start_flow_idx, flow_idx_offset;

    CpswProxyServer_validateHandle(hCpsw);
    CpswAppUtils_absFlowIdx2FlowIdxOffset(hCpsw, host_id, flow_idx, &start_flow_idx, &flow_idx_offset);

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, FlowId:%x, FlowIdOffset:%x\n", __func__, host_id, hCpsw, core_key, flow_idx, flow_idx_offset);

    status = CpswAppUtils_registerDefaultRxFlow(hCpsw, core_key, host_id, start_flow_idx, flow_idx_offset);
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

static int32_t CpswProxyServer_unregisterRxDefaultHandlerCb(uint32_t host_id,
                                                            uint64_t handle,
                                                            uint32_t core_key,
                                                            uint32_t flow_idx)
{
    int32_t status = CPSW_SOK;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    uint32_t start_flow_idx, flow_idx_offset;

    CpswProxyServer_validateHandle(hCpsw);
    CpswAppUtils_absFlowIdx2FlowIdxOffset(hCpsw, host_id, flow_idx, &start_flow_idx, &flow_idx_offset);

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, FlowId:%x\n", __func__, host_id, hCpsw, core_key, flow_idx);

    status = CpswAppUtils_unregisterDefaultRxFlow(hCpsw, core_key, host_id, start_flow_idx, flow_idx_offset);
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

static int32_t CpswProxyServer_freeTxHandlerCb(uint32_t host_id,
                                               uint64_t handle,
                                               uint32_t core_key,
                                               uint32_t tx_cpsw_psil_dst_id)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, TxId:%x\n", __func__, host_id, hCpsw, core_key, tx_cpsw_psil_dst_id);
    CpswProxyServer_validateHandle(hCpsw);

    status = CpswAppUtils_freeTxCh(hCpsw, core_key, host_id, tx_cpsw_psil_dst_id);

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

static int32_t CpswProxyServer_freeRxHandlerCb(uint32_t host_id,
                                                  uint64_t handle,
                                                  uint32_t core_key,
                                                  uint32_t alloc_flow_idx)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    uint32_t start_flow_idx, flow_idx_offset;

    CpswProxyServer_validateHandle(hCpsw);
    CpswAppUtils_absFlowIdx2FlowIdxOffset(hCpsw, host_id, alloc_flow_idx, &start_flow_idx, &flow_idx_offset);
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, RxId:%x RxOffset:%x\n", __func__, host_id, hCpsw, core_key, alloc_flow_idx, flow_idx_offset);

    CpswProxyServer_validateStartIdx(hCpsw, host_id, start_flow_idx);
    status = CpswAppUtils_freeRxFlow(hCpsw,
                                     core_key,
                                     host_id,
                                     flow_idx_offset);

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

static int32_t CpswProxyServer_freeMacHandlerCb(uint32_t host_id,
                                                uint64_t handle,
                                                uint32_t core_key,
                                                u8 *mac_address)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);

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
    CpswProxyServer_validateHandle(hCpsw);

    status = CpswAppUtils_freeMac(hCpsw, core_key, host_id, mac_address);

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

static int32_t CpswProxyServer_detachHandlerCb(uint32_t host_id,
                                               uint64_t handle,
                                               uint32_t core_key)
{
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    CpswMcm_CmdIf *hMcmCmdIf;
    CpswProxyServer_Obj * hProxyServer;
    int32_t status;
    Cpsw_Type cpswType;

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x\n", __func__, host_id, hCpsw, core_key);
    CpswProxyServer_validateHandle(hCpsw);


    hProxyServer = CpswProxyServer_getHandle();
    CpswAppUtils_assert((hProxyServer != NULL) && (hProxyServer->initDone == true));
    status = CpswProxyServer_getCpswType(&hProxyServer->handleTbl, hCpsw, &cpswType);
    CpswAppUtils_assert(status == CPSW_SOK);
    status = CpswProxyServer_getMcmCmdIf(&hProxyServer->handleTbl, cpswType, &hMcmCmdIf);
    CpswAppUtils_assert((status == CPSW_SOK) && (hMcmCmdIf != NULL));
    CpswAppUtils_assert(hMcmCmdIf->hMboxCmd != NULL);
    CpswAppUtils_assert(hMcmCmdIf->hMboxResponse != NULL);

    CpswMcm_coreDetach(hMcmCmdIf, host_id, core_key);
    CpswMcm_releaseHandleInfo(hMcmCmdIf);

    return RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
}

static void CpswProxyServer_printStats(Cpsw_Handle hCpsw,
                                       Cpsw_Type cpswType,
                                       uint32_t coreId)
{
    Cpsw_IoctlPrms prms;
    CpswStats_GenericMacPortInArgs inArgs;
    CpswStats_PortStats portStats;
    int32_t status = CPSW_SOK;
    uint32_t i;

    CPSW_IOCTL_SET_OUT_ARGS(&prms, &portStats);
    status =
        Cpsw_ioctl(hCpsw, coreId, CPSW_STATS_IOCTL_GET_HOSTPORT_STATS,
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

            case CPSW_5G:
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
                           "CpswProxyServer_printStats() failed to get host stats: %d\n",
                           status);
    }

    if (status == CPSW_SOK)
    {
        for (i = 0, inArgs.portNum = CPSW_MAC_PORT_FIRST; i < Cpsw_getMacPortMax(cpswType); i++, inArgs.portNum++)
        {
            CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &portStats);
            status =
                Cpsw_ioctl(hCpsw, coreId, CPSW_STATS_IOCTL_GET_MACPORT_STATS,
                           &prms);
            if (status == CPSW_SOK)
            {
                CpswAppUtils_print("\n External Port %d Statistics\n", CPSW_NORMALIZE_MACPORT(inArgs.portNum));
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

                    case CPSW_5G:
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
                                   "CpswProxyServer_printStats() failed to get MAC stats: %d\n",
                                   status);
            }
        }
    }
}

static int32_t CpswProxyServer_ioctlHandlerCb(uint32_t host_id,
                                              uint64_t handle,
                                              uint32_t core_key,
                                              u32 cmd,
                                              const u8 *inargs,
                                              u32 inargs_len,
                                              u8 *outargs,
                                              uint32_t outargs_len)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    uint64_t inArgsBuf[(RPMSG_KDRV_TP_ETHSWITCH_IOCTL_INARGS_LEN/sizeof(uint64_t)) + 1];
    uint64_t outArgsBuf[(RPMSG_KDRV_TP_ETHSWITCH_IOCTL_OUTARGS_LEN/sizeof(uint64_t)) + 1];

    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, Cmd:%x,InArgsLen:%u, OutArgsLen:%u \n", __func__, host_id, hCpsw, core_key, cmd, inargs_len, outargs_len);
    CpswProxyServer_validateHandle(hCpsw);

    prms.inArgsSize = inargs_len;
    prms.outArgsSize = outargs_len;
    CpswAppUtils_assert(inargs_len <= sizeof(inArgsBuf));
    /* To ensure structure are aligned, copy the inArgs to unit64_t aligned buffer */
    memcpy(inArgsBuf, inargs, inargs_len);
    prms.inArgs = inArgsBuf;
    /* To ensure structure are aligned, use unit64_t aligned buffer for outArgs  */
    CpswAppUtils_assert(outargs_len <= sizeof(outArgsBuf));
    prms.outArgs = outArgsBuf;
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
        /* Copy the outArgs from temporary aligned buffer back to msg buffer */
        memcpy(outargs, outArgsBuf, outargs_len);
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }

    return status;
}

static int32_t CpswProxyServer_regwrHandlerCb(uint32_t host_id,
                                              uint32_t regaddr,
                                              uint32_t regval,
                                              uint32_t *pRegval)
{
    CpswAppUtils_print("Function:%s,HostId:%u, RegAddr:%p, RegVal:%x \n", __func__, host_id, regaddr, regval);

    CSL_REG32_WR(regaddr, regval);

    *pRegval = CSL_REG32_RD(regaddr);

    return RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
}

static int32_t CpswProxyServer_regrdHandlerCb(uint32_t host_id,
                                              uint32_t regaddr,
                                              uint32_t *pRegval)
{
    CpswAppUtils_print("Function:%s,HostId:%u, RegAddr:%p \n", __func__, host_id, regaddr);

    *pRegval = CSL_REG32_RD(regaddr);

    return RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
}

static void CpswProxyServer_printLliEntry(uint32_t entryIdx,
                                          LLI_INFO *entry)
{
    char str[40];

    NtIPN2Str(entry->IPAddr, str);
    CpswAppUtils_print("%d ", entryIdx);
    CpswAppUtils_print("        %-15s  ", str);
    CpswAppUtils_print("  %02X:%02X:%02X:%02X:%02X:%02X",
                       entry->MacAddr[0], entry->MacAddr[1], entry->MacAddr[2],
                       entry->MacAddr[3], entry->MacAddr[4], entry->MacAddr[5]);
    CpswAppUtils_print("\n");
}

static void CpswProxyServer_dumpLliTable(LLI_INFO *llitable,
                                         uint32_t numEntries)
{
    LLI_INFO *entry;
    uint32_t entryIdx;

    CpswAppUtils_print("\n================LLI Table entries=========== \n");
    CpswAppUtils_print("\nNumber of Static ARP Entries: %d \n", numEntries);
    CpswAppUtils_print("\nSNo.      IP Address         MAC Address  \n");
    CpswAppUtils_print("------    -------------      --------------- \n");

    entry = (LLI_INFO *)list_get_head((NDK_LIST_NODE **)&llitable);
    /* start with 1 as when table is printed via telnet it is indexed with 1 */
    entryIdx = 1U;
    while (entry != NULL)
    {
        CpswProxyServer_printLliEntry(entryIdx, entry);
        /* Get the next LLI Entry. */
        entry = (LLI_INFO *)list_get_next((NDK_LIST_NODE *)entry);
        entryIdx++;
    }
}

static int32_t CpswProxyServer_registerIpv4MacHandlerCb(uint32_t host_id,
                                                        uint64_t handle,
                                                        uint32_t core_key,
                                                        uint8_t *mac_address,
                                                        uint8_t *ipv4_addr)
{
    uint32_t numEntries;
    int32_t status;
    uint32_t ipaddr = ((uint32_t)ipv4_addr[0] << 24U) | ((uint32_t)ipv4_addr[1] << 16U) | ((uint32_t)ipv4_addr[2] << 8U) | ((uint32_t)ipv4_addr[3] << 0U);
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    LLI_INFO *llitable = NULL;

    ipaddr = htonl(ipaddr);
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
    CpswProxyServer_validateHandle(hCpsw);

    ConCmdRoute(1, "print", NULL, NULL, NULL);

    status = LLIAddStaticEntryWithFlags(ipaddr,
                                        mac_address,
                                        (FLG_RTE_HOST|FLG_RTE_STATIC|FLG_RTE_PROXYPUB));
    if (status != 0)
    {
        status = LLIAddStaticEntryWithFlags(ipaddr,
                                            mac_address,
                                            (FLG_RTE_HOST|FLG_RTE_STATIC|FLG_RTE_PROXYPUB));
    }

    if (status != 0)
    {
        CpswAppUtils_print("Failed to add Static ARP Entry \n");
    }

    LLIGetStaticARPTable(&numEntries,
                         &llitable);

    CpswProxyServer_dumpLliTable(llitable, numEntries);
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

static int32_t CpswProxyServer_unregisterIpv4MacHandlerCb(uint32_t host_id,
                                                          uint64_t handle,
                                                          uint32_t core_key,
                                                          uint8_t *ipv4_addr)
{
    uint32_t numEntries;
    int32_t status;
    uint32_t ipaddr = ((uint32_t)ipv4_addr[0] << 24U) | ((uint32_t)ipv4_addr[1] << 16U) | ((uint32_t)ipv4_addr[2] << 8U) | ((uint32_t)ipv4_addr[3] << 0U);
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    LLI_INFO *llitable = NULL;

    ipaddr = htonl(ipaddr);
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x,IPv4Addr:%x:%x:%x:%x\n",
                       __func__,
                       host_id,
                       hCpsw,
                       core_key,
                       ipv4_addr[0],
                       ipv4_addr[1],
                       ipv4_addr[2],
                       ipv4_addr[3]);
    CpswProxyServer_validateHandle(hCpsw);

    status = LLIRemoveStaticEntry(ipaddr);
    if (status != 0)
    {
        CpswAppUtils_print("Failed to add Static ARP Entry \n");
    }

    LLIGetStaticARPTable(&numEntries,
                         &llitable);

    CpswProxyServer_dumpLliTable(llitable, numEntries);
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

static int32_t CpswProxyServer_registerIpv6MacHandlerCb(uint32_t host_id,
                                                        uint64_t handle,
                                                        uint32_t core_key,
                                                        uint8_t *mac_address,
                                                        uint8_t *ipv6_addr)
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

static int32_t CpswProxyServer_attachExtHandlerCb(uint32_t host_id,
                                                  uint8_t cpsw_type,
                                                  uint64_t *pId,
                                                  uint32_t *pCoreKey,
                                                  uint32_t *pRxMtu,
                                                  uint32_t *pTxMtu,
                                                  uint32_t txMtuArraySize,
                                                  uint32_t *pFeatures,
                                                  uint32_t *pAllocFlowIdx,
                                                  uint32_t *pTxCpswPsilDstId,
                                                  uint8_t *macAddress)
{
    int32_t status = CPSW_SOK;
    CpswMcm_HandleInfo handleInfo;
    Cpsw_AttachCoreOutArgs attachInfo;
    Cpsw_IoctlPrms prms;
    bool csumOffloadFlag;
    Cpsw_Type cpswType;
    uint32_t start_flow_idx, flow_idx_offset;
    CpswProxyServer_Obj * hProxyServer;
    CpswMcm_CmdIf *hMcmCmdIf;
    enum rpmsg_kdrv_ethswitch_cpsw_type rdevCpswType = (enum rpmsg_kdrv_ethswitch_cpsw_type) cpsw_type;

    status = CpswProxy_mapRdev2CpswType(rdevCpswType, &cpswType);
    if (CPSW_SOK == status)
    {
        CpswAppUtils_print("Function:%s,HostId:%u,CpswType:%u\n", __func__, host_id, cpswType);

        hProxyServer = CpswProxyServer_getHandle();
        CpswAppUtils_assert((hProxyServer != NULL) && (hProxyServer->initDone == true));
        status = CpswProxyServer_getMcmCmdIf(&hProxyServer->handleTbl, cpswType, &hMcmCmdIf);
        if ((status != CPSW_SOK) || (hMcmCmdIf == NULL))
        {
            CpswAppUtils_assert(hProxyServer->getMcmCmdIfCb != NULL);
            hProxyServer->getMcmCmdIfCb(cpswType, &hMcmCmdIf);
            CpswAppUtils_assert(hMcmCmdIf != NULL);
            status = CPSW_SOK;
        }
    }

    if (CPSW_SOK == status)
    {
        CpswAppUtils_assert(hMcmCmdIf->hMboxCmd != NULL);
        CpswAppUtils_assert(hMcmCmdIf->hMboxResponse != NULL);

        CpswMcm_acquireHandleInfo(hMcmCmdIf, &handleInfo);
        CpswMcm_coreAttach(hMcmCmdIf, host_id, &attachInfo);

        *pId = (uint64_t)(handleInfo.hCpsw);
        *pCoreKey = attachInfo.coreKey;
        *pRxMtu = attachInfo.rxMtu;
        CpswAppUtils_assert(txMtuArraySize ==
                            CPSW_UTILS_ARRAYSIZE(attachInfo.txMtu));
        memcpy(pTxMtu, attachInfo.txMtu, sizeof(attachInfo.txMtu));
        *pFeatures = 0;
        CPSW_IOCTL_SET_OUT_ARGS(&prms, &csumOffloadFlag);
        status = Cpsw_ioctl(handleInfo.hCpsw,
                            host_id,
                            CPSW_HOSTPORT_IS_CSUM_OFFLOAD_ENABLE,
                            &prms);

        CpswAppUtils_assert(status == CPSW_SOK);

        if (csumOffloadFlag)
        {
            *pFeatures |= RPMSG_KDRV_TP_ETHSWITCH_FEATURE_TXCSUM;
        }
    }

    if (CPSW_SOK == status)
    {
        status = CpswAppUtils_allocRxFlow(handleInfo.hCpsw,
                                          attachInfo.coreKey,
                                          host_id,
                                          &start_flow_idx,
                                          &flow_idx_offset);
        if (CPSW_SOK == status)
        {
            CpswProxyServer_validateStartIdx(handleInfo.hCpsw, host_id, start_flow_idx);
            *pAllocFlowIdx = start_flow_idx + flow_idx_offset;
        }
    }

    if (CPSW_SOK == status)
    {
        status = CpswAppUtils_allocTxCh(handleInfo.hCpsw,
                                        attachInfo.coreKey,
                                        host_id,
                                        pTxCpswPsilDstId);
    }

    if (CPSW_SOK == status)
    {
        status = CpswAppUtils_allocMac(handleInfo.hCpsw,
                                       attachInfo.coreKey,
                                       host_id,
                                       macAddress);
    }

    if (status != CPSW_SOK)
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        CpswProxyServer_addHandleEntry(&hProxyServer->handleTbl, handleInfo.hCpsw, cpswType, hMcmCmdIf);
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }

    return status;
}

static void CpswProxyServer_clientNotifyHandlerCb(uint32_t host_id,
                                                  uint64_t handle,
                                                  uint32_t core_key,
                                                  enum rpmsg_kdrv_ethswitch_client_notify_type notifyid,
                                                  uint8_t *notify_info,
                                                  uint32_t notify_info_len)
{
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    Cpsw_Type cpswType;
    int32_t status;
    CpswProxyServer_Obj *hProxyServer;
#define STRINGIFY(x) # x
#define XSTRINGIFY(x) STRINGIFY(x)
    char *notify_type_str[] = {XSTRINGIFY(RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_DUMPSTATS),
                               XSTRINGIFY(RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_CUSTOM)};

    CpswAppUtils_assert(notifyid < CPSW_UTILS_ARRAYSIZE(notify_type_str));
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x,NotifyId:%s,NotifyLen\n", __func__, host_id, core_key, hCpsw, notify_type_str[notifyid], notify_info_len);
    hProxyServer = CpswProxyServer_getHandle();
    CpswAppUtils_assert((hProxyServer != NULL) && (hProxyServer->initDone == true));
    status = CpswProxyServer_getCpswType(&hProxyServer->handleTbl, hCpsw, &cpswType);
    CpswAppUtils_assert(CPSW_SOK == status);

    switch (notifyid)
    {
        case RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_DUMPSTATS:
        {
            int32_t status;

            CpswProxyServer_validateHandle(hCpsw);

            CPSW_IOCTL_SET_NO_ARGS(&prms);
            status = Cpsw_ioctl(hCpsw, host_id, CPSW_ALE_IOCTL_DUMP_TABLE,
                                &prms);
            CpswAppUtils_assert(status == CPSW_SOK);

            CPSW_IOCTL_SET_NO_ARGS(&prms);
            status = Cpsw_ioctl(hCpsw, host_id, CPSW_ALE_IOCTL_DUMP_POLICER_ENTRIES,
                                &prms);

            CpswAppUtils_assert(status == CPSW_SOK);

            CpswProxyServer_printStats(hCpsw, cpswType, host_id);
            break;
        }
        case RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_CUSTOM:
        {
            CpswProxyServer_validateHandle(hCpsw);

            if (hProxyServer->notifyCb != NULL)
            {
                hProxyServer->notifyCb(host_id, hCpsw, cpswType, core_key, notifyid, notify_info, notify_info_len);
            }
            break;
        }
        default:
            /* unhandled notify.do nothing */
            break;
    }
}

static void  CpswProxyServer_initDeviceDataHandlerCb(uint32_t host_id,
                                                     struct rpmsg_kdrv_ethswitch_device_data *eth_dev_data)
{
    CpswProxyServer_Obj *hProxyServer;

    hProxyServer = CpswProxyServer_getHandle();
    CpswAppUtils_assert((hProxyServer != NULL) && (hProxyServer->initDone == true));
    CpswAppUtils_assert(hProxyServer->initEthfwDeviceDataCb != NULL);
    hProxyServer->initEthfwDeviceDataCb(host_id, eth_dev_data);
}

static int32_t CpswProxyServer_registerEthertypeHandlerCb(uint32_t host_id,
                                                          uint64_t handle,
                                                          uint32_t core_key,
                                                          u16 ether_type,
                                                          uint32_t flow_idx)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    uint32_t start_flow_idx, flow_idx_offset;
    Cpsw_IoctlPrms prms;
    CpswAle_SetPolicerEntryOutArgs setPolicerOutArgs;
    CpswAle_SetPolicerEntryInArgs setPolicerInArgs;

    CpswProxyServer_validateHandle(hCpsw);
    CpswAppUtils_absFlowIdx2FlowIdxOffset(hCpsw, host_id, flow_idx, &start_flow_idx, &flow_idx_offset);
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, Ethertype:%x, FlowIdx:%u, FlowIdxOffset:%u\n",
                       __func__,
                       host_id,
                       hCpsw,
                       core_key,
                       ether_type,
                       flow_idx,
                       flow_idx_offset);

    memset(&setPolicerInArgs, 0, sizeof(setPolicerInArgs));
    setPolicerInArgs.policerMatch.policerMatchEnableMask = CPSW_ALE_POLICER_MATCH_ETHERTYPE;
    setPolicerInArgs.policerMatch.etherType.etherType = ether_type;
    setPolicerInArgs.threadIdEnable                   = TRUE;
    setPolicerInArgs.threadId                         = flow_idx_offset;
    setPolicerInArgs.peakRateInBitsPerSec             = 0;
    setPolicerInArgs.commitRateInBitsPerSec           = 0;

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setPolicerInArgs, &setPolicerOutArgs);

    status = Cpsw_ioctl(hCpsw,host_id, CPSW_ALE_IOCTL_SET_POLICER, &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
                           "Cpsw_ioctl() failed CPSW_ALE_IOCTL_SET_POLICER: %d\n",
                           status);
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }

    return status;
}

static int32_t CpswProxyServer_unregisterEthertypeHandlerCb(uint32_t host_id,
                                                            uint64_t handle,
                                                            uint32_t core_key,
                                                            u16 ether_type,
                                                            uint32_t flow_idx)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    uint32_t start_flow_idx, flow_idx_offset;
    Cpsw_IoctlPrms prms;
    CpswAle_DelPolicerEntryInArgs delPolicerInArgs;

    CpswProxyServer_validateHandle(hCpsw);
    CpswAppUtils_absFlowIdx2FlowIdxOffset(hCpsw, host_id, flow_idx, &start_flow_idx, &flow_idx_offset);
    CpswAppUtils_print("Function:%s,HostId:%u,Handle:%p,CoreKey:%x, Ethertype:%x, FlowIdx:%u, FlowIdOffset:%u\n",
                       __func__,
                       host_id,
                       hCpsw,
                       core_key,
                       ether_type,
                       flow_idx,
                       flow_idx_offset);

    memset(&delPolicerInArgs, 0, sizeof(delPolicerInArgs));
    delPolicerInArgs.policerMatch.policerMatchEnableMask = CPSW_ALE_POLICER_MATCH_ETHERTYPE;
    delPolicerInArgs.policerMatch.etherType.etherType = ether_type;
    delPolicerInArgs.delAleEntryMask = CPSW_ALE_POLICER_TABLEENTRY_DELETE_ETHERTYPE;

    CPSW_IOCTL_SET_IN_ARGS(&prms, &delPolicerInArgs);

    status = Cpsw_ioctl(hCpsw,host_id, CPSW_ALE_IOCTL_DEL_POLICER, &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
                           "Failed Cpsw_ioctl CPSW_ALE_IOCTL_DEL_POLICER : %d\n",
                           status);
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
    }

    return status;
}

static int32_t CpswProxyServer_registerRemoteTimerHandlerCb(uint32_t host_id,
                                                            uint8_t *name,
                                                            uint64_t handle,
                                                            uint32_t core_key,
                                                            uint8_t timer_id,
                                                            uint8_t hwPushNum)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    CpswCpts_RegisterHwPushCbInArgs hwPushCbInArgs;
    Cpsw_Type cpswType;
    CpswProxyServer_Obj *hProxyServer;

    hProxyServer = CpswProxyServer_getHandle();
    CpswAppUtils_assert((hProxyServer != NULL) && (hProxyServer->initDone == true));
    status = CpswProxyServer_getCpswType(&hProxyServer->handleTbl, hCpsw, &cpswType);

    /* Register hardware push callback */
    hwPushCbInArgs.hwPushNum = (CpswCpts_HwPush)hwPushNum;
    hwPushCbInArgs.pHwPushNotifyCb = CpswProxyServer_hwPushNotifyFxn;
    hwPushCbInArgs.pHwPushNotifyCbArg = (void *)hProxyServer;
    CPSW_IOCTL_SET_IN_ARGS(&prms, &hwPushCbInArgs);
    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_CPTS_IOCTL_REGISTER_HW_PUSH_CALLBACK,
                        &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("Failed Cpsw_ioctl CPSW_CPTS_IOCTL_REGISTER_HW_PUSH_CALLBACK : %d\n",
                           status);
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_EFAIL;
    }
    else
    {
        status = RPMSG_KDRV_TP_ETHSWITCH_CMDSTATUS_OK;
        /* Configure timesync router */
        status = CpswAppUtils_setTimeSyncRouter(cpswType,
                                                timer_id,
                                                CPSW_CPTS_HW_PUSH_NORMALIZE((CpswCpts_HwPush)hwPushNum) +
                                                CPSWPROXY_CPSW9G_HWPUSH_BASE);
    }

    return status;
}

static int32_t CpswProxyServer_unregisterRemoteTimerHandlerCb(uint32_t host_id,
                                                              uint8_t *name,
                                                              uint64_t handle,
                                                              uint32_t core_key,
                                                              uint8_t hwPushNum)
{
    int32_t status;
    Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)handle);
    Cpsw_IoctlPrms prms;
    Cpsw_Type cpswType;
    CpswProxyServer_Obj *hProxyServer;

    hProxyServer = CpswProxyServer_getHandle();
    CpswAppUtils_assert((hProxyServer != NULL) && (hProxyServer->initDone == true));
    status = CpswProxyServer_getCpswType(&hProxyServer->handleTbl, hCpsw, &cpswType);

    /* Unregister hardware push callback */
    hwPushNum = (CpswCpts_HwPush)hwPushNum;
    CPSW_IOCTL_SET_IN_ARGS(&prms, &hwPushNum);
    status = Cpsw_ioctl(hCpsw,
                        host_id,
                        CPSW_CPTS_IOCTL_UNREGISTER_HW_PUSH_CALLBACK,
                        &prms);

    /* Clear timesync router configuration for hardware push,
     * Note: This assumes input signal is stopped */
    status = CpswAppUtils_setTimeSyncRouter(cpswType,
                                            0U,
                                            CPSW_CPTS_HW_PUSH_NORMALIZE((CpswCpts_HwPush)hwPushNum) +
                                            CPSWPROXY_CPSW9G_HWPUSH_BASE);

    return status;
}

static rdevEthSwitchServerCbFxn_t CpswProxyRdevEthSwitchServerCbFxnTbl =
{
    .attach_handler                 = CpswProxyServer_attachHandlerCb,
    .attach_ext_handler             = CpswProxyServer_attachExtHandlerCb,
    .alloc_tx_handler               = CpswProxyServer_allocTxHandlerCb,
    .alloc_rx_handler               = CpswProxyServer_allocRxHandlerCb,
    .alloc_mac_handler              = CpswProxyServer_allocMacHandlerCb,
    .register_mac_handler           = CpswProxyServer_registerMacHandlerCb,
    .unregister_mac_handler         = CpswProxyServer_unregisterMacHandlerCb,
    .register_rx_default_handler    = CpswProxyServer_registerRxDefaultHandlerCb,
    .unregister_rx_default_handler  = CpswProxyServer_unregisterRxDefaultHandlerCb,
    .free_tx_handler                = CpswProxyServer_freeTxHandlerCb,
    .free_rx_handler                = CpswProxyServer_freeRxHandlerCb,
    .free_mac_handler               = CpswProxyServer_freeMacHandlerCb,
    .detach_handler                 = CpswProxyServer_detachHandlerCb,
    .ioctl_handler                  = CpswProxyServer_ioctlHandlerCb,
    .regwr_handler                  = CpswProxyServer_regwrHandlerCb,
    .regrd_handler                  = CpswProxyServer_regrdHandlerCb,
    .ipv4_register_mac_handler      = CpswProxyServer_registerIpv4MacHandlerCb,
    .ipv6_register_mac_handler      = CpswProxyServer_registerIpv6MacHandlerCb,
    .ipv4_unregister_mac_handler    = CpswProxyServer_unregisterIpv4MacHandlerCb,
    .client_notify_handler          = CpswProxyServer_clientNotifyHandlerCb,
    .init_device_data_handler       = CpswProxyServer_initDeviceDataHandlerCb,
    .register_ethertype_handler     = CpswProxyServer_registerEthertypeHandlerCb,
    .unregister_ethertype_handler   = CpswProxyServer_unregisterEthertypeHandlerCb,
    .register_remotetimer_handler   = CpswProxyServer_registerRemoteTimerHandlerCb,
    .unregister_remotetimer_handler = CpswProxyServer_unregisterRemoteTimerHandlerCb,
};


int32_t CpswProxyServer_init(CpswProxyServer_Config_t *cfg)
{
    SemaphoreP_Params sem_params;
    CpswProxyServer_Obj * hProxyServer;
    app_remote_device_init_prm_t remote_dev_init_prm;
    rdevEthSwitchServerInitPrm_t remote_ethswitch_init_prm;
    rdevEthSwitchServerInstPrm_t *inst;
    int32_t i;
    int32_t status;

    hProxyServer = CpswProxyServer_getHandle();

    CpswAppUtils_assert((hProxyServer != NULL) && (hProxyServer->initDone == false));
    SemaphoreP_Params_init(&sem_params);
    sem_params.mode = SemaphoreP_Mode_BINARY;
    hProxyServer->rdevStartSem = SemaphoreP_create(0, &sem_params);
    CpswAppUtils_assert(hProxyServer->rdevStartSem != NULL);


    hProxyServer->getMcmCmdIfCb = cfg->getMcmCmdIfCb;
    hProxyServer->initEthfwDeviceDataCb = cfg->initEthfwDeviceDataCb;
    hProxyServer->notifyCb = cfg->notifyCb;
    hProxyServer->handleTbl.numEntries = 0;
    appRemoteDeviceInitParamsInit(&remote_dev_init_prm);

    remote_dev_init_prm.rpmsg_buf_size = CPSWPROXY_RDEV_MSGSIZE;
    remote_dev_init_prm.remote_device_endpt = cfg->rpmsgEndPointId;
    remote_dev_init_prm.wait_sem = hProxyServer->rdevStartSem;

    status = appRemoteDeviceInit(&remote_dev_init_prm);

    CpswAppUtils_assert(status == 0);

    rdevEthSwitchServerInitPrmSetDefault(&remote_ethswitch_init_prm);

    remote_ethswitch_init_prm.rpmsg_buf_size = CPSWPROXY_RDEV_MSGSIZE;
    remote_ethswitch_init_prm.num_instances = cfg->numRemoteCores;
    remote_ethswitch_init_prm.cb = CpswProxyRdevEthSwitchServerCbFxnTbl;

    CpswAppUtils_assert(cfg->numRemoteCores <= CPSW_UTILS_ARRAYSIZE(remote_ethswitch_init_prm.inst_prm));
    CpswAppUtils_assert(cfg->numRemoteCores <= CPSW_UTILS_ARRAYSIZE(cfg->remoteCoreCfg));

    for ( i = 0 ; i < cfg->numRemoteCores; i++)
    {
        inst = &remote_ethswitch_init_prm.inst_prm[i];
        inst->host_id = cfg->remoteCoreCfg[i].remoteCoreId;
        strncpy((char *)&inst->name[0], cfg->remoteCoreCfg[i].serverName, ETHREMOTECFG_SERVER_MAX_NAME_LEN);
    }
    status = rdevEthSwitchServerInit(&remote_ethswitch_init_prm);
    CpswAppUtils_assert(status == 0);

    status = CpswProxyServer_initAutosarEthDeviceEp(hProxyServer, cfg);
    CpswAppUtils_assert(status == 0);

    status = CpswProxyServer_initNotifyServiceEp(hProxyServer, cfg);
    CpswAppUtils_assert(status == 0);

    hProxyServer->initDone = true;
    CpswAppUtils_print("Remote demo device (core : mcu2_0) .....\r\n");
    return CPSW_SOK;
}

int32_t  CpswProxyServer_start(void)
{
    CpswProxyServer_Obj * hProxyServer;

    hProxyServer = CpswProxyServer_getHandle();
    CpswAppUtils_assert((hProxyServer != NULL) && (hProxyServer->initDone == true));

    SemaphoreP_post(hProxyServer->rdevStartSem);
    return CPSW_SOK;
}

static int32_t CpswProxyServer_initAutosarEthDeviceEp(CpswProxyServer_Obj * hProxyServer, CpswProxyServer_Config_t * cfg)
{
    Error_Block     eb;
    Task_Params     taskParams;
    int32_t retVal = CPSW_SOK;
    RPMessage_Params comChParam;
    uint32_t  localEp;

    hProxyServer->ethDrvObj.dstProc = cfg->autosarEthDriverRemoteCoreId;
    RPMessageParams_init(&comChParam);
    comChParam.numBufs = CPSWPROXY_AUTOSAR_ETHDRIVER_NUM_RPMSG_BUFS;
    comChParam.buf = g_CpswProxyServerAutosarRpmsgBuf;
    comChParam.bufSize = sizeof(g_CpswProxyServerAutosarRpmsgBuf);
    comChParam.requestedEndpt = cfg->autosarEthDeviceEndPointId;
    hProxyServer->ethDrvObj.hAutosarEthRpMsgEp = RPMessage_create(&comChParam, &localEp);

    if (NULL == hProxyServer->ethDrvObj.hAutosarEthRpMsgEp)
    {
        CpswAppUtils_print("Could not create communication channel \n");
        retVal = CPSW_EFAIL;
    }

    if (CPSW_SOK == retVal)
    {
        if (localEp != cfg->autosarEthDeviceEndPointId)
        {
            CpswAppUtils_print("Could not create required End Point");
        }
        else
        {
            hProxyServer->ethDrvObj.localEp = localEp;
        }
    }

    if (CPSW_SOK == retVal)
    {
        Error_init(&eb);

        /* Initialize the task params */
        Task_Params_init(&taskParams);
        taskParams.instance->name = CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_NAME;
        taskParams.priority     = CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_PRIORITY;
        taskParams.arg0         = (UArg) hProxyServer;

        hProxyServer->ethDrvObj.hAutosarEthTsk = Task_create(CpswProxyServer_autosarEthDriverTaskFxn, &taskParams, &eb);
        if(Error_check(&eb))
        {
            retVal = CPSW_EFAIL;
            CpswAppUtils_print("Could not create a Task \n");
        }
    }

    return retVal;
}

static int32_t CpswProxyServer_initNotifyServiceEp(CpswProxyServer_Obj * hProxyServer, CpswProxyServer_Config_t * cfg)
{
    Error_Block     eb;
    Task_Params     taskParams;
    int32_t retVal = CPSW_SOK;
    RPMessage_Params comChParam;
    uint32_t  localEp;
    EventP_Params eventParams;
    uint8_t i = 0;

    hProxyServer->notifyServiceObj.notifyServiceCpswType = cfg->notifyServiceCpswType;
    hProxyServer->notifyServiceObj.dstProc = cfg->notifyServiceRemoteCoreId;
    RPMessageParams_init(&comChParam);
    comChParam.numBufs = CPSW_REMOTE_NOTIFY_SERVICE_NUM_RPMSG_BUFS;
    comChParam.buf = g_CpswProxyServerNotifyServiceRpmsgBuf;
    comChParam.bufSize = sizeof(g_CpswProxyServerNotifyServiceRpmsgBuf);
    comChParam.requestedEndpt = CPSW_REMOTE_NOTIFY_SERVICE_ENDPT_ID;
    hProxyServer->notifyServiceObj.hNotifyServicRpMsgEp = RPMessage_create(&comChParam, &localEp);

    if (NULL == hProxyServer->notifyServiceObj.hNotifyServicRpMsgEp)
    {
        CpswAppUtils_print("Could not create communication channel\n");
        retVal = CPSW_EFAIL;
    }

    if (CPSW_SOK == retVal)
    {
        if (localEp != CPSW_REMOTE_NOTIFY_SERVICE_ENDPT_ID)
        {
            CpswAppUtils_print("Could not create required End Point");
        }
        else
        {
            hProxyServer->notifyServiceObj.localEp = localEp;
        }
    }

    /* Announce service */
    if (CPSW_SOK == retVal)
    {
        retVal = RPMessage_announce(RPMESSAGE_ALL,
                                    CPSW_REMOTE_NOTIFY_SERVICE_ENDPT_ID,
                                    CPSW_REMOTE_NOTIFY_SERVICE);
    }

    /* Create Event to notify task */
    if (CPSW_SOK == retVal)
    {
        EventP_Params_init(&eventParams);

        for (i = 0U; i < CPSW_CPTS_HW_PUSH_COUNT_MAX; i++)
        {
            hProxyServer->notifyServiceObj.hwPushNotifyEventId[i] = (1U << i);
        }

        hProxyServer->notifyServiceObj.hHwPushNotifyServiceEvent = EventP_create(&eventParams);

        if (hProxyServer->notifyServiceObj.hHwPushNotifyServiceEvent == NULL)
        {
            retVal = CPSW_EFAIL;
            CpswAppUtils_print("Could not create an Event \n");
        }
    }

    if (CPSW_SOK == retVal)
    {
        Error_init(&eb);

        /* Initialize the task params */
        Task_Params_init(&taskParams);
        taskParams.instance->name = CPSW_REMOTE_NOTIFY_SERVICE_TASK_NAME;
        taskParams.priority     = CPSW_REMOTE_NOTIFY_SERVICE_TASK_PRIORITY;
        taskParams.arg0         = (UArg) hProxyServer;

        hProxyServer->notifyServiceObj.hNotifyServiceTsk = Task_create(CpswProxyServer_notifyServiceTaskFxn, &taskParams, &eb);
        if(Error_check(&eb))
        {
            retVal = CPSW_EFAIL;
            CpswAppUtils_print("Could not create a Task \n");
        }
    }

    return retVal;
}

static void  CpswProxyServer_initDeviceData(const struct rpmsg_kdrv_ethswitch_device_data *src, Eth_RpcDeviceData *dst)
{
    dst->header.messageId = ETH_RPC_CMD_TYPE_FWINFO_RES;
    dst->header.messageLen = sizeof(*dst);

    dst->uartId = src->uart_id;
    dst->uartConnected = src->uart_connected;
    dst->permissionFlags = src->permission_flags;
    CPSW_UTILS_ARRAY_COPY(dst->fwVer.commitHash ,src->fw_ver.commit_hash);
    CPSW_UTILS_ARRAY_COPY(dst->fwVer.date , src->fw_ver.date);
    dst->fwVer.major = src->fw_ver.major;
    dst->fwVer.minor = src->fw_ver.minor;
    CPSW_UTILS_ARRAY_COPY(dst->fwVer.month , src->fw_ver.month);
    dst->fwVer.rev = src->fw_ver.rev;
    memcpy(dst->fwVer.year , src->fw_ver.year, sizeof(dst->fwVer.year));
}



static Void CpswProxyServer_autosarEthDriverTaskFxn(UArg arg0, UArg arg1)
{
    int32_t rtnVal = IPC_SOK;
    uint32_t remoteProcId, remoteEndPt;
    CpswProxyServer_Obj * hProxyServer = (CpswProxyServer_Obj *)arg0;
    uint32_t remoteProc, remoteEp;
    uint16_t len;
    uint64_t msgBuffer[(CPSWPROXY_AUTOSAR_ETHDRIVER_MSG_SIZE / sizeof(uint64_t))];


    /* Wait for Remote EP to active */
    if (IPC_SOK != RPMessage_getRemoteEndPt(hProxyServer->ethDrvObj.dstProc,
                                            ETH_RPC_REMOTE_SERVICE,
                                            &remoteProcId,
                                            &remoteEndPt,
                                            BIOS_WAIT_FOREVER))
    {
        CpswAppUtils_print("CpswProxyServer: Remote AUTOSAR Ethernet Device locate failed");
    }
    else
    {
        struct rpmsg_kdrv_ethswitch_device_data eth_dev_data;
        Eth_RpcDeviceData deviceData;
        bool exitTask = false;

        CpswAppUtils_assert(hProxyServer->initEthfwDeviceDataCb != NULL);
        hProxyServer->initEthfwDeviceDataCb(remoteProcId, &eth_dev_data);

        CpswProxyServer_initDeviceData(&eth_dev_data, &deviceData);

        /* Send the EthFw Device Data to AUTOSAR EthDriver on location of
         * end point
         */
        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                remoteProcId,
                                remoteEndPt,
                                hProxyServer->ethDrvObj.localEp,
                                (Ptr)&deviceData,
                                sizeof(deviceData));
        CpswAppUtils_assert(IPC_SOK == rtnVal);

        while (!exitTask)
        {
            rtnVal = RPMessage_recv(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                    (Ptr)msgBuffer,
                                    &len,
                                    &remoteEp,
                                    &remoteProc,
                                    IPC_RPMESSAGE_TIMEOUT_FOREVER);
            if (IPC_SOK == rtnVal)
            {
                Eth_RpcMessageHeader *header;
                int32_t status;

                CpswAppUtils_assert(len <= sizeof(msgBuffer));
                CpswAppUtils_assert(remoteEp == remoteEndPt);
                CpswAppUtils_assert(remoteProcId == remoteProc);

                header = (Eth_RpcMessageHeader *)msgBuffer;

                switch(header->messageId)
                {
                    case ETH_RPC_CMD_TYPE_ATTACH_EXT_REQ:
                    {
                        Eth_RpcAttachExtendedRequest  *attachReq = (Eth_RpcAttachExtendedRequest  *)msgBuffer;
                        Eth_RpcAttachExtendedResponse attachRes;
                        enum rpmsg_kdrv_ethswitch_cpsw_type rdevCpswType;
                        uint32_t allocFlowIdx;

                        CpswAppUtils_assert((attachReq->header.messageId == ETH_RPC_CMD_TYPE_ATTACH_EXT_REQ)
                                            &&
                                            (attachReq->header.messageLen == sizeof(*attachReq)));
                        status = CpswProxy_mapEthRpc2RdevCpswType((Eth_RpcCpswType)attachReq->cpswType, &rdevCpswType);
                        if (CPSW_SOK == status)
                        {
                            status =  CpswProxyServer_attachExtHandlerCb(remoteProc,
                                                                         rdevCpswType,
                                                                         &attachRes.id,
                                                                         &attachRes.coreKey,
                                                                         &attachRes.rxMtu,
                                                                         attachRes.txMtu,
                                                                         CPSW_UTILS_ARRAYSIZE(attachRes.txMtu),
                                                                         &attachRes.features,
                                                                         &allocFlowIdx,
                                                                         &attachRes.txCpswPsilDstId,
                                                                         attachRes.macAddress);

                            if (CPSW_SOK == status)
                            {
                                Cpsw_Handle hCpsw = (Cpsw_Handle)((uintptr_t)attachRes.id);

                                attachRes.allocFlowIdxBase = CpswAppUtils_getStartFlowIdx(hCpsw, remoteProc);
                                CpswAppUtils_assert(allocFlowIdx >= attachRes.allocFlowIdxBase);
                                attachRes.allocFlowIdxOffset =  allocFlowIdx - attachRes.allocFlowIdxBase;
                            }
                        }
                        if (CPSW_SOK == status)
                        {
                            attachRes.info.status =  ETH_RPC_CMDSTATUS_OK;
                        }
                        else
                        {
                            attachRes.info.status =  ETH_RPC_CMDSTATUS_EFAIL;
                        }
                        attachRes.header.messageId = ETH_RPC_CMD_TYPE_ATTACH_EXT_RES;
                        attachRes.header.messageLen = sizeof(attachRes);
                        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                                remoteProcId,
                                                remoteEndPt,
                                                hProxyServer->ethDrvObj.localEp,
                                                &attachRes,
                                                sizeof(attachRes));
                        CpswAppUtils_assert(IPC_SOK == rtnVal);
                        break;
                    }
                    case ETH_RPC_CMD_TYPE_DETACH_REQ:
                    {
                        Eth_RpcDetachRequest *detachReq = (Eth_RpcDetachRequest *)msgBuffer;
                        Eth_RpcDetachResponse detachRes;

                        CpswAppUtils_assert((detachReq->header.messageId == ETH_RPC_CMD_TYPE_DETACH_REQ)
                                            &&
                                            (detachReq->header.messageLen == sizeof(*detachReq)));

                        status =  CpswProxyServer_detachHandlerCb(remoteProcId,
                                                                  detachReq->info.id,
                                                                  detachReq->info.coreKey);

                        if (CPSW_SOK == status)
                        {
                            detachRes.info.status =  ETH_RPC_CMDSTATUS_OK;
                        }
                        else
                        {
                            detachRes.info.status =  ETH_RPC_CMDSTATUS_EFAIL;
                        }
                        detachRes.header.messageId = ETH_RPC_CMD_TYPE_DETACH_RES;
                        detachRes.header.messageLen = sizeof(detachRes);
                        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                                remoteProcId,
                                                remoteEndPt,
                                                hProxyServer->ethDrvObj.localEp,
                                                &detachRes,
                                                sizeof(detachRes));
                        CpswAppUtils_assert(IPC_SOK == rtnVal);

                        break;
                    }

                    case ETH_RPC_CMD_TYPE_REGISTER_DEFAULTFLOW_REQ:
                    {
                        Eth_RpcRegisterRxDefaultRequest *registerDefaultReq = (Eth_RpcRegisterRxDefaultRequest *)msgBuffer;
                        Eth_RpcRegisterRxDefaultResponse registerDefaultRes;

                        CpswAppUtils_assert((registerDefaultReq->header.messageId == ETH_RPC_CMD_TYPE_REGISTER_DEFAULTFLOW_REQ)
                                            &&
                                            (registerDefaultReq->header.messageLen == sizeof(*registerDefaultReq)));

                        status =  CpswProxyServer_registerRxDefaultHandlerCb(remoteProcId,
                                                                  registerDefaultReq->info.id,
                                                                  registerDefaultReq->info.coreKey,
                                                                  registerDefaultReq->defaultFlowIdx);

                        if (CPSW_SOK == status)
                        {
                            registerDefaultRes.info.status =  ETH_RPC_CMDSTATUS_OK;
                        }
                        else
                        {
                            registerDefaultRes.info.status =  ETH_RPC_CMDSTATUS_EFAIL;
                        }

                        registerDefaultRes.header.messageId = ETH_RPC_CMD_TYPE_REGISTER_DEFAULTFLOW_RES;
                        registerDefaultRes.header.messageLen = sizeof(registerDefaultRes);

                        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                                remoteProcId,
                                                remoteEndPt,
                                                hProxyServer->ethDrvObj.localEp,
                                                &registerDefaultRes,
                                                sizeof(registerDefaultRes));
                        CpswAppUtils_assert(IPC_SOK == rtnVal);

                        break;
                    }
                    case ETH_RPC_CMD_TYPE_REGISTER_MAC_REQ:
                    {
                        Eth_RpcRegisterMacRequest * registerMacReq = (Eth_RpcRegisterMacRequest * )msgBuffer;
                        Eth_RpcRegisterMacResponse registerMacRes;

                        CpswAppUtils_assert((registerMacReq->header.messageId == ETH_RPC_CMD_TYPE_REGISTER_MAC_REQ)
                                            &&
                                            (registerMacReq->header.messageLen == sizeof(*registerMacReq)));

                        status =  CpswProxyServer_registerMacHandlerCb(remoteProcId,
                                                                  registerMacReq->info.id,
                                                                  registerMacReq->info.coreKey,
                                                                  registerMacReq->macAddress,
                                                                  registerMacReq->flowIdx);

                        if (CPSW_SOK == status)
                        {
                            registerMacRes.info.status =  ETH_RPC_CMDSTATUS_OK;
                        }
                        else
                        {
                            registerMacRes.info.status =  ETH_RPC_CMDSTATUS_EFAIL;
                        }

                        registerMacRes.header.messageId = ETH_RPC_CMD_TYPE_REGISTER_MAC_RES;
                        registerMacRes.header.messageLen = sizeof(registerMacRes);

                        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                                remoteProcId,
                                                remoteEndPt,
                                                hProxyServer->ethDrvObj.localEp,
                                                &registerMacRes,
                                                sizeof(registerMacRes));
                        CpswAppUtils_assert(IPC_SOK == rtnVal);

                        break;
                    }
                    case ETH_RPC_CMD_TYPE_UNREGISTER_MAC_REQ:
                    {
                        Eth_RpcUnregisterMacRequest *unregisterMacReq = (Eth_RpcUnregisterMacRequest *)msgBuffer;
                        Eth_RpcUnregisterMacResponse unregisterMacRes;

                        CpswAppUtils_assert((unregisterMacReq->header.messageId == ETH_RPC_CMD_TYPE_UNREGISTER_MAC_REQ)
                                            &&
                                            (unregisterMacReq->header.messageLen == sizeof(*unregisterMacReq)));

                        status =  CpswProxyServer_unregisterMacHandlerCb(remoteProcId,
                                                                  unregisterMacReq->info.id,
                                                                  unregisterMacReq->info.coreKey,
                                                                  unregisterMacReq->macAddress,
                                                                  unregisterMacReq->flowIdx);

                        if (CPSW_SOK == status)
                        {
                            unregisterMacRes.info.status =  ETH_RPC_CMDSTATUS_OK;
                        }
                        else
                        {
                            unregisterMacRes.info.status =  ETH_RPC_CMDSTATUS_EFAIL;
                        }

                        unregisterMacRes.header.messageId = ETH_RPC_CMD_TYPE_UNREGISTER_MAC_RES;
                        unregisterMacRes.header.messageLen = sizeof(unregisterMacRes);

                        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                                remoteProcId,
                                                remoteEndPt,
                                                hProxyServer->ethDrvObj.localEp,
                                                &unregisterMacRes,
                                                sizeof(unregisterMacRes));
                        CpswAppUtils_assert(IPC_SOK == rtnVal);

                        break;
                    }
                    case ETH_RPC_CMD_TYPE_UNREGISTER_DEFAULTFLOW_REQ:
                    {
                        Eth_RpcUnregisterRxDefaultRequest *unregisterDefaultReq = (Eth_RpcUnregisterRxDefaultRequest *)msgBuffer;
                        Eth_RpcUnregisterRxDefaultResponse unregisterDefaultRes;

                        CpswAppUtils_assert((unregisterDefaultReq->header.messageId == ETH_RPC_CMD_TYPE_UNREGISTER_DEFAULTFLOW_REQ)
                                            &&
                                            (unregisterDefaultReq->header.messageLen == sizeof(*unregisterDefaultReq)));

                        status =  CpswProxyServer_unregisterRxDefaultHandlerCb(remoteProcId,
                                                                  unregisterDefaultReq->info.id,
                                                                  unregisterDefaultReq->info.coreKey,
                                                                  unregisterDefaultReq->defaultFlowIdx);

                        if (CPSW_SOK == status)
                        {
                            unregisterDefaultRes.info.status =  ETH_RPC_CMDSTATUS_OK;
                        }
                        else
                        {
                            unregisterDefaultRes.info.status =  ETH_RPC_CMDSTATUS_EFAIL;
                        }

                        unregisterDefaultRes.header.messageId = ETH_RPC_CMD_TYPE_UNREGISTER_DEFAULTFLOW_RES;
                        unregisterDefaultRes.header.messageLen = sizeof(unregisterDefaultRes);

                        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                                remoteProcId,
                                                remoteEndPt,
                                                hProxyServer->ethDrvObj.localEp,
                                                &unregisterDefaultRes,
                                                sizeof(unregisterDefaultRes));
                        CpswAppUtils_assert(IPC_SOK == rtnVal);

                        break;
                    }
                    case ETH_RPC_CMD_TYPE_IOCTL_REQ:
                    {
                        Eth_RpcIoctlRequest *ioctlReq = (Eth_RpcIoctlRequest *)msgBuffer;
                        Eth_RpcIoctlResponse ioctlRes;

                        CpswAppUtils_assert((ioctlReq->header.messageId == ETH_RPC_CMD_TYPE_IOCTL_REQ)
                                             &&
                                             (ioctlReq->header.messageLen == sizeof(*ioctlReq)));

                        status =  CpswProxyServer_ioctlHandlerCb(remoteProcId,
                                                                 ioctlReq->info.id,
                                                                 ioctlReq->info.coreKey,
                                                                 ioctlReq->cmd,
                                                                 (const u8 *)ioctlReq->inargs,
                                                                 ioctlReq->inargsLen,
                                                                 (u8 *)ioctlRes.outargs,
                                                                 ioctlReq->outargsLen);

                        if (CPSW_SOK == status)
                        {
                            ioctlRes.info.status =  ETH_RPC_CMDSTATUS_OK;
                        }
                        else
                        {
                            ioctlRes.info.status =  ETH_RPC_CMDSTATUS_EFAIL;
                        }

                        /* Set IOCTL cmd and outArgs len in response so that
                         * it can be processed on client side without keeping
                         * track of IOCTL response belongs to which IOCTL request
                         */
                        ioctlRes.cmd = ioctlReq->cmd;
                        ioctlRes.outargsLen = ioctlReq->outargsLen;
                        ioctlRes.header.messageId = ETH_RPC_CMD_TYPE_IOCTL_RES;
                        ioctlRes.header.messageLen = sizeof(ioctlRes);

                        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                                remoteProcId,
                                                remoteEndPt,
                                                hProxyServer->ethDrvObj.localEp,
                                                &ioctlRes,
                                                sizeof(ioctlRes));
                        CpswAppUtils_assert(IPC_SOK == rtnVal);

                        break;
                    }
                    case ETH_RPC_CMD_TYPE_IPV4_MAC_REGISTER_REQ:
                    {
                         Eth_RpcIpv4RegisterMacRequest *registerIpv4Req = (Eth_RpcIpv4RegisterMacRequest *)msgBuffer;
                         Eth_RpcIpv4RegisterMacResponse registerIpv4Res;

                        CpswAppUtils_assert((registerIpv4Req->header.messageId == ETH_RPC_CMD_TYPE_IPV4_MAC_REGISTER_REQ)
                                            &&
                                            (registerIpv4Req->header.messageLen == sizeof(*registerIpv4Req)));

                        status =  CpswProxyServer_registerIpv4MacHandlerCb(remoteProcId,
                                                                 registerIpv4Req->info.id,
                                                                 registerIpv4Req->info.coreKey,
                                                                 registerIpv4Req->macAddress,
                                                                 registerIpv4Req->ipv4Addr);

                        if (CPSW_SOK == status)
                        {
                            registerIpv4Res.info.status =  ETH_RPC_CMDSTATUS_OK;
                        }
                        else
                        {
                            registerIpv4Res.info.status =  ETH_RPC_CMDSTATUS_EFAIL;
                        }

                        registerIpv4Res.header.messageId = ETH_RPC_CMD_TYPE_IPV4_MAC_REGISTER_RES;
                        registerIpv4Res.header.messageLen = sizeof(registerIpv4Res);

                        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                                remoteProcId,
                                                remoteEndPt,
                                                hProxyServer->ethDrvObj.localEp,
                                                &registerIpv4Res,
                                                sizeof(registerIpv4Res));
                        CpswAppUtils_assert(IPC_SOK == rtnVal);

                        break;
                    }
                    case ETH_RPC_CMD_TYPE_IPV4_MAC_UNREGISTER_REQ:
                    {
                         Eth_RpcIpv4UnregisterMacRequest *unregisterIpv4Req = (Eth_RpcIpv4UnregisterMacRequest *)msgBuffer;
                         Eth_RpcIpv4UnregisterMacResponse unregisterIpv4Res;

                        CpswAppUtils_assert((unregisterIpv4Req->header.messageId == ETH_RPC_CMD_TYPE_IPV4_MAC_UNREGISTER_REQ)
                                            &&
                                            (unregisterIpv4Req->header.messageLen == sizeof(*unregisterIpv4Req)));

                        status =  CpswProxyServer_unregisterIpv4MacHandlerCb(remoteProcId,
                                                                 unregisterIpv4Req->info.id,
                                                                 unregisterIpv4Req->info.coreKey,
                                                                 unregisterIpv4Req->ipv4Addr);

                        if (CPSW_SOK == status)
                        {
                            unregisterIpv4Res.info.status =  ETH_RPC_CMDSTATUS_OK;
                        }
                        else
                        {
                            unregisterIpv4Res.info.status =  ETH_RPC_CMDSTATUS_EFAIL;
                        }

                        unregisterIpv4Res.header.messageId = ETH_RPC_CMD_TYPE_IPV4_MAC_UNREGISTER_RES;
                        unregisterIpv4Res.header.messageLen = sizeof(unregisterIpv4Res);

                        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                                remoteProcId,
                                                remoteEndPt,
                                                hProxyServer->ethDrvObj.localEp,
                                                &unregisterIpv4Res,
                                                sizeof(unregisterIpv4Res));
                        CpswAppUtils_assert(IPC_SOK == rtnVal);

                        break;
                    }
                    case ETH_RPC_CMD_TYPE_C2S_NOTIFY:
                    {
                         Eth_RpcC2SNotify *c2sNotify = (Eth_RpcC2SNotify *)msgBuffer;
                         enum rpmsg_kdrv_ethswitch_client_notify_type rdevNotifyType;

                        CpswAppUtils_assert((c2sNotify->header.messageId == ETH_RPC_CMD_TYPE_C2S_NOTIFY)
                                            &&
                                            (c2sNotify->header.messageLen == sizeof(*c2sNotify)));

                        status = CpswProxy_mapEthRpcClientNotify2RdevClientNotifyType((Eth_RpcClientNotifyType)c2sNotify->notifyid, &rdevNotifyType);
                        if (CPSW_SOK == status)
                        {
                            CpswProxyServer_clientNotifyHandlerCb(remoteProcId,
                                                                  c2sNotify->info.id,
                                                                  c2sNotify->info.coreKey,
                                                                  rdevNotifyType,
                                                                  (uint8_t *)c2sNotify->notifyInfo,
                                                                  c2sNotify->notifyInfoLen);
                        }
                        break;
                    }
                    default:
                    {
                        Eth_RpcS2CNotify s2cNotify;

                        s2cNotify.notifyid = ETH_RPC_SERVERNOTIFY_UNKNOWNCMD;
                        s2cNotify.notifyInfoLen = sizeof(*header);
                        memcpy(s2cNotify.notifyInfo, header, sizeof(*header));
                        s2cNotify.header.messageId = ETH_RPC_CMD_TYPE_S2C_NOTIFY;
                        s2cNotify.header.messageLen = sizeof(s2cNotify);
                        rtnVal = RPMessage_send(hProxyServer->ethDrvObj.hAutosarEthRpMsgEp,
                                                remoteProcId,
                                                remoteEndPt,
                                                hProxyServer->ethDrvObj.localEp,
                                                &s2cNotify,
                                                sizeof(s2cNotify));
                        CpswAppUtils_assert(IPC_SOK == rtnVal);

                        break;
                    }
                }
            }
        }
    }

}

static void CpswProxyServer_hwPushNotifyFxn(void *arg, CpswCpts_HwPush hwPushNum)
{

    if (arg != NULL)
    {
        CpswProxyServer_Obj * hProxyServer = (CpswProxyServer_Obj *)arg;

        /* Post Event */
        EventP_post(hProxyServer->notifyServiceObj.hHwPushNotifyServiceEvent,
                    hProxyServer->notifyServiceObj.hwPushNotifyEventId[CPSW_CPTS_HW_PUSH_NORMALIZE(hwPushNum)]);
    }
}

static Void CpswProxyServer_notifyServiceTaskFxn(UArg arg0, UArg arg1)
{
    int32_t rtnVal = IPC_SOK;
    Cpsw_Handle hCpsw;
    CpswProxyServer_Obj * hProxyServer = (CpswProxyServer_Obj *)arg0;
    uint64_t msgBuffer[(CPSW_REMOTE_NOTIFY_SERVICE_RPC_MSG_SIZE / sizeof(uint64_t))];
    volatile bool exitTask = false;
    uint32_t i = 0U, events = 0U;
    Cpsw_IoctlPrms prms;
    CpswCpts_LookUpEventInArgs lookupEventInArgs;
    CpswCpts_Event lookupEventOutArgs;

    while (!exitTask)
    {
        /*Wait 1ms for hardware push event, then move on to other events*/
        events = EventP_pend(hProxyServer->notifyServiceObj.hHwPushNotifyServiceEvent,
                             EventP_ID_NONE,
                             CPSWPROXY_CPTS_HWPUSH_EVENTS_OR_MASK,
                             1U);

        /* Lookup for timestamp if it is a hardware push notification*/
        if (events)
        {
            rtnVal = CpswProxyServer_getCpswHandle(&hProxyServer->handleTbl,
                                                   hProxyServer->notifyServiceObj.notifyServiceCpswType,
                                                   &hCpsw);

            if ((rtnVal == CPSW_SOK) && (NULL != hCpsw))
            {
                for ( i = 0U; i < CPSW_CPTS_HW_PUSH_COUNT_MAX; i++)
                {
                    if(CPSW_IS_BIT_SET(events, i))
                    {
                        CpswRemoteNotifyService_HwPushMsg *hwPushMsg = (CpswRemoteNotifyService_HwPushMsg *)msgBuffer;

                        memset(hwPushMsg, 0, sizeof(*hwPushMsg));
                        hwPushMsg->header.messageId = CPSW_REMOTE_NOTIFY_SERVICE_CMD_HWPUSH;
                        hwPushMsg->header.messageLen = sizeof(*hwPushMsg);
                        hwPushMsg->cpswType = hProxyServer->notifyServiceObj.notifyServiceCpswType;
                        hwPushMsg->hwPushNum = (CpswCpts_HwPush)(i + 1U);

                        lookupEventInArgs.eventType = CPSW_CPTS_EVENTTYPE_HW_TS_PUSH;
                        lookupEventInArgs.hwPushNum = hwPushMsg->hwPushNum;
                        lookupEventInArgs.portNum = 0U;
                        lookupEventInArgs.seqId = 0U;
                        lookupEventInArgs.domain  = 0U;

                        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &lookupEventInArgs, &lookupEventOutArgs);
                        rtnVal = Cpsw_ioctl(hCpsw,
                                            hProxyServer->notifyServiceObj.dstProc,
                                            CPSW_CPTS_IOCTL_LOOKUP_EVENT,
                                            &prms);
                        if (rtnVal == CPSW_SOK)
                        {
                            hwPushMsg->timeStamp = lookupEventOutArgs.tsVal;

                            rtnVal = RPMessage_send(hProxyServer->notifyServiceObj.hNotifyServicRpMsgEp,
                                                    hProxyServer->notifyServiceObj.dstProc,
                                                    CPSW_REMOTE_NOTIFY_SERVICE_ENDPT_ID,
                                                    hProxyServer->notifyServiceObj.localEp,
                                                    hwPushMsg,
                                                    sizeof(*hwPushMsg));
                            CpswAppUtils_assert(IPC_SOK == rtnVal);
                        }
                    }
                }
            }
        }
    }
}

