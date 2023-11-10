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

/* EthFwTrace id for this module, must be unique within ETHFW */
#define ETHFWTRACE_MOD_ID 0x201

#include <stdio.h>
#include <stdint.h>

#ifdef QNX_OS
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/slogcodes.h>
#else
#if defined(__KLOCWORK__)
#include <stdlib.h>
#endif
#endif

/* OSAL */
#include <ti/osal/osal.h>
#include <ti/osal/MutexP.h>
#include <ti/osal/SemaphoreP.h>
#include <ti/osal/TaskP.h>
#include <ti/osal/MailboxP.h>

#include <ethremotecfg/protocol/ethremotecfg.h>
#include <ethremotecfg/client/include/cpsw_proxy.h>
#include <ethremotecfg/protocol/ethremotecfg_virtport.h>

#include <ti/drv/ipc/ipc.h>
#include <ti/drv/enet/enet.h>

/* EthFw utils header files */
#include <ti/drv/enet/examples/utils/include/enet_apputils.h>

#if defined(SAFERTOS)
#include "SafeRTOS_API.h"
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* Maximum number of supported CpswProxy clients */
#define CPSWPROXY_CLIENT_MAX                            (2U)

#define CPSWPROXY_RES_MSGSIZE                           (256U)

#define CPSWPROXY_RES_MSGCOUNT                          (3U)

#define CPSWPROXY_CONNECT_RETRY_MS                      (10U)

#define CPSWPROXY_LOCATE_TIMEOUT                        (10U)

#if defined(SAFERTOS)
#define CPSWPROXY_RES_MBOX_SIZE                         (CPSWPROXY_RES_MSGSIZE * CPSWPROXY_RES_MSGCOUNT + safertosapiQUEUE_OVERHEAD_BYTES)
#else
#define CPSWPROXY_RES_MBOX_SIZE                         (CPSWPROXY_RES_MSGSIZE * CPSWPROXY_RES_MSGCOUNT)
#endif

#define CPSWPROXY_CLIENT_CMD_TASK_NAME                  "R5CLIENTDEVICE"
#ifdef QNX_OS
#define CPSWPROXY_RDEVCMD_TSK_PRI                       (22U)
#else
#define CPSWPROXY_RDEVCMD_TSK_PRI                       (2U)
#endif
#define CPSWPROXY_IPC_TASK_STACKALIGN                   (8192U)

/*! Remote notify service task stack size */
#define CPSWPROXY_NOTIFY_SERVICE_CLIENT_TASK_STACKSIZE  (16U * 1024U)

#define CPSWPROXY_NOTIFY_SERVICE_TASK_STACKALIGN        CPSWPROXY_NOTIFY_SERVICE_CLIENT_TASK_STACKSIZE

#define CPSWPROXY_NOTIFY_SERVICE_TASK_PRIORITY          (2U)

#if defined(__KLOCWORK__)
#define CpswProxy_assert(cond)               do { if (!(cond)) abort(); } while (0)
#else
#define CpswProxy_assert(cond)                                   \
    (CpswProxy_assertLocal((bool) (cond), (const char *) # cond, \
                    (const char *) __FILE__, (int32_t) __LINE__))
#endif

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/*!
 * \brief CPSW Remote notify service callback handlers
 */
typedef struct CpswProxy_NotifyServiceCallbackHandlers_s
{
    /*! Hardware push notify handler */
     CpswProxy_hwPushNotifyCbFxn hwPushCb;

    /*! Hardware push notify arguments */
    void *hwPushCbArg;
} CpswProxy_NotifyServiceCallbackHandlers;

typedef struct CpswProxy_notifyServiceObj_s
{
    /* Task handle for remote notify service */
    TaskP_Handle hNotifyServiceTsk;

    /* RPMessage handle for remote notify service */
    RPMessage_Handle hNotifyServicRpMsgEp;

    /* Local endpoint for notify service in proxy client */
    uint32_t localEp;

    /* Notify service callback handlers, cbFxn and cbArgs */
    CpswProxy_NotifyServiceCallbackHandlers cb;

    /* Buffer to store received messages for remote notify service */
    uint8_t rpmsgBuf[ETHREMOTECFG_IPC_DATA_SIZE] __attribute__ ((aligned(8192)));

    /* Notify service task stack buffer */
    uint8_t taskStack[CPSWPROXY_NOTIFY_SERVICE_CLIENT_TASK_STACKSIZE] __attribute__ ((aligned(CPSWPROXY_NOTIFY_SERVICE_TASK_STACKALIGN)));
} CpswProxy_notifyServiceObj;
typedef struct CpswProxy_ClientObj_s
{
    CpswProxy_Config cfg;

    /* Whether client object is already in used by an app */
    bool inUse;

    /* Token used after attaching to ETHFW */
    uint32_t token;

    /* Sequential request id number */
    uint32_t reqId;

    /* Features supported by related virtual port */
    uint32_t features;
} CpswProxy_ClientObj;

