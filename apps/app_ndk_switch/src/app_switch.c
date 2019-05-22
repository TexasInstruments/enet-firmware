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

/*!
 * \file     app_secondflow.c
 *
 * \brief    This file contains the app_secondflow test implementation.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <xdc/runtime/Error.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <ti/osal/osal.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/udma/udma.h>

#include <ti/csl/csl_cpswitch.h>
#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils_cfg.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpswapp_ethutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_multiclientmanager.h>

#include "app_switch.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define TEST_LEN                    (500U)
#define APP_TSK_STACK_UART          (10U * 1024U)

#define APP_MENU_OPTION_VLAN        (1U)
#define APP_MENU_OPTION_MULTICAST   (2U)
#define APP_MENU_OPTION_RATE_LIM    (3U)
#define APP_MENU_OPTION_INTERVLAN   (4U)
#define APP_MENU_OPTION_SHOW_ALE    (5U)

/* ========================================================================== */
/*                         Structures and Enums                               */
/* ========================================================================== */

typedef struct
{
    /* CPSW instance type */
    Cpsw_Type            cpswType;

    /* CPSW driver handle */
    Cpsw_Handle          hCpsw;

    /* UDMA driver handle */
    Udma_DrvHandle       hUdmaDrv;

    /* UDMA RX flow handle */
    CpswDma_RxFlowHandle hRxFlow;

    /* RX flow index */
    uint32_t             rxFlowIdx;

    /* RX free queue */
    CpswDma_PktInfoQ     rxFreeQ;

    /* RX ready queue */
    CpswDma_PktInfoQ     rxReadyQ;

    /* UDMA TX channel handle */
    CpswDma_TxChHandle   hTxCh;

    /* TX free queue */
    CpswDma_PktInfoQ     txFreePktInfoQ;

    /* TX Eth packet memory */
    uint8_t              txBufMem[CPSW_APPMEMUTILS_NUM_TX_PKTS][CPSWAPPUTILS_ALIGN(ETH_MAX_FRAME_LEN)]
                                __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));

    /* TX DMA packet info memory */
    CpswDma_PktInfo      txFreePktInfo[CPSW_APPMEMUTILS_NUM_TX_PKTS];

    /* Host port address */
    uint8_t              hostMacAddr[ETH_MAC_ADDR_LEN];
} CpswApp_Obj;

/* ========================================================================== */
/*                 Internal Function Declarations                             */
/* ========================================================================== */

static void CpswApp_addUnicastAddressEntry(uint8_t macAddr[],
                                           uint32_t portNum);
static void CpswApp_addClasifierEntry(uint8_t srcMacAddr[],
                                      uint32_t flowId);
static void CpswApp_showAleTableAndPolicer(void);
static int32_t CpswApp_pktRxTx(void);
static int32_t CpswSwitchApp_getRxTxHandle(void);
static uint32_t CpswApp_retrieveFreeTxPkts(void);
static uint32_t CpswApp_receivePkts(void);
static void CpswApp_createUartMenuTask(void);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* Broadcast address */
static uint8_t bcastAddr[ETH_MAC_ADDR_LEN] =
{
    0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU
};

/* Src Mac address for flow 1*/
static uint8_t flowAddr1[ETH_MAC_ADDR_LEN] =
{
    0x80U, 0xcdU, 0xefU, 0xfeU, 0xdcU, 0xbaU
};

static Task_Handle taskUartMenu;

/* Switch menu 0 string */
static const char gCpswSwitchMenu[] =
{
    "\r\n================================================="
    "\r\n                   Switch Options                "
    "\r\n================================================="
    "\r\n 1. Enable/Disable VLAN "
    "\r\n 2. Enable/Disable Multicast"
    "\r\n 3. Enable/Disable Rate Limiting "
    "\r\n 4. Enable/Disable InterVLAN "
    "\r\n Enter your choice: "
    "\r\n"
};

/* UART Menu stack */
static uint8_t gAppTskStackUart[APP_TSK_STACK_UART] __attribute__((aligned(32)));

