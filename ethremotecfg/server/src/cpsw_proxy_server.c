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

#include <stdio.h>
#include <stdint.h>

/* PDK header files */
#include <ti/osal/osal.h>
#include <ti/osal/SemaphoreP.h>
#include <ti/osal/TaskP.h>
#include <ti/osal/EventP.h>
#include <ti/drv/ipc/ipc.h>

/* EthFw utils header files */
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/core/enet_dma.h>
#include <ti/drv/enet/include/core/enet_mod_hostport.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/enet/examples/utils/include/enet_apputils.h>
#include <ti/drv/enet/examples/utils/include/enet_mcm.h>
#include <ti/drv/enet/examples/utils/include/enet_apprm.h>

/* EthFw utils header files */
#include <ethremotecfg/protocol/ethremotecfg.h>
#include <ethremotecfg/protocol/ethremotecfg_virtport.h>
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

#define CPSWPROXY_ENET2PROXY_ERR(x)                      (((x) == ENET_SOK) ? \
                                                          CPSWPROXYSERVER_SOK : \
                                                          CPSWPROXYSERVER_EFAIL)

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

    /* Handle to Enet LLD */
    Enet_Handle hEnet;

    /* Enet LLD core key */
    uint32_t coreKey;

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
} CpswProxyServer_ClientObj;

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
    /* Instance Id of the CPSW server object */
    uint32_t instId;
    /* set to true when proxy server has been initialized */
    bool initDone;
    /* Client object for storing all the data associated with the client */
    CpswProxyServer_ClientObj clientObj[CPSWPROXYSERVER_REMOTE_CLIENT_MAX];
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
                                                uint32_t remoteCoreId,
                                                uint32_t coreKey,
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
        .initDone              = FALSE,
    };

    return (&gProxyServerObj);
}

