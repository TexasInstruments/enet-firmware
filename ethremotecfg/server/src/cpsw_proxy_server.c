/*
 *
 * Copyright (c) 2020-2023 Texas Instruments Incorporated
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

/* EthFwTrace id for this module, must be unique within ETHFW */
#define ETHFWTRACE_MOD_ID 0x106

/* PDK header files */
#include <ti/osal/osal.h>
#include <ti/drv/ipc/ipc.h>

/* EthFw utils header files */
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/enet/examples/utils/include/enet_apputils.h>
#include <ti/drv/enet/examples/utils/include/enet_mcm.h>
#include <ti/drv/enet/examples/utils/include/enet_apprm.h>

/* EthFw utils header files */
#include <ethremotecfg/server/include/ethfw_virtport.h>
#include <utils/ethfw_common/include/ethfw_trace.h>
#include "cpsw_proxy_server.h"
#include "ethfw_mcast_priv.h"
#include "ethfw_arp_priv.h"
#include "ethfw_vlan_priv.h"
#if defined(ETHFW_VEPA_SUPPORT)
#include "ethfw_vepa_priv.h"
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define CPSWPROXY_CPSW9G_HWPUSH_BASE                     (26U)

#define CPSWPROXY_CPTS_HWPUSH_EVENTS_OR_MASK             (0xFFU)

#define CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_NAME            ("ASRETHDEVICE")

#define CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_PRIORITY        (2U)

#if defined(SAFERTOS)
#define CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_STACK           (16U * 1024U)
#define CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_ALIGN           CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_STACK
#else
#define CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_STACK           (0x4000U)
#define CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_ALIGN           (32U)
#endif

#define CPSWPROXY_AUTOSAR_ETHDRIVER_MSG_SIZE             (496U + 32U)

#define CPSWPROXY_AUTOSAR_ETHDRIVER_NUM_RPMSG_BUFS       (256U)

#define CPSWPROXY_AUTOSAR_ETHDRIVER_RPMSG_OBJ_SIZE       (256U)

#define CPSWPROXY_AUTOSAR_ETHDRIVER_DATA_SIZE            (CPSWPROXY_AUTOSAR_ETHDRIVER_MSG_SIZE * \
                                                          CPSWPROXY_AUTOSAR_ETHDRIVER_NUM_RPMSG_BUFS + \
                                                          CPSWPROXY_AUTOSAR_ETHDRIVER_RPMSG_OBJ_SIZE)

#define CPSWPROXY_ENET2RPMSG_ERR(x)                      (((x) == ENET_SOK) ? \
                                                          ETHREMOTECFG_CMDSTATUS_OK : \
                                                          ETHREMOTECFG_CMDSTATUS_EFAIL)

#define CPSWPROXY_ETH_CLIENT_TASK_NAME                   ("ETHREMOTEDEVICE")

#define CPSWPROXY_ETH_CLIENT_TASK_PRIORITY               (2U)

#if defined(SAFERTOS)
#define CPSWPROXY_ETH_CLIENT_TASK_STACK                  (16U * 1024U)
#define CPSWPROXY_ETH_CLIENT_TASK_ALIGN                  CPSWPROXY_ETH_CLIENT_TASK_STACK
#else
#define CPSWPROXY_ETH_CLIENT_TASK_STACK                  (0x4000U)
#define CPSWPROXY_ETH_CLIENT_TASK_ALIGN                  (32U)
#endif

#define CPSWPROXY_IPC_TASK_STACKALIGN                    (8192U)

#define ENET_COREKEY_CONVERT_MAGIC_NUM                   0U
#define CPSWPROXY_VIRTPORT_2_TOKEN(virtPort)             (((virtPort) * 100U) + ENET_COREKEY_CONVERT_MAGIC_NUM)
#define CPSWPROXY_TOKEN_2_VIRTPORT(token)                (((token) - ENET_COREKEY_CONVERT_MAGIC_NUM) / 100U)

/*! Remote notify service data size */
#define CPSWPROXY_NOTIFY_SERVICE_DATA_SIZE               ETHREMOTECFG_IPC_DATA_SIZE

/*! Remote notify service task stack size */
#define CPSWPROXY_NOTIFY_SERVICE_SERVER_TASK_STACKSIZE   (16U * 1024U)

#define CPSWPROXY_NOTIFY_SERVICE_SERVER_TASK_STACKALIGN  CPSWPROXY_NOTIFY_SERVICE_SERVER_TASK_STACKSIZE

/*! Remote notify service task name */
#define CPSWPROXY_NOTIFY_SERVICE_TASK_NAME               ("NOTIFY_SERVICE_TASK")

#define CPSWPROXY_NOTIFY_SERVICE_TASK_PRIORITY           (2U)

#define CPSWPROXY_PRINT_STATS_NONZERO(str, val)          ETHFWTRACE_INFO_IF(((val) != 0ULL), str, val)
#define CPSWPROXY_PRINT_STATS_IDX_NONZERO(str, idx, val) ETHFWTRACE_INFO_IF(((val) != 0ULL), str, idx, val)
#define CPSWPROXY_CLIENT_PRIMARY_REL_FLOW_IDX            (0U)
#define CPSWPROXY_CLIENT_INVALID_FLOW_IDX_OFFSET         (0xFFU)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/*
 * Client object
 */
typedef struct CpswProxyServer_ClientObj_s
{
    /* Whether client object is grabbed by a remote client */
    bool inUse;

    /* Token given to the remote client */
    uint32_t token;

    /* Virtual port id */
    EthRemoteCfg_VirtPort virtPort;

    /* Remote core id */
    uint32_t coreId;

    /* Client Id */
    uint32_t clientId;

    /* Features supported by related virtual port */
    uint32_t features;

    /* Allocated flow index base */
    uint32_t flowIdxBase;

    /* Allocated flow index offset */
    uint32_t flowIdxOffset;

    /* TX PSIL peer destination thread id */
    uint32_t psilDstId;

    /* Allocated MAC address */
    uint8_t macAddr[ENET_MAC_ADDR_LEN];

    /* End point of respective remote client */
    uint32_t remoteEp;

    /* If the client object has finished its teardown and became idle */
    bool isIdle;

#if defined(ETHFW_VEPA_SUPPORT)
    /* Vlan check count - register vlan once per virtual client for VEPA */
    uint32_t vlanRefCnt;
#endif
} CpswProxyServer_ClientObj;

/*
 * Remote Core Object, whose index points to the core Id of the remote core
 */
typedef struct CpswProxyServer_RemoteCoreObj_s
{
    /* Enet RM Reference Cnt per remote core */
    int32_t rmRefCnt;
    /* Enet LLD attachInfo, unique per coreId */
    EnetPer_AttachCoreOutArgs attachInfo;
    /* Client object for storing all the data associated with the client */
    CpswProxyServer_ClientObj clientObj[CPSWPROXYSERVER_REMOTE_CLIENT_MAX];
} CpswProxyServer_RemoteCoreObj;

/*
 * Client object handle
 */
typedef CpswProxyServer_ClientObj *CpswProxyServer_ClientHandle;

typedef struct CpswProxyServer_AsrServiceObj_s
{
    /* Task handle for AUTOSAR IPC communication */
    TaskP_Handle                 hAutosarEthTsk;
    /* RPMessage handle for AUTOSAR IPC communication */
    RPMessage_Handle             hAutosarEthRpMsgEp;
    /* Processor Id of AUTOSAR client */
    uint32_t                     dstProc;
    /* Local endpoint for the AUTOSAR client */
    uint32_t                     localEp;
    /* virtual port allocated for the AUTOSAR client, cuurently supports one virtPort */
    EthRemoteCfg_VirtPort        virtPort;
} CpswProxyServer_AsrServiceObj;

typedef struct CpswProxyServer_NotifyServiceObj_s
{
    Enet_Type                    notifyServiceCpswType;
    TaskP_Handle                 hNotifyServiceTsk;
    EventP_Handle                hHwPushNotifyServiceEvent;
    uint32_t                     hwPushNotifyEventId[CPSW_CPTS_HWPUSH_COUNT_MAX];
    RPMessage_Handle             hNotifyServicRpMsgEp;
    uint32_t                     hwPush2CoreIdMap[CPSW_CPTS_HWPUSH_COUNT_MAX];
    uint32_t                     hwPush2TokenMap[CPSW_CPTS_HWPUSH_COUNT_MAX];
    uint32_t                     dstProcMask;
    uint32_t                     localEp;
    uint32_t                     remoteEp;
} CpswProxyServer_NotifyServiceObj;

typedef struct CpswProxyServer_ClientServiceObj_s
{
    TaskP_Handle                 hClientServiceTsk;
    RPMessage_Handle             hClientServicRpMsgEp;
    SemaphoreP_Handle            rpmsgStartSem;
    uint32_t                     localEp;
} CpswProxyServer_ClientServiceObj;

typedef struct CpswProxyServer_Obj_s
{
    /* Mutex object used to protect get/free CpswProxy_ClientObjs */
    MutexP_Object mutexObj;
    /* Handle to mutexObj */
    MutexP_Handle hMutex;
    /* enetType of the server object */
    Enet_Type enetType;
    /* coreId of the master core (Server) */
    uint32_t masterCoreId;
    /* Instance Id of the CPSW server object */
    uint32_t instId;
    /* set to true when proxy server has been initialized */
    bool initDone;
    /* Alloc Object holds the data allocated to a given client by the server */
    CpswProxyServer_AllocObj allocObj[CPSWPROXYSERVER_REMOTE_CLIENT_ALLOC_MAX];
    /* callback which populates Ethernet Firmware device data */
    CpswProxyServer_InitEthfwDeviceDataCb initEthfwDeviceDataCb;
    /* Callback to retrieve Mcm cmd handle of ETHFW */
    CpswProxyServer_GetMcmCmdIfCb         getMcmCmdIfCb;
    /* Callback for handling C2S notify message */
    CpswProxyServer_NotifyCb              notifyCb;
    /* Object containing the data required to communicate with AUTOSAR client */
    CpswProxyServer_AsrServiceObj         ethDrvObj[CPSWPROXYSERVER_AUTOSAR_REMOTE_CLIENT_MAX];
    /* Object contaning the data required to communicate with other remote clients */
    CpswProxyServer_ClientServiceObj      clientServiceObj;
    /* Object for notify service required to communicate with remote clients */
    CpswProxyServer_NotifyServiceObj      notifyServiceObj;
    /* ALE port mask for all supported MAC ports */
    uint32_t alePortMask;
    /* ALE port mask for MAC only ports */
    uint32_t aleMacOnlyPortMask;
    /* ALE port mask for switch only ports */
    uint32_t aleSwitchOnlyPortMask;
    /* Default VLAN id to be used for MAC ports configured in MAC-only mode */
    uint16_t dfltVlanIdMacOnlyPorts;
    /* Default VLAN id to be used for MAC ports configured in switch mode (non MAC-only) */
    uint16_t dfltVlanIdSwitchPorts;
    /* Enet Mcm Cmd handle */
    EnetMcm_CmdIf  *hMcmCmdIf;
    /* Handle to Enet LLD, unique per CPSW instance */
    Enet_Handle hEnet;
    /* Remote core object for holding all core specific information */
    CpswProxyServer_RemoteCoreObj coreObj[IPC_MAX_PROCS];
    /* set to true when checksum offload is enabled */
    bool csumOffloadEn;
    /* Virtual port configurations */
    CpswProxyServer_VirtPortCfg virtPortCfg[CPSWPROXYSERVER_REMOTE_CLIENT_VIRTPORT_MAX];
    /* Number of remote virtual ports that remotes cores can attach to */
    uint32_t numVirtPorts;
} CpswProxyServer_Obj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */
static int32_t CpswProxyServer_initAutosarEthDeviceEp(CpswProxyServer_Obj * hServer,
                                                      CpswProxyServer_Config_t * cfg,
                                                      uint32_t clientInst);

static void CpswProxyServer_autosarEthDriverTaskFxn(void* arg0, void* arg1);

static void CpswProxyServer_remoteClientEthDriverTaskFxn(void* arg0, void* arg1);

static void CpswProxyServer_validateStartIdx(Enet_Handle hEnet,
                                             uint32_t hostId,
                                             uint32_t rxFlowStartId);

static int32_t CpswProxyServer_initRemoteClientEthDeviceEp(CpswProxyServer_Obj * hServer,
                                                           CpswProxyServer_Config_t * cfg);

static void CpswProxyServer_clientNotifyHandlerCb(uint32_t token,
                                                  uint32_t hostId,
                                                  EthRemoteCfg_NotifyType notifyid,
                                                  uint8_t *notify_info,
                                                  uint32_t notify_info_len);

static void CpswProxyServer_initClientHandle(CpswProxyServer_Config_t *cfg);

static int32_t CpswProxyServer_regMacPortFlow(Enet_Handle hEnet,
                                              uint32_t coreKey,
                                              uint32_t remoteCoreId,
                                              Enet_MacPort macPort,
                                              uint8_t *macAddr,
                                              uint32_t flowStartIdx,
                                              uint32_t flowIdx);

static int32_t CpswProxyServer_unregMacPortFlow(Enet_Handle hEnet,
                                                uint32_t coreKey,
                                                uint32_t remoteCoreId,
                                                Enet_MacPort macPort,
                                                uint8_t *macAddr,
                                                uint32_t flowStartIdx,
                                                uint32_t flowIdx);

static void CpswProxyServer_hwPushNotifyFxn(void *arg, CpswCpts_HwPush hwPushNum);

static void CpswProxyServer_notifyServiceTaskFxn(void* arg0, void* arg1);

static int32_t CpswProxyServer_initNotifyServiceEp(CpswProxyServer_Obj * hServer,
                                                   CpswProxyServer_Config_t * cfg);

static int32_t CpswProxyServer_sendNotify(CpswProxyServer_ClientHandle hClient,
                                          uint32_t notifyId);

static uint32_t CpswProxyServer_getAbsTxChNumber(uint32_t chRelPriority,
                                                 EthRemoteCfg_VirtPort virtPort);

static uint32_t CpswProxyServer_getClientTxChRxFlowNum(EthRemoteCfg_VirtPort virtPort,
                                                       uint32_t *pNumTxCh,
                                                       uint32_t *pNumRxFlow);

static int32_t CpswProxyServer_createCustomPolicer(Enet_Handle hEnet,
                                                   uint32_t coreId,
                                                   CpswAle_SetPolicerEntryInPartitionInArgs *customPolInArgs);

static int32_t CpswProxyServer_deleteCustomPolicer(Enet_Handle hEnet,
                                                   uint32_t coreId,
                                                   CpswAle_SetPolicerEntryInPartitionInArgs *customPolInArgs);

static int32_t CpswProxyServer_getRxFlowInfo(EthRemoteCfg_VirtPort virtPort,
                                             uint32_t relFlowIdx,
                                             EthFwQos_RxFlowInfo **rxFlowInfo);

/* Returns relative flow index from rx flows assigned to this virtual port */
static int32_t CpswProxyServer_getRelFlowIdx(EthRemoteCfg_VirtPort virtPort,
                                             uint32_t *relFlowIdx,
                                             uint32_t rxFlowIdxOffset);

/* Stores the flowIdxOffset assigned to a specific flow in virtual port configuration */
static void CpswProxyServer_saveRxFlowOffset(EthRemoteCfg_VirtPort virtPort,
                                             uint32_t relFlowIdx,
                                             uint32_t rxFlowIdxOffset);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/**< Buffer to store received messages. 256 messages of 512 bytes +
        space for book-keeping */
static uint8_t g_CpswProxyServerAutosarRpmsgBuf[CPSWPROXYSERVER_AUTOSAR_REMOTE_CLIENT_MAX][CPSWPROXY_AUTOSAR_ETHDRIVER_DATA_SIZE]  __attribute__ ((aligned(CPSWPROXY_IPC_TASK_STACKALIGN)));

/**< StackBuffer for different tasks */
static uint8_t gCpswProxyServer_autosarEthDriverTaskStackBuf[CPSWPROXYSERVER_AUTOSAR_REMOTE_CLIENT_MAX][CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_STACK] __attribute__ ((aligned(CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_ALIGN)));

static uint8_t gCpswProxyServerRpmsgbuf[ETHREMOTECFG_IPC_DATA_SIZE] __attribute__ ((aligned(1024)));

static uint8_t gCpswProxyServer_remoteClientEthDriverTaskStackBuf[CPSWPROXY_ETH_CLIENT_TASK_STACK] __attribute__ ((aligned(CPSWPROXY_IPC_TASK_STACKALIGN)));

/**< Buffer to store received messages. 256 messages of 512 bytes +
        space for book-keeping */
static uint8_t g_CpswProxyServerNotifyServiceRpmsgBuf[CPSWPROXY_NOTIFY_SERVICE_DATA_SIZE]  __attribute__ ((aligned(8192)));

static uint8_t gCpswProxyServer_notifyServiceTaskStackBuf[CPSWPROXY_NOTIFY_SERVICE_SERVER_TASK_STACKSIZE] __attribute__ ((aligned(CPSWPROXY_NOTIFY_SERVICE_SERVER_TASK_STACKALIGN)));

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

static CpswProxyServer_Obj *CpswProxyServer_getHandle(void)
{
    static CpswProxyServer_Obj gProxyServerObj =
    {
#if defined(SOC_J7200)
        .enetType              = ENET_CPSW_5G,
#elif defined(SOC_J721E) || defined(SOC_J784S4)
        .enetType              = ENET_CPSW_9G,
#endif
        .initEthfwDeviceDataCb = NULL,
        .getMcmCmdIfCb         = NULL,
        .initDone              = BFALSE,
        .masterCoreId          = IPC_MCU2_0,
    };

    return (&gProxyServerObj);
}

static CpswProxyServer_ClientHandle CpswProxyServer_allocClient(uint32_t remoteProcId,
                                                                uint32_t remoteEndPt,
                                                                uint32_t virtPort)
{
    CpswProxyServer_Obj *hServer = CpswProxyServer_getHandle();
    CpswProxyServer_ClientHandle hClient = NULL;
    uint32_t i;

    MutexP_lock(hServer->hMutex, MutexP_WAIT_FOREVER);

    for (i = 0U; i < ENET_ARRAYSIZE(hServer->coreObj[remoteProcId].clientObj); i++)
    {
        hClient = &hServer->coreObj[remoteProcId].clientObj[i];
        if ((hClient->inUse) && (hClient->virtPort == virtPort))
        {
            ETHFWTRACE_WARN("client has already been allocated");
            break;
        }
        else if (!hClient->inUse)
        {
            hClient->inUse = BTRUE;
            hClient->virtPort  = virtPort;
            hClient->token = ETHREMOTECFG_TOKEN_NONE;
            hClient->remoteEp = remoteEndPt;
            hClient->isIdle = BFALSE;
#if defined(ETHFW_VEPA_SUPPORT)
            hClient->vlanRefCnt = 0U;
#endif
            break;
        }
    }

    MutexP_unlock(hServer->hMutex);

    return hClient;
}