CpswApp_Obj gCpswSwitchAppObj;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int32_t CpswApp_secondFlowTest(uint32_t iteration)
{
    CpswAppIf_HandleInfo handleInfo;
    CpswDma_PktInfoQ fqPktInfoQ;
    CpswDma_PktInfoQ cqPktInfoQ;
    int32_t status;

    /* Get CPSW & UDMA Drv Handle */
    CpswAppIf_getHandles(&handleInfo);

    gCpswSwitchAppObj.hCpsw = handleInfo.hCpsw;
    gCpswSwitchAppObj.hUdmaDrv = handleInfo.hUdmaDrv;
    gCpswSwitchAppObj.cpswType = handleInfo.cpswType;

    if (gCpswSwitchAppObj.hCpsw  == NULL)
    {
        status = CPSW_EFAIL;
        CpswAppUtils_print("Failed to open CPSW: %d\n", status);
        CpswAppUtils_assert(gCpswSwitchAppObj.hCpsw == NULL);
    }

    if (gCpswSwitchAppObj.hUdmaDrv == NULL)
    {
        status = CPSW_EFAIL;
        CpswAppUtils_print("Failed to Get UDMA Handle: %d\n", status);
        CpswAppUtils_assert(gCpswSwitchAppObj.hUdmaDrv == NULL);
    }

    CpswApp_createUartMenuTask();

    status = CpswSwitchApp_getRxTxHandle();

    if (status == CPSW_SOK)
    {
        status = CpswApp_pktRxTx();
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Failed to enable host port: %d\n", status);
        }
    }

    /* Close RX Flow */
    status = CpswDma_closeRxFlow(gCpswSwitchAppObj.hRxFlow, &fqPktInfoQ, &cqPktInfoQ);
    if (status == CPSW_SOK)
    {
        Cpsw_rmFreeFlowIndex(gCpswSwitchAppObj.hCpsw, gCpswSwitchAppObj.rxFlowIdx);
        CpswAppUtils_freeQs(&fqPktInfoQ, &cqPktInfoQ);
    }
    else
    {
        CpswAppUtils_print("CpswDma_closeRxFlow() failed: %d\n", status);
    }

    /* Close TX channel */
    status = CpswDma_disableTxEvent(gCpswSwitchAppObj.hTxCh);
    status = CpswDma_closeTxCh(gCpswSwitchAppObj.hTxCh, &fqPktInfoQ, &cqPktInfoQ);
    if (status == CPSW_SOK)
    {
        CpswAppUtils_freeQs(&fqPktInfoQ, &cqPktInfoQ);
    }
    else
    {
        CpswAppUtils_print("CpswDma_closeTxCh() failed: %d\n", status);
    }

    if (status == CPSW_SOK)
    {
        CpswAppIf_releaseHandles(gCpswSwitchAppObj.cpswType);
        CpswAppUtils_print("Cpsw Flow 1 application completed\n");
    }
    else
    {
        CpswAppUtils_print("Cpsw Flow 1 application failed to complete\n");
    }

    return 0;
}

void CpswApp_rxIsrFxn(Udma_EventHandle eventHandle,
                      uint32_t         eventType,
                      void            *appData)
{}

void CpswApp_txIsrFxn(Udma_EventHandle eventHandle,
                      uint32_t         eventType,
                      void            *appData)
{}

void CpswApp_setVLANentry(uint32_t vlanId,
                          uint32_t vlanMemberMask,
                          uint32_t isEnable)
{
    Cpsw_IoctlPrms prms;
    CpswAle_VlanEntryInfo vlanEntry;
    CpswAle_AddEntryOutArgs vlanOutInfo;
    CpswAle_GetVlanEntryOutArgs getVlanOutArgs;
    CpswAle_VlanIdInfo getVlanInArgs;
    int32_t status = CPSW_SOK;

    if (isEnable)
    {
        vlanEntry.disallowIPFragmentation = FALSE;
        vlanEntry.forceUntaggedEgressMask = 0U;
        vlanEntry.limitIPNxtHdr = FALSE;
        vlanEntry.noLearnMask  = 0U;
        vlanEntry.regMcastFloodMask = vlanMemberMask;
        vlanEntry.unregMcastFloodMask = 0U;
        vlanEntry.vidIngressCheck = FALSE;
        vlanEntry.vlanMemberList = vlanMemberMask;
        vlanEntry.vlanIdInfo.vlanId = vlanId;
        vlanEntry.vlanIdInfo.outerVlanFlag = FALSE;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &vlanEntry, &vlanOutInfo);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_ADD_VLAN, &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("setAleEntry() CPSW_ALE_IOCTL_ADD_VLAN failed: %d\n", status);
        }

        getVlanInArgs.vlanId = vlanId;
        getVlanInArgs.outerVlanFlag = FALSE;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &getVlanInArgs, &getVlanOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_LOOKUP_VLAN, &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("setVLANentry() failed CPSW_ALE_IOCTL_LOOKUP_VLAN: %d\n", status);
        }
        else
        {
            CpswAppUtils_assert(vlanOutInfo.aleEntryIndex == getVlanOutArgs.aleEntryIndex);
            CpswAppUtils_assert(vlanEntry.disallowIPFragmentation  == getVlanOutArgs.disallowIPFragmentation);
            CpswAppUtils_assert(vlanEntry.forceUntaggedEgressMask == getVlanOutArgs.forceUntaggedEgressMask);
            CpswAppUtils_assert(vlanEntry.limitIPNxtHdr == getVlanOutArgs.limitIPNxtHdr);
            CpswAppUtils_assert(vlanEntry.noLearnMask == getVlanOutArgs.noLearnMask);
            CpswAppUtils_assert(vlanEntry.regMcastFloodMask == getVlanOutArgs.regMcastFloodMask);
            CpswAppUtils_assert(vlanEntry.unregMcastFloodMask == getVlanOutArgs.unregMcastFloodMask);
            CpswAppUtils_assert(vlanEntry.vidIngressCheck == getVlanOutArgs.vidIngressCheck);
            CpswAppUtils_assert(vlanEntry.vlanMemberList == getVlanOutArgs.vlanMemberList);
            CpswAppUtils_print("Added the VLAN entry successfully\n");
        }
    }
    else
    {
        getVlanInArgs.vlanId = vlanId;
        getVlanInArgs.outerVlanFlag = FALSE;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &getVlanInArgs, &getVlanOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_LOOKUP_VLAN, &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("setVLANentry() CPSW_ALE_IOCTL_LOOKUP_VLAN failed: %d\n", status);
        }
        else
        {
            CPSW_IOCTL_SET_IN_ARGS(&prms, &getVlanInArgs);
            status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_REMOVE_VLAN, &prms);
            if (status != CPSW_SOK)
            {
                CpswAppUtils_print("setVLANentry() CPSW_ALE_IOCTL_REMOVE_VLAN failed: %d\n", status);
            }
            else
            {
                CpswAppUtils_print("Removed the VLAN entry successfully\n");
            }
        }
    }
}