typedef struct CpswProxy_Obj_s
{
    /* Mutex object used to protect get/free CpswProxy_ClientObjs */
    MutexP_Object mutexObj;

    /* Handle to mutexObj */
    MutexP_Handle hMutex;

    /* Array of client objects. Size of this array determines the number of virtual ports
     * that can be used by this core */
    CpswProxy_ClientObj clientObj[CPSWPROXY_CLIENT_MAX];

    /* Master core id where Cpsw Proxy Server runs */
    uint32_t masterCoreId;

    /* Endpoint associated with the underlying remote_device used by CpswProxyServer */
    uint32_t masterEndpt;

    /* Local endpoint for handling messages from ETHFW */
    uint32_t localEndpt;

    /* RPMessage handle for ETHFW Proxy Service */
    RPMessage_Handle hEthfwServiceRpMsg;

    /* Timestamp event notify service */
    CpswProxy_notifyServiceObj notifyServiceObj;

#ifdef QNX_OS
    int chid;
    int coid;
#else
    /* Mailbox handle for response messages */
    MailboxP_Handle hResMbx;

    /* Mailbox Buffer for storing all the response messages */
    uint8_t resMbxBuf[CPSWPROXY_RES_MBOX_SIZE] __attribute__ ((aligned(32)));
#endif

    /* Task handle for message handling */
    TaskP_Handle hMsgHandlerTsk;
} CpswProxy_Obj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static void CpswProxy_sendCmd(CpswProxy_Handle hProxy,
                              uint32_t reqType,
                              void *reqMsg,
                              uint16_t reqLen,
                              void *resMsg,
                              uint16_t resLen);

static void CpswProxy_msgHandlerTskFxn(void* arg0,
                                       void* arg1);

static void CpswProxy_notifyServiceTskFxn(void* a0, void* a1);

#ifdef QNX_OS
static void slog_printf(const char *pcString, ...);
#define System_printf slog_printf
#else
// TODO: Need to replace with Ipc_Trace_printf
#define System_printf printf
#endif

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

static CpswProxy_Obj gCpswProxy;

static uint8_t gCpswProxyClientRpMsgbuf[ETHREMOTECFG_IPC_DATA_SIZE] __attribute__ ((aligned(1024)));

static uint8_t msgHandlerTaskBuf[IPC_TASK_STACKSIZE] __attribute__ ((section(".bss:taskStackSection"))) __attribute__ ((aligned(CPSWPROXY_IPC_TASK_STACKALIGN)));

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

#if !defined(__KLOCWORK__)
static void CpswProxy_assertLocal(bool condition,
                                  const char *str,
                                  const char *fileName,
                                  int32_t lineNum)
{
    volatile static bool gCpswProxyAssertWaitInLoop = TRUE;

    if (!(condition))
    {
        System_printf("Assertion @ Line: %d in %s: %s : failed !!!\n",
                           lineNum, fileName, str);
#ifdef QNX_OS
        gCpswProxyAssertWaitInLoop = FALSE;
#endif
        while (gCpswProxyAssertWaitInLoop)
        {
        }
    }

    return;
}
#endif

#ifdef QNX_OS
static void slog_printf(const char *pcString, ...)
{
    char printBuffer[256];
    va_list arguments;

    if (256 < strlen(pcString))
    {
        assert(false);
    }

    /* Start the varargs processing */
    va_start(arguments, pcString);
    vsnprintf(printBuffer, sizeof(printBuffer), pcString, arguments);

    slogf(_SLOGC_NETWORK, _SLOG_INFO, printBuffer);

    /* End the varargs processing */
    va_end(arguments);
}
#endif

int32_t CpswProxy_createEndpts(void)
{
    RPMessage_Params params;
    int32_t status = ENET_SOK;

    /* Create the RPMSG endpoint */
    RPMessageParams_init(&params);
    params.requestedEndpt = RPMESSAGE_ANY;
    params.buf            = gCpswProxyClientRpMsgbuf;
    params.bufSize        = sizeof(gCpswProxyClientRpMsgbuf);

    gCpswProxy.hEthfwServiceRpMsg = RPMessage_create(&params, &gCpswProxy.localEndpt);
    if (gCpswProxy.hEthfwServiceRpMsg == NULL)
    {
        status = ENET_EFAIL;
        System_printf("Failed to create endpt: %d\n", status);
    }

    return status;
}

void CpswProxy_deleteEndpts(void)
{
    int32_t status = ENET_SOK;

    if (gCpswProxy.hEthfwServiceRpMsg != NULL)
    {
        status = RPMessage_delete(&gCpswProxy.hEthfwServiceRpMsg);
        if (status != IPC_SOK)
        {
            System_printf("Failed to delete endpt: %d\n", status);
        }

        gCpswProxy.hEthfwServiceRpMsg = NULL;
    }
}

static void CpswProxy_createNotifyServiceTask(void)
{
    CpswProxy_notifyServiceObj *notifyObj = &gCpswProxy.notifyServiceObj;
    TaskP_Params taskParams;

    TaskP_Params_init(&taskParams);
    taskParams.priority  = CPSWPROXY_NOTIFY_SERVICE_TASK_PRIORITY;
    taskParams.arg0      = (void *)notifyObj;
    taskParams.stack     = &notifyObj->taskStack[0];
    taskParams.stacksize = sizeof(notifyObj->taskStack);

    notifyObj->hNotifyServiceTsk = TaskP_create(&CpswProxy_notifyServiceTskFxn, &taskParams);
    CpswProxy_assert(notifyObj->hNotifyServiceTsk != NULL);
}