static void CpswProxyServer_freeClient(CpswProxyServer_ClientHandle hClient)
{
    CpswProxyServer_Obj *hServer = CpswProxyServer_getHandle();

    MutexP_lock(hServer->hMutex, MutexP_WAIT_FOREVER);
    memset(hClient, 0, sizeof(*hClient));
    hClient->inUse = BFALSE;
    hClient->token = ETHREMOTECFG_TOKEN_NONE;
    MutexP_unlock(hServer->hMutex);
}

static CpswProxyServer_ClientHandle CpswProxyServer_getClient(uint32_t remoteProcId,
                                                              uint32_t token)
{
    CpswProxyServer_Obj *hServer = CpswProxyServer_getHandle();
    CpswProxyServer_ClientHandle hClient = NULL;
    EnetMcm_CmdIf *hMcmCmdIf = NULL;
    uint32_t i;

    MutexP_lock(hServer->hMutex, MutexP_WAIT_FOREVER);

    for (i = 0U; i < ENET_ARRAYSIZE(hServer->coreObj[remoteProcId].clientObj); i++)
    {
        hClient = &hServer->coreObj[remoteProcId].clientObj[i];

        if (hClient->inUse && (hClient->token == token))
        {
            /* Found */
            break;
        }
    }

    MutexP_unlock(hServer->hMutex);

    return hClient;
}

int32_t CpswProxyServer_getIdleClientCnt(uint32_t *attachedClients,
                                         uint32_t *idleClients)
{
    CpswProxyServer_Obj *hServer = CpswProxyServer_getHandle();
    CpswProxyServer_ClientHandle hClient = NULL;
    int32_t status = ETHFW_SOK;
    *attachedClients = 0U;
    *idleClients = 0U;
    uint32_t i;
    uint32_t j;

    for (i = 0U; i < ENET_ARRAYSIZE(hServer->coreObj); i++)
    {
        for (j = 0U; j < ENET_ARRAYSIZE(hServer->coreObj[i].clientObj); j++)
        {
            hClient = &hServer->coreObj[i].clientObj[j];

            if (hClient->inUse && (hClient->token != ETHREMOTECFG_TOKEN_NONE))
            {
                (*attachedClients)++;

                if (hClient->isIdle)
                {
                    (*idleClients)++;
                }
            }
        }
    }
    return status;
}

static int32_t CpswProxyServer_sendNotify(CpswProxyServer_ClientHandle hClient,
                                          uint32_t notifyId)
{
    CpswProxyServer_Obj *hServer = CpswProxyServer_getHandle();
    EthRemoteCfg_CommonNotify notifyMsg;
    RPMessage_Handle handle = NULL;
    uint32_t srcEndPt = 0U;
    uint32_t clientInst;
    int32_t status = ETHFW_SOK;

    notifyMsg.hdr.common.msgType = ETHREMOTECFG_MSGTYPE_NOTIFY;
    notifyMsg.hdr.common.clientId = hClient->clientId;
    notifyMsg.hdr.common.token = hClient->token;
    notifyMsg.hdr.notifyType = notifyId;

    if (hClient->clientId == ETHREMOTECFG_CLIENTID_AUTOSAR)
    {
        for (clientInst = 0U; clientInst < CPSWPROXYSERVER_AUTOSAR_REMOTE_CLIENT_MAX; clientInst++)
        {
            if( hServer->ethDrvObj[clientInst].dstProc == hClient->coreId)
            {
                handle = hServer->ethDrvObj[clientInst].hAutosarEthRpMsgEp;
                srcEndPt = hServer->ethDrvObj[clientInst].localEp;
                break;
            }
        }
    }
    else
    {
        handle = hServer->clientServiceObj.hClientServicRpMsgEp;
        srcEndPt = hServer->clientServiceObj.localEp;
    }

    if (handle == NULL)
    {
        status = ETHFW_EFAIL;
        ETHFWTRACE_ERR(status, "Couldn't find core %u client handle", hClient->coreId);
    }

    if (status == ETHFW_SOK)
    {
        ETHFWTRACE_INFO("NOTIFY | S2C | core=%u endpt=%u token=%d notifyType=%u",
                        hClient->coreId,
                        hClient->remoteEp,
                        (int32_t)notifyMsg.hdr.common.token,
                        notifyId);

        ETHFWTRACE_DBG("S2C | msgType=%u token=%d clientId=%u notifyType=%u len=%u (%u.%u->%u.%u)",
                       notifyMsg.hdr.common.msgType,
                       (int32_t)notifyMsg.hdr.common.token,
                       notifyMsg.hdr.common.clientId,
                       notifyMsg.hdr.notifyType,
                       sizeof(notifyMsg),
                       EnetSoc_getCoreId(), srcEndPt,
                       hClient->coreId, hClient->remoteEp);

        status = RPMessage_send(handle,
                                hClient->coreId,
                                hClient->remoteEp,
                                srcEndPt,
                                &notifyMsg,
                                sizeof(notifyMsg));
        ETHFWTRACE_ERR_IF((status != IPC_SOK), status, "Failed to send notify msg via IPC");
    }

    return status;
}

int32_t CpswProxyServer_bcastNotify(uint32_t notifyId)
{
    CpswProxyServer_Obj *hServer = CpswProxyServer_getHandle();
    CpswProxyServer_ClientHandle hClient = NULL;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    uint32_t i;
    uint32_t j;

    for (i = 0U; i < ENET_ARRAYSIZE(hServer->coreObj); i++)
    {
        for (j = 0U; j < ENET_ARRAYSIZE(hServer->coreObj[i].clientObj); j++)
        {
            hClient = &hServer->coreObj[i].clientObj[j];

            if (hClient->inUse)
            {
                /* Set isIdle flag to false for next iteration of recovery */
                hClient->isIdle = BFALSE;
                status = CpswProxyServer_sendNotify(hClient, notifyId);
                ETHFWTRACE_ERR_IF((ETHFW_SOK != status), status,
                                  "Send Notification failed for coreId: %u", i);
            }
            TaskP_sleep(50);
        }
    }

    return status;
}

static int32_t CpswProxyServer_getPortMask(uint32_t clientId,
                                           uint32_t hostId,
                                           uint32_t *switchPortMask,
                                           uint32_t *macPortMask)
{
    CpswProxyServer_Obj *hServer = NULL;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    uint32_t i;

    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && hServer->initDone == BTRUE);

    for (i = 0U; i < CPSWPROXYSERVER_REMOTE_CLIENT_ALLOC_MAX; i++)
    {
        if ((hServer->allocObj[i].clientId == clientId) && (hServer->allocObj[i].remoteProcId == hostId))
        {
            *switchPortMask = hServer->allocObj[i].virtSwitchPortMask;
            *macPortMask = hServer->allocObj[i].virtMacPortMask;
            break;
        }
    }
    if (i >= CPSWPROXYSERVER_REMOTE_CLIENT_ALLOC_MAX)
    {
        status = ETHREMOTECFG_CMDSTATUS_EBADARGS;
        ETHFWTRACE_ERR(status, "No port mask found for clientId: %u and coreId: %u",
                       clientId, hostId);
    }

    return status;
}

static int32_t CpswProxyServer_VirtPortAllocCb(uint32_t clientId,
                                               uint32_t hostId,
                                               uint32_t *switchPortMask,
                                               uint32_t *macPortMask)
{
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    if (clientId < CPSWPROXYSERVER_REMOTE_CLIENT_ALLOC_MAX)
    {
        status = CpswProxyServer_getPortMask(clientId, hostId, switchPortMask, macPortMask);
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EBADARGS;
        ETHFWTRACE_ERR(status, "Invalid client id %u", clientId);
    }

    return status;
}


static int32_t CpswProxyServer_attachHandlerCb(CpswProxyServer_ClientHandle hClient,
                                               uint32_t hostId,
                                               uint32_t portId,
                                               uint32_t *pRxMtu,
                                               uint32_t *pTxMtu,
                                               uint32_t txMtuArraySize,
                                               uint32_t *pFeatures,
                                               uint32_t *pNumTxCh,
                                               uint32_t *pNumRxFlow)
{
    CpswProxyServer_Obj *hServer = NULL;
    EnetPer_AttachCoreOutArgs attachInfo;
    Enet_IoctlPrms prms;
    EthRemoteCfg_VirtPort virtPort = ETHREMOTECFG_VIRTPORT_DENORM(portId);
    bool isMacPort = EthFwVirtPort_isMacPort(virtPort);
    bool csumEnable;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        if (hServer->coreObj[hostId].rmRefCnt == 0U)
        {
            EnetMcm_coreAttach(hServer->hMcmCmdIf, hostId, &attachInfo);
            memcpy(&hServer->coreObj[hostId].attachInfo, &attachInfo, sizeof(attachInfo));
        }

        hServer->coreObj[hostId].rmRefCnt++;

        attachInfo = hServer->coreObj[hostId].attachInfo;
        *pRxMtu = attachInfo.rxMtu;
        EnetAppUtils_assert(txMtuArraySize == ENET_ARRAYSIZE(attachInfo.txMtu));
        memcpy(pTxMtu, attachInfo.txMtu, sizeof(attachInfo.txMtu));

        *pFeatures = 0U;
        if (hServer->csumOffloadEn)
        {
            *pFeatures |= ETHREMOTECFG_FEATURE_TXCSUM;
        }
        if (!isMacPort)
        {
            *pFeatures |= ETHREMOTECFG_FEATURE_MC_FILTER;
        }

        /* If number of tx channels and rx flows allocated are required by the client
         * i.e. called not called from CpswProxyServer_attachExtHandlerCb attach */
        if (pNumTxCh != NULL && pNumRxFlow != NULL)
        {
            status = CpswProxyServer_getClientTxChRxFlowNum(virtPort, pNumTxCh, pNumRxFlow);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                               "Failed to get tx and rx channels for virtual port %d", virtPort);
        }

        /* Save parameters in client object */
        hClient->token     = CPSWPROXY_VIRTPORT_2_TOKEN(virtPort);
        hClient->coreId    = hostId;
        hClient->features  = *pFeatures;
    }
    else
    {
        ETHFWTRACE_ERR(status, "Attach failed for coreId: %u", hostId);
    }

    return status;
}


static int32_t CpswProxyServer_attachExtHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                  uint32_t hostId,
                                                  uint32_t portId,
                                                  uint32_t *pRxMtu,
                                                  uint32_t *pTxMtu,
                                                  uint32_t txMtuArraySize,
                                                  uint32_t *pFeatures,
                                                  uint32_t *pRxFlowIdxBase,
                                                  uint32_t *pRxFlowIdxOffset,
                                                  uint32_t *pTxPsilDstId,
                                                  uint32_t *pRxPsilSrcId,
                                                  uint8_t *macAddr)
{
    CpswProxyServer_Obj *hServer = NULL;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    uint32_t coreKey;

    /* Actual attach operation */
    status = CpswProxyServer_attachHandlerCb(hClient, hostId, portId, pRxMtu, pTxMtu, txMtuArraySize, pFeatures, NULL, NULL);
    EnetAppUtils_assert(ETHREMOTECFG_CMDSTATUS_OK == status);

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        /* Check that server itself is ready */
        hServer = CpswProxyServer_getHandle();
        if ((hServer != NULL) && (hServer->initDone == BTRUE))
        {
            coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
        }
        else
        {
            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            ETHFWTRACE_ERR(status, "ETHFW server is not ready");
            EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
        }
    }

    /* Allocate RX flow */
    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        status = EnetAppUtils_allocRxFlow(hServer->hEnet,
                                          coreKey,
                                          hostId,
                                          pRxFlowIdxBase,
                                          pRxFlowIdxOffset);
        if (ENET_SOK == status)
        {
            CpswProxyServer_validateStartIdx(hServer->hEnet, hostId, *pRxFlowIdxBase);
            CpswProxyServer_saveRxFlowOffset(hClient->virtPort, CPSWPROXY_CLIENT_PRIMARY_REL_FLOW_IDX, *pRxFlowIdxOffset);
        }
    }

    /* Allocate TX channel */
    if (ENET_SOK == status)
    {
        status = EnetAppUtils_allocTxCh(hServer->hEnet,
                                        coreKey,
                                        hostId,
                                        pTxPsilDstId);
    }

    /* Allocate MAC address */
    if (ENET_SOK == status)
    {
        status = EnetAppUtils_allocMac(hServer->hEnet,
                                       coreKey,
                                       hostId,
                                       macAddr);
    }

    if (ENET_SOK == status)
    {
        /* RX PSIL thread id */
        *pRxPsilSrcId = EnetSoc_getRxChPeerId(hServer->enetType, 0U, 0U);

        /* Save parameters in client object */
        hClient->flowIdxBase   = *pRxFlowIdxBase;
        hClient->flowIdxOffset = *pRxFlowIdxOffset;
        hClient->psilDstId     = *pTxPsilDstId;
        EnetUtils_copyMacAddr(&hClient->macAddr[0U], macAddr);
    }

    ETHFWTRACE_ERR_IF((ENET_SOK != status), status, "Attach Ext failed for coreId: %u", hostId);

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_allocTxHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                uint32_t hostId,
                                                uint32_t *pTxPsilDstId,
                                                uint32_t chRelPriority)
{
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    CpswProxyServer_Obj *hServer = NULL;
    uint32_t coreKey;
    uint32_t txChNum;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        /* Get absolute tx channel number from relative tx channel */
        txChNum = CpswProxyServer_getAbsTxChNumber(chRelPriority, hClient->virtPort);
        EnetAppUtils_assert(txChNum != ENET_RM_TXCHNUM_INVALID);
        status = EnetAppUtils_allocAbsTxCh(hServer->hEnet,
                                          coreKey,
                                          hostId,
                                          pTxPsilDstId,
                                          txChNum);

        if (ENET_SOK == status)
        {
            hClient->psilDstId = *pTxPsilDstId;
        }
        else
        {
            ETHFWTRACE_ERR(status, "Failed to alloc TX channel");
        }
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_allocRxHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                uint32_t hostId,
                                                uint32_t *pRxFlowIdxBase,
                                                uint32_t *pRxFlowIdxOffset,
                                                uint32_t *pRxPsilSrcId,
                                                uint32_t relFlowIdx)
{
    CpswProxyServer_Obj *hServer = NULL;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    uint32_t coreKey;
    EthFwQos_RxFlowInfo *rxFlowInfo;
    uint32_t i;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        *pRxPsilSrcId = EnetSoc_getRxChPeerId(hServer->enetType, 0U, 0U);

        status = EnetAppUtils_allocRxFlow(hServer->hEnet,
                                            coreKey,
                                            hostId,
                                            pRxFlowIdxBase,
                                            pRxFlowIdxOffset);

        if (ENET_SOK == status)
        {
            CpswProxyServer_validateStartIdx(hServer->hEnet, hostId, *pRxFlowIdxBase);

            hClient->flowIdxBase   = *pRxFlowIdxBase;
            hClient->flowIdxOffset = *pRxFlowIdxOffset;
        }
        else
        {
            ETHFWTRACE_ERR(status, "Failed to alloc RX channel");
        }

        if (status == ENET_SOK)
        {
            CpswProxyServer_saveRxFlowOffset(hClient->virtPort, relFlowIdx, *pRxFlowIdxOffset);
            status = CpswProxyServer_getRxFlowInfo(hClient->virtPort, relFlowIdx, &rxFlowInfo);
            ETHFWTRACE_ERR_IF((status != ETHREMOTECFG_CMDSTATUS_OK), status,
                               "Failed to get virtual port %u rx flow %u info",
                               hClient->virtPort,
                               pRxFlowIdxOffset);
            if (status == ETHREMOTECFG_CMDSTATUS_OK)
            {
                /* Flow has been allocated, create custom policer for it (if any) */
                for (i = 0U; i < rxFlowInfo->numCustomPolicers; i++)
                {
                    rxFlowInfo->customPolicersInArgs[i]->threadId = *pRxFlowIdxOffset;
                    status = CpswProxyServer_createCustomPolicer(hServer->hEnet, hostId, rxFlowInfo->customPolicersInArgs[i]);
                    ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                                       "Failed to create custom policer for virtual port %u flow %u",
                                       hClient->virtPort,
                                       *pRxFlowIdxOffset);
                }
            }
        }
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_allocMacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                 uint32_t hostId,
                                                 uint8_t *macAddr)
{
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    CpswProxyServer_Obj *hServer = NULL;
    uint32_t coreKey;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        status = EnetAppUtils_allocMac(hServer->hEnet,
                                       coreKey,
                                       hostId,
                                       macAddr);
        if (ENET_SOK == status)
        {
            EnetUtils_copyMacAddr(&hClient->macAddr[0U], macAddr);
        }
        else
        {
            ETHFWTRACE_ERR(status, "Failed to alloc MAC Address");
        }
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_detachHandlerCb(CpswProxyServer_ClientHandle hClient,
                                               uint32_t hostId)
{
    CpswProxyServer_Obj *hServer = NULL;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    uint32_t coreKey;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        /* Detach from MCM */
        EnetAppUtils_assert(hServer->hMcmCmdIf != NULL);
        hServer->coreObj[hostId].rmRefCnt--;
        if (hServer->coreObj[hostId].rmRefCnt == 0U)
        {
            EnetMcm_coreDetach(hServer->hMcmCmdIf, hostId, coreKey);
        }

        EnetMcm_releaseHandleInfo(hServer->hMcmCmdIf);
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to detach the coreId: %u", hostId);
    }

    return status;
}