void CpswApp_setAleMulticastEntry(uint8_t macAddr[],
                                  uint32_t vlanId,
                                  uint32_t vlanMemberMask,
                                  uint32_t isEnable)
{
    Cpsw_IoctlPrms prms;
    CpswAle_AddEntryOutArgs setMcastOutArgs;
    CpswAle_SetMcastEntryInArgs setMcastInArgs;
    CpswAle_MacAddrInfo getMcastInArgs;
    CpswAle_GetMcastEntryOutArgs getMcastOutArgs;
    int32_t status;

    memcpy(&setMcastInArgs.addr.addr[0], macAddr, sizeof(setMcastInArgs.addr.addr));
    setMcastInArgs.addr.vlanId = vlanId;

    if (isEnable)
    {
        setMcastInArgs.info.superFlag = false;
        setMcastInArgs.info.fwdState = CPSW_ALE_FWDSTLVL_FWD_LRN;
        setMcastInArgs.info.portMask = vlanMemberMask;
        setMcastInArgs.info.numIgnBits = 0;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setMcastInArgs, &setMcastOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_ADD_MULTICAST, &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("setAleMulticastEntry() CPSW_ALE_IOCTL_ADD_MULTICAST failed: %d\n", status);
        }
        else
        {
            memcpy(&getMcastInArgs.addr[0U], macAddr, sizeof(getMcastInArgs.addr));
            getMcastInArgs.vlanId = vlanId;
            CPSW_IOCTL_SET_INOUT_ARGS(&prms, &getMcastInArgs, &getMcastOutArgs);

            status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_LOOKUP_MULTICAST, &prms);
            if (status == CPSW_SOK)
            {
                CpswAppUtils_print("Added the Multicast entry successfully\n");
            }
        }
    }
    else
    {
        memcpy(&getMcastInArgs.addr[0U], macAddr, sizeof(getMcastInArgs.addr));
        getMcastInArgs.vlanId = vlanId;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &getMcastInArgs, &getMcastOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_LOOKUP_MULTICAST, &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Multicast entry doesn't exist\n");
        }
        else
        {
            CPSW_IOCTL_SET_IN_ARGS(&prms, &getMcastInArgs);
            status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_REMOVE_ADDR, &prms);
            if(status == CPSW_SOK)
            {
                CpswAppUtils_print("Removed the Multicast entry successfully\n");
            }
        }
    }
}