static void CpswProxy_notifyServiceTskFxn(void* a0, void* a1)
{
    int32_t ret = CPSWPROXY_SOK;
    CpswProxy_notifyServiceObj *notifyObj = (CpswProxy_notifyServiceObj *)a0;
    RPMessage_Params rpmsgPrm;
    uint32_t localEp;
    uint32_t remoteProcId, remoteEndPt;
    uint32_t remoteProc, remoteEp;
    EthRemoteCfg_NotifyHdr *header = NULL;
    uint16_t len;
    uint64_t msgBuffer[ETHREMOTECFG_IPC_MSG_SIZE / sizeof(uint64_t)];
    volatile bool exitTask = false;
    void *data;
    EthRemoteCfg_NotifyServiceHwPushMsg *hwPushMsg;

    data = (void *)msgBuffer;
    /* Create RPMsg */
    RPMessageParams_init(&rpmsgPrm);
    rpmsgPrm.requestedEndpt = ETHREMOTECFG_NOTIFY_SERVICE_ENDPT_ID;
    rpmsgPrm.buf = notifyObj->rpmsgBuf;
    rpmsgPrm.bufSize = sizeof(notifyObj->rpmsgBuf);
    rpmsgPrm.numBufs = ETHREMOTECFG_IPC_NUM_MSG_BUFS;

    notifyObj->hNotifyServicRpMsgEp = RPMessage_create(&rpmsgPrm, &localEp);

    if (NULL == notifyObj->hNotifyServicRpMsgEp)
    {
        System_printf("Could not create communication channel\n");
        ret = CPSWPROXY_EFAIL;
    }

    if (CPSWPROXY_SOK == ret)
    {
        if (localEp != ETHREMOTECFG_NOTIFY_SERVICE_ENDPT_ID)
        {
            System_printf("Could not create required End Point");
        }
        else
        {
            notifyObj->localEp = localEp;
        }

        /* Wait for Remote EP to active */
        ret = RPMessage_getRemoteEndPt(gCpswProxy.masterCoreId,
                                       ETHREMOTECFG_REMOTE_NOTIFY_SERVICE,
                                       &remoteProcId,
                                       &remoteEndPt,
                                       IPC_RPMESSAGE_TIMEOUT_FOREVER);
        if(ret != 0)
        {
            System_printf("Remote Notify service locate failed\n");
        }

        while (!exitTask)
        {
            ret = RPMessage_recv(notifyObj->hNotifyServicRpMsgEp,
                                 data,
                                 &len,
                                 &remoteEp,
                                 &remoteProc,
                                 IPC_RPMESSAGE_TIMEOUT_FOREVER);
            if (IPC_SOK == ret)
            {
                CpswProxy_assert(len <= sizeof(msgBuffer));
                CpswProxy_assert(remoteEp == remoteEndPt);
                CpswProxy_assert(remoteProcId == remoteProc);

                /* Process received message */
                header = (EthRemoteCfg_NotifyHdr *)data;
                switch(header->notifyType)
                {
                case ETHREMOTECFG_NOTIFY_HWPUSH:
                    {
                        hwPushMsg = (EthRemoteCfg_NotifyServiceHwPushMsg *)data;
                        if (len <= sizeof(data))
                        {
                            System_printf("len is not matching data provided (data len: %u, len: %u)\n", sizeof(data), len);
                        }
                        else if (notifyObj->cb.hwPushCb != NULL)
                        {
                            notifyObj->cb.hwPushCb((CpswCpts_HwPush)hwPushMsg->hwPushNum,
                                                   hwPushMsg->timeStamp,
                                                   notifyObj->cb.hwPushCbArg);
                        }

                        break;
                    }
                    default:
                    {
                        System_printf("CpswProxy_notifyServiceTskFxn: Received unknown notify command: %u\n", header->notifyType);
                        break;
                    }
                }
            }
        }
    }
}