static int32_t CpswProxyServer_freeTxHandlerCb(CpswProxyServer_ClientHandle hClient,
                                               uint32_t hostId,
                                               uint32_t pTxPsilDstId)
{
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    CpswProxyServer_Obj *hServer = NULL;
    uint32_t coreKey;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        status = EnetAppUtils_freeTxCh(hServer->hEnet,
                                       coreKey,
                                       hostId,
                                       pTxPsilDstId);

        if (ENET_SOK == status)
        {
            hClient->psilDstId = 0U;
        }
        else
        {
            ETHFWTRACE_ERR(status, "Failed to free Tx channel with Id: %u", pTxPsilDstId);
        }
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_freeRxHandlerCb(CpswProxyServer_ClientHandle hClient,
                                               uint32_t hostId,
                                               uint32_t rxFlowIdxBase,
                                               uint32_t rxFlowIdxOffset)
{
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    CpswProxyServer_Obj *hServer = NULL;
    uint32_t coreKey;
    uint32_t relFlowIdx = 0U;
    EthFwQos_RxFlowInfo *rxFlowInfo;
    uint32_t i;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (status == ETHREMOTECFG_CMDSTATUS_OK)
    {
        status = CpswProxyServer_getRelFlowIdx(hClient->virtPort, &relFlowIdx, rxFlowIdxOffset);
        ETHFWTRACE_ERR_IF((status != ETHREMOTECFG_CMDSTATUS_OK), status,
                           "Failed to get relative flow index for virtual port %u flow %u",
                            hClient->virtPort,
                            rxFlowIdxOffset);
        if (ETHREMOTECFG_CMDSTATUS_OK == status)
        {
            status = CpswProxyServer_getRxFlowInfo(hClient->virtPort, relFlowIdx, &rxFlowInfo);
            ETHFWTRACE_ERR_IF((status != ETHREMOTECFG_CMDSTATUS_OK), status,
                               "Failed to get virtual port %u rx flow %u info",
                                hClient->virtPort,
                                rxFlowIdxOffset);
            /* Remove custom policers for this flow (if any) */
            for (i = 0U; i < rxFlowInfo->numCustomPolicers; i++)
            {
                status = CpswProxyServer_deleteCustomPolicer(hServer->hEnet, hostId, rxFlowInfo->customPolicersInArgs[i]);
                ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                                   "Failed to delete custom policer for virtual port %u flow %u",
                                    hClient->virtPort,
                                    rxFlowIdxOffset);
            }
            /* Remove flowIdxOffset assigned to specific flow from virtual port config */
            CpswProxyServer_saveRxFlowOffset(hClient->virtPort, relFlowIdx, CPSWPROXY_CLIENT_INVALID_FLOW_IDX_OFFSET);
        }
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        CpswProxyServer_validateStartIdx(hServer->hEnet, hostId, rxFlowIdxBase);
        status = EnetAppUtils_freeRxFlow(hServer->hEnet,
                                         coreKey,
                                         hostId,
                                         rxFlowIdxOffset);

        if (ENET_SOK == status)
        {
            hClient->flowIdxBase   = 0U;
            hClient->flowIdxOffset = 0U;
        }
        else
        {
            ETHFWTRACE_ERR(status, "Failed to free Rx flow with base: %u,"
                           "offset: %u", rxFlowIdxBase, rxFlowIdxOffset);
        }
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_freeMacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                uint32_t hostId,
                                                uint8_t *macAddr)
{
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    CpswProxyServer_Obj *hServer = NULL;
    uint32_t coreKey;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        status = EnetAppUtils_freeMac(hServer->hEnet,
                                      coreKey,
                                      hostId,
                                      macAddr);

        if (ENET_SOK ==status)
        {
            memcpy(&hClient->macAddr[0U], 0U, ENET_MAC_ADDR_LEN);
        }
        else
        {
            ETHFWTRACE_ERR(status, "Failed to free the macAddr=%02x:%02x:%02x:%02x:%02x:%02x",
                           macAddr[0U], macAddr[1U], macAddr[2U],
                           macAddr[3U], macAddr[4U], macAddr[5U]);
        }
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_registerMacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                    uint32_t hostId,
                                                    uint8_t *macAddr,
                                                    uint32_t flowIdxBase,
                                                    uint32_t flowIdxOffset)
{
    CpswProxyServer_Obj *hServer = NULL;
    Enet_MacPort macPort;
    bool isSwitchPort;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    uint32_t coreKey;
    uint32_t relFlowIdx;
#if defined(ETHFW_VEPA_SUPPORT)
    struct eth_addr hwAddr;
#endif

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    /* Make sure that CpswProxyServer_registerMacHandlerCb is requested on primary flow */
    status = CpswProxyServer_getRelFlowIdx(hClient->virtPort, &relFlowIdx, flowIdxOffset);
    ETHFWTRACE_ERR_IF((status != ETHREMOTECFG_CMDSTATUS_OK), status,
                       "Failed to get relative flow index for virtual port %u flow %u",
                       hClient->virtPort,
                       flowIdxOffset);
    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        if(relFlowIdx != CPSWPROXY_CLIENT_PRIMARY_REL_FLOW_IDX)
        {
            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            ETHFWTRACE_ERR(status, "Failed to register MAC addr on extended flow, must be on primary flow");
        }
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        CpswProxyServer_validateStartIdx(hServer->hEnet, hostId, flowIdxBase);

        isSwitchPort = EthFwVirtPort_isSwitchPort(hClient->virtPort);
        if (isSwitchPort)
        {
            /* Setup MAC address based classifier to the requested RX flow */
            status = EnetAppUtils_regDstMacRxFlow(hServer->hEnet,
                                                    coreKey,
                                                    hostId,
                                                    flowIdxBase,
                                                    flowIdxOffset,
                                                    macAddr);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to setup MAC addr based route");

#if defined(ETHFW_VEPA_SUPPORT)
            if ((hClient->vlanRefCnt == 0U) && (status == ENET_SOK))
            {
                SMEMCPY(&hwAddr, macAddr, ETH_HWADDR_LEN);
                status = EthFwVepa_registerClient(hServer->hEnet, hostId, flowIdxOffset,
                                                    hServer->dfltVlanIdSwitchPorts,
                                                    hClient->virtPort,
                                                    &hwAddr);
                ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                                    "Failed to register client core %u "
                                    "macAddr %02x:%02x:%02x:%02x:%02x:%02x into VEPA table",
                                    hostId,
                                    macAddr[0], macAddr[1], macAddr[2],
                                    macAddr[3], macAddr[4], macAddr[5]);
            }
            if (status == ETHFW_SOK)
            {
                hClient->vlanRefCnt++;
            }
#endif
        }
        else
        {
            macPort = EthFwVirtPort_getMacPort(hClient->virtPort);

            /* Setup MAC port based classifier to the requested RX flow */
            status = CpswProxyServer_regMacPortFlow(hServer->hEnet,
                                                    coreKey,
                                                    hostId,
                                                    macPort,
                                                    macAddr,
                                                    flowIdxBase,
                                                    flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to setup MAC port based route");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to register macAddr=%02x:%02x:%02x:%02x:%02x:%02x",
                        macAddr[0U], macAddr[1U], macAddr[2U],
                        macAddr[3U], macAddr[4U], macAddr[5U]);
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_unregisterMacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                      uint32_t hostId,
                                                      uint8_t *macAddr,
                                                      uint32_t flowIdxBase,
                                                      uint32_t flowIdxOffset)
{
    CpswProxyServer_Obj *hServer = NULL;
    Enet_MacPort macPort;
    bool isSwitchPort;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    uint32_t coreKey;
    uint32_t relFlowIdx;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    /* Make sure that CpswProxyServer_unregisterMacHandlerCb is requested on primary flow */
    status = CpswProxyServer_getRelFlowIdx(hClient->virtPort, &relFlowIdx, flowIdxOffset);
    ETHFWTRACE_ERR_IF((status != ETHREMOTECFG_CMDSTATUS_OK), status,
                       "Failed to get relative flow index for virtual port %u flow %u",
                       hClient->virtPort,
                       flowIdxOffset);
    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        if(relFlowIdx != CPSWPROXY_CLIENT_PRIMARY_REL_FLOW_IDX)
        {
            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            ETHFWTRACE_ERR(status, "Failed to unregister MAC addr on extended flow, must be on primary flow");
        }
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        CpswProxyServer_validateStartIdx(hServer->hEnet, hostId, flowIdxBase);

        isSwitchPort = EthFwVirtPort_isSwitchPort(hClient->virtPort);
        if (isSwitchPort)
        {
            /* Teardown MAC address based classifier */
            status = EnetAppUtils_unregDstMacRxFlow(hServer->hEnet,
                                                    coreKey,
                                                    hostId,
                                                    flowIdxBase,
                                                    flowIdxOffset,
                                                    macAddr);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to teardown MAC addr based route");

#if defined(ETHFW_VEPA_SUPPORT)
            if (status == ENET_SOK)
            {
                hClient->vlanRefCnt--;
            }
            if ((hClient->vlanRefCnt == 0U) && (status == ENET_SOK))
            {
                status = EthFwVepa_unregisterClient(hServer->hEnet, hostId, flowIdxOffset,
                                                    hServer->dfltVlanIdSwitchPorts,
                                                    hClient->virtPort);
                ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                                "Failed to unregister client core %u "
                                "macAddr %02x:%02x:%02x:%02x:%02x:%02x into VEPA table",
                                hostId,
                                macAddr[0], macAddr[1], macAddr[2],
                                macAddr[3], macAddr[4], macAddr[5]);
            }
#endif
        }
        else
        {
            macPort = EthFwVirtPort_getMacPort(hClient->virtPort);

            /* Teardown MAC port based classifier */
            status = CpswProxyServer_unregMacPortFlow(hServer->hEnet,
                                                    coreKey,
                                                    hostId,
                                                    macPort,
                                                    macAddr,
                                                    flowIdxBase,
                                                    flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to teardown MAC port based route");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to de-register macAddr=%02x:%02x:%02x:%02x:%02x:%02x",
                        macAddr[0U], macAddr[1U], macAddr[2U],
                        macAddr[3U], macAddr[4U], macAddr[5U]);
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_registerIPv4MacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                        uint32_t hostId,
                                                        uint8_t *macAddr,
                                                        uint8_t *ipAddr)
{
    CpswProxyServer_Obj *hServer = NULL;
#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
    ip4_addr_t ip4Addr;
    struct eth_addr hwAddr;
#endif
    bool isSwitchPort;
    uint16_t vlanId = 0U;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        isSwitchPort = EthFwVirtPort_isSwitchPort(hClient->virtPort);
        if (isSwitchPort)
        {
            /* Add IPv4:MAC address to ETHFW ARP table */
#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
            IP4_ADDR(&ip4Addr, ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
            SMEMCPY(&hwAddr, macAddr, ETH_HWADDR_LEN);

            status = EthFwArp_addAddr(&ip4Addr, &hwAddr, vlanId);
            if (status != ETHFW_SOK)
            {
                status = ETHREMOTECFG_CMDSTATUS_EFAIL;
                ETHFWTRACE_ERR(status, "Failed to add ARP entry");
            }
            else
            {
                EthFwArp_printTable();
            }
#endif
        }
        else
        {
            /* ETHFW ARP table is supported only on virtual switch ports.
            * Virtual MAC ports don't needed proxy ARP because all traffic is already
            * forwarded to the remote client */
            status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
            ETHFWTRACE_ERR(status, "IPv4:MAC registration is not supported on virtual MAC ports");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to register ipAddr=%u.%u.%u.%u",
                       ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
    }

    return status;
}

static int32_t CpswProxyServer_deregisterIPv4MacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                          uint32_t hostId,
                                                          uint8_t *ipAddr)
{
    CpswProxyServer_Obj *hServer = NULL;
#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
    ip4_addr_t ip4Addr;
#endif
    bool isSwitchPort;
    uint16_t vlanId = 0U;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        isSwitchPort = EthFwVirtPort_isSwitchPort(hClient->virtPort);
        if (isSwitchPort)
        {
            /* Remove IP address from ETHFW proxy ARP table */
#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
            IP4_ADDR(&ip4Addr, ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);

            status = EthFwArp_delAddr(&ip4Addr, vlanId);
            if (status != ETHFW_SOK)
            {
                status = ETHREMOTECFG_CMDSTATUS_EFAIL;
                ETHFWTRACE_ERR(status, "Failed to remove ARP entry");
            }
            else
            {
                EthFwArp_printTable();
            }
#endif
        }
        else
        {
            /* ETHFW ARP table is supported only on virtual switch ports */
            status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
            ETHFWTRACE_ERR(status, "IPv4:MAC deregistration is not supported on virtual MAC ports");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to de-register ipAddr=%u.%u.%u.%u",
                       ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
    }

    return status;
}

static int32_t CpswProxyServer_vlanJoinHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                 uint32_t coreId,
                                                 uint16_t vlanId,
                                                 const uint8_t *macAddr,
                                                 uint32_t flowIdxBase,
                                                 uint32_t flowIdxOffset)
{
    CpswProxyServer_Obj *hServer = NULL;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        status = EthFwVlan_join(hServer->hEnet,
                            hClient->virtPort,
                            vlanId,
                            macAddr,
                            flowIdxOffset);
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                          "Failed to join VLAN %u", vlanId);
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to join the vlan");
    }

    return status;
}

static int32_t CpswProxyServer_vlanLeaveHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                  uint32_t coreId,
                                                  uint16_t vlanId,
                                                  const uint8_t *macAddr,
                                                  uint32_t flowIdxBase,
                                                  uint32_t flowIdxOffset)
{
    CpswProxyServer_Obj *hServer = NULL;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        status = EthFwVlan_leave(hServer->hEnet,
                             hClient->virtPort,
                             vlanId,
                             macAddr,
                             flowIdxOffset);
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                      "Failed to leave VLAN %u", vlanId);
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to leave the vlan");
    }

    return status;
}

int32_t CpswProxyServer_promiscModeHandlerCb(CpswProxyServer_ClientHandle hClient,
                                             uint32_t hostId,
                                             bool enable)
{
    CpswProxyServer_Obj *hServer = NULL;
    Enet_MacPort macPort;
    Enet_IoctlPrms prms;
    bool isMacPort;
    uint32_t cmd;
    int32_t status = ENET_SOK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        isMacPort = EthFwVirtPort_isMacPort(hClient->virtPort);
        if (isMacPort)
        {
            /* Enable promiscuous mode on virtual MAC ports (MAC-only CAF) */
            macPort = EthFwVirtPort_getMacPort(hClient->virtPort);
            ENET_IOCTL_SET_IN_ARGS(&prms, &macPort);

            cmd = enable ? CPSW_ALE_IOCTL_ENABLE_PROMISC_MODE : CPSW_ALE_IOCTL_DISABLE_PROMISC_MODE;

            status = Enet_ioctl(hServer->hEnet, hostId, cmd, &prms);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                            "Failed to %s promiscuous mode on MAC port %u",
                            enable ? "enable" : "disable", ENET_MACPORT_ID(macPort));

            status = CPSWPROXY_ENET2RPMSG_ERR(status);
        }
        else
        {
            /* Promiscuous mode is not supported on virtual switch ports */
            status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
            ETHFWTRACE_ERR(status, "Promiscuous mode is not supported on virtual switch ports");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to perform promiscous %s", enable ? "enable" : "disable" );
    }

    return status;
}

static void CpswProxyServer_validateStartIdx(Enet_Handle hEnet,
                                             uint32_t hostId,
                                             uint32_t rxFlowStartId)
{
    uint32_t p0FlowIdOffset;

    p0FlowIdOffset = EnetAppUtils_getStartFlowIdx(hEnet, hostId);
    EnetAppUtils_assert(rxFlowStartId == p0FlowIdOffset);
}

static int32_t CpswProxyServer_regMacPortFlow(Enet_Handle hEnet,
                                              uint32_t coreKey,
                                              uint32_t remoteCoreId,
                                              Enet_MacPort macPort,
                                              uint8_t *macAddr,
                                              uint32_t flowStartIdx,
                                              uint32_t flowIdx)
{
    CpswAle_SetUcastEntryInArgs ucastInArgs;
    CpswAle_SetPolicerEntryInPartitionInArgs polInArgs;
    CpswAle_SetPolicerEntryOutArgs polOutArgs;
    Enet_IoctlPrms prms;
    uint32_t entryIdx;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    if (EnetUtils_isMcastAddr(macAddr))
    {
        status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
        ETHFWTRACE_ERR(status, "Port %u: mcast not supported", ENET_MACPORT_ID(macPort));
    }

    /* Add unicast address */
    if (status == ETHREMOTECFG_CMDSTATUS_OK)
    {
        ucastInArgs.addr.vlanId  = 0U;
        ucastInArgs.info.portNum = CPSW_ALE_HOST_PORT_NUM;
        ucastInArgs.info.blocked = BFALSE;
        ucastInArgs.info.secure  = BTRUE;
        ucastInArgs.info.super   = BFALSE;
        ucastInArgs.info.ageable = BFALSE;
        ucastInArgs.info.trunk   = BFALSE;
        EnetUtils_copyMacAddr(&ucastInArgs.addr.addr[0U], macAddr);

        ENET_IOCTL_SET_INOUT_ARGS(&prms, &ucastInArgs, &entryIdx);

        status = Enet_ioctl(hEnet, remoteCoreId, CPSW_ALE_IOCTL_ADD_UCAST, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                          "Port %u: failed to add ucast entry", ENET_MACPORT_ID(macPort));
    }

    /* Setup policer with "port" as match criteria */
    if (status == ENET_SOK)
    {
        memset(&polInArgs, 0, sizeof(polInArgs));

        polInArgs.policerMatch.policerMatchEnMask = CPSW_ALE_POLICER_MATCH_PORT;
        polInArgs.policerMatch.portNum   = CPSW_ALE_MACPORT_TO_ALEPORT(macPort);
        polInArgs.threadIdEn             = BTRUE;
        polInArgs.threadId               = flowIdx;
        polInArgs.peakRateInBitsPerSec   = 0;
        polInArgs.commitRateInBitsPerSec = 0;
        polInArgs.policerPartLevel       = CPSW_ALE_POLICER_PARTITION_LEVEL_3;
        ENET_IOCTL_SET_INOUT_ARGS(&prms, &polInArgs, &polOutArgs);

        status = Enet_ioctl(hEnet, remoteCoreId, CPSW_ALE_IOCTL_SET_POLICER_IN_PARTITION, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                          "Failed to set port %u policer", ENET_MACPORT_ID(macPort));
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_unregMacPortFlow(Enet_Handle hEnet,
                                                uint32_t coreKey,
                                                uint32_t remoteCoreId,
                                                Enet_MacPort macPort,
                                                uint8_t *macAddr,
                                                uint32_t flowStartIdx,
                                                uint32_t flowIdx)
{
    CpswAle_DelPolicerEntryInArgs delPolInArgs;
    CpswAle_PolicerMatchParams polMatch;
    CpswAle_PolicerEntryOutArgs polOutArgs;
    CpswAle_MacAddrInfo macAddrInfo;
    Enet_IoctlPrms prms;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    if (EnetUtils_isMcastAddr(macAddr))
    {
        status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
        ETHFWTRACE_ERR(status, "Port %u: mcast not supported", ENET_MACPORT_ID(macPort));
    }

    /* Remove policer with "port" match criteria */
    if (status == ETHREMOTECFG_CMDSTATUS_OK)
    {
        memset(&polMatch, 0, sizeof(polMatch));
        polMatch.policerMatchEnMask = CPSW_ALE_POLICER_MATCH_PORT;
        polMatch.portNum = CPSW_ALE_MACPORT_TO_ALEPORT(macPort);

        ENET_IOCTL_SET_INOUT_ARGS(&prms, &polMatch, &polOutArgs);

        status = Enet_ioctl(hEnet, remoteCoreId, CPSW_ALE_IOCTL_GET_POLICER, &prms);
        if (status == ENET_SOK)
        {
            if ((polOutArgs.threadIdEn == BTRUE) &&
                (polOutArgs.threadId == flowIdx))
            {
                status = ENET_SOK;
            }
            else
            {
                status = ENET_EINVALIDPARAMS;
                ETHFWTRACE_ERR(status, "Invalid policer thread cfg (threadIdEn=%u threadId=%u)",
                               polOutArgs.threadIdEn, polOutArgs.threadId);
            }
        }

        if (status == ENET_SOK)
        {
            memset(&delPolInArgs, 0, sizeof(delPolInArgs));
            delPolInArgs.policerMatch = polMatch;

            ENET_IOCTL_SET_IN_ARGS(&prms, &delPolInArgs);

            status = Enet_ioctl(hEnet, remoteCoreId, CPSW_ALE_IOCTL_DEL_POLICER, &prms);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                              "Invalid port %u policer flow %d", ENET_MACPORT_ID(macPort), flowIdx);
        }

        status = CPSWPROXY_ENET2RPMSG_ERR(status);
    }

    /* Remove unicast address */
    if (status == ETHREMOTECFG_CMDSTATUS_OK)
    {
        macAddrInfo.vlanId = 0U;
        EnetUtils_copyMacAddr(&macAddrInfo.addr[0U], macAddr);

        ENET_IOCTL_SET_IN_ARGS(&prms, &macAddrInfo);

        status = Enet_ioctl(hEnet, remoteCoreId, CPSW_ALE_IOCTL_REMOVE_ADDR, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                          "Port %u: failed to remove ucast entry", ENET_MACPORT_ID(macPort));

        status = CPSWPROXY_ENET2RPMSG_ERR(status);
    }

    return status;
}

static int32_t CpswProxyServer_registerRxDefaultHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                          uint32_t hostId,
                                                          uint32_t flowIdxBase,
                                                          uint32_t flowIdxOffset)
{
    CpswProxyServer_Obj *hServer = NULL;
    bool isSwitchPort;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    uint32_t coreKey;
    uint32_t relFlowIdx;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    /* Make sure that CpswProxyServer_registerRxDefaultHandlerCb is requested on primary flow */
    status = CpswProxyServer_getRelFlowIdx(hClient->virtPort, &relFlowIdx, flowIdxOffset);
    ETHFWTRACE_ERR_IF((status != ETHREMOTECFG_CMDSTATUS_OK), status,
                       "Failed to get relative flow index for virtual port %u flow %u",
                       hClient->virtPort,
                       flowIdxOffset);
    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        if(relFlowIdx != CPSWPROXY_CLIENT_PRIMARY_REL_FLOW_IDX)
        {
            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            ETHFWTRACE_ERR(status, "Failed to register rx default flow on extended flow, must be on primary flow");
        }
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        CpswProxyServer_validateStartIdx(hServer->hEnet, hostId, flowIdxBase);

        isSwitchPort = EthFwVirtPort_isSwitchPort(hClient->virtPort);
        if (isSwitchPort)
        {
            status = EnetAppUtils_regDfltRxFlow(hServer->hEnet,
                                                coreKey, 
                                                hostId, 
                                                flowIdxBase, 
                                                flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to register default flow");
            status = CPSWPROXY_ENET2RPMSG_ERR(status);
        }
        else
        {
            /* RX default flow is supported only on virtual switch ports */
            status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
            ETHFWTRACE_ERR(status, "Default flow setting is not supported on virtual MAC ports");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to register default flow for Rx flowId"
                       "base:%u, offset:%u", flowIdxBase, flowIdxOffset);
    }

    return status;
}