static void CpswApp_setPolicerEntryRateLimit(uint8_t portNum,
                                             uint32_t rate,
                                             uint8_t srcMacAddr[],
                                             uint8_t dstMacAddr[],
                                             uint32_t isEnable)
{
    Cpsw_IoctlPrms prms;
    CpswAle_SetPolicerEntryOutArgs setPolicerOutArgs;
    CpswAle_SetPolicerEntryInArgs setPolicerInArgs;
    CpswAle_PolicerMatchParams getPolicerInArgs;
    CpswAle_GetPolicerEntryOutArgs getPolicerOutArgs;
    CpswAle_DelPolicerEntryInArgs delPolicerInArgs;
    int32_t status;

    if (isEnable)
    {
        setPolicerInArgs.policerMatch.policerMatchEnableMask   = (CPSW_ALE_POLICER_MATCH_PORT |
                                                                  CPSW_ALE_POLICER_MATCH_MACSRC |
                                                                  CPSW_ALE_POLICER_MATCH_MACDST);
        setPolicerInArgs.policerMatch.portNum                  = portNum;
        setPolicerInArgs.policerMatch.portIsTrunk              = FALSE;
        memcpy(&setPolicerInArgs.policerMatch.srcMacAddr.addr[0U],
               srcMacAddr,
               sizeof(setPolicerInArgs.policerMatch.srcMacAddr.addr));
        setPolicerInArgs.policerMatch.srcMacAddr.vlanId = 0;
        memcpy(&setPolicerInArgs.policerMatch.dstMacAddr.addr[0U],
               dstMacAddr,
               sizeof(setPolicerInArgs.policerMatch.dstMacAddr.addr));
        setPolicerInArgs.policerMatch.dstMacAddr.vlanId = 0;
        setPolicerInArgs.threadIdEnable                        = FALSE;
        setPolicerInArgs.peakRateInBitsPerSec                  = rate;
        setPolicerInArgs.commitRateInBitsPerSec                = 5000000U;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setPolicerInArgs, &setPolicerOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_SET_POLICER, &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("setPolicerEntryRateLimit() CPSW_ALE_IOCTL_SET_POLICER failed: %d\n",
                               status);
        }

        getPolicerInArgs.policerMatchEnableMask   = (CPSW_ALE_POLICER_MATCH_PORT |
                                                     CPSW_ALE_POLICER_MATCH_MACSRC |
                                                     CPSW_ALE_POLICER_MATCH_MACDST);
        getPolicerInArgs.portNum                  = portNum;
        getPolicerInArgs.portIsTrunk              = FALSE;
        memcpy(&getPolicerInArgs.srcMacAddr.addr[0U],
               srcMacAddr,
               sizeof(getPolicerInArgs.srcMacAddr.addr));
        getPolicerInArgs.srcMacAddr.vlanId = 0U;
        memcpy(&getPolicerInArgs.dstMacAddr.addr[0U],
               dstMacAddr,
               sizeof(getPolicerInArgs.dstMacAddr.addr));
        getPolicerInArgs.dstMacAddr.vlanId = 0U;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &getPolicerInArgs, &getPolicerOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_GET_POLICER, &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("setPolicerEntryRateLimit() CPSW_ALE_IOCTL_GET_POLICER failed: %d\n",
                status);
        }
        else
        {
            CpswAppUtils_assert(setPolicerOutArgs.policerEntryIndex ==
                                getPolicerOutArgs.policerEntryIndex);
            CpswAppUtils_assert(setPolicerInArgs.policerMatch.policerMatchEnableMask ==
                                getPolicerOutArgs.policerMatchEnableMask);
            CpswAppUtils_assert(setPolicerInArgs.policerMatch.portNum ==
                                getPolicerOutArgs.port);
            CpswAppUtils_assert(setPolicerOutArgs.srcMacAleEntryIndex ==
                                getPolicerOutArgs.srcMacAleEntryIndex);
            CpswAppUtils_assert(setPolicerOutArgs.dstMacAleEntryIndex ==
                                getPolicerOutArgs.dstMacAleEntryIndex);

            if (setPolicerInArgs.peakRateInBitsPerSec != 0U)
            {
                CpswAppUtils_assert((setPolicerInArgs.peakRateInBitsPerSec / getPolicerOutArgs.peakRateInBitsPerSec) == 1U);
            }
            else
            {
                CpswAppUtils_assert(getPolicerOutArgs.peakRateInBitsPerSec == 0U);
            }

            if (setPolicerInArgs.commitRateInBitsPerSec != 0U)
            {
                CpswAppUtils_assert((setPolicerInArgs.commitRateInBitsPerSec/ getPolicerOutArgs.commitRateInBitsPerSec) == 1U);
            }
            else
            {
                CpswAppUtils_assert(getPolicerOutArgs.commitRateInBitsPerSec == 0U);
            }

            CpswAppUtils_print("Added rate limiting successfully\n");
        }

    }
    else
    {
        getPolicerInArgs.policerMatchEnableMask   = (CPSW_ALE_POLICER_MATCH_PORT |
                                                     CPSW_ALE_POLICER_MATCH_MACSRC |
                                                     CPSW_ALE_POLICER_MATCH_MACDST);
        getPolicerInArgs.portNum                  = portNum;
        getPolicerInArgs.portIsTrunk              = FALSE;
        memcpy(&getPolicerInArgs.srcMacAddr.addr[0U],
               srcMacAddr,
               sizeof(getPolicerInArgs.srcMacAddr.addr));
        getPolicerInArgs.srcMacAddr.vlanId = 0U;
        memcpy(&getPolicerInArgs.dstMacAddr.addr[0U],
               dstMacAddr,
               sizeof(getPolicerInArgs.dstMacAddr.addr));
        getPolicerInArgs.dstMacAddr.vlanId = 0U;

        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &getPolicerInArgs, &getPolicerOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_GET_POLICER, &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("setPolicerEntryRateLimit() CPSW_ALE_IOCTL_GET_POLICER failed: %d\n",
                               status);
        }
        else
        {
            delPolicerInArgs.policerMatch.policerMatchEnableMask =  (CPSW_ALE_POLICER_MATCH_PORT |
                                                                     CPSW_ALE_POLICER_MATCH_MACSRC |
                                                                     CPSW_ALE_POLICER_MATCH_MACDST);
            delPolicerInArgs.policerMatch.portNum = portNum;
            delPolicerInArgs.policerMatch.portIsTrunk = FALSE;
            memcpy(&delPolicerInArgs.policerMatch.srcMacAddr.addr[0U],
                   srcMacAddr,
                   sizeof(delPolicerInArgs.policerMatch.srcMacAddr));
            delPolicerInArgs.policerMatch.srcMacAddr.vlanId = 0U;
            memcpy(&delPolicerInArgs.policerMatch.dstMacAddr.addr[0U],
                   dstMacAddr,
                   sizeof(delPolicerInArgs.policerMatch.dstMacAddr));
            delPolicerInArgs.policerMatch.dstMacAddr.vlanId = 0U;
            delPolicerInArgs.delAleEntry = TRUE;
            CPSW_IOCTL_SET_IN_ARGS(&prms, &getPolicerInArgs);

            status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_DEL_POLICER, &prms);
            if (status != CPSW_SOK)
            {
                CpswAppUtils_print("setPolicerEntryRateLimit() CPSW_ALE_IOCTL_DEL_POLICER failed: %d\n",
                                   status);
            }
            else
            {
                CpswAppUtils_print("Removed rate limiting successfully\n");
            }
        }
    }
}

static void CpswApp_getMacAddr(uint8_t macAddr[])
{
    uint32_t tmp[ETH_MAC_ADDR_LEN];
    uint32_t i;
    int32_t status;

    do
    {
        status = UART_scanFmt("%x %x %x %x %x %x",
                              &tmp[0], &tmp[1], &tmp[2],
                              &tmp[3], &tmp[4], &tmp[5]);
        if (status != S_PASS)
        {
            CpswAppUtils_print("Invalid MAC address, try again...\n");
        }
    }
    while (status != S_PASS);


    /* Typecast to 8-bit integer */
    for (i = 0U; i < ETH_MAC_ADDR_LEN; i++)
    {
        macAddr[i] = (uint8_t)tmp[i];
    }
}