void CpswProxy_init(void)
{
    int32_t status = ENET_SOK;
#ifndef QNX_OS
    MailboxP_Params params;
#endif
    TaskP_Params taskParams;

    memset(&gCpswProxy, 0, sizeof(gCpswProxy));

    gCpswProxy.hMutex = MutexP_create(&gCpswProxy.mutexObj);

#ifdef QNX_OS
    gCpswProxy.chid = ChannelCreate(0);
    CpswProxy_assert(gCpswProxy.chid != -1);
    gCpswProxy.coid = ConnectAttach(ND_LOCAL_NODE, 0, gCpswProxy.chid, _NTO_SIDE_CHANNEL, 0);
    CpswProxy_assert(gCpswProxy.coid != -1);
#else
    MailboxP_Params_init(&params);
    params.name    = (uint8_t *)"ResponseMbx";
    params.size    = CPSWPROXY_RES_MSGSIZE;
    params.count   = CPSWPROXY_RES_MSGCOUNT;
    params.buf     = (void *)gCpswProxy.resMbxBuf;
    params.bufsize = sizeof(gCpswProxy.resMbxBuf);

    gCpswProxy.hResMbx = MailboxP_create(&params);
    CpswProxy_assert(gCpswProxy.hResMbx != NULL);
#endif

    status = CpswProxy_createEndpts();
    if (status != ENET_SOK)
    {
        System_printf("Failed to create all required endpts: %d\n", status);
    }

    if (ENET_SOK == status)
    {
        /* Initialize the task params */
        TaskP_Params_init(&taskParams);
        taskParams.name         = CPSWPROXY_CLIENT_CMD_TASK_NAME;
        taskParams.priority     = CPSWPROXY_RDEVCMD_TSK_PRI;
        taskParams.arg0         = (void *)&gCpswProxy;
        taskParams.stack        = &msgHandlerTaskBuf[0];
        taskParams.stacksize    = sizeof(msgHandlerTaskBuf);

        gCpswProxy.hMsgHandlerTsk = TaskP_create(&CpswProxy_msgHandlerTskFxn, &taskParams);
        if (gCpswProxy.hMsgHandlerTsk == NULL)
        {
            System_printf("Could not create message handler task\n");
            CpswProxy_assert(gCpswProxy.hMsgHandlerTsk != NULL);
        }
    }
}

void CpswProxy_deinit(void)
{
    CpswProxy_deleteEndpts();

#ifdef QNX_OS
    ConnectDetach(gCpswProxy.coid);
    ChannelDestroy(gCpswProxy.chid);
#else
    MailboxP_delete(gCpswProxy.hResMbx);
#endif

    MutexP_delete(gCpswProxy.hMutex);
}

int32_t CpswProxy_connect(void)
{
    int32_t status;

    /* Check if remote_device has been initialized on the server side */
    status = RPMessage_getRemoteEndPt(RPMESSAGE_ANY,
                                      ETHREMOTECFG_FRAMEWORK_SERVICE_NAME,
                                      &gCpswProxy.masterCoreId,
                                      &gCpswProxy.masterEndpt,
                                      CPSWPROXY_LOCATE_TIMEOUT);
    if (status != IPC_SOK)
    {
        System_printf("Remote Device Framework Endpoint locate failed. Retrying !!!\n");
    }
    else
    {
        System_printf("Remote Device Framework Endpoint located. Remote Core Id:%u, Remote End Point:%u\n",
                       gCpswProxy.masterCoreId, gCpswProxy.masterEndpt);

        /* Create time sync notify task */
        CpswProxy_createNotifyServiceTask();
    }

    return status;
}

static CpswProxy_Handle CpswProxy_getHandle(uint32_t token)
{
    CpswProxy_Handle hProxy = NULL;
    CpswProxy_Handle hProxyClient;
    uint32_t i;

    MutexP_lock(gCpswProxy.hMutex, MutexP_WAIT_FOREVER);

    for (i = 0U; i < ENET_ARRAYSIZE(gCpswProxy.clientObj); i++)
    {
        hProxyClient = &gCpswProxy.clientObj[i];

        if ((hProxyClient->inUse) &&
            (hProxyClient->token == token))
        {
            hProxy = hProxyClient;
            break;
        }
    }

    MutexP_unlock(gCpswProxy.hMutex);

    return hProxy;
}

static CpswProxy_Handle CpswProxy_allocHandle(void)
{
    CpswProxy_Handle hProxy = NULL;
    uint32_t i;

    MutexP_lock(gCpswProxy.hMutex, MutexP_WAIT_FOREVER);

    for (i = 0U; i < ENET_ARRAYSIZE(gCpswProxy.clientObj); i++)
    {
        if (!gCpswProxy.clientObj[i].inUse)
        {
            hProxy = &gCpswProxy.clientObj[i];
            hProxy->inUse = true;
            break;
        }
    }

    MutexP_unlock(gCpswProxy.hMutex);

    return hProxy;
}

static void CpswProxy_freeHandle(CpswProxy_Handle hProxy)
{
    MutexP_lock(gCpswProxy.hMutex, MutexP_WAIT_FOREVER);
    memset(hProxy, 0, sizeof(*hProxy));
    hProxy->inUse = false;
    MutexP_unlock(gCpswProxy.hMutex);
}

static void CpswProxy_instanceInit(CpswProxy_Handle hProxy, const CpswProxy_Config *cfg)
{
    hProxy->cfg      = *cfg;
    hProxy->features = 0U;
    hProxy->reqId    = 0U;
    hProxy->token    = ETHREMOTECFG_TOKEN_NONE;
}

static void CpswProxy_instanceDeinit(CpswProxy_Handle hProxy)
{
    memset(&hProxy->cfg, 0, sizeof(hProxy->cfg));
    hProxy->features = 0U;
    hProxy->reqId    = 0U;
}

CpswProxy_Handle CpswProxy_open(const CpswProxy_Config *cfg)
{
    CpswProxy_Handle hProxy;

    /* Get a handle to a free CpswProxy object, if any */
    hProxy = CpswProxy_allocHandle();
    if (hProxy == NULL)
    {
        System_printf("All CpswProxy instances for this core are already in use\n");
    }
    else
    {
        /* Create CpswProxy's mailboxes, init cmd and notify tasks */
        CpswProxy_instanceInit(hProxy, cfg);
    }

    return hProxy;
}

