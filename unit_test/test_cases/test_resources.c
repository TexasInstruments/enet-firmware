/*
 *
 * Copyright (c) 2024 Texas Instruments Incorporated
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

/*!
 * \file test_resources.c
 *
 * \brief EthFw UT functions for testing resources.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* EthFwTrace id for this module, must be unique within ETHFW */
#define ETHFWTRACE_MOD_ID 0x803

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ti/osal/MutexP.h>

/* Enet LLD header files */
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>

/* EthFw header files */
#include <ethremotecfg/client/include/cpsw_proxy.h>
#include <ethremotecfg/protocol/ethremotecfg.h>
#include <ethremotecfg/protocol/ethremotecfg_virtport.h>
#include <utils/ethfw_common/include/ethfw_trace.h>
#include <utils/ethfw_common/include/ethfw_utils.h>

#include <unity.h>
#include "ethfw_test_cases.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
CpswProxy_Handle gTestProxy;
/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

void EthFwUT_allocTxCmdTest1(void)
{
    EthRemoteCfg_AllocTxReq req;
    EthRemoteCfg_AllocTxRes res;
    uint32_t relChPriority = 0U;
    int32_t status;

    req.chRelPriority = relChPriority;
    memset(&res, 0, sizeof(EthRemoteCfg_AllocTxRes));

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_ALLOC_TX,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        goto end;

    /* Free the allocated channel */
    status = CpswProxy_freeTxCh(gTestProxy,
                                res.txPsilDstId);
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_allocTxCmdNegTest(void)
{
    EthRemoteCfg_AllocTxReq req;
    EthRemoteCfg_AllocTxRes res;
    int32_t status;

    /* Send invalid priority */
    req.chRelPriority = 0xFFU;
    memset(&res, 0, sizeof(EthRemoteCfg_AllocTxRes));

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_ALLOC_TX,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_freeTxCmdTest1(void)
{
    EthRemoteCfg_FreeTxReq req;
    EthRemoteCfg_StatusRes res;
    uint32_t txPSILId;
    uint32_t relChPriority = 0U;
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    /* For freeing a TX channel, we need to allocate one first */
    status = CpswProxy_allocTxCh(gTestProxy,
                                 &txPSILId,
                                 relChPriority);
    if (status != CPSWPROXY_SOK)
        goto end;

    req.txPsilDstId = txPSILId;

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_FREE_TX,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_freeTxCmdNegTest(void)
{
    EthRemoteCfg_FreeTxReq req;
    EthRemoteCfg_StatusRes res;
    int32_t status;

    /* Send invalid txPsilDstId */
    req.txPsilDstId = 0xFFU;
    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_FREE_TX,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_allocRxCmdTest1(void)
{
    EthRemoteCfg_AllocRxReq req;
    EthRemoteCfg_AllocRxRes res;
    int32_t status;

    req.flowIdx = 0U;
    memset(&res, 0, sizeof(EthRemoteCfg_AllocRxRes));

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_ALLOC_RX,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        goto end;

    /* Free the allocated flow */
    status = CpswProxy_freeRxFlow(gTestProxy,
                                  res.rxFlowIdxBase,
                                  res.rxFlowIdxOffset);
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_freeRxCmdTest1(void)
{
    EthRemoteCfg_FreeRxReq req;
    EthRemoteCfg_StatusRes res;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;
    uint32_t flowIdx = 0U;
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    /* For freeing a RX flow, we need to allocate one RX flow first */
    status = CpswProxy_allocRxFlow(gTestProxy,
                                   &rxStartFlowId,
                                   &rxFlowIdOffset,
                                   flowIdx);
    if (status != CPSWPROXY_SOK)
        goto end;

    req.rxFlowIdxBase = rxStartFlowId;
    req.rxFlowIdxOffset = rxFlowIdOffset;

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_FREE_RX,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_allocRxCmdNegTest(void)
{
    EthRemoteCfg_AllocRxReq req;
    EthRemoteCfg_AllocRxRes res;
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_AllocRxRes));

    /* Send invalid priority */
    req.flowIdx = 0xFFU;

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_ALLOC_RX,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_freeRxCmdNegTest(void)
{
    EthRemoteCfg_FreeRxReq req;
    EthRemoteCfg_StatusRes res;
    int32_t status;

    /* Send invalid priority */
    req.rxFlowIdxBase = 0xFFU;
    req.rxFlowIdxOffset = 0xFFFU;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_FREE_RX,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_allocMacCmdTest1(void)
{
    EthRemoteCfg_CommonReq req;
    EthRemoteCfg_AllocMacRes res;
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_AllocMacRes));

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_ALLOC_MAC,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        goto end;

    /* Free the allocated MAC */
    status = CpswProxy_freeMac(gTestProxy,
                               res.macAddr);
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_freeMacCmdTest1(void)
{
    EthRemoteCfg_FreeMacReq req;
    EthRemoteCfg_StatusRes res;
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    /* To free a MAC, we need to allocate one first */
    status = CpswProxy_allocMac(gTestProxy,
                                macAddr);
    if (status != CPSWPROXY_SOK)
        goto end;

    memcpy(req.macAddr, macAddr, ETHREMOTECFG_MACADDRLEN);

    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_FREE_MAC,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_registerMacCmdTest(void)
{
    EthRemoteCfg_MacAddrRxFlowReq req;
    EthRemoteCfg_StatusRes res;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;
    uint32_t flowIdx = 0U;
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    status = CpswProxy_allocRxFlow(gTestProxy,
                                   &rxStartFlowId,
                                   &rxFlowIdOffset,
                                   flowIdx);
    if (status != CPSWPROXY_SOK)
        goto end;

    status = CpswProxy_allocMac(gTestProxy,
                                macAddr);
    if (status != CPSWPROXY_SOK)
        goto err_alloc;

    req.flowIdxBase   = rxStartFlowId;
    req.flowIdxOffset = rxFlowIdOffset;
    memcpy(&req.macAddr[0U], macAddr, ETHREMOTECFG_MACADDRLEN);

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_REGISTER_MAC,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
    goto err_regmac;

    /* Deregister the MAC */
    status = CpswProxy_unregisterDstMacRxFlow(gTestProxy,
                                              rxStartFlowId,
                                              rxFlowIdOffset,
                                              macAddr);

err_regmac:
    /* Free the MAC */
    status = CpswProxy_freeMac(gTestProxy,
                               macAddr);
err_alloc:
    /* Free the allocated RX flow */
    status = CpswProxy_freeRxFlow(gTestProxy,
                                  rxStartFlowId,
                                  rxFlowIdOffset);
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_registerMacCmdNegTest(void)
{
    EthRemoteCfg_MacAddrRxFlowReq req;
    EthRemoteCfg_StatusRes res;
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN] = {0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6};
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));
    memcpy(&req.macAddr[0U], macAddr, ETHREMOTECFG_MACADDRLEN);

    /* Send invalid values */
    req.flowIdxBase   = 0xFF;
    req.flowIdxOffset = 0xFFF;

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_REGISTER_MAC,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_unregisterMacCmdTest(void)
{
    EthRemoteCfg_MacAddrRxFlowReq req;
    EthRemoteCfg_StatusRes res;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;
    uint32_t flowIdx = 0U;
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    status = CpswProxy_allocRxFlow(gTestProxy,
                                   &rxStartFlowId,
                                   &rxFlowIdOffset,
                                   flowIdx);
    if (status != CPSWPROXY_SOK)
    goto end;

    status = CpswProxy_allocMac(gTestProxy,
                                macAddr);
    if (status != CPSWPROXY_SOK)
        goto err_alloc;

    status = CpswProxy_registerDstMacRxFlow(gTestProxy,
                                            rxStartFlowId,
                                            rxFlowIdOffset,
                                            macAddr);
    if (status != CPSWPROXY_SOK)
        goto err_regmac;

    req.flowIdxBase   = rxStartFlowId;
    req.flowIdxOffset = rxFlowIdOffset;
    memcpy(&req.macAddr[0U], macAddr, ETHREMOTECFG_MACADDRLEN);

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_DEREGISTER_MAC,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));

err_regmac:
    /* Free the MAC */
    status = CpswProxy_freeMac(gTestProxy,
                               macAddr);
err_alloc:
    /* Free the allocated RX flow */
    status = CpswProxy_freeRxFlow(gTestProxy,
                                  rxStartFlowId,
                                  rxFlowIdOffset);
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_unregisterMacCmdNegTest(void)
{
    EthRemoteCfg_MacAddrRxFlowReq req;
    EthRemoteCfg_StatusRes res;
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN] = {0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6};
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));
    memcpy(&req.macAddr[0U], macAddr, ETHREMOTECFG_MACADDRLEN);

    /* Send invalid values */
    req.flowIdxBase   = 0xFF;
    req.flowIdxOffset = 0xFFF;

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_DEREGISTER_MAC,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_registerDefaultRxFlowCmdTest(void)
{
    EthRemoteCfg_RxDefaultFlowRegisterReq req;
    EthRemoteCfg_StatusRes res;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;
    uint32_t flowIdx = 0U;
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    status = CpswProxy_allocRxFlow(gTestProxy,
                                   &rxStartFlowId,
                                   &rxFlowIdOffset,
                                   flowIdx);
    if (status != CPSWPROXY_SOK)
        goto end;

    req.flowIdxBase   = rxStartFlowId;
    req.flowIdxOffset = rxFlowIdOffset;

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_SET_RX_DEFAULTFLOW,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        goto err_setdef;

    /* Unregister RX flow */
    status = CpswProxy_unregisterDefaultRxFlow(gTestProxy,
                                               rxStartFlowId,
                                               rxFlowIdOffset);

err_setdef:
    /* Free the allocated RX flow */
    status = CpswProxy_freeRxFlow(gTestProxy,
                                  rxStartFlowId,
                                  rxFlowIdOffset);
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_registerDefaultRxFlowCmdNegTest(void)
{
    EthRemoteCfg_RxDefaultFlowRegisterReq req;
    EthRemoteCfg_StatusRes res;
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    /* Send invalid values */
    req.flowIdxBase   = 0xFF;
    req.flowIdxOffset = 0xFFF;

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_SET_RX_DEFAULTFLOW,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_unregisterDefaultRxFlowCmdTest(void)
{
    EthRemoteCfg_RxDefaultFlowRegisterReq req;
    EthRemoteCfg_StatusRes res;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;
    uint32_t flowIdx = 0U;
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    status = CpswProxy_allocRxFlow(gTestProxy,
                                   &rxStartFlowId,
                                   &rxFlowIdOffset,
                                   flowIdx);
    if (status != CPSWPROXY_SOK)
        goto end;

    status = CpswProxy_registerDefaultRxFlow(gTestProxy,
                                             rxStartFlowId,
                                             rxFlowIdOffset);
    if (status != CPSWPROXY_SOK)
        goto err_setdef;

    req.flowIdxBase   = rxStartFlowId;
    req.flowIdxOffset = rxFlowIdOffset;

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_DEL_RX_DEFAULTFLOW,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));

err_setdef:
    /* Free the allocated RX flow */
    status = CpswProxy_freeRxFlow(gTestProxy,
                                  rxStartFlowId,
                                  rxFlowIdOffset);
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_unregisterDefaultRxFlowCmdNegTest(void)
{
    EthRemoteCfg_RxDefaultFlowRegisterReq req;
    EthRemoteCfg_StatusRes res;
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    /* Send invalid values */
    req.flowIdxBase   = 0xFF;
    req.flowIdxOffset = 0xFFF;

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_DEL_RX_DEFAULTFLOW,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}

void EthFwUT_registerIPV4AddrCmdTest(void)
{
    EthRemoteCfg_IPv4AddrRegisterReq req;
    EthRemoteCfg_StatusRes res;
    uint32_t rxStartFlowId;
    uint32_t rxFlowIdOffset;
    uint32_t flowIdx = 0U;
    uint8_t macAddr[ETHREMOTECFG_MACADDRLEN];
    uint8_t ipv4Addr[ETHREMOTECFG_IPV4ADDRLEN] = {192, 168, 0, 10};
    int32_t status;

    memset(&res, 0, sizeof(EthRemoteCfg_StatusRes));

    /* Alloc a RX flow */
    status = CpswProxy_allocRxFlow(gTestProxy,
                                   &rxStartFlowId,
                                   &rxFlowIdOffset,
                                   flowIdx);
    if (status != CPSWPROXY_SOK)
        goto end;

    /* Alloc a MAC */
    status = CpswProxy_allocMac(gTestProxy,
                                macAddr);
    if (status != CPSWPROXY_SOK)
        goto err_alloc;

    /* Register the MAC */
    status = CpswProxy_registerDstMacRxFlow(gTestProxy,
                                            rxStartFlowId,
                                            rxFlowIdOffset,
                                            macAddr);
    if (status != CPSWPROXY_SOK)
        goto err_regmac;

    memcpy(req.ipAddr, ipv4Addr, ETHREMOTECFG_IPV4ADDRLEN);
    memcpy(req.macAddr, macAddr, ETHREMOTECFG_MACADDRLEN);

    /* Send request to server and wait for response */
    status = CpswProxy_sendCmd(gTestProxy, ETHREMOTECFG_CMD_REGISTER_IPv4,
                               &req.hdr, sizeof(req),
                               &res.hdr, sizeof(res));
    if (status != CPSWPROXY_SOK)
        goto err_ipv4;

    /* unregister the ipv4 address */
    status = CpswProxy_unregisterIPV4Addr(gTestProxy,
                                            ipv4Addr);

err_ipv4:
    /* Unregister the MAC */
    status = CpswProxy_unregisterDstMacRxFlow(gTestProxy,
                                              rxStartFlowId,
                                              rxFlowIdOffset,
                                              macAddr);
err_regmac:
    /* Free the MAC */
    status = CpswProxy_freeMac(gTestProxy,
                               macAddr);
err_alloc:
    /* Free the allocated RX flow */
    status = CpswProxy_freeRxFlow(gTestProxy,
                                  rxStartFlowId,
                                  rxFlowIdOffset);
end:
    if (status == CPSWPROXY_SOK)
        TEST_PASS();
    else
        TEST_FAIL();
}


void EthFwUT_testResources(void *args)
{
    uint32_t txMtu[ENET_PRI_NUM];
    uint32_t hostPortRxMtu;
    uint32_t numTxCh;
    uint32_t numRxFlow;
    int32_t status;

    gTestProxy = (CpswProxy_Handle)args;
    TEST_ASSERT_NOT_NULL(gTestProxy);

    status = CpswProxy_attach(gTestProxy,
                              ETHREMOTECFG_SWITCH_PORT_1,
                              &hostPortRxMtu,
                              txMtu,
                              &numTxCh,
                              &numRxFlow);

    ETHFWTRACE_ERR_IF((status != CPSWPROXY_SOK), status, "Failed to attach to Ethernet device.");
    TEST_ASSERT_FALSE(status != CPSWPROXY_SOK);

    UnityBegin("test_resources.c");

    RUN_TEST(EthFwUT_allocTxCmdTest1, 0);

    // RUN_TEST(EthFwUT_allocTxCmdNegTest,  0);

    RUN_TEST(EthFwUT_freeTxCmdTest1,  0);

    RUN_TEST(EthFwUT_freeTxCmdNegTest,  0);

    RUN_TEST(EthFwUT_allocRxCmdTest1,  0);

    RUN_TEST(EthFwUT_freeRxCmdTest1,  0);

    RUN_TEST(EthFwUT_allocRxCmdNegTest,  0);

    RUN_TEST(EthFwUT_freeRxCmdNegTest,  0);

    RUN_TEST(EthFwUT_allocMacCmdTest1,  0);

    RUN_TEST(EthFwUT_freeMacCmdTest1,  0);

    RUN_TEST(EthFwUT_registerMacCmdTest,  0);

    RUN_TEST(EthFwUT_registerMacCmdNegTest,  0);

    RUN_TEST(EthFwUT_unregisterMacCmdTest,  0);

    RUN_TEST(EthFwUT_unregisterMacCmdNegTest,  0);

    // RUN_TEST(EthFwUT_registerDefaultRxFlowCmdTest,  0);

    // RUN_TEST(EthFwUT_registerDefaultRxFlowCmdNegTest,  0);
    
    // RUN_TEST(EthFwUT_unregisterDefaultRxFlowCmdTest,  0);

    // RUN_TEST(EthFwUT_unregisterDefaultRxFlowCmdNegTest,  0);

    RUN_TEST(EthFwUT_registerIPV4AddrCmdTest,  0);
    
    UnityEnd();

    status = CpswProxy_detach(gTestProxy);
    ETHFWTRACE_ERR_IF((status != CPSWPROXY_SOK), status, "Failed to detach from Ethernet device.");
    TEST_ASSERT_FALSE(status != CPSWPROXY_SOK);
}