static Void UartMenuTskFxn(UArg a0, UArg a1)
{
    bool runflag = true;

    while(runflag)
    {
        uint8_t mcastAddr[ETH_MAC_ADDR_LEN];
        uint8_t srcMacAddr[ETH_MAC_ADDR_LEN];
        uint8_t dstMacAddr[ETH_MAC_ADDR_LEN];
        uint8_t ingressPortNum, egressPortNum;
        uint32_t vlanId, portmask, isEnable, rate;
        int32_t choice = 0U;

        Task_sleep(1000 * 10);
        CpswAppUtils_print("%s", gCpswSwitchMenu);
        UART_scanFmt("%d", &choice);

        switch(choice)
        {
            case APP_MENU_OPTION_VLAN:
                CpswAppUtils_print("\n Enter VLAN ID: ");
                UART_scanFmt("%d", &vlanId);

                CpswAppUtils_print("\n Enter PortMask: ");
                UART_scanFmt("%d", &portmask);

                CpswAppUtils_print("\n Enable/Disable(1/0): ");
                UART_scanFmt("%d", &isEnable);

                CpswApp_setVLANentry(vlanId, portmask, isEnable);
                break;

            case APP_MENU_OPTION_MULTICAST:
                CpswAppUtils_print("\n Enter Multicast Address: ");
                CpswApp_getMacAddr(&mcastAddr[0U]);

                CpswAppUtils_print("\n Enter VLAN ID: ");
                UART_scanFmt("%d", &vlanId);

                CpswAppUtils_print("\n Enter PortMask: ");
                UART_scanFmt("%d", &portmask);

                CpswAppUtils_print("\n Enable/Disable(1/0): ");
                UART_scanFmt("%d", &isEnable);

                CpswApp_setAleMulticastEntry(&mcastAddr[0U], vlanId, portmask, isEnable);
                break;

            case APP_MENU_OPTION_RATE_LIM:
                CpswAppUtils_print("\n Enter Ingress Port Num: ");
                UART_scanFmt("%d", &ingressPortNum);

                CpswAppUtils_print("\n Enter Egress Port Num: ");
                UART_scanFmt("%d", &egressPortNum);

                CpswAppUtils_print("\n Enter Src Mac Address: ");
                CpswApp_getMacAddr( &srcMacAddr[0U]);

                CpswAppUtils_print("\n Enter Dst Mac Address: ");
                CpswApp_getMacAddr( &dstMacAddr[0U]);

                CpswAppUtils_print("\n Enable/Disable(1/0): ");
                UART_scanFmt("%d", &isEnable);

                if (isEnable)
                {
                    CpswAppUtils_print("\n Enter Rate: ");
                    UART_scanFmt("%d", &rate);
                }

                CpswApp_addUnicastAddressEntry(&srcMacAddr[0U], ingressPortNum);
                CpswApp_addUnicastAddressEntry(&dstMacAddr[0U], egressPortNum);
                CpswApp_setPolicerEntryRateLimit(ingressPortNum, rate,
                                                 &srcMacAddr[0U], &dstMacAddr[0U],
                                                 isEnable);
                break;

            case APP_MENU_OPTION_INTERVLAN:
                CpswAppUtils_print("\n InterVLAN is currently not implemented\n");
                break;

            case APP_MENU_OPTION_SHOW_ALE:
                CpswApp_showAleTableAndPolicer();
                break;

            default:
                break;
        }

    }
}

static void CpswApp_createUartMenuTask(void)
{
    Task_Params params;
    Error_Block eb;

    Error_init(&eb);

    /* Initialize the task params. Set the task priority higher than the
     * default priority (1) */
    Task_Params_init(&params);
    params.priority  = 2U;
    params.stack     = gAppTskStackUart;
    params.stackSize = sizeof(gAppTskStackUart);
    taskUartMenu = Task_create(UartMenuTskFxn, &params, &eb);
    if (taskUartMenu == NULL)
    {
        BIOS_exit(0);
    }
}

static void CpswApp_setTxChPrms(CpswDma_OpenTxChPrms *pTxChPrms)
{
    pTxChPrms->cpswInstance = gCpswSwitchAppObj.cpswType;
    pTxChPrms->hUdmaDrv     = gCpswSwitchAppObj.hUdmaDrv;

    /* TODO this should be taken from CPSW RM */
    pTxChPrms->chNum               = CPSW_DMA_TX_CH_NUM(1);

    pTxChPrms->ringMemAllocFxn     = &CpswAppMemUtils_allocRingMemFxn;
    pTxChPrms->ringMemFreeFxn      = &CpswAppMemUtils_freeRingMemFxn;

    pTxChPrms->numTxPkts           = CPSW_APPMEMUTILS_NUM_TX_PKTS;
    pTxChPrms->disableCacheOpsFlag = false;

    pTxChPrms->dmaDescAllocFxn     = &CpswAppMemUtils_allocDmaDescFxn;
    pTxChPrms->dmaDescFreeFxn      = &CpswAppMemUtils_freeDmaDescFxn;

    pTxChPrms->udmaEvtCfg.eventCb     = &CpswApp_txIsrFxn;
    pTxChPrms->udmaEvtCfg.hEventCbArg = &gCpswSwitchAppObj.hTxCh;

    pTxChPrms->hCallbackArg        = &gCpswSwitchAppObj.hTxCh;
}