static CpswProxyServer_ClientHandle CpswProxyServer_allocClient(uint32_t remoteEndPt)
{
    CpswProxyServer_Obj *hServer = CpswProxyServer_getHandle();
    CpswProxyServer_ClientHandle hClient = NULL;
    uint32_t i;

    MutexP_lock(hServer->hMutex, MutexP_WAIT_FOREVER);

    for (i = 0U; i < ENET_ARRAYSIZE(hServer->clientObj); i++)
    {
        hClient = &hServer->clientObj[i];
        if (!hClient->inUse)
        {
            hClient->inUse = true;
            hClient->token = ETHREMOTECFG_TOKEN_NONE;
            hClient->remoteEp = remoteEndPt;
            hClient->isIdle = false;
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
    hClient->inUse = false;
    hClient->token = ETHREMOTECFG_TOKEN_NONE;
    MutexP_unlock(hServer->hMutex);
}

static CpswProxyServer_ClientHandle CpswProxyServer_getClient(uint32_t token)
{
    CpswProxyServer_Obj *hServer = CpswProxyServer_getHandle();
    CpswProxyServer_ClientHandle hClient = NULL;
    EnetMcm_CmdIf *hMcmCmdIf = NULL;
    uint32_t i;

    MutexP_lock(hServer->hMutex, MutexP_WAIT_FOREVER);

    for (i = 0U; i < ENET_ARRAYSIZE(hServer->clientObj); i++)
    {
        hClient = &hServer->clientObj[i];

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
    int32_t status = CPSWPROXYSERVER_SOK;
    *attachedClients = 0U;
    *idleClients = 0U;
    uint32_t i;

    for (i = 0U; i < ENET_ARRAYSIZE(hServer->clientObj); i++)
    {
        hClient = &hServer->clientObj[i];

        if (hClient->inUse && (hClient->token != ETHREMOTECFG_TOKEN_NONE))
        {
            (*attachedClients)++;

            if (hClient->isIdle)
            {
                (*idleClients)++;
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
    uint32_t srcEndPt;
    uint32_t clientInst;
    int32_t status = CPSWPROXYSERVER_SOK;

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
        ETHFWTRACE_ERR(ETHFW_EFAIL, "Couldn't find core %u client handle", hClient->coreId);
    }

    status = RPMessage_send(handle, hClient->coreId, hClient->remoteEp, srcEndPt, &notifyMsg, sizeof(notifyMsg));

    return status;
}

int32_t CpswProxyServer_bcastNotify(uint32_t notifyId)
{
    CpswProxyServer_Obj *hServer = CpswProxyServer_getHandle();
    CpswProxyServer_ClientHandle hClient = NULL;
    int32_t status = CPSWPROXYSERVER_SOK;
    uint32_t i;

    for (i = 0U; i < ENET_ARRAYSIZE(hServer->clientObj); i++)
    {
        hClient = &hServer->clientObj[i];

        if (hClient->inUse)
        {
            status = CpswProxyServer_sendNotify(hClient, notifyId);
        }
        TaskP_sleep(50);
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
    EnetAppUtils_assert((hServer != NULL) && hServer->initDone==true);

    for (i = 0; i < CPSWPROXYSERVER_REMOTE_CLIENT_ALLOC_MAX; i++)
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
    }

    return status;
}

static int32_t CpswProxyServer_VirtPortAllocCb(uint32_t clientId,
                                               uint32_t hostId,
                                               uint32_t *switchPortMask,
                                               uint32_t *macPortMask)
{
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    if (clientId < CPSWPROXYSERVER_REMOTE_CLIENT_MAX)
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
                                               uint32_t *pFeatures)
{
    CpswProxyServer_Obj *hServer = NULL;
    EnetMcm_CmdIf *hMcmCmdIf;
    EnetMcm_HandleInfo handleInfo;
    EnetPer_AttachCoreOutArgs attachInfo;
    Enet_IoctlPrms prms;
    EthRemoteCfg_VirtPort virtPort = ETHREMOTECFG_VIRTPORT_DENORM(portId);
    bool isMacPort = EthRemoteCfg_isMacPort(virtPort);
    bool csumOffloadFlag;
    int32_t status = ENET_SOK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    /* Get MCM cmd handle */
    EnetAppUtils_assert(hServer->getMcmCmdIfCb != NULL);
    hServer->getMcmCmdIfCb(hServer->enetType, &hMcmCmdIf);
    EnetAppUtils_assert(hMcmCmdIf != NULL);
    EnetAppUtils_assert(hMcmCmdIf->hMboxCmd != NULL);
    EnetAppUtils_assert(hMcmCmdIf->hMboxResponse != NULL);
    hServer->hMcmCmdIf = hMcmCmdIf;

    /* Connect to MCM on client's behalf (using its hostId) */
    EnetMcm_acquireHandleInfo(hMcmCmdIf, &handleInfo);
    EnetMcm_coreAttach(hMcmCmdIf, hostId, &attachInfo);

    *pRxMtu = attachInfo.rxMtu;
    EnetAppUtils_assert(txMtuArraySize == ENET_ARRAYSIZE(attachInfo.txMtu));
    memcpy(pTxMtu, attachInfo.txMtu, sizeof(attachInfo.txMtu));

    /* FIXME - This is global setting */
    ENET_IOCTL_SET_OUT_ARGS(&prms, &csumOffloadFlag);
    status = Enet_ioctl(handleInfo.hEnet, hostId, ENET_HOSTPORT_IS_CSUM_OFFLOAD_ENABLED, &prms);
    EnetAppUtils_assert(status == ENET_SOK);

    *pFeatures = 0U;
    if (csumOffloadFlag)
    {
        *pFeatures |= ETHREMOTECFG_FEATURE_TXCSUM;
    }
    else
    {
        *pFeatures |= ETHREMOTECFG_FEATURE_MC_FILTER;
    }

    /* Save parameters in client object */
    hClient->token     = CPSWPROXY_VIRTPORT_2_TOKEN(virtPort);
    hClient->virtPort  = virtPort;
    hClient->coreId    = hostId;
    hClient->hEnet     = handleInfo.hEnet;
    hClient->coreKey   = attachInfo.coreKey;
    hClient->features  = *pFeatures;

    return CPSWPROXY_ENET2RPMSG_ERR(status);
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
                                                  uint8_t *macAddr)
{
    int32_t status = ENET_SOK;

    /* Actual attach operation */
    status = CpswProxyServer_attachHandlerCb(hClient, hostId, portId, pRxMtu, pTxMtu, txMtuArraySize, pFeatures);
    EnetAppUtils_assert(ENET_SOK == status);

    /* Allocate RX flow */
    if (CPSWPROXYSERVER_SOK == status)
    {
        status = EnetAppUtils_allocRxFlow(hClient->hEnet,
                                          hClient->coreKey,
                                          hostId,
                                          pRxFlowIdxBase,
                                          pRxFlowIdxOffset);
        if (ENET_SOK == status)
        {
            CpswProxyServer_validateStartIdx(hClient->hEnet, hostId, *pRxFlowIdxBase);
        }
    }

    /* Allocate TX channel */
    if (CPSWPROXYSERVER_SOK == status)
    {
        status = EnetAppUtils_allocTxCh(hClient->hEnet,
                                        hClient->coreKey,
                                        hostId,
                                        pTxPsilDstId);
    }

    /* Allocate MAC address */
    if (CPSWPROXYSERVER_SOK == status)
    {
        status = EnetAppUtils_allocMac(hClient->hEnet,
                                       hClient->coreKey,
                                       hostId,
                                       macAddr);
    }

    /* Save parameters in client object */
    hClient->flowIdxBase   = *pRxFlowIdxBase;
    hClient->flowIdxOffset = *pRxFlowIdxOffset;
    hClient->psilDstId     = *pTxPsilDstId;
    EnetUtils_copyMacAddr(&hClient->macAddr[0U], macAddr);

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_allocTxHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                uint32_t hostId,
                                                uint32_t *pTxPsilDstId)
{
    int32_t status;

    status = EnetAppUtils_allocTxCh(hClient->hEnet,
                                    hClient->coreKey,
                                    hostId,
                                    pTxPsilDstId);

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_allocRxHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                uint32_t hostId,
                                                uint32_t *pRxFlowIdxBase,
                                                uint32_t *pRxFlowIdxOffset)
{
    int32_t status;

    status = EnetAppUtils_allocRxFlow(hClient->hEnet,
                                      hClient->coreKey,
                                      hostId,
                                      pRxFlowIdxBase,
                                      pRxFlowIdxOffset);
    if (ENET_SOK == status)
    {
        CpswProxyServer_validateStartIdx(hClient->hEnet, hostId, *pRxFlowIdxBase);
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_allocMacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                 uint32_t hostId,
                                                 uint8_t *macAddr)
{
    int32_t status;

    status = EnetAppUtils_allocMac(hClient->hEnet,
                                   hClient->coreKey,
                                   hostId,
                                   macAddr);

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_detachHandlerCb(CpswProxyServer_ClientHandle hClient,
                                               uint32_t hostId)
{
    CpswProxyServer_Obj *hServer = NULL;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    /* Detach from MCM */
    EnetAppUtils_assert(hServer->hMcmCmdIf != NULL);
    EnetMcm_coreDetach(hServer->hMcmCmdIf, hostId, hClient->coreKey);
    EnetMcm_releaseHandleInfo(hServer->hMcmCmdIf);

    return status;
}

static int32_t CpswProxyServer_freeTxHandlerCb(CpswProxyServer_ClientHandle hClient,
                                               uint32_t hostId,
                                               uint32_t pTxPsilDstId)
{
    int32_t status;

    status = EnetAppUtils_freeTxCh(hClient->hEnet,
                                   hClient->coreKey,
                                   hostId,
                                   pTxPsilDstId);

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_freeRxHandlerCb(CpswProxyServer_ClientHandle hClient,
                                               uint32_t hostId,
                                               uint32_t pRxFlowIdxBase,
                                               uint32_t pRxFlowIdxOffset)
{
    int32_t status;

    CpswProxyServer_validateStartIdx(hClient->hEnet, hostId, pRxFlowIdxBase);
    status = EnetAppUtils_freeRxFlow(hClient->hEnet,
                                     hClient->coreKey,
                                     hostId,
                                     pRxFlowIdxOffset);

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_freeMacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                uint32_t hostId,
                                                uint8_t *macAddr)
{
    int32_t status;

    status = EnetAppUtils_freeMac(hClient->hEnet,
                                  hClient->coreKey,
                                  hostId, macAddr);

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
    int32_t status = ENET_SOK;
#if defined(ETHFW_VEPA_SUPPORT)
    struct eth_addr hwAddr;
#endif

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    CpswProxyServer_validateStartIdx(hClient->hEnet, hostId, flowIdxBase);

    isSwitchPort = EthRemoteCfg_isSwitchPort(hClient->virtPort);
    if (isSwitchPort)
    {
        /* Setup MAC address based classifier to the requested RX flow */
        status = EnetAppUtils_regDstMacRxFlow(hClient->hEnet,
                                              hClient->coreKey,
                                              hostId,
                                              flowIdxBase,
                                              flowIdxOffset,
                                              macAddr);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to setup MAC addr based route");

#if defined(ETHFW_VEPA_SUPPORT)
        if (status == ENET_SOK)
        {
            SMEMCPY(&hwAddr, macAddr, ETH_HWADDR_LEN);
            /* vlanId of 0 indicates do not use VLAN */
            status = EthFwVepa_registerClient(hClient->hEnet, hostId, flowIdxOffset, 0, hClient->virtPort, &hwAddr);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                              "Failed to register client core %u "
                              "macAddr %02x:%02x:%02x:%02x:%02x:%02x into VEPA table",
                              hostId,
                              macAddr[0], macAddr[1], macAddr[2],
                              macAddr[3], macAddr[4], macAddr[5]);
        }
#endif
    }
    else
    {
        macPort = EthRemoteCfg_getMacPort(hClient->virtPort);

        /* Setup MAC port based classifier to the requested RX flow */
        status = CpswProxyServer_regMacPortFlow(hClient->hEnet,
                                                hClient->coreKey,
                                                hostId,
                                                macPort,
                                                macAddr,
                                                flowIdxBase,
                                                flowIdxOffset);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to setup MAC port based route");
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
    int32_t status = ENET_SOK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    CpswProxyServer_validateStartIdx(hClient->hEnet, hostId, flowIdxBase);

    isSwitchPort = EthRemoteCfg_isSwitchPort(hClient->virtPort);
    if (isSwitchPort)
    {
        /* Teardown MAC address based classifier */
        status = EnetAppUtils_unregDstMacRxFlow(hClient->hEnet,
                                                hClient->coreKey,
                                                hostId,
                                                flowIdxBase,
                                                flowIdxOffset,
                                                macAddr);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to teardown MAC addr based route");

#if defined(ETHFW_VEPA_SUPPORT)
        if (status == ENET_SOK)
        {
            /* vlanId of 0 indicates do not use VLAN */
            status = EthFwVepa_unregisterClient(hClient->hEnet, hostId, flowIdxOffset, 0, hClient->virtPort);
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
        macPort = EthRemoteCfg_getMacPort(hClient->virtPort);

        /* Teardown MAC port based classifier */
        status = CpswProxyServer_unregMacPortFlow(hClient->hEnet,
                                                  hClient->coreId,
                                                  hostId,
                                                  macPort,
                                                  macAddr,
                                                  flowIdxBase,
                                                  flowIdxOffset);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to teardown MAC port based route");
    }

    return CPSWPROXY_ENET2RPMSG_ERR(status);
}

static int32_t CpswProxyServer_registerIPv4MacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                        uint32_t hostId,
                                                        uint8_t *macAddr,
                                                        uint8_t *ipAddr)
{
    CpswProxyServer_Obj *hServer = NULL;
    uint32_t ipaddr;
#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
    ip4_addr_t ip4Addr;
    struct eth_addr hwAddr;
#endif
    bool isSwitchPort;
    uint16_t vlanId = 0U;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    isSwitchPort = EthRemoteCfg_isSwitchPort(hClient->virtPort);
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

    return status;
}

static int32_t CpswProxyServer_deregisterIPv4MacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                          uint32_t hostId,
                                                          uint8_t *ipAddr)
{
    CpswProxyServer_Obj *hServer = NULL;
    uint32_t ipaddr;
#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
    ip4_addr_t ip4Addr;
#endif
    bool isSwitchPort;
    uint16_t vlanId = 0U;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    isSwitchPort = EthRemoteCfg_isSwitchPort(hClient->virtPort);
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
    int32_t status = ENET_SOK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    status = EthFwVlan_join(hClient->hEnet,
                            hClient->virtPort,
                            vlanId,
                            macAddr,
                            flowIdxOffset);
    ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                      "Failed to join VLAN %u", vlanId);

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
    int32_t status = ENET_SOK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    status = EthFwVlan_leave(hClient->hEnet,
                             hClient->virtPort,
                             vlanId,
                             macAddr,
                             flowIdxOffset);
    ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                      "Failed to leave VLAN %u", vlanId);

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
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    isMacPort = EthRemoteCfg_isMacPort(hClient->virtPort);
    if (isMacPort)
    {
        /* Enable promiscuous mode on virtual MAC ports (MAC-only CAF) */
        macPort = EthRemoteCfg_getMacPort(hClient->virtPort);
        ENET_IOCTL_SET_IN_ARGS(&prms, &macPort);

        cmd = enable ? CPSW_ALE_IOCTL_ENABLE_PROMISC_MODE : CPSW_ALE_IOCTL_DISABLE_PROMISC_MODE;

        status = Enet_ioctl(hClient->hEnet, hostId, cmd, &prms);
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
    Cpsw_PortRxFlowInfo portRxFlow;
    CpswAle_SetUcastEntryInArgs ucastInArgs;
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
        ucastInArgs.info.blocked = false;
        ucastInArgs.info.secure  = true;
        ucastInArgs.info.super   = false;
        ucastInArgs.info.ageable = false;
        ucastInArgs.info.trunk   = false;
        EnetUtils_copyMacAddr(&ucastInArgs.addr.addr[0U], macAddr);

        ENET_IOCTL_SET_INOUT_ARGS(&prms, &ucastInArgs, &entryIdx);

        status = Enet_ioctl(hEnet, remoteCoreId, CPSW_ALE_IOCTL_ADD_UCAST, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                          "Port %u: failed to add ucast entry", ENET_MACPORT_ID(macPort));

        status = CPSWPROXY_ENET2RPMSG_ERR(status);
    }

    /* Setup policer with "port" as match criteria */
    if (status == ETHREMOTECFG_CMDSTATUS_OK)
    {
        portRxFlow.coreKey  = coreKey;
        portRxFlow.startIdx = flowStartIdx;
        portRxFlow.flowIdx  = flowIdx;
        portRxFlow.macPort  = macPort;

        ENET_IOCTL_SET_IN_ARGS(&prms, &portRxFlow);

        status = Enet_ioctl(hEnet, remoteCoreId, CPSW_IOCTL_REGISTER_PORT_RX_FLOW, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                          "Port %u: failed to register RX flow", ENET_MACPORT_ID(macPort));

        status = CPSWPROXY_ENET2RPMSG_ERR(status);
    }

    return status;
}

static int32_t CpswProxyServer_unregMacPortFlow(Enet_Handle hEnet,
                                                uint32_t remoteCoreId,
                                                uint32_t coreKey,
                                                Enet_MacPort macPort,
                                                uint8_t *macAddr,
                                                uint32_t flowStartIdx,
                                                uint32_t flowIdx)
{
    Cpsw_PortRxFlowInfo portRxFlow;
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
        portRxFlow.coreKey  = coreKey;
        portRxFlow.startIdx = flowStartIdx;
        portRxFlow.flowIdx  = flowIdx;
        portRxFlow.macPort  = macPort;

        ENET_IOCTL_SET_IN_ARGS(&prms, &portRxFlow);

        status = Enet_ioctl(hEnet, remoteCoreId, CPSW_IOCTL_UNREGISTER_PORT_RX_FLOW, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                          "Port %u: failed to unregister RX flow", ENET_MACPORT_ID(macPort));

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

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    CpswProxyServer_validateStartIdx(hClient->hEnet, hostId, flowIdxBase);

    isSwitchPort = EthRemoteCfg_isSwitchPort(hClient->virtPort);
    if (isSwitchPort)
    {
        status = EnetAppUtils_regDfltRxFlow(hClient->hEnet, hClient->coreKey, hostId, flowIdxBase, flowIdxOffset);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to register default flow");
        status = CPSWPROXY_ENET2RPMSG_ERR(status);
    }
    else
    {
        /* RX default flow is supported only on virtual switch ports */
        status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
        ETHFWTRACE_ERR(status, "Default flow setting is not supported on virtual MAC ports");
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

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    CpswProxyServer_validateStartIdx(hClient->hEnet, hostId, flowIdxBase);

    isSwitchPort = EthRemoteCfg_isSwitchPort(hClient->virtPort);
    if (isSwitchPort)
    {
        status = EnetAppUtils_unregDfltRxFlow(hClient->hEnet, hClient->coreKey, hostId, flowIdxBase, flowIdxOffset);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to register default flow");
        status = CPSWPROXY_ENET2RPMSG_ERR(status);
    }
    else
    {
        /* RX default flow is supported only on virtual switch ports */
        status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
        ETHFWTRACE_ERR(status, "Default flow setting is not supported on virtual MAC ports");
    }

    return status;
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
        ETHFWTRACE_INFO("\n Port 0 Statistics");
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
                EnetAppUtils_printHostPortStats9G(st);
                break;
            }

            default:
            {
                EnetAppUtils_assert(false);
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
                ETHFWTRACE_INFO("\n External Port %d Statistics", ENET_MACPORT_ID(portNum));
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
                        EnetAppUtils_printMacPortStats9G(st);
                        break;
                    }

                    default:
                    {
                        EnetAppUtils_assert(false);
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
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    *speed  = ENET_SPEED_10MBIT;
    *duplex = ENET_DUPLEX_HALF;

    isMacPort = EthRemoteCfg_isMacPort(hClient->virtPort);
    if (isMacPort)
    {
        phyInArgs.macPort = EthRemoteCfg_getMacPort(hClient->virtPort);

        ENET_IOCTL_SET_INOUT_ARGS(&prms, &phyInArgs, isLinked);

        status = Enet_ioctl(hClient->hEnet, hostId, ENET_PHY_IOCTL_IS_LINKED, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                          "Failed to get port %u link status", ENET_MACPORT_ID(phyInArgs.macPort));

        //FIXME: link speed/duplex IOCTL has to be PHY agnostics (i.e. MAC-to-MAC mode).
        if (*isLinked)
        {
            ENET_IOCTL_SET_INOUT_ARGS(&prms, &phyInArgs, &phyOutArgs);

            status = Enet_ioctl(hClient->hEnet, hostId, ENET_PHY_IOCTL_GET_LINK_MODE, &prms);
            ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                              "Failed to get port %u link params", ENET_MACPORT_ID(phyInArgs.macPort));
        }

        *speed  = phyOutArgs.speed;
        *duplex = phyOutArgs.duplexity;
        status = CPSWPROXY_ENET2RPMSG_ERR(status);
    }
    else
    {
        *isLinked = true;
        *speed    = ENET_SPEED_1GBIT;
        *duplex   = ENET_DUPLEX_FULL;
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
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    isSwitchPort = EthRemoteCfg_isSwitchPort(hClient->virtPort);
    if (isSwitchPort)
    {
        memset(&setPolicerInArgs, 0, sizeof(setPolicerInArgs));
        setPolicerInArgs.policerMatch.policerMatchEnMask = CPSW_ALE_POLICER_MATCH_ETHERTYPE;
        setPolicerInArgs.policerMatch.etherType = etherType;
        setPolicerInArgs.threadIdEn = TRUE;
        setPolicerInArgs.threadId   = flowIdxOffset;
        setPolicerInArgs.peakRateInBitsPerSec   = 0U;
        setPolicerInArgs.commitRateInBitsPerSec = 0U;

        ENET_IOCTL_SET_INOUT_ARGS(&prms, &setPolicerInArgs, &setPolicerOutArgs);

        status = Enet_ioctl(hClient->hEnet, hostId, CPSW_ALE_IOCTL_SET_POLICER, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                          "Failed to setup EtherType based route");
        status = CPSWPROXY_ENET2RPMSG_ERR(status);
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
        ETHFWTRACE_ERR(status, "EtherType route is not supported on virtual MAC ports");
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
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    isSwitchPort = EthRemoteCfg_isSwitchPort(hClient->virtPort);
    if (!isSwitchPort)
    {
        memset(&delPolicerInArgs, 0, sizeof(delPolicerInArgs));
        delPolicerInArgs.policerMatch.policerMatchEnMask = CPSW_ALE_POLICER_MATCH_ETHERTYPE;
        delPolicerInArgs.policerMatch.etherType = etherType;
        delPolicerInArgs.aleEntryMask = CPSW_ALE_POLICER_TABLEENTRY_DELETE_ETHERTYPE;

        ENET_IOCTL_SET_IN_ARGS(&prms, &delPolicerInArgs);

        status = Enet_ioctl(hClient->hEnet, hostId, CPSW_ALE_IOCTL_DEL_POLICER, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                          "Failed to teardown EtherType based route");
        status = CPSWPROXY_ENET2RPMSG_ERR(status);
    }
    else
    {
        status = ETHREMOTECFG_CMDSTATUS_ENOTSUPPORTED;
        ETHFWTRACE_ERR(status, "EtherType route is not supported on virtual MAC ports");
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
    int32_t status = ENET_SOK;

    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    if (hwPushNum >= CPSW_CPTS_HWPUSH_COUNT_MAX)
    {
        status = ETHREMOTECFG_CMDSTATUS_EBADARGS;
        ETHFWTRACE_ERR(status, "Invalid HW push num %u", hwPushNum);
    }

    /* Register hardware push callback */
    if (status == ENET_SOK)
    {
        hwPushCbInArgs.hwPushNum = (CpswCpts_HwPush)hwPushNum;
        hwPushCbInArgs.hwPushNotifyCb = CpswProxyServer_hwPushNotifyFxn;
        hwPushCbInArgs.hwPushNotifyCbArg = (void *)hServer;

        ENET_IOCTL_SET_IN_ARGS(&prms, &hwPushCbInArgs);

        status = Enet_ioctl(hClient->hEnet, hostId, CPSW_CPTS_IOCTL_REGISTER_HWPUSH_CALLBACK, &prms);
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

    if (hwPushNum >= CPSW_CPTS_HWPUSH_COUNT_MAX)
    {
        status = ETHREMOTECFG_CMDSTATUS_EBADARGS;
        ETHFWTRACE_ERR(status, "Invalid HW push num %u", hwPushNum);
    }

    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    /* Unregister hardware push callback */
    if (status == ENET_SOK)
    {
        hwPushNum = (CpswCpts_HwPush)hwPushNum;
        ENET_IOCTL_SET_IN_ARGS(&prms, &hwPushNum);
        status = Enet_ioctl(hClient->hEnet, hostId, CPSW_CPTS_IOCTL_UNREGISTER_HWPUSH_CALLBACK, &prms);
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
    int32_t status;
    Enet_Handle hEnet = hClient->hEnet;
    Enet_IoctlPrms prms;
    uint64_t inArgsBuf[(ETHREMOTECFG_IOCTL_INARGS_LEN/sizeof(uint64_t)) + 1];
    uint64_t outArgsBuf[(ETHREMOTECFG_IOCTL_OUTARGS_LEN/sizeof(uint64_t)) + 1];

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

        status = Enet_ioctl(hEnet, hostId, cmd, &prms);
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
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

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
        if ((vlanId == 0) || (vlanId == ETHREMOTECFG_ETHSWITCH_VLAN_USE_DFLT))
        {
            if (EthRemoteCfg_isSwitchPort(hClient->virtPort))
            {
                vlanId = hServer->dfltVlanIdSwitchPorts;
            }
            else
            {
                vlanId = hServer->dfltVlanIdMacOnlyPorts;
            }
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
        status = EthFwMcast_filterAddMac(hClient->virtPort, hClient->hEnet,
                                         macAddr, vlanId, flowIdxOffset, hostId);
        if (status != ETHFW_SOK)
        {
            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            ETHFWTRACE_ERR(status, "Failed to add multicast");
        }
    }

    return status;
}

static int32_t CpswProxyServer_filterDelMacHandlerCb(CpswProxyServer_ClientHandle hClient,
                                                     uint32_t hostId,
                                                     uint8_t *macAddr,
                                                     uint16_t vlanId)
{
    CpswProxyServer_Obj *hServer;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;

    /* Check that server itself is ready */
    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    if (!EnetUtils_isMcastAddr(macAddr))
    {
        status = ETHREMOTECFG_CMDSTATUS_EBADARGS;
        ETHFWTRACE_ERR(status, "Addr is not multicast, cannot add/delete to filter");
    }

    /* Check if client is part of the VLAN */
    if (status == ETHREMOTECFG_CMDSTATUS_OK)
    {
        if ((vlanId == 0) || (vlanId == ETHREMOTECFG_ETHSWITCH_VLAN_USE_DFLT))
        {
            /* If client is not in a VLAN, it will pass VLAN id 0 or VLAN_USE_DFLT,
             * but we need to use the actual default VLAN id */
            if (EthRemoteCfg_isSwitchPort(hClient->virtPort))
            {
                vlanId = hServer->dfltVlanIdSwitchPorts;
            }
            else
            {
                vlanId = hServer->dfltVlanIdMacOnlyPorts;
            }
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
        status = EthFwMcast_filterDelMac(hClient->virtPort, hClient->hEnet,
                                         macAddr, vlanId, hostId);
        if (status != ETHFW_SOK)
        {
            status = ETHREMOTECFG_CMDSTATUS_EFAIL;
            ETHFWTRACE_ERR(status, "Failed to remove multicast");
        }
    }

    return status;
}

int32_t CpswProxyServer_init(CpswProxyServer_Config_t *cfg)
{
    SemaphoreP_Params sem_params;
    CpswProxyServer_Obj *hServer;
    RPMessage_Params cntrlParam;
    int32_t i;
    int32_t status = CPSWPROXYSERVER_SOK;

    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == false));

    hServer->instId = cfg->instId;

    hServer->dfltVlanIdMacOnlyPorts = cfg->dfltVlanIdMacOnlyPorts;
    hServer->dfltVlanIdSwitchPorts  = cfg->dfltVlanIdSwitchPorts;

    hServer->alePortMask = cfg->enabledPortMask;
    hServer->aleMacOnlyPortMask = cfg->macOnlyPortMask;

    if ((hServer->aleMacOnlyPortMask & hServer->alePortMask) !=
        hServer->aleMacOnlyPortMask)
    {
        status = CPSWPROXYSERVER_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "MAC ports required for virtual MAC ports are not enabled");
    }

    hServer->aleSwitchOnlyPortMask = (hServer->alePortMask &
                                      ~hServer->aleMacOnlyPortMask);

    memcpy(&hServer->allocObj, &cfg->allocObj, sizeof(cfg->allocObj));

    if (status == CPSWPROXYSERVER_SOK)
    {
        hServer->hMutex = MutexP_create(&hServer->mutexObj);

        SemaphoreP_Params_init(&sem_params);
        sem_params.mode = SemaphoreP_Mode_BINARY;
        hServer->clientServiceObj.rpmsgStartSem = SemaphoreP_create(0, &sem_params);
        EnetAppUtils_assert(hServer->clientServiceObj.rpmsgStartSem != NULL);

        hServer->getMcmCmdIfCb = cfg->getMcmCmdIfCb;
        hServer->initEthfwDeviceDataCb = cfg->initEthfwDeviceDataCb;

        CpswProxyServer_initClientHandle(cfg);

        ETHFWTRACE_INFO("Virtual port configuration:");

        EnetAppUtils_assert(cfg->autosarEthVirtPortNum <= ENET_ARRAYSIZE(cfg->autosarPortCfg));
        EnetAppUtils_assert(cfg->autosarEthVirtPortNum <= ENET_ARRAYSIZE(g_CpswProxyServerAutosarRpmsgBuf));
        EnetAppUtils_assert(cfg->autosarEthVirtPortNum <= ENET_ARRAYSIZE(hServer->ethDrvObj));
        EnetAppUtils_assert(cfg->autosarEthVirtPortNum <= ENET_ARRAYSIZE(gCpswProxyServer_autosarEthDriverTaskStackBuf));

        for (i = 0U; i < cfg->autosarEthVirtPortNum; i++)
        {
            status = CpswProxyServer_initAutosarEthDeviceEp(hServer, cfg, i);
            EnetAppUtils_assert(status == CPSWPROXYSERVER_SOK);
        }

        status = CpswProxyServer_initRemoteClientEthDeviceEp(hServer, cfg);
        EnetAppUtils_assert(status == CPSWPROXYSERVER_SOK);

        status = CpswProxyServer_initNotifyServiceEp(hServer, cfg);
        EnetAppUtils_assert(status == CPSWPROXYSERVER_SOK);

        hServer->initDone = true;
    }

    ETHFWTRACE_INFO("CpswProxyServer: initialization %s (core: mcu2_0)",
                    (status == CPSWPROXYSERVER_SOK) ? "completed" : "failed");

    return status;
}

static int32_t CpswProxyServer_dumpStatsCb(CpswProxyServer_ClientHandle hClient,
                                           uint32_t hostId)
{
    Enet_Handle hEnet = hClient->hEnet;
    CpswProxyServer_Obj *hServer;
    int32_t status = ETHREMOTECFG_CMDSTATUS_OK;
    Enet_IoctlPrms prms;

    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    ENET_IOCTL_SET_NO_ARGS(&prms);
    status = Enet_ioctl(hEnet, hostId, CPSW_ALE_IOCTL_DUMP_TABLE, &prms);
    EnetAppUtils_assert(status == ENET_SOK);

    ENET_IOCTL_SET_NO_ARGS(&prms);
    status = Enet_ioctl(hEnet, hostId, CPSW_ALE_IOCTL_DUMP_POLICER_ENTRIES, &prms);
    EnetAppUtils_assert(status == ENET_SOK);

    CpswProxyServer_printStats(hEnet, hServer->enetType, hostId);

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
            hClient = CpswProxyServer_allocClient(remoteEndPt);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_attachHandlerCb(hClient,
                                                     remoteProcId,
                                                     req->virtPort,
                                                     &res->rxMtu,
                                                     &res->txMtu[0U],
                                                     ENET_ARRAYSIZE(res->txMtu),
                                                     &res->features);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                              "Failed to attach virtual port %u core %u",
                              req->virtPort, remoteProcId);

            token = hClient->token;
            hClient->clientId = clientId;
            resLen = sizeof(EthRemoteCfg_AttachRes);

            ETHFWTRACE_INFO("ATTACH | S2C | token=%u rxMtu=%u features=%x",
                            token, res->rxMtu, res->features);
            break;
        }
        case ETHREMOTECFG_CMD_ATTACH_EXT:
        {
            EthRemoteCfg_AttachReq *req = (EthRemoteCfg_AttachReq *)reqBuf;
            EthRemoteCfg_AttachExtRes *res = (EthRemoteCfg_AttachExtRes *)resBuf;

            ETHFWTRACE_INFO("ATTACH_EXT | C2S | core=%u endpt=%u virtPort=%u",
                            remoteProcId, remoteEndPt, req->virtPort);

            /* Allocate a client object */
            hClient = CpswProxyServer_allocClient(remoteEndPt);
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
                                                        &res->macAddr[0U]);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                              "Failed to attach (ext) virtual port %u core %u",
                              req->virtPort, remoteProcId);

            token = hClient->token;
            hClient->clientId = clientId;
            resLen = sizeof(EthRemoteCfg_AttachExtRes);

            ETHFWTRACE_INFO("ATTACH_EXT | S2C | token=%u rxMtu=%u features=%x flow=%u,%u "
                            "psil=%u macAddr=%02x:%02x:%02x:%02x:%02x:%02x",
                            token, res->rxMtu, res->features,
                            res->rxFlowIdxBase, res->rxFlowIdxOffset,
                            res->txPsilDstId,
                            res->macAddr[0U], res->macAddr[1U], res->macAddr[2U],
                            res->macAddr[3U], res->macAddr[4U], res->macAddr[5U]);
            break;
        }
        case ETHREMOTECFG_CMD_DETACH:
        {
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DETACH | C2S | core=%u endpt=%u token=%u",
                            remoteProcId, remoteEndPt, token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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
            EthRemoteCfg_AllocTxRes *res = (EthRemoteCfg_AllocTxRes *)resBuf;

            ETHFWTRACE_INFO("ALLOC_TX | C2S | core=%u endpt=%u token=%u",
                            remoteProcId, remoteEndPt, token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_allocTxHandlerCb(hClient,
                                                      remoteProcId,
                                                      &res->txPsilDstId);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to alloc TX channel");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("ALLOC_TX | S2C | psil=%u, status=%d",
                            res->txPsilDstId, status);
            break;
        }
        case ETHREMOTECFG_CMD_ALLOC_RX:
        {
            EthRemoteCfg_AllocRxRes *res = (EthRemoteCfg_AllocRxRes *)resBuf;

            ETHFWTRACE_INFO("ALLOC_RX | C2S | core=%u endpt=%u token=%u",
                            remoteProcId, remoteEndPt, token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_allocRxHandlerCb(hClient,
                                                      remoteProcId,
                                                      &res->rxFlowIdxBase,
                                                      &res->rxFlowIdxOffset);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to alloc RX flow");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("ALLOC_RX | S2C | rxflow=%u,%u, status=%d",
                            res->rxFlowIdxBase, res->rxFlowIdxOffset, status);
            break;
        }
        case ETHREMOTECFG_CMD_ALLOC_MAC:
        {
            EthRemoteCfg_AllocMacRes *res = (EthRemoteCfg_AllocMacRes *)resBuf;

            ETHFWTRACE_INFO("ALLOC_MAC | C2S | core=%u endpt=%u token=%u",
                            remoteProcId, remoteEndPt, token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_allocMacHandlerCb(hClient,
                                                      remoteProcId,
                                                      &res->macAddr[0U]);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to alloc MAC addr");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("ALLOC_MAC | S2C | psil=%u, macAddr=%02x:%02x:%02x:%02x:%02x:%02x",
                            res->macAddr[0U], res->macAddr[1U], res->macAddr[2U],
                            res->macAddr[3U], res->macAddr[4U], res->macAddr[5U]);
            break;
        }
        case ETHREMOTECFG_CMD_FREE_TX:
        {
            EthRemoteCfg_FreeTxReq *req = (EthRemoteCfg_FreeTxReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("FREE_TX | C2S | core=%u endpt=%u token=%u psil=%u",
                            remoteProcId, remoteEndPt, token, req->txPsilDstId);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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

            ETHFWTRACE_INFO("FREE_RX | C2S | core=%u endpt=%u token=%u flowidx=%u,%u",
                            remoteProcId, remoteEndPt, token, req->rxFlowIdxBase, req->rxFlowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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

            ETHFWTRACE_INFO("FREE_MAC | C2S | core=%u endpt=%u token=%u "
                            "macAdd=%02x:%02x:%02x:%02x:%02x:%02x",
                            remoteProcId, remoteEndPt, token,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U]);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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

            ETHFWTRACE_INFO("REGISTER_MAC | C2S | core=%u endpt=%u token=%u "
                            "macAdd=%02x:%02x:%02x:%02x:%02x:%02x flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, token,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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

            ETHFWTRACE_INFO("DEREGISTER_MAC | C2S | core=%u endpt=%u token=%u "
                            "macAdd=%02x:%02x:%02x:%02x:%02x:%02x flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, token,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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

            ETHFWTRACE_INFO("REGISTER_IPv4 | C2S | core=%u endpt=%u token=%u "
                            "ipAddr=%u.%u.%u.%u macAdd=%02x:%02x:%02x:%02x:%02x:%02x",
                            remoteProcId, remoteEndPt, token,
                            req->ipAddr[0U], req->ipAddr[1U], req->ipAddr[2U], req->ipAddr[3U],
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U]);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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

            ETHFWTRACE_INFO("DEREGISTER_IPv4 | C2S | core=%u endpt=%u token=%u ipAddr=%u.%u.%u.%u",
                            remoteProcId, remoteEndPt, token,
                            req->ipAddr[0U], req->ipAddr[1U], req->ipAddr[2U], req->ipAddr[3U]);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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

            ETHFWTRACE_INFO("JOIN_VLAN | C2S | core=%u endpt=%u token=%u vlanId=%u "
                            "macAdd=%x:%x:%x:%x:%x:%x flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, token, req->vlanId,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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

            ETHFWTRACE_INFO("LEAVE_VLAN | C2S | core=%u endpt=%u token=%u vlanId=%u "
                            "macAdd=%x:%x:%x:%x:%x:%x flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, token, req->vlanId,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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

            ETHFWTRACE_INFO("ENABLE_PROMISC | C2S | core=%u endpt=%u token=%u",
                            remoteProcId, remoteEndPt, token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_promiscModeHandlerCb(hClient,
                                                          remoteProcId,
                                                          true);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to enable promisc mode");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("ENABLE_PROMISC | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_DISABLE_PROMISC:
        {
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)resBuf;

            ETHFWTRACE_INFO("DISABLE_PROMISC | C2S | core=%u endpt=%u token=%u",
                            remoteProcId, remoteEndPt, token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_promiscModeHandlerCb(hClient,
                                                          remoteProcId,
                                                          false);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to disable promisc mode");

            resLen = sizeof(*res);

            ETHFWTRACE_INFO("DISABLE_PROMISC | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_SET_RX_DEFAULTFLOW:
        {
            EthRemoteCfg_RxDefaultFlowRegisterReq *req = (EthRemoteCfg_RxDefaultFlowRegisterReq *)reqBuf;
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)res;

            ETHFWTRACE_INFO("SET_RX_DEFAULTFLOW | C2S | core=%u endpt=%u token=%u flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, token, req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)res;

            ETHFWTRACE_INFO("DEL_RX_DEFAULTFLOW | C2S | core=%u endpt=%u token=%u flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, token, req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)res;

            ETHFWTRACE_INFO("REGISTER_MATCH_ETHTYPE | C2S | core=%u endpt=%u token=%u "
                            "ethType=%x flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, token,
                            req->ethertype, req->flowIdxBase, req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)res;

            ETHFWTRACE_INFO("DEREGISTER_MATCH_ETHTYPE | C2S | core=%u endpt=%u token=%u ethType=%x",
                            remoteProcId, remoteEndPt, token, req->ethertype);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)res;

            ETHFWTRACE_INFO("ADD_FILTER_MAC | C2S | core=%u endpt=%u token=%u "
                            "macAdd=%x:%x:%x:%x:%x:%x vlanId=%u flowIdx=%u,%u",
                            remoteProcId, remoteEndPt, token,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->vlanId, req->flowIdxBase,req->flowIdxOffset);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)res;

            ETHFWTRACE_INFO("DEL_FILTER_MAC | C2S | core=%u endpt=%u token=%u "
                            "macAdd=%x:%x:%x:%x:%x:%x vlanId=%u",
                            remoteProcId, remoteEndPt, token,
                            req->macAddr[0U], req->macAddr[1U], req->macAddr[2U],
                            req->macAddr[3U], req->macAddr[4U], req->macAddr[5U],
                            req->vlanId);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
            EnetAppUtils_assert(hClient != NULL);

            status = CpswProxyServer_isLinkUpCb(hClient,
                                                remoteProcId,
                                                &res->isLinked,
                                                &res->speed,
                                                &res->duplexity);
            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to get port link params");

            resLen = sizeof(*res);
            break;
        }
        case ETHREMOTECFG_CMD_READ_REGISTER:
        {
            EthRemoteCfg_RegReadReq *req = (EthRemoteCfg_RegReadReq *)reqBuf;
            EthRemoteCfg_RegReadRes *res = (EthRemoteCfg_RegReadRes *)resBuf;

            ETHFWTRACE_INFO("READ_REGISTER | C2S | core=%u endpt=%u reg=0x%08x",
                            remoteProcId, remoteEndPt, req->addr);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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
            EthRemoteCfg_StatusRes *res = (EthRemoteCfg_StatusRes *)res;

            ETHFWTRACE_INFO("WRITE_REGISTER | C2S | core=%u endpt=%u reg=0x%08x val=0x%08x",
                            remoteProcId, remoteEndPt, req->addr, req->val);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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
            hClient = CpswProxyServer_getClient(token);
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
            hClient = CpswProxyServer_getClient(token);
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
            EthRemoteCfg_StatusRes *res;

            ETHFWTRACE_INFO("TEARDOWN_COMPLETION | C2S | core=%u endpt=%u token=%u",
                            remoteProcId, remoteEndPt, token);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
            EnetAppUtils_assert(hClient != NULL);

            hClient->isIdle = true;
            resLen = sizeof(*res);
            status = ETHREMOTECFG_CMDSTATUS_OK;

            ETHFWTRACE_INFO("TEARDOWN_COMPLETION | S2C | status=%d", status);
            break;
        }
        case ETHREMOTECFG_CMD_IOCTL:
        {
            EthRemoteCfg_IoctlReq *req = (EthRemoteCfg_IoctlReq *)reqBuf;
            EthRemoteCfg_IoctlRes *res;

            ETHFWTRACE_INFO("IOCTL | C2S | core=%u endpt=%u cmd=%x inArgsLen=%u inArgs=%p outArgsLen=%u",
                            remoteProcId, remoteEndPt, req->cmd,
                            req->inArgsLen, req->inArgs, req->outArgsLen);

            /* Get client object for token */
            hClient = CpswProxyServer_getClient(token);
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
            hClient = CpswProxyServer_getClient(token);
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

    ETHFWTRACE_INFO("S2C | msgType=%u token=%u clientId=%u resId=%u status=%d (ep=%u->%u)",
                   resHdr->common.msgType,
                   resHdr->common.token,
                   resHdr->common.clientId,
                   resHdr->resId,
                   resHdr->status,
                   localEp, remoteEndPt);

    rtnVal = RPMessage_send(hMsgHandle, remoteProcId, remoteEndPt, localEp, &resBuf, resLen);
    ETHFWTRACE_ERR_IF((rtnVal != IPC_SOK), rtnVal, "Failed to send msg via IPC");
    EnetAppUtils_assert(IPC_SOK == rtnVal);
}

static int32_t CpswProxyServer_initRemoteClientEthDeviceEp(CpswProxyServer_Obj *hServer,
                                                           CpswProxyServer_Config_t * cfg)
{
    TaskP_Params taskParams;
    int32_t status = CPSWPROXYSERVER_SOK;
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
        status = CPSWPROXYSERVER_EFAIL;
        ETHFWTRACE_ERR(status, "Could not create communication channel for endpoint %d",
                       comParams.requestedEndpt);
    }
    else
    {
        SemaphoreP_post(hServer->clientServiceObj.rpmsgStartSem);
    }

    if (CPSWPROXYSERVER_SOK == status)
    {
        hServer->clientServiceObj.localEp = localEp;
    }

    if (CPSWPROXYSERVER_SOK == status)
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
            status = CPSWPROXYSERVER_EFAIL;
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
        exitTask = false;

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
    int32_t retVal = CPSWPROXYSERVER_SOK;
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
        retVal = CPSWPROXYSERVER_EFAIL;
        ETHFWTRACE_ERR(retVal, "Could not create communication channel for endpoint %d",
                       comChParam.requestedEndpt);
    }

    if (CPSWPROXYSERVER_SOK == retVal)
    {
        hServer->ethDrvObj[clientInst].localEp = localEp;
    }

    if (CPSWPROXYSERVER_SOK == retVal)
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
            retVal = CPSWPROXYSERVER_EFAIL;
            ETHFWTRACE_ERR(retVal, "Could not create task for endpoint %d", comChParam.requestedEndpt);
        }
    }

    return retVal;
}

static int32_t CpswProxyServer_initNotifyServiceEp(CpswProxyServer_Obj * hServer, CpswProxyServer_Config_t * cfg)
{
    TaskP_Params taskParams;
    int32_t retVal = CPSWPROXYSERVER_SOK;
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
    }

    RPMessageParams_init(&comChParam);
    comChParam.numBufs = ETHREMOTECFG_IPC_NUM_MSG_BUFS;
    comChParam.buf = g_CpswProxyServerNotifyServiceRpmsgBuf;
    comChParam.bufSize = sizeof(g_CpswProxyServerNotifyServiceRpmsgBuf);
    hServer->notifyServiceObj.hNotifyServicRpMsgEp = RPMessage_create(&comChParam, &localEp);

    if (NULL == hServer->notifyServiceObj.hNotifyServicRpMsgEp)
    {
        retVal = CPSWPROXYSERVER_EFAIL;
        ETHFWTRACE_ERR(retVal, "Could not create communication channel");
    }

    if (CPSWPROXYSERVER_SOK == retVal)
    {
        hServer->notifyServiceObj.localEp = localEp;
    }

    /* Announce service */
    if (CPSWPROXYSERVER_SOK == retVal)
    {
        retVal = RPMessage_announce(RPMESSAGE_ALL,
                                    hServer->notifyServiceObj.localEp,
                                    ETHREMOTECFG_REMOTE_NOTIFY_SERVICE);
        ETHFWTRACE_ERR_IF((retVal != IPC_SOK), retVal, "Failed to annount notify server");
    }

    /* Create Event to notify task */
    if (CPSWPROXYSERVER_SOK == retVal)
    {
        EventP_Params_init(&eventParams);

        for (i = 0U; i < CPSW_CPTS_HWPUSH_COUNT_MAX; i++)
        {
            hServer->notifyServiceObj.hwPushNotifyEventId[i] = (1U << i);
        }

        hServer->notifyServiceObj.hHwPushNotifyServiceEvent = EventP_create(&eventParams);

        if (hServer->notifyServiceObj.hHwPushNotifyServiceEvent == NULL)
        {
            retVal = CPSWPROXYSERVER_EFAIL;
            ETHFWTRACE_ERR(retVal, "Could not create an event");
        }
    }

    if (CPSWPROXYSERVER_SOK == retVal)
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
            retVal = CPSWPROXYSERVER_EFAIL;
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
    volatile bool exitTask = false;
    Enet_IoctlPrms prms;
    CpswCpts_Event lookupEventInArgs;
    CpswCpts_Event lookupEventOutArgs;
    uint32_t i = 0U;
    uint32_t events = 0U;
    uint32_t remoteCoreId;

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

                        memset(hwPushMsg, 0, sizeof(*hwPushMsg));
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
                                ENET_IS_BIT_SET(hServer->notifyServiceObj.dstProcMask, remoteCoreId))
                            {
                                hwPushMsg->timeStamp = lookupEventOutArgs.tsVal;

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
        bool exitTask = false;

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

    hClient = CpswProxyServer_getClient(token);
    EnetAppUtils_assert(hClient != NULL);

    hServer = CpswProxyServer_getHandle();
    EnetAppUtils_assert((hServer != NULL) && (hServer->initDone == true));

    hEnet = hClient->hEnet;
    EnetAppUtils_assert(hEnet != NULL);

    coreKey = hClient->coreKey;

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
        ETHFWTRACE_ERR(ETHFW_EINVALIDPARAMS, "Invalid number of alloc objects%u, max %u",
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
