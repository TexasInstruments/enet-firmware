/*
 *  Copyright (c) Texas Instruments Incorporated 2024
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*!
 * \file  ethfw_ipc.h
 *
 * \brief Ethernet Firmware IPC interface.
 */

#ifndef ETHFW_IPC_H_
#define ETHFW_IPC_H_
/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <ethfw_al.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                                 Macros                                     */
/* ========================================================================== */

#define ETHFW_IPC_INVALID_RPMSG_ENDPOINT           (~1U)

/* ========================================================================== */
/*                         Structures and Enums                               */
/* ========================================================================== */

/*!
 * \brief RPMessage create params
 *
 * Common RPMessage params for Jacinto and Sitara
 */
typedef struct EthFwIpc_RpmsgCreateParams_s
{
    /*! Requested Endpoint - Any or next available */
    uint32_t  requestedEndpt;
    /*! Maximum number of buffers to allocate for queuing received messages. */
    uint32_t  numBufs;
    /*!  Buffer pointer to store RPMessage Object */
    void*     buf;
    /*! Buffer Size. Recommended Size is (512*256 + 256) */
    uint32_t  bufSize;
} EthFwIpc_RpmsgCreateParams;

/* ========================================================================== */
/*                         Global Variables Declarations                      */
/* ========================================================================== */

typedef void *EthFwIpc_RpmsgHandle;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

int32_t EthFwIpc_init(uint32_t selfId,
                      uint16_t numProc,
                      uint32_t procArray[IPC_MAX_PROCS],
                      void (*func)(const char *str));

int32_t EthFwIpc_initVirtIO(uint16_t numProc,
                            void *vqObj,
                            void *vringAddr,
                            uint32_t vringBufSize);

int32_t EthFwIpc_initRpmsg(void *rpmsgBuff, void *taskBuff, uint32_t selfCoreId);

int32_t EthFwIpc_isRemoteReady(uint32_t coreId, uint32_t timeout);

int32_t EthFwIpc_lateInit(uint32_t coreId);

void EthFwIpc_initRpmsgParams(EthFwIpc_RpmsgCreateParams *params);

EthFwIpc_RpmsgHandle EthFwIpc_createRpmsg(EthFwIpc_RpmsgCreateParams *params);

int32_t EthFwIpc_sendRpmsg(EthFwIpc_RpmsgHandle handle,
                           uint32_t dstProcId,
                           uint32_t dstEndPt,
                           uint32_t srcEndPt,
                           void *data,
                           uint32_t len);

int32_t EthFwIpc_recvRpmsg(EthFwIpc_RpmsgHandle handle,
                           void *data,
                           uint16_t *len,
                           uint32_t *remoteEndPt,
                           uint32_t *remoteProcId,
                           uint32_t timeout);

int32_t EthFwIpc_getRemoteEndPt(uint32_t currProcId,
                                const char* name,
                                uint32_t *remoteProcId,
                                uint32_t *remoteEndPt,
                                uint32_t timeout);

int32_t EthFwIpc_announceAll(uint32_t localEp,
                             const char* name);

int32_t EthFwIpc_announce(uint32_t remoteProcId,
                          uint32_t localEp,
                          const char* name);

void EthFwIpc_unblockRpmsg(EthFwIpc_RpmsgHandle handle);

int32_t EthFwIpc_deleteRpmsg(EthFwIpc_RpmsgHandle handle);


#ifdef __cplusplus
}
#endif

#endif /* ETHFW_IPC_H_ */