static void CpswApp_setRxflowPrms(CpswDma_OpenRxFlowPrms *pRxFlowPrms)
{
    pRxFlowPrms->cpswInstance           = gCpswSwitchAppObj.cpswType;
    pRxFlowPrms->hUdmaDrv               = gCpswSwitchAppObj.hUdmaDrv;

    pRxFlowPrms->ringMemAllocFxn        = &CpswAppMemUtils_allocRingMemFxn;
    pRxFlowPrms->ringMemFreeFxn         = &CpswAppMemUtils_freeRingMemFxn;

    pRxFlowPrms->udmaEvtCfg.eventCb     = &CpswApp_rxIsrFxn;
    pRxFlowPrms->udmaEvtCfg.hEventCbArg = &gCpswSwitchAppObj.hRxFlow;

    pRxFlowPrms->numRxPkts              = CPSW_APPMEMUTILS_NUM_RX_PKTS;
    pRxFlowPrms->maxPktLength           = CPSW_APPMEMUTILS_LARGE_POOL_PKT_SIZE;

    pRxFlowPrms->disableCacheOpsFlag    = false;
    pRxFlowPrms->dmaDescAllocFxn        = &CpswAppMemUtils_allocDmaDescFxn;
    pRxFlowPrms->dmaDescFreeFxn         = &CpswAppMemUtils_freeDmaDescFxn;

    pRxFlowPrms->hCallbackArg           = &gCpswSwitchAppObj.hRxFlow;

}

static void CpswApp_initTxFreePktQ(void)
{
    CpswDma_PktInfo *pktInfo;
    uint32_t i;

    /* Initialize all queues */
    CpswUtils_initQ(&gCpswSwitchAppObj.txFreePktInfoQ);

    /* Initialize TX EthPkts and queue them to txFreePktInfoQ */
    for (i = 0U; i < CPSWAPPUTILS_ARRAY_SIZE(gCpswSwitchAppObj.txFreePktInfo); i++)
    {
        pktInfo = &gCpswSwitchAppObj.txFreePktInfo[i];

        CpswDma_pktInfoInit(pktInfo);

        memset (&pktInfo->node, 0U, sizeof(pktInfo->node));
        pktInfo->bufPtr     = (uint8_t *) &gCpswSwitchAppObj.txBufMem[i][0U];
        pktInfo->orgBufLen  = ETH_MAX_FRAME_LEN;
        pktInfo->userBufLen = ETH_MAX_FRAME_LEN;
        pktInfo->appPriv    = NULL;

        CpswUtils_enQ(&gCpswSwitchAppObj.txFreePktInfoQ, &pktInfo->node);
    }

    CpswAppUtils_print("initQs() txFreePktInfoQ initialized with %d pkts\n",
                       CpswUtils_getQCount(&gCpswSwitchAppObj.txFreePktInfoQ));
}

static void CpswApp_initRxReadyPktQ(void)
{
    CpswDma_PktInfoQ rxReadyQ;
    CpswDma_PktInfo *pPktInfo;
    uint32_t i;
    int32_t status;

    CpswUtils_initQ(&gCpswSwitchAppObj.rxFreeQ);
    CpswUtils_initQ(&rxReadyQ);

    for (i = 0U; i < CPSW_APPMEMUTILS_NUM_RX_PKTS; i++)
    {
        pPktInfo = CpswAppMemUtils_allocEthPktFxn(&gCpswSwitchAppObj,
                                                  CPSW_APPMEMUTILS_LARGE_POOL_PKT_SIZE,
                                                  UDMA_CACHELINE_ALIGNMENT);
        CpswAppUtils_assert(pPktInfo != NULL);
        CpswUtils_enQ(&gCpswSwitchAppObj.rxFreeQ, &pPktInfo->node);
    }

    /* Retrieve any CPSW packets which are ready */
    status = CpswDma_retrieveRxPackets(gCpswSwitchAppObj.hRxFlow, &rxReadyQ);
    CpswAppUtils_assert(status == CPSW_SOK);

    /* There should not be any packet with DMA during init */
    CpswAppUtils_assert(CpswUtils_getQCount(&rxReadyQ) == 0U);

    CpswAppUtils_submitRxPackets(gCpswSwitchAppObj.hRxFlow,
                                 &gCpswSwitchAppObj.rxFreeQ);

    /* Assert here as during init no. of DMA descriptors should be equal to
     * no. of free Ethernet buffers available with app */
    CpswAppUtils_assert( 0U == CpswUtils_getQCount(&gCpswSwitchAppObj.rxFreeQ));
}

static void CpswApp_addUnicastAddressEntry(uint8_t macAddr[],
                                           uint32_t portNum)
{
    Cpsw_IoctlPrms prms;
    CpswAle_AddEntryOutArgs setUcastOutArgs;
    CpswAle_SetUcastEntryInArgs setUcastInArgs;
    int32_t status;

    memcpy (&setUcastInArgs.addr.addr[0U], macAddr, sizeof(setUcastInArgs.addr.addr));
    setUcastInArgs.addr.vlanId = 0U;
    setUcastInArgs.info.portNum = portNum;
    setUcastInArgs.info.blocked = false;
    setUcastInArgs.info.secure = false;
    setUcastInArgs.info.super = 0U;
    setUcastInArgs.info.ageable = false;
    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setUcastInArgs, &setUcastOutArgs);

    status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_ADD_UNICAST, &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("addUnicastAddressEntry() CPSW_ALE_IOCTL_ADD_UNICAST failed: %d\n",
                           status);
    }
}