static int32_t CpswProxyServer_deregisterRxDefaultHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                            uint32_t hostId,
                                                            uint32_t flowIdxBase,
                                                            uint32_t flowIdxOffset)
{
    CpswProxyServer_Obj *hServer = NULL;
    bool isSwitchPort;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    uint32_t coreKey;
    uint32_t relFlowIdx;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer != NULL) && (hServer->initDone == BTRUE))
    {
        coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    /* Make sure that CpswProxyServer_deregisterRxDefaultHandlerCb is requested on primary flow */
    status = CpswProxyServer_getRelFlowIdx(hClient->virtPort, &relFlowIdx, flowIdxOffset);
    ETHFWTRACE_ERR_IF((status != ETHREMOTECFG_CMDSTATUS_OK), status,
                       "Failed to get relative flow index for virtual port %u flow %u",
                       hClient->virtPort,
                       flowIdxOffset);
    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        if(relFlowIdx != CPSWPROXY_CLIENT_PRIMARY_REL_FLOW_IDX)
        {
            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            ETHFWTRACE_ERR(status, "Failed to deregister rx default flow on extended flow, must be on primary flow");
        }
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        CpswProxyServer_validateStartIdx(hServer->hEnet, hostId, flowIdxBase);

        isSwitchPort = EthFwVirtPort_isSwitchPort(hClient->virtPort);
        if (isSwitchPort)
        {
            status = EnetAppUtils_unregDfltRxFlow(hServer->hEnet,
                                                  coreKey,
                                                  hostId, 
                                                  flowIdxBase, 
                                                  flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to register default flow");
            status = CPSWPROXY_ENET2RPMSG_ERR(status);
        }
        else
        {
            /* RX default flow is supported only on virtual switch ports */
            status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
            ETHFWTRACE_ERR(status, "Default flow setting is not supported on virtual MAC ports");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to de-register default flow for Rx flowId"
                       "base:%u, offset:%u", flowIdxBase, flowIdxOffset);
    }

    return status;
}

static void CpswProxyServer_printHostPortStats(CpswStats_HostPort_Ng *st)
{
    uint_fast32_t i;

    CPSWPROXY_PRINT_STATS_NONZERO("  rxGoodFrames            = %llu", st->rxGoodFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxBcastFrames           = %llu", st->rxBcastFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxMcastFrames           = %llu", st->rxMcastFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxCrcErrors             = %llu", st->rxCrcErrors);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxOversizedFrames       = %llu", st->rxOversizedFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxUndersizedFrames      = %llu", st->rxUndersizedFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxFragments             = %llu", st->rxFragments);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleDrop                 = %llu", st->aleDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleOverrunDrop          = %llu", st->aleOverrunDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxOctets                = %llu", st->rxOctets);
    CPSWPROXY_PRINT_STATS_NONZERO("  txGoodFrames            = %llu", st->txGoodFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txBcastFrames           = %llu", st->txBcastFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txMcastFrames           = %llu", st->txMcastFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txOctets                = %llu", st->txOctets);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames64          = %llu", st->octetsFrames64);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames65to127     = %llu", st->octetsFrames65to127);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames128to255    = %llu", st->octetsFrames128to255);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames256to511    = %llu", st->octetsFrames256to511);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames512to1023   = %llu", st->octetsFrames512to1023);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames1024        = %llu", st->octetsFrames1024);
    CPSWPROXY_PRINT_STATS_NONZERO("  netOctets               = %llu", st->netOctets);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxBottomOfFifoDrop      = %llu", st->rxBottomOfFifoDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  portMaskDrop            = %llu", st->portMaskDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxTopOfFifoDrop         = %llu", st->rxTopOfFifoDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleRateLimitDrop        = %llu", st->aleRateLimitDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleVidIngressDrop       = %llu", st->aleVidIngressDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleDAEqSADrop           = %llu", st->aleDAEqSADrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleBlockDrop            = %llu", st->aleBlockDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleSecureDrop           = %llu", st->aleSecureDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleAuthDrop             = %llu", st->aleAuthDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownUcast         = %llu", st->aleUnknownUcast);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownUcastBcnt     = %llu", st->aleUnknownUcastBcnt);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownMcast         = %llu", st->aleUnknownMcast);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownMcastBcnt     = %llu", st->aleUnknownMcastBcnt);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownBcast         = %llu", st->aleUnknownBcast);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownBcastBcnt     = %llu", st->aleUnknownBcastBcnt);
    CPSWPROXY_PRINT_STATS_NONZERO("  alePolicyMatch          = %llu", st->alePolicyMatch);
    CPSWPROXY_PRINT_STATS_NONZERO("  alePolicyMatchRed       = %llu", st->alePolicyMatchRed);
    CPSWPROXY_PRINT_STATS_NONZERO("  alePolicyMatchYellow    = %llu", st->alePolicyMatchYellow);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleMultSADrop           = %llu", st->aleMultSADrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleDualVlanDrop         = %llu", st->aleDualVlanDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleLenErrorDrop         = %llu", st->aleLenErrorDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleIpNextHdrDrop        = %llu", st->aleIpNextHdrDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleIPv4FragDrop         = %llu", st->aleIPv4FragDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietRxAssemblyErr        = %llu", st->ietRxAssemblyErr);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietRxAssemblyOk         = %llu", st->ietRxAssemblyOk);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietRxSmdError           = %llu", st->ietRxSmdError);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietRxFrag               = %llu", st->ietRxFrag);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietTxHold               = %llu", st->ietTxHold);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietTxFrag               = %llu", st->ietTxFrag);
    CPSWPROXY_PRINT_STATS_NONZERO("  txMemProtectError       = %llu", st->txMemProtectError);

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPri); i++)
    {
        CPSWPROXY_PRINT_STATS_IDX_NONZERO("  txPri[%u]                = %llu", i, st->txPri[i]);
    }

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPriBcnt); i++)
    {
        CPSWPROXY_PRINT_STATS_IDX_NONZERO("  txPriBcnt[%u]            = %llu", i, st->txPriBcnt[i]);
    }

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPriDrop); i++)
    {
        CPSWPROXY_PRINT_STATS_IDX_NONZERO("  txPriDrop[%u]            = %llu", i, st->txPriDrop[i]);
    }

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPriDropBcnt); i++)
    {
        CPSWPROXY_PRINT_STATS_IDX_NONZERO("  txPriDropBcnt[%u]        = %llu", i, st->txPriDropBcnt[i]);
    }
}

static void CpswProxyServer_printMacPortStats(CpswStats_MacPort_Ng *st)
{
    uint_fast32_t i;

    CPSWPROXY_PRINT_STATS_NONZERO("  rxGoodFrames            = %llu", st->rxGoodFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxBcastFrames           = %llu", st->rxBcastFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxMcastFrames           = %llu", st->rxMcastFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxPauseFrames           = %llu", st->rxPauseFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxCrcErrors             = %llu", st->rxCrcErrors);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxAlignCodeErrors       = %llu", st->rxAlignCodeErrors);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxOversizedFrames       = %llu", st->rxOversizedFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxJabberFrames          = %llu", st->rxJabberFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxUndersizedFrames      = %llu", st->rxUndersizedFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxFragments             = %llu", st->rxFragments);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleDrop                 = %llu", st->aleDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleOverrunDrop          = %llu", st->aleOverrunDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxOctets                = %llu", st->rxOctets);
    CPSWPROXY_PRINT_STATS_NONZERO("  txGoodFrames            = %llu", st->txGoodFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txBcastFrames           = %llu", st->txBcastFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txMcastFrames           = %llu", st->txMcastFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txPauseFrames           = %llu", st->txPauseFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txDeferredFrames        = %llu", st->txDeferredFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txCollisionFrames       = %llu", st->txCollisionFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txSingleCollFrames      = %llu", st->txSingleCollFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txMultipleCollFrames    = %llu", st->txMultipleCollFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txExcessiveCollFrames   = %llu", st->txExcessiveCollFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  txLateCollFrames        = %llu", st->txLateCollFrames);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxIPGError              = %llu", st->rxIPGError);
    CPSWPROXY_PRINT_STATS_NONZERO("  txCarrierSenseErrors    = %llu", st->txCarrierSenseErrors);
    CPSWPROXY_PRINT_STATS_NONZERO("  txOctets                = %llu", st->txOctets);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames64          = %llu", st->octetsFrames64);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames65to127     = %llu", st->octetsFrames65to127);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames128to255    = %llu", st->octetsFrames128to255);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames256to511    = %llu", st->octetsFrames256to511);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames512to1023   = %llu", st->octetsFrames512to1023);
    CPSWPROXY_PRINT_STATS_NONZERO("  octetsFrames1024        = %llu", st->octetsFrames1024);
    CPSWPROXY_PRINT_STATS_NONZERO("  netOctets               = %llu", st->netOctets);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxBottomOfFifoDrop      = %llu", st->rxBottomOfFifoDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  portMaskDrop            = %llu", st->portMaskDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  rxTopOfFifoDrop         = %llu", st->rxTopOfFifoDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleRateLimitDrop        = %llu", st->aleRateLimitDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleVidIngressDrop       = %llu", st->aleVidIngressDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleDAEqSADrop           = %llu", st->aleDAEqSADrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleBlockDrop            = %llu", st->aleBlockDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleSecureDrop           = %llu", st->aleSecureDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleAuthDrop             = %llu", st->aleAuthDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownUcast         = %llu", st->aleUnknownUcast);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownUcastBcnt     = %llu", st->aleUnknownUcastBcnt);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownMcast         = %llu", st->aleUnknownMcast);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownMcastBcnt     = %llu", st->aleUnknownMcastBcnt);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownBcast         = %llu", st->aleUnknownBcast);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleUnknownBcastBcnt     = %llu", st->aleUnknownBcastBcnt);
    CPSWPROXY_PRINT_STATS_NONZERO("  alePolicyMatch          = %llu", st->alePolicyMatch);
    CPSWPROXY_PRINT_STATS_NONZERO("  alePolicyMatchRed       = %llu", st->alePolicyMatchRed);
    CPSWPROXY_PRINT_STATS_NONZERO("  alePolicyMatchYellow    = %llu", st->alePolicyMatchYellow);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleMultSADrop           = %llu", st->aleMultSADrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleDualVlanDrop         = %llu", st->aleDualVlanDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleLenErrorDrop         = %llu", st->aleLenErrorDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleIpNextHdrDrop        = %llu", st->aleIpNextHdrDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  aleIPv4FragDrop         = %llu", st->aleIPv4FragDrop);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietRxAssemblyErr        = %llu", st->ietRxAssemblyErr);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietRxAssemblyOk         = %llu", st->ietRxAssemblyOk);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietRxSmdError           = %llu", st->ietRxSmdError);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietRxFrag               = %llu", st->ietRxFrag);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietTxHold               = %llu", st->ietTxHold);
    CPSWPROXY_PRINT_STATS_NONZERO("  ietTxFrag               = %llu", st->ietTxFrag);
    CPSWPROXY_PRINT_STATS_NONZERO("  txMemProtectError       = %llu", st->txMemProtectError);

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPri); i++)
    {
        CPSWPROXY_PRINT_STATS_IDX_NONZERO("  txPri[%u]                = %llu", i, st->txPri[i]);
    }

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPriBcnt); i++)
    {
        CPSWPROXY_PRINT_STATS_IDX_NONZERO("  txPriBcnt[%u]            = %llu", i, st->txPriBcnt[i]);
    }

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPriDrop); i++)
    {
        CPSWPROXY_PRINT_STATS_IDX_NONZERO("  txPriDrop[%u]            = %llu", i, st->txPriDrop[i]);
    }

    for (i = 0U; i < ENET_ARRAYSIZE(st->txPriDropBcnt); i++)
    {
        CPSWPROXY_PRINT_STATS_IDX_NONZERO("  txPriDropBcnt[%u]        = %llu", i, st->txPriDropBcnt[i]);
    }
}

static void CpswProxyServer_printStats(Enet_Handle hEnet,
                                       Enet_Type enetType,
                                       uint32_t coreId)
{
    Enet_IoctlPrms prms;
    Enet_MacPort portNum;
    CpswStats_PortStats portStats;
    int32_t status = ENET_SOK;
    uint32_t i;

    ENET_IOCTL_SET_OUT_ARGS(&prms, &portStats);
    status = Enet_ioctl(hEnet, coreId, ENET_STATS_IOCTL_GET_HOSTPORT_STATS, &prms);
    if (status == ENET_SOK)
    {
        ETHFWTRACE_INFO("");
        ETHFWTRACE_INFO(" Host Port Statistics");
        ETHFWTRACE_INFO("-----------------------------------------");
        switch (enetType)
        {
            case ENET_CPSW_2G:
            {
                CpswStats_HostPort_2g *st;

                st = (CpswStats_HostPort_2g *)&portStats;
                EnetAppUtils_printHostPortStats2G(st);
                break;
            }

            case ENET_CPSW_5G:
            case ENET_CPSW_9G:
            {
                CpswStats_HostPort_Ng *st;

                st = (CpswStats_HostPort_Ng *)&portStats;
                CpswProxyServer_printHostPortStats(st);
                break;
            }

            default:
            {
                EnetAppUtils_assert(BFALSE);
                break;
            }
        }

        ETHFWTRACE_INFO("");
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to get host stats");
    }

    if (status == ENET_SOK)
    {
        for (i = 0, portNum = ENET_MAC_PORT_FIRST; i < Enet_getMacPortMax(enetType, 0u); i++, portNum++)
        {
            ENET_IOCTL_SET_INOUT_ARGS(&prms, &portNum, &portStats);
            status = Enet_ioctl(hEnet, coreId, ENET_STATS_IOCTL_GET_MACPORT_STATS, &prms);
            if (status == ENET_SOK)
            {
                ETHFWTRACE_INFO("");
                ETHFWTRACE_INFO(" External Port %d Statistics", ENET_MACPORT_ID(portNum));
                ETHFWTRACE_INFO("-----------------------------------------");
                switch (enetType)
                {
                    case ENET_CPSW_2G:
                    {
                        CpswStats_MacPort_2g *st;

                        st = (CpswStats_MacPort_2g *)&portStats;
                        EnetAppUtils_printMacPortStats2G(st);
                        break;
                    }

                    case ENET_CPSW_5G:
                    case ENET_CPSW_9G:
                    {
                        CpswStats_MacPort_Ng *st;

                        st = (CpswStats_MacPort_Ng *)&portStats;
                        CpswProxyServer_printMacPortStats(st);
                        break;
                    }

                    default:
                    {
                        EnetAppUtils_assert(BFALSE);
                        break;
                    }
                }

                ETHFWTRACE_INFO("");
            }
            else
            {
                ETHFWTRACE_ERR(status, "Failed to get MAC %u stats", ENET_MACPORT_ID(portNum));
            }
        }
    }
}

static int32_t CpswProxyServer_isLinkUpCb(CpswProxyServer_ClientHandle hClient,
                                          uint32_t hostId,
                                          bool *isLinked,
                                          uint32_t *speed,
                                          uint32_t *duplex)
{
    CpswProxyServer_Obj *hServer = NULL;
    Enet_IoctlPrms prms;
    EnetPhy_GenericInArgs phyInArgs;
    EnetMacPort_LinkCfg phyOutArgs;
    bool isMacPort;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        *speed  = ENET_SPEED_10MBIT;
        *duplex = ENET_DUPLEX_HALF;

        isMacPort = EthFwVirtPort_isMacPort(hClient->virtPort);
        if (isMacPort)
        {
            phyInArgs.macPort = EthFwVirtPort_getMacPort(hClient->virtPort);

            ENET_IOCTL_SET_INOUT_ARGS(&prms, &phyInArgs, isLinked);

            memset(&phyOutArgs, 0, sizeof(phyOutArgs));

            status = Enet_ioctl(hServer->hEnet, hostId, ENET_PHY_IOCTL_IS_LINKED, &prms);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                            "Failed to get port %u link status", ENET_MACPORT_ID(phyInArgs.macPort));

            //FIXME: link speed/duplex IOCTL has to be PHY agnostics (i.e. MAC-to-MAC mode).
            if (*isLinked)
            {
                ENET_IOCTL_SET_INOUT_ARGS(&prms, &phyInArgs, &phyOutArgs);

                status = Enet_ioctl(hServer->hEnet, hostId, ENET_PHY_IOCTL_GET_LINK_MODE, &prms);
                ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                                "Failed to get port %u link params", ENET_MACPORT_ID(phyInArgs.macPort));

                if (status == ENET_SOK)
                {
                    *speed  = phyOutArgs.speed;
                    *duplex = phyOutArgs.duplexity;
                }
            }

            status = CPSWPROXY_ENET2RPMSG_ERR(status);
        }
        else
        {
            *isLinked = BTRUE;
            *speed    = ENET_SPEED_1GBIT;
            *duplex   = ENET_DUPLEX_FULL;
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to get the Link status");
    }

    return status;
}
static int32_t CpswProxyServer_regReadHandlerCb(uint32_t addr,
                                                uint32_t *val)
{
    *val = CSL_REG32_RD(addr);

    return ETHREMOTECFG_CMDSTATUS_OK;
}