void CpswProxy_close(CpswProxy_Handle hProxy)
{
    /* Delete CpswProxy's mailboxes, close tasks */
    CpswProxy_instanceDeinit(hProxy);

    /* Release handle to CpswProxy object */
    CpswProxy_freeHandle(hProxy);
}

void CpswProxy_allocRxFlow(CpswProxy_Handle hProxy,
                           uint32_t *rxFlowStartIdx,
                           uint32_t *rxFlowIdx)
{
    EthRemoteCfg_CommonReq req;
    EthRemoteCfg_AllocRxRes res;

    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_ALLOC_RX, &req, sizeof(req), &res, sizeof(res));

    *rxFlowStartIdx   = res.rxFlowIdxBase;
    *rxFlowIdx = res.rxFlowIdxOffset;
}

void CpswProxy_allocMac(CpswProxy_Handle hProxy,
                        uint8_t *macAddr)
{
    EthRemoteCfg_CommonReq req;
    EthRemoteCfg_AllocMacRes res;

    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_ALLOC_MAC, &req, sizeof(req), &res, sizeof(res));

    memcpy(macAddr, res.macAddr, ETHREMOTECFG_MACADDRLEN);
}

void CpswProxy_registerDefaultRxFlow(CpswProxy_Handle hProxy,
                                     uint32_t rxFlowStartIdx,
                                     uint32_t freeRxFlowIdx)
{
    EthRemoteCfg_RxDefaultFlowRegisterReq req;
    EthRemoteCfg_StatusRes res;

    req.flowIdxBase = rxFlowStartIdx;
    req.flowIdxOffset = freeRxFlowIdx;

    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_SET_RX_DEFAULTFLOW, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_unregisterDefaultRxFlow(CpswProxy_Handle hProxy,
                                       uint32_t rxFlowStartIdx,
                                       uint32_t freeRxFlowIdx)
{
    EthRemoteCfg_RxDefaultFlowRegisterReq req;
    EthRemoteCfg_StatusRes res;

    req.flowIdxBase = rxFlowStartIdx;
    req.flowIdxOffset = freeRxFlowIdx;

    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_DEL_RX_DEFAULTFLOW, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_registerDstMacRxFlow(CpswProxy_Handle hProxy,
                                    uint32_t rxFlowStartIdx,
                                    uint32_t freeRxFlowIdx,
                                    const uint8_t *macAddr)
{
    EthRemoteCfg_MacAddrRxFlowReq req;
    EthRemoteCfg_StatusRes res;

    req.flowIdxBase = rxFlowStartIdx;
    req.flowIdxOffset = freeRxFlowIdx;
    memcpy(&req.macAddr[0U], macAddr, ETHREMOTECFG_MACADDRLEN);

    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_REGISTER_MAC, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_unregisterDstMacRxFlow(CpswProxy_Handle hProxy,
                                      uint32_t rxFlowStartIdx,
                                      uint32_t freeRxFlowIdx,
                                      const uint8_t *macAddr)
{
    EthRemoteCfg_MacAddrRxFlowReq req;
    EthRemoteCfg_StatusRes res;

    req.flowIdxBase = rxFlowStartIdx;
    req.flowIdxOffset = freeRxFlowIdx;
    memcpy(&req.macAddr[0U], macAddr, ETHREMOTECFG_MACADDRLEN);

    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_DEREGISTER_MAC, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_freeMac(CpswProxy_Handle hProxy,
                       const uint8_t *macAddr)
{
    EthRemoteCfg_FreeMacReq req;
    EthRemoteCfg_StatusRes res;

    memcpy(req.macAddr, macAddr, ETHREMOTECFG_MACADDRLEN);

    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_FREE_MAC, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_freeRxFlow(CpswProxy_Handle hProxy,
                          uint32_t rxFlowStartIdx,
                          uint32_t rxFlowIdx)
{
    EthRemoteCfg_FreeRxReq req;
    EthRemoteCfg_StatusRes res;

    req.rxFlowIdxBase = rxFlowStartIdx;
    req.rxFlowIdxOffset = rxFlowIdx;

    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_FREE_RX, &req, sizeof(req), &res, sizeof(res));
}


void CpswProxy_allocTxCh(CpswProxy_Handle hProxy,
                         uint32_t *txPSILThreadId)
{
    EthRemoteCfg_CommonReq req;
    EthRemoteCfg_AllocTxRes res;

    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_ALLOC_TX, &req, sizeof(req), &res, sizeof(res));

    *txPSILThreadId  = res.txPsilDstId;
}

void CpswProxy_freeTxCh(CpswProxy_Handle hProxy,
                        uint32_t txChNum)
{
    EthRemoteCfg_FreeTxReq req;
    EthRemoteCfg_StatusRes res;

    req.txPsilDstId = txChNum;

    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_FREE_TX, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_attach(CpswProxy_Handle hProxy,
                      EthRemoteCfg_VirtPort virtPort,
                      uint32_t *rxMtu,
                      uint32_t *txMtu)
{
    EthRemoteCfg_AttachReq req;
    EthRemoteCfg_AttachRes res;
    uint32_t i;

    req.virtPort = virtPort;

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_ATTACH, &req, sizeof(req), &res, sizeof(res));

    hProxy->token    = res.hdr.common.token;
    hProxy->features = res.features;

    *rxMtu = res.rxMtu;
    for (i = 0U; i < ENET_ARRAYSIZE(res.txMtu); i++)
    {
        txMtu[i] = res.txMtu[i];
    }
}

void CpswProxy_attachExtended(CpswProxy_Handle hProxy,
                              EthRemoteCfg_VirtPort virtPort,
                              uint32_t *rxMtu,
                              uint32_t *txMtu,
                              uint32_t *txPSILThreadId,
                              uint32_t *rxFlowIdxBase,
                              uint32_t *rxFlowIdxOffset,
                              uint8_t *macAddr)
{
    EthRemoteCfg_AttachReq req;
    EthRemoteCfg_AttachExtRes res;
    uint32_t i;

    req.virtPort = virtPort;

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_ATTACH_EXT, &req, sizeof(req), &res, sizeof(res));

    hProxy->token    = res.hdr.common.token;
    hProxy->features = res.features;

    *rxMtu = res.rxMtu;
    for (i = 0U; i < ENET_ARRAYSIZE(res.txMtu); i++)
    {
        txMtu[i] = res.txMtu[i];
    }

    *txPSILThreadId  = res.txPsilDstId;
    *rxFlowIdxBase   = res.rxFlowIdxBase;
    *rxFlowIdxOffset = res.rxFlowIdxOffset;

    memcpy(macAddr, res.macAddr, ETHREMOTECFG_MACADDRLEN);
}

void CpswProxy_detach(CpswProxy_Handle hProxy)
{
    EthRemoteCfg_CommonReq req;
    EthRemoteCfg_StatusRes res;

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_DETACH, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_registerIPV4Addr(CpswProxy_Handle hProxy,
                                uint8_t *macAddr,
                                uint8_t *ipv4Addr)
{
    EthRemoteCfg_IPv4AddrRegisterReq req;
    EthRemoteCfg_StatusRes res;

    memcpy(req.ipAddr, ipv4Addr, ETHREMOTECFG_IPV4ADDRLEN);
    memcpy(req.macAddr, macAddr, ETHREMOTECFG_MACADDRLEN);

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_REGISTER_IPv4, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_unregisterIPV4Addr(CpswProxy_Handle hProxy,
                                  uint8_t *ipv4Addr)
{
    EthRemoteCfg_IPv4AddrDeregisterReq req;
    EthRemoteCfg_StatusRes res;

    memcpy(req.ipAddr, ipv4Addr, ETHREMOTECFG_IPV4ADDRLEN);

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_DEREGISTER_IPv4, &req, sizeof(req), &res, sizeof(res));
}

bool CpswProxy_isPhyLinked(CpswProxy_Handle hProxy)
{
    EthRemoteCfg_CommonReq req;
    EthRemoteCfg_PortLinkStatusRes res;
    bool isLinked;

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_PORT_LINK_STATUS, &req, sizeof(req), &res, sizeof(res));

    return res.isLinked;
}

void CpswProxy_registerEthertypeRxFlow(CpswProxy_Handle hProxy,
                                       uint32_t rxFlowStartIdx,
                                       uint32_t freeRxFlowIdx,
                                       uint16_t etherType)
{
    EthRemoteCfg_MatchEthertypeAddReq req;
    EthRemoteCfg_StatusRes res;

    req.ethertype = etherType;
    req.flowIdxBase = rxFlowStartIdx;
    req.flowIdxOffset = freeRxFlowIdx;

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_REGISTER_MATCH_ETHTYPE, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_unregisterEthertypeRxFlow(CpswProxy_Handle hProxy,
                                         uint16_t etherType)
{
    EthRemoteCfg_MatchEthertypeDelReq req;
    EthRemoteCfg_StatusRes res;

    req.ethertype = etherType;

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_DEREGISTER_MATCH_ETHTYPE, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_registerRemoteTimer(CpswProxy_Handle hProxy,
                                   uint8_t timerId,
                                   uint8_t hwPushNum)
{
    EthRemoteCfg_RemoteTimerRegisterReq req;
    EthRemoteCfg_StatusRes res;

    req.hwPushNum = hwPushNum;
    req.timerId = timerId;

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_REGISTER_REMOTE_TIMER, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_unregisterRemoteTimer(CpswProxy_Handle hProxy,
                                     uint8_t hwPushNum)
{
    EthRemoteCfg_RemoteTimerDeregisterReq req;
    EthRemoteCfg_StatusRes res;

    req.hwPushNum = hwPushNum;

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_DEREGISTER_REMOTE_TIMER, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_setPromiscMode(CpswProxy_Handle hProxy,
                              bool enable)
{
    EthRemoteCfg_CommonReq req;
    EthRemoteCfg_StatusRes res;
    uint32_t reqType;

    reqType = enable ? ETHREMOTECFG_CMD_ENABLE_PROMISC : ETHREMOTECFG_CMD_DISABLE_PROMISC;

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, reqType, &req, sizeof(req), &res, sizeof(res));
}

void CpswProxy_getDumpStats(CpswProxy_Handle hProxy)
{
    EthRemoteCfg_CommonReq req;
    EthRemoteCfg_StatusRes res;

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_DUMP, &req, sizeof(req), &res, sizeof(res));
}

int32_t CpswProxy_joinVlan(CpswProxy_Handle hProxy,
                           uint32_t flowIdxBase,
                           uint32_t flowIdxOffset,
                           const uint8_t *macAddr,
                           uint16_t vlanId)
{
    EthRemoteCfg_VlanJoinReq req;
    EthRemoteCfg_StatusRes res;

    req.vlanId = vlanId;
    req.flowIdxBase = flowIdxBase;
    req.flowIdxOffset = flowIdxOffset;
    memcpy(&req.macAddr[0U], macAddr, ETHREMOTECFG_MACADDRLEN);

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_JOIN_VLAN, &req, sizeof(req), &res, sizeof(res));

    return res.hdr.status;
}

int32_t CpswProxy_leaveVlan(CpswProxy_Handle hProxy,
                            uint32_t flowIdxBase,
                            uint32_t flowIdxOffset,
                            const uint8_t *macAddr,
                            uint16_t vlanId)
{
    EthRemoteCfg_VlanLeaveReq req;
    EthRemoteCfg_StatusRes res;

    req.vlanId = vlanId;
    req.flowIdxBase = flowIdxBase;
    req.flowIdxOffset = flowIdxOffset;
    memcpy(&req.macAddr[0U], macAddr, ETHREMOTECFG_MACADDRLEN);

    /* Send request to server and wait for response */
    CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_LEAVE_VLAN, &req, sizeof(req), &res, sizeof(res));

    return res.hdr.status;
}

void CpswProxy_filterAddMac(CpswProxy_Handle hProxy,
                            uint32_t rxFlowStartIdx,
                            uint32_t freeRxFlowIdx,
                            const uint8_t *macAddr,
                            uint16_t vlanId)
{
    EthRemoteCfg_FilterMacAddReq req;
    EthRemoteCfg_StatusRes res;
    int32_t status = CPSWPROXY_SOK;

    if (!EnetUtils_isMcastAddr(macAddr))
    {
        System_printf("%s: MAC addr is not multicast\n", __func__);
        status = CPSWPROXY_EINVALIDPARAMS;
    }

    if (status == CPSWPROXY_SOK)
    {
        /* Request specific params */
        req.vlanId  = vlanId;
        req.flowIdxBase = rxFlowStartIdx;
        req.flowIdxOffset = freeRxFlowIdx;
        memcpy(req.macAddr, macAddr, ETHREMOTECFG_MACADDRLEN);

        /* Send request to server and wait for response */
        CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_ADD_FILTER_MAC, &req, sizeof(req), &res, sizeof(res));
    }
}

void CpswProxy_filterDelMac(CpswProxy_Handle hProxy,
                            const uint8_t *macAddr,
                            uint16_t vlanId)
{
    EthRemoteCfg_FilterMacDelReq req;
    EthRemoteCfg_StatusRes res;
    int32_t status = CPSWPROXY_SOK;

    if (!EnetUtils_isMcastAddr(macAddr))
    {
        System_printf("%s: MAC addr is not multicast\n", __func__);
        status = CPSWPROXY_EINVALIDPARAMS;
    }

    if (status == CPSWPROXY_SOK)
    {
        /* Request specific params */
        req.vlanId  = vlanId;
        memcpy(req.macAddr, macAddr, ETHREMOTECFG_MACADDRLEN);

        /* Send request to server and wait for response */
        CpswProxy_sendCmd(hProxy, ETHREMOTECFG_CMD_DEL_FILTER_MAC, &req, sizeof(req), &res, sizeof(res));
    }
}

static void CpswProxy_msgHandlerTskFxn(void* arg0,
                                       void* arg1)
{
    CpswProxy_Handle hProxy = (CpswProxy_Handle)arg0;
    uint32_t remoteProcId;
    uint32_t remoteEndpt;
    MailboxP_Status mbxStatus;
    uint64_t msgBuf[ETHREMOTECFG_IPC_MSG_SIZE / sizeof(uint64_t)];
    bool exitTask = false;
    int32_t status = IPC_SOK;
    uint16_t len;

    while (!exitTask)
    {
        status = RPMessage_recv(gCpswProxy.hEthfwServiceRpMsg,
                                (void *)msgBuf,
                                &len,
                                &remoteEndpt,
                                &remoteProcId,
                                IPC_RPMESSAGE_TIMEOUT_FOREVER);
        if (IPC_SOK == status)
        {
            CpswProxy_assert(len <= sizeof(msgBuf));
            if ((gCpswProxy.masterCoreId != remoteProcId) ||
                (gCpswProxy.masterEndpt != remoteEndpt))
            {
                status = ENET_EUNEXPECTED;
                System_printf("Unexpected response (remoteCoreId=%u, remoteEndpt=%u)\n",
                             remoteProcId, remoteEndpt);
            }

            if (ENET_SOK == status)
            {
                EthRemoteCfg_MsgHdr *msgHdr;

                msgHdr = (EthRemoteCfg_MsgHdr *)msgBuf;
                if (msgHdr->msgType == ETHREMOTECFG_MSGTYPE_RESPONSE)
                {
                    EthRemoteCfg_ResHdr *resHdr = (EthRemoteCfg_ResHdr *)msgHdr;

                    System_printf("S2C | msgType=%u token=%u clientId=%u resId=%u status=%d\n",
                                 resHdr->common.msgType,
                                 resHdr->common.token,
                                 resHdr->common.clientId,
                                 resHdr->resId,
                                 resHdr->status);

                    if ((resHdr->common.token != ETHREMOTECFG_TOKEN_NONE) &&
                            !(resHdr->resType == ETHREMOTECFG_CMD_ATTACH || resHdr->resType == ETHREMOTECFG_CMD_ATTACH_EXT))
                    {
                        hProxy = CpswProxy_getHandle(resHdr->common.token);

                        if (resHdr->resId > hProxy->reqId)
                        {
                            System_printf("Got wrong resId (exp: %u, got: %u)\n", hProxy->reqId, resHdr->resId);
                            CpswProxy_assert(false);
                        }
                    }

                    mbxStatus = MailboxP_post(gCpswProxy.hResMbx, msgBuf, MailboxP_WAIT_FOREVER);
                    CpswProxy_assert(mbxStatus == MailboxP_OK);
                }
                else
                {
                    status = ENET_EUNEXPECTED;
                    System_printf("Unexpected message type %u\n", msgHdr->msgType);
                }
            }
        }
    }
}

static void CpswProxy_sendCmd(CpswProxy_Handle hProxy,
                              uint32_t reqType,
                              void *reqMsg,
                              uint16_t reqLen,
                              void *resMsg,
                              uint16_t resLen)
{
    EthRemoteCfg_ReqHdr *hdr = (EthRemoteCfg_ReqHdr *)reqMsg;
    uint64_t resBuf[ETHREMOTECFG_IPC_MSG_SIZE / sizeof(uint64_t)];
    MailboxP_Status mbxStatus;
    int32_t status = IPC_SOK;

    CpswProxy_assert(hProxy != NULL);

    hdr->common.msgType  = ETHREMOTECFG_MSGTYPE_REQUEST;
    hdr->common.clientId = ETHREMOTECFG_CLIENTID_RTOS;
    hdr->common.token    = hProxy->token;

    hdr->reqType = reqType;
    hdr->reqId   = hProxy->reqId++;

    System_printf("C2S | msgType=%u token=%u clientId=%u reqType=%u reqId=%u\n",
                 hdr->common.msgType,
                 hdr->common.token,
                 hdr->common.clientId,
                 hdr->reqType,
                 hdr->reqId);

    status = RPMessage_send(gCpswProxy.hEthfwServiceRpMsg,
                            gCpswProxy.masterCoreId,
                            gCpswProxy.masterEndpt,
                            gCpswProxy.localEndpt,
                            (void *)reqMsg,
                            reqLen);
    CpswProxy_assert(IPC_SOK == status);

    mbxStatus = MailboxP_pend(gCpswProxy.hResMbx, resBuf, MailboxP_WAIT_FOREVER);

    memcpy(resMsg, resBuf, resLen);
}

void CpswProxy_sendNotify(CpswProxy_Handle hProxy,
                          uint8_t notifyId,
                          uint8_t *notifyInfo,
                          uint32_t notifyInfoLength)
{
#if 0
    CpswProxy_CmdMsg msg;

    msg.req.u.notify.notify_id = notifyId;
    msg.req.u.notify.notify_info = notifyInfo;
    msg.req.u.notify.notify_len = notifyInfoLength;
    CpswProxy_sendCmd(hProxy, CPSWPROXY_RDEVCMD_NOTIFY, &msg);
#endif
}

int32_t CpswProxy_registerHwPushNotifyCb(CpswProxy_hwPushNotifyCbFxn cbFxn,
                                         void *cbArg)
{
    CpswProxy_notifyServiceObj *notifyObj = &gCpswProxy.notifyServiceObj;
    int status = CPSWPROXY_SOK;

    if (NULL != cbFxn)
    {
        if (notifyObj->cb.hwPushCb == NULL)
        {
            notifyObj->cb.hwPushCb = cbFxn;
            notifyObj->cb.hwPushCbArg = cbArg;
        }
        else
        {
            status = CPSWPROXY_EALREADYOPEN;
        }
    }
    else
    {
        status = CPSWPROXY_EBADARGS;
    }

    return status;
}

void CpswProxy_unregisterHwPushNotifyCb(void)
{
    CpswProxy_notifyServiceObj *notifyObj = &gCpswProxy.notifyServiceObj;

    notifyObj->cb.hwPushCb = NULL;
    notifyObj->cb.hwPushCbArg = NULL;
}