static void CpswApp_addClasifierEntry(uint8_t srcMacAddr[],
                                      uint32_t flowId)
{
    Cpsw_IoctlPrms prms;
    CpswAle_SetPolicerEntryOutArgs setPolicerEntryOutArgs;
    CpswAle_SetPolicerEntryInArgs setPolicerEntryInArgs;
    int32_t status;

    setPolicerEntryInArgs.policerMatch.policerMatchEnableMask = CPSW_ALE_POLICER_MATCH_MACSRC;
    memcpy(&setPolicerEntryInArgs.policerMatch.srcMacAddr.addr[0U],
           srcMacAddr,
           sizeof(setPolicerEntryInArgs.policerMatch.srcMacAddr.addr));
    setPolicerEntryInArgs.policerMatch.srcMacAddr.vlanId = 0U;
    setPolicerEntryInArgs.threadIdEnable = true;
    setPolicerEntryInArgs.threadId = flowId;
    setPolicerEntryInArgs.peakRateInBitsPerSec = 0U;
    setPolicerEntryInArgs.commitRateInBitsPerSec = 0U;

    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setPolicerEntryInArgs, &setPolicerEntryOutArgs);

    status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_SET_POLICER, &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("addClasifierEntry() CPSW_ALE_IOCTL_SET_POLICER failed: %d\n",
                           status);
    }

    CpswApp_addUnicastAddressEntry(srcMacAddr, 1U);
}

static void CpswApp_showAleTableAndPolicer(void)
{
    Cpsw_IoctlPrms prms;
    int32_t status;

    CPSW_IOCTL_SET_NO_ARGS(&prms);
    status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_DUMP_TABLE, &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("showAleTableAndPolicer() CPSW_ALE_IOCTL_DUMP_TABLE failed: %d\n",
                           status);
    }

    CPSW_IOCTL_SET_NO_ARGS(&prms);
    status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw, CPSW_ALE_IOCTL_DUMP_POLICER_ENTRIES, &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("showAleTableAndPolicer() CPSW_ALE_IOCTL_DUMP_POLICER_ENTRIES failed: %d\n",
                           status);
    }
}

static int32_t CpswSwitchApp_getRxTxHandle(void)
{
    CpswDma_OpenTxChPrms cpswTxChCfg;
    CpswDma_OpenRxFlowPrms cpswRxFlowCfg;
    int32_t status = CPSW_SOK;

    /* Open the CPSW TX channel  */
    CpswApp_initTxFreePktQ();

    /* Set configuration parameters */
    CpswDma_initTxChParams(&cpswTxChCfg);
    CpswApp_setTxChPrms(&cpswTxChCfg);
    gCpswSwitchAppObj.hTxCh = CpswDma_openTxCh(&cpswTxChCfg);
    if (NULL != gCpswSwitchAppObj.hTxCh)
    {
        status = CpswDma_enableTxEvent(gCpswSwitchAppObj.hTxCh);
        if (CPSW_SOK != status)
        {
            CpswAppUtils_print("getRxTxHandle() failed enable TX event: %d\n", status);
            status = CPSW_EFAIL;
        }
    }
    else
    {
        CpswAppUtils_print("getRxTxHandle() failed to open TX channel\n");
        status = CPSW_EFAIL;
    }

    /* Open the CPSW RX flow */
    if (status == CPSW_SOK)
    {
        CpswDma_initRxFlowParams(&cpswRxFlowCfg);
        CpswApp_setRxflowPrms(&cpswRxFlowCfg);
        cpswRxFlowCfg.flowIdx = Cpsw_rmAllocFlowIndex(gCpswSwitchAppObj.hCpsw);

        gCpswSwitchAppObj.hRxFlow = CpswDma_openRxFlow(&cpswRxFlowCfg);
        if (NULL == gCpswSwitchAppObj.hRxFlow)
        {
            CpswAppUtils_print("getRxTxHandle() failed to open RX flow\n");
            Cpsw_rmFreeFlowIndex(gCpswSwitchAppObj.hCpsw,
                                 cpswRxFlowCfg.flowIdx);
            CpswAppUtils_assert (NULL != gCpswSwitchAppObj.hRxFlow);
        }
        else
        {
            /* Save flow idx */
            gCpswSwitchAppObj.rxFlowIdx = cpswRxFlowCfg.flowIdx;

            /* Submit all ready RX buffers to DMA.*/
            CpswApp_initRxReadyPktQ();
        }
    }

    if (status == CPSW_SOK)
    {
        CpswApp_addClasifierEntry(&flowAddr1[0U], gCpswSwitchAppObj.rxFlowIdx);
    }

    return status;
}