static int32_t CpswProxyServer_regWriteHandlerCb(uint32_t reg,
                                                 uint32_t val)
{
    CSL_REG32_WR(reg, val);

    return ETHREMOTECFG_CMDSTATUS_OK;
}

static int32_t CpswProxyServer_registerEthertypeHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                          uint32_t hostId,
                                                          uint16_t etherType,
                                                          uint32_t flowIdxBase,
                                                          uint32_t flowIdxOffset)
{
    CpswProxyServer_Obj *hServer = NULL;
    Enet_IoctlPrms prms;
    CpswAle_SetPolicerEntryInArgs setPolicerInArgs;
    CpswAle_SetPolicerEntryOutArgs setPolicerOutArgs;
    bool isSwitchPort;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        isSwitchPort = EthFwVirtPort_isSwitchPort(hClient->virtPort);
        if (isSwitchPort)
        {
            memset(&setPolicerInArgs, 0, sizeof(setPolicerInArgs));
            setPolicerInArgs.policerMatch.policerMatchEnMask = CPSW_ALE_POLICER_MATCH_ETHERTYPE;
            setPolicerInArgs.policerMatch.etherType = etherType;
            setPolicerInArgs.threadIdEn = BTRUE;
            setPolicerInArgs.threadId   = flowIdxOffset;
            setPolicerInArgs.peakRateInBitsPerSec   = 0U;
            setPolicerInArgs.commitRateInBitsPerSec = 0U;

            ENET_IOCTL_SET_INOUT_ARGS(&prms, &setPolicerInArgs, &setPolicerOutArgs);

            status = Enet_ioctl(hServer->hEnet, hostId, CPSW_ALE_IOCTL_SET_POLICER, &prms);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                            "Failed to setup EtherType based route");
            status = CPSWPROXY_ENET2RPMSG_ERR(status);
        }
        else
        {
            status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
            ETHFWTRACE_ERR(status, "EtherType route is not supported on virtual MAC ports");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to register Ethertype: %u",etherType);
    }

    return status;
}

static int32_t CpswProxyServer_deregisterEthertypeHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                            uint32_t hostId,
                                                            uint16_t etherType)
{
    CpswProxyServer_Obj *hServer = NULL;
    Enet_IoctlPrms prms;
    CpswAle_DelPolicerEntryInArgs delPolicerInArgs;
    bool isSwitchPort;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        isSwitchPort = EthFwVirtPort_isSwitchPort(hClient->virtPort);
        if (!isSwitchPort)
        {
            memset(&delPolicerInArgs, 0, sizeof(delPolicerInArgs));
            delPolicerInArgs.policerMatch.policerMatchEnMask = CPSW_ALE_POLICER_MATCH_ETHERTYPE;
            delPolicerInArgs.policerMatch.etherType = etherType;
            delPolicerInArgs.aleEntryMask = CPSW_ALE_POLICER_TABLEENTRY_DELETE_ETHERTYPE;

            ENET_IOCTL_SET_IN_ARGS(&prms, &delPolicerInArgs);

            status = Enet_ioctl(hServer->hEnet, hostId, CPSW_ALE_IOCTL_DEL_POLICER, &prms);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                            "Failed to teardown EtherType based route");
            status = CPSWPROXY_ENET2RPMSG_ERR(status);
        }
        else
        {
            status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
            ETHFWTRACE_ERR(status, "EtherType route is not supported on virtual MAC ports");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to de-register Ethertype: %u",etherType);
    }

    return status;
}

static int32_t CpswProxyServer_registerRemoteTimerHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                            uint32_t hostId,
                                                            uint8_t timerId,
                                                            uint8_t hwPushNum)
{
    Enet_IoctlPrms prms;
    CpswCpts_RegisterHwPushCbInArgs hwPushCbInArgs;
    CpswProxyServer_Obj *hServer;
    uint32_t hwPushNorm = CPSW_CPTS_HWPUSH_NORM((CpswCpts_HwPush)hwPushNum);
    uint32_t instId = 0U;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (hwPushNum >= CPSW_CPTS_HWPUSH_COUNT_MAX)
    {
        status = ENET_EBADARGS;
        ETHFWTRACE_ERR(status, "Invalid HW push num %u", hwPushNum);
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        /* Register hardware push callback */
        if (status == ETHREMOTECFG_CMDSTATUS_OK)
        {
            hwPushCbInArgs.hwPushNum = (CpswCpts_HwPush)hwPushNum;
            hwPushCbInArgs.hwPushNotifyCb = CpswProxyServer_hwPushNotifyFxn;
            hwPushCbInArgs.hwPushNotifyCbArg = (void *)hServer;

            ENET_IOCTL_SET_IN_ARGS(&prms, &hwPushCbInArgs);

            status = Enet_ioctl(hServer->hEnet, hostId, CPSW_CPTS_IOCTL_REGISTER_HWPUSH_CALLBACK, &prms);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                            "Failed to register CPTS HW Push callback");
        }

        /* Configure timesync router */
        if (status == ENET_SOK)
        {
            status = EnetAppUtils_setTimeSyncRouter(hServer->enetType,
                                                    instId,
                                                    timerId,
                                                    hwPushNorm + CPSWPROXY_CPSW9G_HWPUSH_BASE);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to set TSR");
        }

        if (status == ENET_SOK)
        {
            hServer->notifyServiceObj.hwPush2CoreIdMap[hwPushNorm] = hostId;
            hServer->notifyServiceObj.hwPush2TokenMap[hwPushNorm] = hClient->token;
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to register remote timer");
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_unregisterRemoteTimerHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                              uint32_t hostId,
                                                              uint8_t hwPushNum)
{
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    Enet_IoctlPrms prms;
    CpswProxyServer_Obj *hServer;
    uint32_t hwPushNorm = CPSW_CPTS_HWPUSH_NORM((CpswCpts_HwPush)hwPushNum);
    uint32_t instId = 0U;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (hwPushNum >= CPSW_CPTS_HWPUSH_COUNT_MAX)
    {
        status = ENET_EBADARGS;
        ETHFWTRACE_ERR(status, "Invalid HW push num %u", hwPushNum);
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        /* Unregister hardware push callback */
        if (status == ENET_SOK)
        {
            hwPushNum = (CpswCpts_HwPush)hwPushNum;
            ENET_IOCTL_SET_IN_ARGS(&prms, &hwPushNum);
            status = Enet_ioctl(hServer->hEnet, hostId, CPSW_CPTS_IOCTL_UNREGISTER_HWPUSH_CALLBACK, &prms);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                            "Failed to deregister CPTS HW push callback");
        }

        /* Clear timesync router configuration for hardware push,
        * Note: This assumes input signal is stopped */
        if (status == ENET_SOK)
        {
            status = EnetAppUtils_setTimeSyncRouter(hServer->enetType,
                                                    instId,
                                                    0U,
                                                    hwPushNorm + CPSWPROXY_CPSW9G_HWPUSH_BASE);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to set TSR");
        }

        if (status == ENET_SOK)
        {
            /* Use IPC_MAX_PROCS as invalid core id */
            hServer->notifyServiceObj.hwPush2CoreIdMap[hwPushNorm] = IPC_MAX_PROCS;
            hServer->notifyServiceObj.hwPush2TokenMap[hwPushNorm] = ETHREMOTECFG_TOKEN_NONE;
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, " Failed to de-register remote timer");
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_ioctlHandlerCb(CpswProxyServer_ClientHandle hClient,
                                              uint32_t hostId,
                                              uint32_t cmd,
                                              const uint8_t *inargs,
                                              uint32_t inargsLen,
                                              uint8_t *outargs,
                                              uint32_t outargsLen)
{
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    CpswProxyServer_Obj *hServer;
    Enet_IoctlPrms prms;
    uint64_t inArgsBuf[(ETHREMOTECFG_IOCTL_INARGS_LEN/sizeof(uint64_t)) + 1];
    uint64_t outArgsBuf[(ETHREMOTECFG_IOCTL_OUTARGS_LEN/sizeof(uint64_t)) + 1];

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        /* Skip PHY link status check prints as they happen too often */
        if (cmd != ENET_PER_IOCTL_IS_PORT_LINK_UP)
        {
            prms.inArgsSize = inargsLen;
            prms.outArgsSize = outargsLen;
            EnetAppUtils_assert(inargsLen <= sizeof(inArgsBuf));
            /* To ensure structure are aligned, copy the inArgs to unit64_t aligned buffer */
            memcpy(inArgsBuf, inargs, inargsLen);
            prms.inArgs = inArgsBuf;
            /* To ensure structure are aligned, use unit64_t aligned buffer for outArgs  */
            EnetAppUtils_assert(outargsLen <= sizeof(outArgsBuf));
            prms.outArgs = outArgsBuf;
            if (prms.inArgsSize == 0)
            {
                prms.inArgs = NULL;
            }

            if (prms.outArgsSize == 0)
            {
                prms.outArgs = NULL;
            }

            status = Enet_ioctl(hServer->hEnet, hostId, cmd, &prms);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to run IOCTL 0x%x", cmd);

            if (status == ENET_SOK)
            {
                /* Copy the outArgs from temporary aligned buffer back to msg buffer */
                memcpy(outargs, outArgsBuf, outargsLen);
            }
            status = CPSWPROXY_ENET2RPMSG_ERR(status);
        }
        else
        {
            status = ETHREMOTECFG_CMDSTATUS_EBADARGS;
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Unsupported IOCTL cmd 0x%x", cmd);
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to perform the IOCTL command: %u", cmd);
    }

    return status;
}


static int32_t CpswProxyServer_filterAddMacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                     uint32_t hostId,
                                                     uint8_t *macAddr,
                                                     uint16_t vlanId,
                                                     uint32_t flowIdxBase,
                                                     uint32_t flowIdxOffset)
{
    CpswProxyServer_Obj *hServer = NULL;
    uint16_t hwVlanId = vlanId;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    uint32_t relFlowIdx;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    /* Make sure that CpswProxyServer_filterAddMacHandlerCb is requested on primary flow */
    status = CpswProxyServer_getRelFlowIdx(hClient->virtPort, &relFlowIdx, flowIdxOffset);
    ETHFWTRACE_ERR_IF((status != ETHREMOTECFG_CMDSTATUS_OK), status,
                       "Failed to get relative flow index for virtual port %u flow %u",
                       hClient->virtPort,
                       flowIdxOffset);
    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        if(relFlowIdx != CPSWPROXY_CLIENT_PRIMARY_REL_FLOW_IDX)
        {
            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            ETHFWTRACE_ERR(status, "Failed to add filter MAC on extended flow, must be on primary flow");
        }
    }

    if (!EnetUtils_isMcastAddr(macAddr))
    {
        status = ETHREMOTECFG_CMDSTATUS_EBADARGS;
        ETHFWTRACE_ERR(status, "Addr is not multicast, cannot add/delete to filter");
    }

    /* Check if client is part of the VLAN */
    if (status == ETHREMOTECFG_CMDSTATUS_OK)
    {
        /* If client is not in a VLAN, it will pass VLAN id 0 or VLAN_USE_DFLT,
         * but we need to use the actual default VLAN id */
        if ((vlanId == 0U) || (vlanId == ETHREMOTECFG_ETHSWITCH_VLAN_USE_DFLT))
        {
            vlanId = 0U;
            hwVlanId = hServer->dfltVlanIdSwitchPorts;
        }
        else
        {
            /* Check if the client has joined the VLAN */
            if (!EthFwVlan_isInVlan(hClient->virtPort, vlanId))
            {
                status = ETHREMOTECFG_CMDSTATUS_EBADARGS;
                ETHFWTRACE_ERR(status, "Virtual port %u is not part of VLAN %u", hClient->virtPort, vlanId);
            }
        }
    }

    /* Add multicast (shared or exclusive), reject reserved ones */
    if (status == ETHREMOTECFG_CMDSTATUS_OK)
    {
        status = EthFwMcast_filterAddMac(hClient->virtPort, hServer->hEnet,
                                         macAddr, vlanId, hwVlanId, flowIdxOffset, hostId);
        if (status != ETHFW_SOK)
        {
            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            ETHFWTRACE_ERR(status, "Failed to add multicast");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to add MacAddr:%02x:%02x:%02x:%02x:%02x:%02x to"
                       "the multicast filter", macAddr[0U], macAddr[1U], macAddr[2U],
                       macAddr[3U], macAddr[4U], macAddr[5U]);
    }

    return status;
}

static int32_t CpswProxyServer_filterDelMacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                     uint32_t hostId,
                                                     uint8_t *macAddr,
                                                     uint16_t vlanId)
{
    CpswProxyServer_Obj *hServer;
    uint16_t hwVlanId = vlanId;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (!EnetUtils_isMcastAddr(macAddr))
    {
        status = ETHREMOTECFG_CMDSTATUS_EBADARGS;
        ETHFWTRACE_ERR(status, "Addr is not multicast, cannot add/delete to filter");
    }

    /* Check if client is part of the VLAN */
    if (status == ETHREMOTECFG_CMDSTATUS_OK)
    {
        /* If client is not in a VLAN, it will pass VLAN id 0 or VLAN_USE_DFLT,
         * but we need to use the actual default VLAN id */
        if ((vlanId == 0U) || (vlanId == ETHREMOTECFG_ETHSWITCH_VLAN_USE_DFLT))
        {
            vlanId = 0U;
            hwVlanId = hServer->dfltVlanIdSwitchPorts;
        }
        else
        {
            /* Check if the client has joined the VLAN */
            if (!EthFwVlan_isInVlan(hClient->virtPort, vlanId))
            {
                status = ETHREMOTECFG_CMDSTATUS_EBADARGS;
                ETHFWTRACE_ERR(status, "Virtual port %u is not part of VLAN %u",
                               hClient->virtPort, vlanId);
            }
        }
    }

    /* Delete multicast (shared or exclusive), reject reserved ones */
    if (status == ETHREMOTECFG_CMDSTATUS_OK)
    {
        status = EthFwMcast_filterDelMac(hClient->virtPort, hServer->hEnet,
                                         macAddr, vlanId, hwVlanId, hostId);
        if (status != ETHFW_SOK)
        {
            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            ETHFWTRACE_ERR(status, "Failed to remove multicast");
        }
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to delete MacAddr:%02x:%02x:%02x:%02x:%02x:%02x to"
                       "the multicast filter", macAddr[0U], macAddr[1U], macAddr[2U],
                       macAddr[3U], macAddr[4U], macAddr[5U]);
    }

    return status;
}

int32_t CpswProxyServer_init(CpswProxyServer_Config_t *cfg)
{
    SemaphoreP_Params sem_params;
    CpswProxyServer_Obj *hServer;
    RPMessage_Params cntrlParam;
    EnetMcm_CmdIf *hMcmCmdIf;
    Enet_IoctlPrms prms;
    EnetMcm_HandleInfo handleInfo;
    bool csumEnable;
    int32_t i;
    int32_t status = ETHFW_SOK;

    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BFALSE));

    hServer->instId = cfg->instId;
    memset(&hServer->coreObj, 0, sizeof(hServer->coreObj));

    hServer->dfltVlanIdMacOnlyPorts = cfg->dfltVlanIdMacOnlyPorts;
    hServer->dfltVlanIdSwitchPorts  = cfg->dfltVlanIdSwitchPorts;

    hServer->alePortMask = cfg->enabledPortMask;
    hServer->aleMacOnlyPortMask = cfg->macOnlyPortMask;

    hServer->getMcmCmdIfCb = cfg->getMcmCmdIfCb;
    hServer->initEthfwDeviceDataCb = cfg->initEthfwDeviceDataCb;

    if ((hServer->aleMacOnlyPortMask & hServer->alePortMask) !=
        hServer->aleMacOnlyPortMask)
    {
        status = ETHFW_EFAIL;
        ETHFWTRACE_ERR(status, "MAC ports required for virtual MAC ports are not enabled");
    }

    hServer->aleSwitchOnlyPortMask = (hServer->alePortMask &
                                      ~hServer->aleMacOnlyPortMask);

    /* Get MCM cmd handle */
    EnetAppUtils_assert(hServer->getMcmCmdIfCb != NULL);
    hServer->getMcmCmdIfCb(hServer->enetType, &hMcmCmdIf);
    EnetAppUtils_assert(hMcmCmdIf != NULL);
    EnetAppUtils_assert(hMcmCmdIf->hMboxCmd != NULL);
    EnetAppUtils_assert(hMcmCmdIf->hMboxResponse != NULL);
    hServer->hMcmCmdIf = hMcmCmdIf;

    /* Connect to MCM on client's behalf (using its hostId) */
    EnetMcm_acquireHandleInfo(hMcmCmdIf, &handleInfo);
    hServer->hEnet = handleInfo.hEnet;

    ENET_IOCTL_SET_OUT_ARGS(&prms, &csumEnable);
    status = Enet_ioctl(handleInfo.hEnet, hServer->masterCoreId,
                        ENET_HOSTPORT_IS_CSUM_OFFLOAD_ENABLED, &prms);
    if (ENET_SOK == status)
    {
        hServer->csumOffloadEn = csumEnable;
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ENET_HOSTPORT_IS_CSUM_OFFLOAD_ENABLED IOCTL failed");
    }

    memcpy(&hServer->allocObj, &cfg->allocObj, sizeof(cfg->allocObj));

    if (status == ETHFW_SOK)
    {
        hServer->hMutex = MutexP_create(&hServer->mutexObj);

        SemaphoreP_Params_init(&sem_params);
        sem_params.mode = SemaphoreP_Mode_BINARY;
        hServer->clientServiceObj.rpmsgStartSem = SemaphoreP_create(0, &sem_params);
        EnetAppUtils_assert(hServer->clientServiceObj.rpmsgStartSem != NULL);

        hServer->numVirtPorts = cfg->numVirtPorts;
        for (i = 0U; i < hServer->numVirtPorts; i++)
        {
            hServer->virtPortCfg[i] = cfg->virtPortCfg[i];
        }

        CpswProxyServer_initClientHandle(cfg);

        ETHFWTRACE_INFO("Virtual port configuration:");

        EnetAppUtils_assert(cfg->autosarEthVirtPortNum <= ENET_ARRAYSIZE(cfg->autosarPortCfg));
        EnetAppUtils_assert(cfg->autosarEthVirtPortNum <= ENET_ARRAYSIZE(g_CpswProxyServerAutosarRpmsgBuf));
        EnetAppUtils_assert(cfg->autosarEthVirtPortNum <= ENET_ARRAYSIZE(hServer->ethDrvObj));
        EnetAppUtils_assert(cfg->autosarEthVirtPortNum <= ENET_ARRAYSIZE(gCpswProxyServer_autosarEthDriverTaskStackBuf));

        for (i = 0U; i < cfg->autosarEthVirtPortNum; i++)
        {
            status = CpswProxyServer_initAutosarEthDeviceEp(hServer, cfg, i);
            EnetAppUtils_assert(status == ETHFW_SOK);
        }

        status = CpswProxyServer_initRemoteClientEthDeviceEp(hServer, cfg);
        EnetAppUtils_assert(status == ETHFW_SOK);

        status = CpswProxyServer_initNotifyServiceEp(hServer, cfg);
        EnetAppUtils_assert(status == ETHFW_SOK);

        hServer->initDone = BTRUE;
    }

    ETHFWTRACE_INFO("CpswProxyServer: initialization %s (core: mcu2_0)",
                    (status == ETHFW_SOK) ? "completed" : "failed");

    return status;
}