static int32_t CpswApp_pktRxTx(void)
{
    CpswDma_PktInfoQ txSubmitQ;
    CpswDma_PktInfo *pktInfo;
    EthFrame *frame;
    uint32_t loopCntr;
    uint32_t txRetrievePktCnt = 0U;
    uint32_t rxReadyCnt;
    int32_t status = CPSW_SOK;

    /* Transmit a single packet */
    CpswUtils_initQ(&txSubmitQ);

    /* Send one packet from host to all external ports */
    pktInfo = (CpswDma_PktInfo *)CpswUtils_deQ(&gCpswSwitchAppObj.txFreePktInfoQ);
    if (NULL != pktInfo)
    {
        /* Fill the TX Eth frame with test content */
        frame = (EthFrame *) pktInfo->bufPtr;
        memcpy(frame->hdr.dstMac, bcastAddr, ETH_MAC_ADDR_LEN);
        memcpy(frame->hdr.srcMac, &gCpswSwitchAppObj.hostMacAddr[0U], ETH_MAC_ADDR_LEN);
        frame->hdr.etherType = htons(ETHERTYPE_EXPERIMENTAL1);
        memset(&frame->payload[0U], (uint8_t) (0xA5), TEST_LEN);
        pktInfo->userBufLen = TEST_LEN + sizeof (EthFrameHeader);
        pktInfo->appPriv = &gCpswSwitchAppObj;

        /* Enqueue the packet for later transmission */
        CpswUtils_enQ(&txSubmitQ, &pktInfo->node);

        status = CpswAppUtils_submitTxPackets(gCpswSwitchAppObj.hTxCh,
                                              &txSubmitQ);
        /* Retrieve TX free packets */
        if (status == CPSW_SOK)
        {
            while (txRetrievePktCnt != 1U)
            {
                //TODO this is not failure as HW is busy sending packets, we
                // need to wait and again call retrieve packets
                Task_sleep(1);
                txRetrievePktCnt += CpswApp_retrieveFreeTxPkts();
#if DEBUG
                CpswAppUtils_print("pktRxTx() failed to retrieve consumed TX packets: %d\n", status);
#endif
            }
        }
        else
        {
            CpswAppUtils_print("pktRxTx() failed to submit TX packet: %d\n", status);
        }
    }
    else
    {
        CpswAppUtils_print("pktRxTx() failed to dequeue free TX packet\n");
    }

    loopCntr = 1U;
    while(loopCntr)
    {
        /* Sleep so that other flow gets scheduled */
        Task_sleep(1);

        /* Get the packets received so far */
        rxReadyCnt = CpswApp_receivePkts();
        if (rxReadyCnt > 0U)
        {
            /* Consume the received packets and release them */
            pktInfo = (CpswDma_PktInfo *)CpswUtils_deQ(&gCpswSwitchAppObj.rxReadyQ);
            while (NULL != pktInfo)
            {
                /* Consume the packet by just printing its content */
                frame = (EthFrame *)pktInfo->bufPtr;
#ifdef ENABLE_PRINTFRAME
                CpswAppUtils_printFrame(frame,
                                        pktInfo->userBufLen - sizeof(EthFrameHeader));
#endif
                /* Release the received packet */
                CpswUtils_enQ(&gCpswSwitchAppObj.rxFreeQ, &pktInfo->node);
                pktInfo = (CpswDma_PktInfo *)CpswUtils_deQ(&gCpswSwitchAppObj.rxReadyQ);
            }

            /*Submit now processed buffers */
            if (status == CPSW_SOK)
            {
                CpswAppUtils_submitRxPackets(gCpswSwitchAppObj.hRxFlow,
                                             &gCpswSwitchAppObj.rxFreeQ);
            }

            CpswAppUtils_print("Received %d packets\n", rxReadyCnt);
        }
    }

    return status;
}

static uint32_t CpswApp_retrieveFreeTxPkts(void)
{
    CpswDma_PktInfoQ txFreeQ;
    CpswDma_PktInfo *pktInfo;
    uint32_t txFreeQCnt = 0U;
    int32_t status;

    CpswUtils_initQ(&txFreeQ);

    /* Retrieve any CPSW packets that may be free now */
    status = CpswDma_retrieveTxDonePackets(gCpswSwitchAppObj.hTxCh, &txFreeQ);
    if (status == CPSW_SOK)
    {
        txFreeQCnt = CpswUtils_getQCount(&txFreeQ);

        pktInfo = (CpswDma_PktInfo *)CpswUtils_deQ(&txFreeQ);
        while (NULL != pktInfo)
        {
            CpswUtils_enQ(&gCpswSwitchAppObj.txFreePktInfoQ, &pktInfo->node);

            pktInfo = (CpswDma_PktInfo *)CpswUtils_deQ(&txFreeQ);
        }
    }
    else
    {
        CpswAppUtils_print("retrieveFreeTxPkts() failed to retrieve pkts: %d\n", status);
    }

    return txFreeQCnt;
}

static uint32_t CpswApp_receivePkts(void)
{
    CpswDma_PktInfoQ rxReadyQ;
    CpswDma_PktInfo *pktInfo;
    uint32_t rxReadyCnt = 0U;
    int32_t status;

    CpswUtils_initQ(&rxReadyQ);

    /* Retrieve any CPSW packets which are ready */
    status = CpswDma_retrieveRxPackets(gCpswSwitchAppObj.hRxFlow, &rxReadyQ);
    if (status == CPSW_SOK)
    {
        rxReadyCnt = CpswUtils_getQCount(&rxReadyQ);
        /* Queue the received packet to rxReadyQ and pass new ones from rxFreeQ
        **/
        pktInfo = (CpswDma_PktInfo *)CpswUtils_deQ(&rxReadyQ);
        while (pktInfo != NULL)
        {
            CpswUtils_enQ(&gCpswSwitchAppObj.rxReadyQ, &pktInfo->node);
            pktInfo = (CpswDma_PktInfo *)CpswUtils_deQ(&rxReadyQ);
        }
    }
    else
    {
        CpswAppUtils_print("receivePkts() failed to retrieve pkts: %d\n", status);
    }

    return rxReadyCnt;
}