static int32_t CpswProxyServer_dumpStatsCb(CpswProxyServer_ClientHandle hClient,
                                           uint32_t hostId)
{
    CpswProxyServer_Obj *hServer;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    Enet_IoctlPrms prms;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    if ((hServer == NULL) || (hServer->initDone != BTRUE))
    {
        status = ETHREMOTECFG_CMDSTATUS_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW server is not ready");
        EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));
    }

    if (ETHREMOTECFG_CMDSTATUS_OK == status)
    {
        ENET_IOCTL_SET_NO_ARGS(&prms);
        status = Enet_ioctl(hServer->hEnet, hostId, CPSW_ALE_IOCTL_DUMP_TABLE, &prms);
        EnetAppUtils_assert(status == ENET_SOK);

        ENET_IOCTL_SET_NO_ARGS(&prms);
        status = Enet_ioctl(hServer->hEnet, hostId, CPSW_ALE_IOCTL_DUMP_POLICER_ENTRIES, &prms);
        EnetAppUtils_assert(status == ENET_SOK);

        CpswProxyServer_printStats(hServer->hEnet, hServer->enetType, hostId);
    }
    else
    {
        ETHFWTRACE_ERR(status, "Failed to dump stats for coreId:%u", hostId);
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static void CpswProxyServer_clientRequestHandler(RPMessage_Handle hMsgHandle,
                                                 const void *reqBuf,
                                                 uint8_t clientId,
                                                 uint32_t remoteProcId,
                                                 uint32_t remoteEndPt,
                                                 uint32_t localEp)
{
    uint64_t resBuf[ETHREMOTECFG_IPC_MSG_SIZE / sizeof(uint64_t)];
    CpswProxyServer_ClientHandle hClient = NULL;
    EthRemoteCfg_ReqHdr *reqHdr = (EthRemoteCfg_ReqHdr *)reqBuf;  /* FIXME - make it const */
    EthRemoteCfg_ResHdr *resHdr = (EthRemoteCfg_ResHdr *)resBuf;
    uint16_t resLen = 0U;
    int32_t rtnVal = IPC_SOK;
    int32_t status = ENET_SOK;
    uint32_t token;

    token = reqHdr->common.token;

    switch (reqHdr->reqType)
    {
        case ETHREMOTECFG_CMD_VIRT_PORT_INFO:
        {
            EthRemoteCfg_OfferVirtPortRes *res = (EthRemoteCfg_OfferVirtPortRes *)resBuf;

            ETHFWTRACE_INFO("VIRT_PORT_INFO | C2S | core=%u endpt=%u",
                            remoteProcId, remoteEndPt);

            status = CpswProxyServer_VirtPortAllocCb(clientId,
                                                     remoteProcId,
                                                     &res->switchPortMask,
                                                     &res->macPortMask);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                              "Failed to get virtual port allocation");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("VIRT_PORT_INFO | S2C | switchPortMask=%x macPortMask=%x",
                            res->switchPortMask, res->macPortMask);
            break;
        }
        case ETHREMOTECFG_CMD_ATTACH:
        {
            EthRemoteCfg_AttachReq *req = (EthRemoteCfg_AttachReq *)reqBuf;
            EthRemoteCfg_AttachRes *res = (EthRemoteCfg_AttachRes *)resBuf;

            ETHFWTRACE_INFO("ATTACH | C2S | core=%u endpt=%u virtPort=%u",
                            remoteProcId, remoteEndPt, req->virtPort);

            /* Allocate a client object */
            hClient = CpswProxyServer_allocClient(remoteProcId, remoteEndPt, req->virtPort);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_attachHandlerCb(hClient,
                                                     remoteProcId,
                                                     req->virtPort,
                                                     &res->rxMtu,
                                                     &res->txMtu[0U],
                                                     ENET_ARRAYSIZE(res->txMtu),
                                                     &res->features,
                                                     &res->numTxCh,
                                                     &res->numRxFlow);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                              "Failed to attach virtual port %u core %u",
                              req->virtPort, remoteProcId);

            token = hClient->token;
            hClient->clientId = clientId;
            resLen = sizeof(EthRemoteCfg_AttachRes);

            ETHFWTRACE_INFO("ATTACH | S2C | token=%d rxMtu=%u features=%x",
                            (int32_t)token, res->rxMtu, res->features);
            break;
        }
        case ETHREMOTECFG_CMD_ATTACH_EXT:
        {
            EthRemoteCfg_AttachReq *req = (EthRemoteCfg_AttachReq *)reqBuf;
            EthRemoteCfg_AttachExtRes *res = (EthRemoteCfg_AttachExtRes *)resBuf;

            ETHFWTRACE_INFO("ATTACH_EXT | C2S | core=%u endpt=%u virtPort=%u",
                            remoteProcId, remoteEndPt, req->virtPort);

            /* Allocate a client object */
            hClient = CpswProxyServer_allocClient(remoteProcId, remoteEndPt, req->virtPort);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_attachExtHandlerCb(hClient,
                                                        remoteProcId,
                                                        req->virtPort,
                                                        &res->rxMtu,
                                                        res->txMtu,
                                                        ENET_ARRAYSIZE(res->txMtu),
                                                        &res->features,
                                                        &res->rxFlowIdxBase,
                                                        &res->rxFlowIdxOffset,
                                                        &res->txPsilDstId,
                                                        &res->rxPsilSrcId,
                                                        &res->macAddr[0U]);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                              "Failed to attach (ext) virtual port %u core %u",
                              req->virtPort, remoteProcId);

            token = hClient->token;
            hClient->clientId = clientId;
            resLen = sizeof(EthRemoteCfg_AttachExtRes);

            ETHFWTRACE_INFO("ATTACH_EXT | S2C | token=%d rxMtu=%u features=%x flow=%u,%u "
                            "rxPsil=0x%x txPsil=0x%x macAddr=%02x:%02x:%02x:%02x:%02x:%02x",
                            (int32_t)token, res->rxMtu, res->features,
                            res->rxFlowIdxBase, res->rxFlowIdxOffset,
                            res->rxPsilSrcId, res->txPsilDstId,
                            res->macAddr[0U], res->macAddr[1U], res->macAddr[2U],
                            res->macAddr[3U], res->macAddr[4U], res->macAddr[5U]);
            break;
        }
        case ETHREMOTECFG_CMD_DETACH:
        {
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DETACH | C2S | core=%u endpt=%u token=%d",
                            remoteProcId, remoteEndPt, (int32_t)token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_detachHandlerCb(hClient,
                                                     remoteProcId);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                              "Failed to detach virtual port %u core %u",
                              hClient->virtPort, remoteProcId);

            CpswProxyServer_freeClient(hClient);
            resLen = sizeof(*res);

            ETHFWTRACE_INFO("DETACH | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_ALLOC_TX:
        {
            EthRemoteCfg_AllocTxReq *req = (EthRemoteCfg_AllocTxReq *)reqBuf;
            EthRemoteCfg_AllocTxRes *res = (EthRemoteCfg_AllocTxRes *)resBuf;

            ETHFWTRACE_INFO("ALLOC_TX | C2S | core=%u endpt=%u token=%d",
                            remoteProcId, remoteEndPt, (int32_t)token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_allocTxHandlerCb(hClient,
                                                      remoteProcId,
                                                      &res->txPsilDstId,
                                                      req->chRelPriority);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to alloc TX channel");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("ALLOC_TX | S2C | txPsil=0x%x status=%d",
                            res->txPsilDstId, status);
            break;
        }
        case ETHREMOTECFG_CMD_ALLOC_RX:
        {
            EthRemoteCfg_AllocRxReq *req = (EthRemoteCfg_AllocRxReq *)reqBuf;
            EthRemoteCfg_AllocRxRes *res = (EthRemoteCfg_AllocRxRes *)resBuf;

            ETHFWTRACE_INFO("ALLOC_RX | C2S | core=%u endpt=%u token=%d",
                            remoteProcId, remoteEndPt, (int32_t)token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_allocRxHandlerCb(hClient,
                                                      remoteProcId,
                                                      &res->rxFlowIdxBase,
                                                      &res->rxFlowIdxOffset,
                                                      &res->rxPsilSrcId,
                                                      req->flowIdx);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to alloc RX flow");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("ALLOC_RX | S2C | flow=%u,%u rxPsil=0x%x status=%d",
                            res->rxFlowIdxBase, res->rxFlowIdxOffset, res->rxPsilSrcId, status);
            break;
        }
        case ETHREMOTECFG_CMD_ALLOC_MAC:
        {
            EthRemoteCfg_AllocMacRes *res = (EthRemoteCfg_AllocMacRes *)resBuf;

            ETHFWTRACE_INFO("ALLOC_MAC | C2S | core=%u endpt=%u token=%d",
                            remoteProcId, remoteEndPt, (int32_t)token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_allocMacHandlerCb(hClient,
                                                      remoteProcId,
                                                      &res->macAddr[0U]);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to alloc MAC addr");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("ALLOC_MAC | S2C | macAddr=%02x:%02x:%02x:%02x:%02x:%02x status=%d",
                            res->macAddr[0U], res->macAddr[1U], res->macAddr[2U],
                            res->macAddr[3U], res->macAddr[4U], res->macAddr[5U],
                            status);
            break;
        }
        case ETHREMOTECFG_CMD_FREE_TX:
        {
            EthRemoteCfg_FreeTxReq *req = (EthRemoteCfg_FreeTxReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("FREE_TX | C2S | core=%u endpt=%u token=%d txPsil=0x%x",
                            remoteProcId, remoteEndPt, (int32_t)token, req->txPsilDstId);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_freeTxHandlerCb(hClient,
                                                     remoteProcId,
                                                     req->txPsilDstId);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to free TX channel");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("FREE_TX | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_FREE_RX:
        {
            EthRemoteCfg_FreeRxReq *req = (EthRemoteCfg_FreeRxReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("FREE_RX | C2S | core=%u endpt=%u token=%d flowidx=%u,%u",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->rxFlowIdxBase, req->rxFlowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_freeRxHandlerCb(hClient,
                                                     remoteProcId,
                                                     req->rxFlowIdxBase,
                                                     req->rxFlowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to free RX flow");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("FREE_RX | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_FREE_MAC:
        {
            EthRemoteCfg_FreeMacReq *req = (EthRemoteCfg_FreeMacReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("FREE_MAC | C2S | core=%u endpt=%u token=%d "
                            "macAdd=%02x:%02x:%02x:%02x:%02x:%02x",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U]);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_freeMacHandlerCb(hClient,
                                                     remoteProcId,
                                                     req->macAddr);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to free MAC addr");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("FREE_MAC | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_REGISTER_MAC:
        {
            EthRemoteCfg_MacAddrRxFlowReq *req = (EthRemoteCfg_MacAddrRxFlowReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("REGISTER_MAC | C2S | core=%u endpt=%u token=%d "
                            "macAdd=%02x:%02x:%02x:%02x:%02x:%02x flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_registerMacHandlerCb(hClient,
                                                          remoteProcId,
                                                          req->macAddr,
                                                          req->flowIdxBase,
                                                          req->flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to register MAC addr");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("REGISTER_MAC | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_DEREGISTER_MAC:
        {
            EthRemoteCfg_MacAddrRxFlowReq *req = (EthRemoteCfg_MacAddrRxFlowReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DEREGISTER_MAC | C2S | core=%u endpt=%u token=%d "
                            "macAdd=%02x:%02x:%02x:%02x:%02x:%02x flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_unregisterMacHandlerCb(hClient,
                                                            remoteProcId,
                                                            req->macAddr,
                                                            req->flowIdxBase,
                                                            req->flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to unregister MAC addr");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("DEREGISTER_MAC | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_REGISTER_IPv4:
        {
            EthRemoteCfg_IPv4AddrRegisterReq *req = (EthRemoteCfg_IPv4AddrRegisterReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("REGISTER_IPv4 | C2S | core=%u endpt=%u token=%d "
                            "ipAddr=%u.%u.%u.%u macAdd=%02x:%02x:%02x:%02x:%02x:%02x",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->ipAddr[0U], req->ipAddr[1U], req->ipAddr[2U], req->ipAddr[3U],
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U]);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_registerIPv4MacHandlerCb(hClient,
                                                              remoteProcId,
                                                              req->macAddr,
                                                              req->ipAddr);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to register IPv4 addr");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("REGISTER_IPv4 | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_DEREGISTER_IPv4:
        {
            EthRemoteCfg_IPv4AddrDeregisterReq *req = (EthRemoteCfg_IPv4AddrDeregisterReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DEREGISTER_IPv4 | C2S | core=%u endpt=%u token=%d ipAddr=%u.%u.%u.%u",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->ipAddr[0U], req->ipAddr[1U], req->ipAddr[2U], req->ipAddr[3U]);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_deregisterIPv4MacHandlerCb(hClient,
                                                                remoteProcId,
                                                                req->ipAddr);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to deregister IPv4 addr");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("DEREGISTER_IPv4 | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_JOIN_VLAN:
        {
            EthRemoteCfg_VlanJoinReq *req = (EthRemoteCfg_VlanJoinReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("JOIN_VLAN | C2S | core=%u endpt=%u token=%d vlanId=%u "
                            "macAdd=%02x:%02x:%02x:%02x:%02x:%02x flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, (int32_t)token, req->vlanId,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_vlanJoinHandlerCb(hClient,
                                                       remoteProcId,
                                                       req->vlanId,
                                                       req->macAddr,
                                                       req->flowIdxBase,
                                                       req->flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to join VLAN %u", req->vlanId);

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("JOIN_VLAN | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_LEAVE_VLAN:
        {
            EthRemoteCfg_VlanLeaveReq *req = (EthRemoteCfg_VlanLeaveReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("LEAVE_VLAN | C2S | core=%u endpt=%u token=%d vlanId=%u "
                            "macAdd=%02x:%02x:%02x:%02x:%02x:%02x flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, (int32_t)token, req->vlanId,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_vlanLeaveHandlerCb(hClient,
                                                        remoteProcId,
                                                        req->vlanId,
                                                        req->macAddr,
                                                        req->flowIdxBase,
                                                        req->flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to leave VLAN %u", req->vlanId);

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("LEAVE_VLAN | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_ENABLE_PROMISC:
        {
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("ENABLE_PROMISC | C2S | core=%u endpt=%u token=%d",
                            remoteProcId, remoteEndPt, (int32_t)token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_promiscModeHandlerCb(hClient,
                                                          remoteProcId,
                                                          BTRUE);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to enable promisc mode");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("ENABLE_PROMISC | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_DISABLE_PROMISC:
        {
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DISABLE_PROMISC | C2S | core=%u endpt=%u token=%d",
                            remoteProcId, remoteEndPt, (int32_t)token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_promiscModeHandlerCb(hClient,
                                                          remoteProcId,
                                                          BFALSE);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to disable promisc mode");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("DISABLE_PROMISC | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_SET_RX_DEFAULTFLOW:
        {
            EthRemoteCfg_RxDefaultFlowRegisterReq *req = (EthRemoteCfg_RxDefaultFlowRegisterReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("SET_RX_DEFAULTFLOW | C2S | core=%u endpt=%u token=%d flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_registerRxDefaultHandlerCb(hClient,
                                                                remoteProcId,
                                                                req->flowIdxBase,
                                                                req->flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to register RX default flow");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("SET_RX_DEFAULTFLOW | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_DEL_RX_DEFAULTFLOW:
        {
            EthRemoteCfg_RxDefaultFlowRegisterReq *req = (EthRemoteCfg_RxDefaultFlowRegisterReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DEL_RX_DEFAULTFLOW | C2S | core=%u endpt=%u token=%d flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_deregisterRxDefaultHandlerCb(hClient,
                                                                  remoteProcId,
                                                                  req->flowIdxBase,
                                                                  req->flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to deregister RX default flow");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("DEL_RX_DEFAULTFLOW | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_REGISTER_MATCH_ETHTYPE:
        {
            EthRemoteCfg_MatchEthertypeAddReq *req = (EthRemoteCfg_MatchEthertypeAddReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("REGISTER_MATCH_ETHTYPE | C2S | core=%u endpt=%u token=%d "
                            "ethType=%x flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->ethertype, req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_registerEthertypeHandlerCb(hClient,
                                                                remoteProcId,
                                                                req->ethertype,
                                                                req->flowIdxBase,
                                                                req->flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to register EthType flow");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("REGISTER_MATCH_ETHTYPE | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_DEREGISTER_MATCH_ETHTYPE:
        {
            EthRemoteCfg_MatchEthertypeDelReq *req = (EthRemoteCfg_MatchEthertypeDelReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DEREGISTER_MATCH_ETHTYPE | C2S | core=%u endpt=%u token=%d ethType=%x",
                            remoteProcId, remoteEndPt, (int32_t)token, req->ethertype);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_deregisterEthertypeHandlerCb(hClient,
                                                                  remoteProcId,
                                                                  req->ethertype);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to deregister EthType flow");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("DEREGISTER_MATCH_ETHTYPE | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_ADD_FILTER_MAC:
        {
            EthRemoteCfg_FilterMacAddReq *req = (EthRemoteCfg_FilterMacAddReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("ADD_FILTER_MAC | C2S | core=%u endpt=%u token=%d "
                            "macAdd=%02x:%02x:%02x:%02x:%02x:%02x vlanId=%u flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->vlanId, req->flowIdxBase,req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_filterAddMacHandlerCb(hClient,
                                                           remoteProcId,
                                                           req->macAddr,
                                                           req->vlanId,
                                                           req->flowIdxBase,
                                                           req->flowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to add mcast to filter");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("ADD_FILTER_MAC | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_DEL_FILTER_MAC:
        {
            EthRemoteCfg_FilterMacDelReq *req = (EthRemoteCfg_FilterMacDelReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DEL_FILTER_MAC | C2S | core=%u endpt=%u token=%d "
                            "macAdd=%02x:%02x:%02x:%02x:%02x:%02x vlanId=%u",
                            remoteProcId, remoteEndPt, (int32_t)token,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->vlanId);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_filterDelMacHandlerCb(hClient,
                                                           remoteProcId,
                                                           req->macAddr,
                                                           req->vlanId);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to delete mcast from filter");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("DEL_FILTER_MAC | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_PORT_LINK_STATUS:
        {
            EthRemoteCfg_PortLinkStatusRes *res = (EthRemoteCfg_PortLinkStatusRes *)resBuf;

            ETHFWTRACE_VERBOSE("PORT_LINK_STATUS | C2S | core=%u endpt=%u token=%d",
                               remoteProcId, remoteEndPt, (int32_t)token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_isLinkUpCb(hClient,
                                                remoteProcId,
                                                &res->isLinked,
                                                &res->speed,
                                                &res->duplexity);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to get port link params");

            resLen = sizeof(*res);

            ETHFWTRACE_VERBOSE("PORT_LINK_STATUS | S2C | isLinked=%u speed=%u duplex=%u status=%d",
                               res->isLinked, res->speed, res->duplexity, status);
            break;
        }
        case ETHREMOTECFG_CMD_READ_REGISTER:
        {
            EthRemoteCfg_RegReadReq *req = (EthRemoteCfg_RegReadReq *)reqBuf;
            EthRemoteCfg_RegReadRes *res = (EthRemoteCfg_RegReadRes *)resBuf;

            ETHFWTRACE_INFO("READ_REGISTER | C2S | core=%u endpt=%u reg=0x%08x",
                            remoteProcId, remoteEndPt, req->addr);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_regReadHandlerCb(req->addr,
                                                      &res->val);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to read register");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("READ_REGISTER | S2C | val=0x%08x", res->val);
            break;
        }
        case ETHREMOTECFG_CMD_WRITE_REGISTER:
        {
            EthRemoteCfg_RegWriteReq *req = (EthRemoteCfg_RegWriteReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("WRITE_REGISTER | C2S | core=%u endpt=%u reg=0x%08x val=0x%08x",
                            remoteProcId, remoteEndPt, req->addr, req->val);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_regWriteHandlerCb(req->addr,
                                                       req->val);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to write register");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("WRITE_REGISTER | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_REGISTER_REMOTE_TIMER:
        {
            EthRemoteCfg_RemoteTimerRegisterReq *req = (EthRemoteCfg_RemoteTimerRegisterReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("REGISTER_REMOTE_TIMER | C2S | core=%u endpt=%u hwPushNum=%u timerId=%u",
                            remoteProcId, remoteEndPt, req->hwPushNum, req->timerId);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_registerRemoteTimerHandlerCb(hClient,
                                                                  remoteProcId,
                                                                  req->timerId,
                                                                  req->hwPushNum);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to register remote timer");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("REGISTER_REMOTE_TIMER | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_DEREGISTER_REMOTE_TIMER:
        {
            EthRemoteCfg_RemoteTimerDeregisterReq *req = (EthRemoteCfg_RemoteTimerDeregisterReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DEREGISTER_REMOTE_TIMER | C2S | core=%u endpt=%u hwPushNum=%u",
                            remoteProcId, remoteEndPt, req->hwPushNum);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_unregisterRemoteTimerHandlerCb(hClient,
                                                                    remoteProcId,
                                                                    req->hwPushNum);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to unregister remote timer");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("DEREGISTER_REMOTE_TIMER | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_GET_SERVER_STATUS:
        {
            EthRemoteCfg_ServerStatusRes *res = (EthRemoteCfg_ServerStatusRes *)resBuf;

            ETHFWTRACE_INFO("GET_SERVER_STATUS | C2S | core=%u endpt=%u",
                            remoteProcId, remoteEndPt);

            res->status = ETHREMOTECFG_SERVERSTATUS_INIT;
            status = ETHREMOTECFG_CMDSTATUS_OK;

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("GET_SERVER_STATUS | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_TEARDOWN_COMPLETION:
        {
            EthRemoteCfg_CommonReq *req = (EthRemoteCfg_CommonReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("TEARDOWN_COMPLETION | C2S | core=%u endpt=%u token=%d",
                            remoteProcId, remoteEndPt, (int32_t)token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            hClient->isIdle = BTRUE;
            resLen = sizeof(*res);
            status = ETHREMOTECFG_CMDSTATUS_OK;

            ETHFWTRACE_INFO("TEARDOWN_COMPLETION | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_IOCTL:
        {
            EthRemoteCfg_IoctlReq *req = (EthRemoteCfg_IoctlReq *)reqBuf;
            EthRemoteCfg_IoctlRes *res = (EthRemoteCfg_IoctlRes *)resBuf;

            ETHFWTRACE_INFO("IOCTL | C2S | core=%u endpt=%u cmd=%x inArgsLen=%u inArgs=%p outArgsLen=%u",
                            remoteProcId, remoteEndPt, req->cmd,
                            req->inArgsLen, req->inArgs, req->outArgsLen);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status =  CpswProxyServer_ioctlHandlerCb(hClient,
                                                     remoteProcId,
                                                     req->cmd,
                                                     (const uint8_t *)req->inArgs,
                                                     req->inArgsLen,
                                                     (uint8_t *)res->outArgs,
                                                     req->outArgsLen);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to run IOCTL cmd %x", req->cmd);

            /* Set IOCTL cmd and outArgs len in response so that it can be processed on client
             * side without keeping track of IOCTL response belongs to which IOCTL request */
            res->cmd = req->cmd;
            res->outArgsLen = req->outArgsLen;
            resLen = sizeof(*res);

            ETHFWTRACE_INFO("IOCTL | S2C | cmd=%x outArgs=%u status=%d",
                            res->cmd, res->outArgsLen, status);
            break;
        }
        case ETHREMOTECFG_CMD_DUMP:
        {
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DUMP | C2S | core=%u endpt=%u",
                            remoteProcId, remoteEndPt);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(remoteProcId, token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_dumpStatsCb(hClient, remoteProcId);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to dump stats");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("DUMP | S2C | status=%d", status);
            break;
        }
        default:
        {
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            resLen = sizeof(*res);
            ETHFWTRACE_ERR(status, "Unknown cmd %u", reqHdr->reqType);
            break;
        }
    }

    resHdr->common.msgType  = ETHREMOTECFG_MSGTYPE_RESPONSE;
    resHdr->common.token    = token;
    resHdr->common.clientId = clientId;
    resHdr->resType         = reqHdr->reqType;
    resHdr->resId           = reqHdr->reqId;
    resHdr->status          = status;

    ETHFWTRACE_DBG_IF((resHdr->resType != ETHREMOTECFG_CMD_PORT_LINK_STATUS),
                      "S2C | msgType=%u token=%d clientId=%u resId=%u status=%d len=%u (%u.%u->%u.%u)",
                      resHdr->common.msgType,
                      (int32_t)resHdr->common.token,
                      resHdr->common.clientId,
                      resHdr->resId,
                      resHdr->status,
                      resLen,
                      EnetSoc_getCoreId(), localEp,
                      remoteProcId, remoteEndPt);

    ETHFWTRACE_VERBOSE_IF((resHdr->resType == ETHREMOTECFG_CMD_PORT_LINK_STATUS),
                          "S2C | msgType=%u token=%d clientId=%u resId=%u status=%d len=%u (%u.%u->%u.%u)",
                          resHdr->common.msgType,
                          (int32_t)resHdr->common.token,
                          resHdr->common.clientId,
                          resHdr->resId,
                          resHdr->status,
                          resLen,
                          EnetSoc_getCoreId(), localEp,
                          remoteProcId, remoteEndPt);

    rtnVal = RPMessage_send(hMsgHandle, remoteProcId, remoteEndPt, localEp, &resBuf, resLen);
    ETHFWTRACE_ERR_IF((rtnVal != IPC_SOK), rtnVal, "Failed to send msg via IPC");
    EnetAppUtils_assert(IPC_SOK == rtnVal);
}

static int32_t CpswProxyServer_initRemoteClientEthDeviceEp(CpswProxyServer_Obj *hServer,
                                                           CpswProxyServer_Config_t * cfg)
{
    TaskP_Params taskParams;
    int32_t status = ETHFW_SOK;
    RPMessage_Params comParams;
    uint32_t  localEp;

    /* Initialize the param and set memory for HeapMemory for control task */
    RPMessageParams_init(&comParams);
    comParams.buf = gCpswProxyServerRpmsgbuf;
    comParams.bufSize = sizeof(gCpswProxyServerRpmsgbuf);
    comParams.numBufs = ETHREMOTECFG_IPC_NUM_MSG_BUFS;

    hServer->clientServiceObj.hClientServicRpMsgEp = RPMessage_create(&comParams, &localEp);
    if (NULL == hServer->clientServiceObj.hClientServicRpMsgEp)
    {
        status = ETHFW_EFAIL;
        ETHFWTRACE_ERR(status, "Could not create communication channel for endpoint %d",
                       comParams.requestedEndpt);
    }
    else
    {
        SemaphoreP_post(hServer->clientServiceObj.rpmsgStartSem);
    }

    if (ETHFW_SOK == status)
    {
        hServer->clientServiceObj.localEp = localEp;
    }

    if (ETHFW_SOK == status)
    {
        /* Initialize the task params */
        TaskP_Params_init(&taskParams);
        taskParams.name         = CPSWPROXY_ETH_CLIENT_TASK_NAME;
        taskParams.priority     = CPSWPROXY_ETH_CLIENT_TASK_PRIORITY;
        taskParams.arg0         = (void*) hServer;
        taskParams.stack        = &gCpswProxyServer_remoteClientEthDriverTaskStackBuf[0];
        taskParams.stacksize    = CPSWPROXY_ETH_CLIENT_TASK_STACK;

        hServer->clientServiceObj.hClientServiceTsk = TaskP_create(&CpswProxyServer_remoteClientEthDriverTaskFxn, &taskParams);
        if(hServer->clientServiceObj.hClientServiceTsk == NULL)
        {
            status = ETHFW_EFAIL;
            ETHFWTRACE_ERR(status, "Could not create task for endpoint %d", comParams.requestedEndpt);
        }
    }

    return status;
}

static void CpswProxyServer_remoteClientEthDriverTaskFxn(void* arg0, void* arg1)
{
    CpswProxyServer_Obj *hServer = (CpswProxyServer_Obj *)arg0;
    int32_t rtnVal = IPC_SOK;
    uint32_t remoteProcId, remoteEndPt;
    uint16_t len;
    uint64_t msgBuf[ETHREMOTECFG_IPC_MSG_SIZE / sizeof(uint64_t)];
    bool exitTask;
    uint32_t localEp = hServer->clientServiceObj.localEp;
    RPMessage_Handle hClientServicRpMsgEp = hServer->clientServiceObj.hClientServicRpMsgEp;

    rtnVal = RPMessage_announce(RPMESSAGE_ALL, localEp, ETHREMOTECFG_FRAMEWORK_SERVICE_NAME);
    if (IPC_SOK != rtnVal)
    {
        ETHFWTRACE_ERR(rtnVal, "Couldn't announce endpoint for remote clients");
    }
    else
    {
        exitTask = BFALSE;

        while (!exitTask)
        {
            rtnVal = RPMessage_recv(hClientServicRpMsgEp,
                                    (void *)msgBuf,
                                    &len,
                                    &remoteEndPt,
                                    &remoteProcId,
                                    IPC_RPMESSAGE_TIMEOUT_FOREVER);
            if (IPC_SOK == rtnVal)
            {
                int32_t status;
                EthRemoteCfg_MsgHdr *hdr;
                uint8_t clientId;

                EnetAppUtils_assert(len <= sizeof(msgBuf));

                hdr = (EthRemoteCfg_MsgHdr *)msgBuf;
                clientId = hdr->clientId;

                if (hdr->msgType == ETHREMOTECFG_MSGTYPE_REQUEST)
                {
                    CpswProxyServer_clientRequestHandler(hClientServicRpMsgEp,
                                                         &msgBuf,
                                                         clientId,
                                                         remoteProcId,
                                                         remoteEndPt,
                                                         localEp);
                }
                else if (hdr->msgType == ETHREMOTECFG_MSGTYPE_NOTIFY)
                {
                    EthRemoteCfg_NotifyHdr *notifyHdr = (EthRemoteCfg_NotifyHdr *)msgBuf;

                    CpswProxyServer_clientNotifyHandlerCb(notifyHdr->common.token,
                                                          remoteProcId,
                                                          notifyHdr->notifyType,
                                                          (uint8_t*)NULL,
                                                          (uint32_t)NULL);
                }
                else
                {
                    /* to-do: handle response data */
                }
            }
            else
            {
                ETHFWTRACE_ERR(rtnVal, "Failed to receive msg via IPC");
            }
        }
    }
}

static void CpswProxyServer_hwPushNotifyFxn(void *arg, CpswCpts_HwPush hwPushNum)
{
    if (arg != NULL)
    {
        CpswProxyServer_Obj *hServer = (CpswProxyServer_Obj *)arg;

        /* Post Event */
        EventP_post(hServer->notifyServiceObj.hHwPushNotifyServiceEvent,
                    hServer->notifyServiceObj.hwPushNotifyEventId[CPSW_CPTS_HWPUSH_NORM(hwPushNum)]);
    }
}

static int32_t CpswProxyServer_initAutosarEthDeviceEp(CpswProxyServer_Obj *hServer,
                                                      CpswProxyServer_Config_t * cfg,
                                                      uint32_t clientInst)
{
    TaskP_Params taskParams;
    int32_t retVal = ETHFW_SOK;
    RPMessage_Params comChParam;
    uint32_t  localEp;

    hServer->ethDrvObj[clientInst].dstProc = cfg->autosarPortCfg[clientInst].remoteCoreId;
    hServer->ethDrvObj[clientInst].virtPort = cfg->autosarPortCfg[clientInst].portId;

    RPMessageParams_init(&comChParam);
    comChParam.numBufs = CPSWPROXY_AUTOSAR_ETHDRIVER_NUM_RPMSG_BUFS;
    comChParam.buf = g_CpswProxyServerAutosarRpmsgBuf[clientInst];
    comChParam.bufSize = sizeof(g_CpswProxyServerAutosarRpmsgBuf[clientInst]);
    comChParam.requestedEndpt = cfg->autosarEthDeviceEndPointId[clientInst];

    hServer->ethDrvObj[clientInst].hAutosarEthRpMsgEp = RPMessage_create(&comChParam, &localEp);
    if (NULL == hServer->ethDrvObj[clientInst].hAutosarEthRpMsgEp)
    {
        retVal = ETHFW_EFAIL;
        ETHFWTRACE_ERR(retVal, "Could not create communication channel for endpoint %d",
                       comChParam.requestedEndpt);
    }

    if (ETHFW_SOK == retVal)
    {
        hServer->ethDrvObj[clientInst].localEp = localEp;
    }

    if (ETHFW_SOK == retVal)
    {
        /* Initialize the task params */
        TaskP_Params_init(&taskParams);
        taskParams.name         = CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_NAME;
        taskParams.priority     = CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_PRIORITY;
        taskParams.arg0         = (void*) hServer;
        taskParams.arg1         = (void*) clientInst;
        taskParams.stack        = &gCpswProxyServer_autosarEthDriverTaskStackBuf[clientInst][0];
        taskParams.stacksize    = CPSWPROXY_AUTOSAR_ETHDRIVER_TASK_STACK;

        hServer->ethDrvObj[clientInst].hAutosarEthTsk = TaskP_create(&CpswProxyServer_autosarEthDriverTaskFxn, &taskParams);
        if(hServer->ethDrvObj[clientInst].hAutosarEthTsk == NULL)
        {
            retVal = ETHFW_EFAIL;
            ETHFWTRACE_ERR(retVal, "Could not create task for endpoint %d", comChParam.requestedEndpt);
        }
    }

    return retVal;
}

static int32_t CpswProxyServer_initNotifyServiceEp(CpswProxyServer_Obj * hServer, CpswProxyServer_Config_t * cfg)
{
    TaskP_Params taskParams;
    int32_t retVal = ETHFW_SOK;
    RPMessage_Params comChParam;
    uint32_t  localEp;
    EventP_Params eventParams;
    uint8_t i = 0;

    hServer->notifyServiceObj.notifyServiceCpswType = hServer->enetType;
    hServer->notifyServiceObj.dstProcMask = 0U;
    for (i = 0U; i < cfg->numVirtPorts; i++)
    {
        hServer->notifyServiceObj.dstProcMask |= ENET_BIT(cfg->notifyServiceRemoteCoreId[i]);
    }
    for (i = 0U; i < CPSW_CPTS_HWPUSH_COUNT_MAX; i++)
    {
        hServer->notifyServiceObj.hwPush2CoreIdMap[i] = IPC_MAX_PROCS;
        hServer->notifyServiceObj.hwPush2TokenMap[i] = ETHREMOTECFG_TOKEN_NONE;
    }

    RPMessageParams_init(&comChParam);
    comChParam.numBufs = ETHREMOTECFG_IPC_NUM_MSG_BUFS;
    comChParam.buf = g_CpswProxyServerNotifyServiceRpmsgBuf;
    comChParam.bufSize = sizeof(g_CpswProxyServerNotifyServiceRpmsgBuf);
    hServer->notifyServiceObj.hNotifyServicRpMsgEp = RPMessage_create(&comChParam, &localEp);

    if (NULL == hServer->notifyServiceObj.hNotifyServicRpMsgEp)
    {
        retVal = ETHFW_EFAIL;
        ETHFWTRACE_ERR(retVal, "Could not create communication channel");
    }

    if (ETHFW_SOK == retVal)
    {
        hServer->notifyServiceObj.localEp = localEp;
    }

    /* Announce service */
    if (ETHFW_SOK == retVal)
    {
        retVal = RPMessage_announce(RPMESSAGE_ALL,
                                    hServer->notifyServiceObj.localEp,
                                    ETHREMOTECFG_REMOTE_NOTIFY_SERVICE);
        ETHFWTRACE_ERR_IF((retVal != IPC_SOK), retVal, "Failed to annount notify server");
    }

    /* Create Event to notify task */
    if (ETHFW_SOK == retVal)
    {
        EventP_Params_init(&eventParams);

        for (i = 0U; i < CPSW_CPTS_HWPUSH_COUNT_MAX; i++)
        {
            hServer->notifyServiceObj.hwPushNotifyEventId[i] = (1U << i);
        }

        hServer->notifyServiceObj.hHwPushNotifyServiceEvent = EventP_create(&eventParams);

        if (hServer->notifyServiceObj.hHwPushNotifyServiceEvent == NULL)
        {
            retVal = ETHFW_EFAIL;
            ETHFWTRACE_ERR(retVal, "Could not create an event");
        }
    }

    if (ETHFW_SOK == retVal)
    {
        /* Initialize the task params */
        TaskP_Params_init(&taskParams);
        taskParams.name         = CPSWPROXY_NOTIFY_SERVICE_TASK_NAME;
        taskParams.priority     = CPSWPROXY_NOTIFY_SERVICE_TASK_PRIORITY;
        taskParams.arg0         = (void*) hServer;
        taskParams.stack        = &gCpswProxyServer_notifyServiceTaskStackBuf[0];
        taskParams.stacksize    = CPSWPROXY_NOTIFY_SERVICE_SERVER_TASK_STACKSIZE;
        hServer->notifyServiceObj.hNotifyServiceTsk = TaskP_create(&CpswProxyServer_notifyServiceTaskFxn, &taskParams);
        if(hServer->notifyServiceObj.hNotifyServiceTsk == NULL)
        {
            retVal = ETHFW_EFAIL;
            ETHFWTRACE_ERR(retVal, "Could not create a task");
        }
    }

    return retVal;
}

static void CpswProxyServer_notifyServiceTaskFxn(void* arg0, void* arg1)
{
    int32_t rtnVal = ENET_SOK;
    Enet_Handle hEnet;
    EnetMcm_CmdIf *hMcmCmdIf;
    EnetMcm_HandleInfo handleInfo;
    CpswProxyServer_Obj * hServer = (CpswProxyServer_Obj *)arg0;
    uint64_t msgBuffer[(ETHREMOTECFG_IPC_MSG_SIZE / sizeof(uint64_t))];
    volatile bool exitTask = BFALSE;
    Enet_IoctlPrms prms;
    CpswCpts_Event lookupEventInArgs;
    CpswCpts_Event lookupEventOutArgs;
    uint32_t i = 0U;
    uint32_t events = 0U;
    uint32_t remoteCoreId;
    uint32_t token;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert(hServer != NULL);

    /* Get MCM cmd handle */
    EnetAppUtils_assert(hServer->getMcmCmdIfCb != NULL);
    hServer->getMcmCmdIfCb(hServer->enetType, &hMcmCmdIf);
    EnetAppUtils_assert(hMcmCmdIf != NULL);
    EnetAppUtils_assert(hMcmCmdIf->hMboxCmd != NULL);
    EnetAppUtils_assert(hMcmCmdIf->hMboxResponse != NULL);
    hServer->hMcmCmdIf = hMcmCmdIf;

    /* Connect to MCM on client's behalf (using its hostId) */
    EnetMcm_acquireHandleInfo(hMcmCmdIf, &handleInfo);
    hEnet = handleInfo.hEnet;

    while (!exitTask)
    {
        /*Wait 1ms for hardware push event, then move on to other events*/
        events = EventP_wait(hServer->notifyServiceObj.hHwPushNotifyServiceEvent,
                             CPSWPROXY_CPTS_HWPUSH_EVENTS_OR_MASK,
                             EventP_WaitMode_ANY,
                             1U);

        /* Lookup for timestamp if it is a hardware push notification*/
        if (events)
        {
            if ((rtnVal == ENET_SOK) && (NULL != hEnet))
            {
                for (i = 0U; i < CPSW_CPTS_HWPUSH_COUNT_MAX; i++)
                {
                    if(ENET_IS_BIT_SET(events, i))
                    {
                        EthRemoteCfg_NotifyServiceHwPushMsg *hwPushMsg = (EthRemoteCfg_NotifyServiceHwPushMsg *)msgBuffer;

                        remoteCoreId = hServer->notifyServiceObj.hwPush2CoreIdMap[i];
                        token = hServer->notifyServiceObj.hwPush2TokenMap[i];

                        memset(hwPushMsg, 0, sizeof(*hwPushMsg));
                        hwPushMsg->hdr.common.token = token;
                        hwPushMsg->hdr.notifyType = ETHREMOTECFG_NOTIFY_HWPUSH;
                        hwPushMsg->hwPushNum = i + 1U;

                        lookupEventInArgs.eventType = CPSW_CPTS_EVENTTYPE_HW_TS_PUSH;
                        lookupEventInArgs.hwPushNum = (CpswCpts_HwPush) hwPushMsg->hwPushNum;
                        lookupEventInArgs.portNum = 0U;
                        lookupEventInArgs.seqId = 0U;
                        lookupEventInArgs.domain  = 0U;

                        ENET_IOCTL_SET_INOUT_ARGS(&prms, &lookupEventInArgs, &lookupEventOutArgs);
                        rtnVal = Enet_ioctl(hEnet, EnetSoc_getCoreId(), CPSW_CPTS_IOCTL_LOOKUP_EVENT, &prms);
                        if (rtnVal == ENET_SOK)
                        {
                            if ((remoteCoreId != IPC_MAX_PROCS) &&
                                (token != ETHREMOTECFG_TOKEN_NONE) &&
                                ENET_IS_BIT_SET(hServer->notifyServiceObj.dstProcMask, remoteCoreId))
                            {
                                hwPushMsg->timeStamp = lookupEventOutArgs.tsVal;

                                ETHFWTRACE_DBG("NOTIFY | S2C | core=%u endpt=%u token=%d notifyType=%u",
                                               remoteCoreId,
                                               ETHREMOTECFG_NOTIFY_SERVICE_ENDPT_ID,
                                               (int32_t)hwPushMsg->hdr.common.token,
                                               hwPushMsg->hdr.notifyType);

                                ETHFWTRACE_DBG("S2C | msgType=%u token=%d clientId=%u notifyType=%u len=%u (%u.%u->%u.%u)",
                                               hwPushMsg->hdr.common.msgType,
                                               (int32_t)hwPushMsg->hdr.common.token,
                                               hwPushMsg->hdr.common.clientId,
                                               hwPushMsg->hdr.notifyType,
                                               sizeof(*hwPushMsg),
                                               EnetSoc_getCoreId(), hServer->notifyServiceObj.localEp,
                                               remoteCoreId, ETHREMOTECFG_NOTIFY_SERVICE_ENDPT_ID);

                                rtnVal = RPMessage_send(hServer->notifyServiceObj.hNotifyServicRpMsgEp,
                                                        remoteCoreId,
                                                        ETHREMOTECFG_NOTIFY_SERVICE_ENDPT_ID,
                                                        hServer->notifyServiceObj.localEp,
                                                        hwPushMsg,
                                                        sizeof(*hwPushMsg));
                                ETHFWTRACE_ERR_IF((rtnVal != IPC_SOK), rtnVal, "Failed to send tstamp notification");
                            }
                        }
                        else
                        {
                            ETHFWTRACE_ERR(rtnVal, "Failed to get CPTS event info");
                        }
                    }
                }
            }
        }
    }
}


static void CpswProxyServer_autosarEthDriverTaskFxn(void* arg0, void* arg1)
{
    int32_t rtnVal = IPC_SOK;
    uint32_t remoteProcId, remoteEndPt;
    CpswProxyServer_Obj *hServer = (CpswProxyServer_Obj *)arg0;
    uint32_t clientNum = (uint32_t)arg1;
    uint32_t remoteProc, remoteEp;
    uint16_t len;
    uint64_t msgBuf[CPSWPROXY_AUTOSAR_ETHDRIVER_MSG_SIZE / sizeof(uint64_t)];
    uint32_t localdstProc = hServer->ethDrvObj[clientNum].dstProc;
    uint32_t localEp = hServer->ethDrvObj[clientNum].localEp;
    RPMessage_Handle hAutosarEthRpMsgEp = hServer->ethDrvObj[clientNum].hAutosarEthRpMsgEp;
    EthRemoteCfg_DeviceData deviceData;

    /* Wait for Remote EP to active */
    rtnVal = RPMessage_getRemoteEndPt(localdstProc,
                                      ETHREMOTECFG_AUTOSAR_REMOTE_SERVICE_NAME,
                                      &remoteProcId,
                                      &remoteEndPt,
                                      osal_WAIT_FOREVER);
    if (IPC_SOK != rtnVal)
    {
        ETHFWTRACE_ERR(rtnVal, "Remote AUTOSAR Ethernet Device locate failed");
    }
    else
    {
        bool exitTask = BFALSE;

        EnetAppUtils_assert(hServer->initEthfwDeviceDataCb != NULL);
        hServer->initEthfwDeviceDataCb(&deviceData);

        deviceData.hdr.notifyType = ETHREMOTECFG_NOTIFY_FWINFO;
        deviceData.hdr.common.clientId = ETHREMOTECFG_CLIENTID_NONE;
        deviceData.hdr.common.msgType = ETHREMOTECFG_MSGTYPE_NOTIFY;
        deviceData.hdr.common.token = ETHREMOTECFG_TOKEN_NONE;

        /* Send the EthFw Device Data to AUTOSAR EthDriver on location of endpoint */
        rtnVal = RPMessage_send(hAutosarEthRpMsgEp,
                                remoteProcId,
                                remoteEndPt,
                                localEp,
                                (Ptr)&deviceData,
                                sizeof(deviceData));
        EnetAppUtils_assert(IPC_SOK == rtnVal);

        while (!exitTask)
        {
            rtnVal = RPMessage_recv(hAutosarEthRpMsgEp,
                                    (Ptr)msgBuf,
                                    &len,
                                    &remoteEp,
                                    &remoteProc,
                                    IPC_RPMESSAGE_TIMEOUT_FOREVER);
            if (IPC_SOK == rtnVal)
            {
                int32_t status;
                EthRemoteCfg_MsgHdr *hdr;
                uint8_t clientId;

                EnetAppUtils_assert(len <= sizeof(msgBuf));
                EnetAppUtils_assert(remoteEp == remoteEndPt);
                EnetAppUtils_assert(remoteProcId == remoteProc);

                hdr = (EthRemoteCfg_MsgHdr *)msgBuf;
                clientId = hdr->clientId;

                if (hdr->msgType == ETHREMOTECFG_MSGTYPE_REQUEST)
                {
                    uint16_t len;

                    CpswProxyServer_clientRequestHandler(hAutosarEthRpMsgEp, &msgBuf, clientId, remoteProc, remoteEp, localEp);
                }

                else if ( hdr->msgType == ETHREMOTECFG_MSGTYPE_NOTIFY)
                {
                    EthRemoteCfg_NotifyHdr *notifyHdr = (EthRemoteCfg_NotifyHdr *)msgBuf;

                    CpswProxyServer_clientNotifyHandlerCb(notifyHdr->common.token,
                                                          remoteProcId,
                                                          notifyHdr->notifyType,
                                                          (uint8_t *)NULL,
                                                          (uint32_t)NULL);
                }
                else
                {
                    /* to-do: handle response data */
                }
            }
            else
            {
                ETHFWTRACE_ERR(rtnVal, "Failed to receive msg via IPC");
            }
        }
    }
}

static void CpswProxyServer_clientNotifyHandlerCb(uint32_t token,
                                                  uint32_t hostId,
                                                  EthRemoteCfg_NotifyType notifyid,
                                                  uint8_t *notifyInfo,
                                                  uint32_t notifyInfoLen)
{
    CpswProxyServer_ClientHandle hClient;
    Enet_Handle hEnet;
    uint32_t coreKey;
    CpswProxyServer_Obj *hServer;
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    Enet_Type enetType;

    hClient = CpswProxyServer_getClient(hostId, token);
    EnetAppUtils_assert(hClient != NULL);

    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == BTRUE));

    hEnet = hServer->hEnet;
    EnetAppUtils_assert(hEnet != NULL);

    coreKey = hServer->coreObj[hostId].attachInfo.coreKey;
    enetType = hServer->enetType;

    switch (notifyid)
    {
        case ETHREMOTECFG_NOTIFY_CUSTOM:
        {
            if (hServer->notifyCb != NULL)
            {
                hServer->notifyCb(hostId, hEnet, enetType, coreKey, notifyid, notifyInfo, notifyInfoLen);
            }
            break;
        }
        default:
        {
            ETHFWTRACE_ERR(ETHFW_EINVALIDPARAMS, "Invalid notify id %d", notifyid);
            break;
        }
    }
}

static void CpswProxyServer_initClientHandle(CpswProxyServer_Config_t *cfg)
{
    CpswProxyServer_Obj *hServer = NULL;
    uint32_t i,j;

    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert(hServer != NULL);

    if (CPSWPROXYSERVER_REMOTE_CLIENT_ALLOC_MAX < cfg->numAllocObj)
    {
        ETHFWTRACE_ERR(ETHFW_EINVALIDPARAMS, "Invalid number of alloc objects %u, max %u",
                       cfg->numAllocObj, CPSWPROXYSERVER_REMOTE_CLIENT_ALLOC_MAX);
    }

    for (i = 0U; i < cfg->numAllocObj; i++)
    {
        hServer->allocObj[i] = cfg->allocObj[i];
    }
}

int32_t CpswProxyServer_lateAnnounce(uint32_t procId)
{
    int32_t retVal;
    CpswProxyServer_Obj *hServer;

    hServer = CpswProxyServer_getHandle();
    SemaphoreP_pend(hServer->clientServiceObj.rpmsgStartSem, SemaphoreP_WAIT_FOREVER);

    retVal = RPMessage_announce(procId, hServer->clientServiceObj.localEp, ETHREMOTECFG_FRAMEWORK_SERVICE_NAME);
    ETHFWTRACE_INFO_IF((retVal == IPC_SOK), "Announce Endpoint Service to Linux");

    return retVal;
}

static uint32_t CpswProxyServer_getAbsTxChNumber(uint32_t chRelPriority,
                                                 EthRemoteCfg_VirtPort virtPort)
{
    uint32_t i;
    uint32_t txChNum = ENET_RM_TXCHNUM_INVALID;
    CpswProxyServer_Obj *hServer;
    hServer = CpswProxyServer_getHandle();

    for (i = 0U; i < hServer->numVirtPorts; i++)
    {
        if (hServer->virtPortCfg[i].portId == virtPort)
        {
            if (chRelPriority < hServer->virtPortCfg[i].numTxCh)
            {
                txChNum = hServer->virtPortCfg[i].txCh[chRelPriority];
            }
            else
            {
                ETHFWTRACE_ERR(ETHFW_EFAIL, "Failed to get absolute tx channel number, invalid relative channel id %u passed",
                               chRelPriority);
            }
            break;
        }
    }

    return txChNum;
}

static uint32_t CpswProxyServer_getClientTxChRxFlowNum(EthRemoteCfg_VirtPort virtPort,
                                                       uint32_t *pNumTxCh,
                                                       uint32_t *pNumRxFlow)
{
    uint32_t i;
    uint32_t status = ETHFW_EFAIL;
    CpswProxyServer_Obj *hServer;
    hServer = CpswProxyServer_getHandle();

    for (i = 0U; i < hServer->numVirtPorts; i++)
    {
        if (hServer->virtPortCfg[i].portId == virtPort)
        {
            *pNumTxCh = hServer->virtPortCfg[i].numTxCh;
            *pNumRxFlow = hServer->virtPortCfg[i].numRxFlow;
            status = ETHFW_SOK;
            break;
        }
    }

    return status;
}

static int32_t CpswProxyServer_createCustomPolicer(Enet_Handle hEnet,
                                                   uint32_t coreId,
                                                   CpswAle_SetPolicerEntryInPartitionInArgs *customPolInArgs)
{
    CpswAle_SetPolicerEntryOutArgs polOutArgs;
    Enet_IoctlPrms prms;
    int32_t status = ETHFW_EFAIL;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, customPolInArgs, &polOutArgs);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_SET_POLICER_IN_PARTITION, &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                      "Failed to create custom policer for core %u",
                       coreId);

    return status;
}

static int32_t CpswProxyServer_deleteCustomPolicer(Enet_Handle hEnet,
                                                   uint32_t coreId,
                                                   CpswAle_SetPolicerEntryInPartitionInArgs *customPolInArgs)
{
    CpswAle_DelPolicerEntryInArgs polInArgs;
    Enet_IoctlPrms prms;
    int32_t status = ETHFW_EFAIL;

    polInArgs.policerMatch = customPolInArgs->policerMatch;
    /* Remove ALE policer and entry from ALE as well */
    polInArgs.aleEntryMask = CPSW_ALE_POLICER_TABLEENTRY_DELETE_ALL;
    ENET_IOCTL_SET_IN_ARGS(&prms, &polInArgs);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_DEL_POLICER, &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                      "Failed to create custom policer for core %u",
                       coreId);

    return status;
}

static int32_t CpswProxyServer_getRxFlowInfo(EthRemoteCfg_VirtPort virtPort,
                                             uint32_t relFlowIdx,
                                             EthFwQos_RxFlowInfo **rxFlowInfo)
{
    uint32_t i;
    int32_t status = ETHREMOTECFG_CMDSTATUS_EFAIL;
    CpswProxyServer_Obj *hServer;
    hServer = CpswProxyServer_getHandle();

    for (i = 0U; i < hServer->numVirtPorts; i++)
    {
        if (hServer->virtPortCfg[i].portId == virtPort)
        {
            if (relFlowIdx < hServer->virtPortCfg[i].numRxFlow)
            {
                *rxFlowInfo = &hServer->virtPortCfg[i].rxFlowsInfo[relFlowIdx];
                status = ETHREMOTECFG_CMDSTATUS_OK;
            }
            else
            {
                ETHFWTRACE_ERR(status,
                               "Invalid rel flow index %u passed to virtual port %u",
                               relFlowIdx,
                               virtPort);
            }
            break;
        }
    }

    return status;
}

static int32_t CpswProxyServer_getRelFlowIdx(EthRemoteCfg_VirtPort virtPort,
                                             uint32_t *relFlowIdx,
                                             uint32_t rxFlowIdxOffset)
{
    uint32_t i;
    uint32_t j;
    int32_t status = ETHREMOTECFG_CMDSTATUS_EFAIL;
    CpswProxyServer_Obj *hServer;
    hServer = CpswProxyServer_getHandle();

    for (i = 0U; i < hServer->numVirtPorts; i++)
    {
        if (hServer->virtPortCfg[i].portId == virtPort)
        {
            for (j = 0U; j < ENET_CFG_RX_FLOWS_NUM; j++)
            {
                if (hServer->virtPortCfg[i].rxFlowsInfo[j].rxFlowIdxOffset == rxFlowIdxOffset)
                {
                    *relFlowIdx = j;
                    status = ETHREMOTECFG_CMDSTATUS_OK;
                    break;
                }
            }
            break;
        }
    }

    return status;
}

static void CpswProxyServer_saveRxFlowOffset(EthRemoteCfg_VirtPort virtPort,
                                             uint32_t relFlowIdx,
                                             uint32_t rxFlowIdxOffset)
{
    uint32_t i;
    CpswProxyServer_Obj *hServer;
    hServer = CpswProxyServer_getHandle();

    for (i = 0U; i < hServer->numVirtPorts; i++)
    {
        if (hServer->virtPortCfg[i].portId == virtPort)
        {
            hServer->virtPortCfg[i].rxFlowsInfo[relFlowIdx].rxFlowIdxOffset = rxFlowIdxOffset;
            break;
        }
    }
}
