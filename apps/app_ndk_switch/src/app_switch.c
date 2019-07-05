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
 * \file     app_switch.c
 *
 * \brief    This file contains the app_switch test implementation.
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
#include <ti/sysbios/utils/Load.h>

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
    /* Core Id */
    uint32_t coreId;

    /* CPSW driver handle */
    Cpsw_Handle          hCpsw;

    /* UDMA driver handle */
    Udma_DrvHandle       hUdmaDrv;
} CpswApp_Obj;

/* ========================================================================== */
/*                 Internal Function Declarations                             */
/* ========================================================================== */

static void CpswApp_addUnicastAddressEntry(uint8_t macAddr[],
                                           uint32_t portNum);
static void CpswApp_showAleTableAndPolicer(void);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* Switch menu 0 string */
static const char gCpswAppSwitchMenu[] =
{
    "\r\n================================================="
    "\r\n                   Switch Options                "
    "\r\n================================================="
    "\r\n 1. Enable/Disable VLAN "
    "\r\n 2. Enable/Disable Multicast"
    "\r\n 3. Enable/Disable Rate Limiting "
    "\r\n 4. Enable/Disable InterVLAN "
    "\r\n 5. Print ALE & Policer Table "
    "\r\n Enter your choice: "
    "\r\n"
};

/* UART Menu stack */
static uint8_t gAppTskStackUart[APP_TSK_STACK_UART] __attribute__((aligned(32)));

CpswApp_Obj gCpswSwitchAppObj;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

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

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                            gCpswSwitchAppObj.coreId,
                            CPSW_ALE_IOCTL_ADD_VLAN,
                            &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("setAleEntry() CPSW_ALE_IOCTL_ADD_VLAN failed: %d\n", status);
        }

        getVlanInArgs.vlanId = vlanId;
        getVlanInArgs.outerVlanFlag = FALSE;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &getVlanInArgs, &getVlanOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                            gCpswSwitchAppObj.coreId,
                            CPSW_ALE_IOCTL_LOOKUP_VLAN,
                            &prms);
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

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                            gCpswSwitchAppObj.coreId,
                            CPSW_ALE_IOCTL_LOOKUP_VLAN,
                            &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("setVLANentry() CPSW_ALE_IOCTL_LOOKUP_VLAN failed: %d\n", status);
        }
        else
        {
            CPSW_IOCTL_SET_IN_ARGS(&prms, &getVlanInArgs);
            status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                                gCpswSwitchAppObj.coreId,
                                CPSW_ALE_IOCTL_REMOVE_VLAN,
                                &prms);
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

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                            gCpswSwitchAppObj.coreId,
                            CPSW_ALE_IOCTL_ADD_MULTICAST,
                            &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("setAleMulticastEntry() CPSW_ALE_IOCTL_ADD_MULTICAST failed: %d\n", status);
        }
        else
        {
            memcpy(&getMcastInArgs.addr[0U], macAddr, sizeof(getMcastInArgs.addr));
            getMcastInArgs.vlanId = vlanId;
            CPSW_IOCTL_SET_INOUT_ARGS(&prms, &getMcastInArgs, &getMcastOutArgs);

            status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                                gCpswSwitchAppObj.coreId,
                                CPSW_ALE_IOCTL_LOOKUP_MULTICAST,
                                &prms);
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

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                            gCpswSwitchAppObj.coreId,
                            CPSW_ALE_IOCTL_LOOKUP_MULTICAST,
                            &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Multicast entry doesn't exist\n");
        }
        else
        {
            CPSW_IOCTL_SET_IN_ARGS(&prms, &getMcastInArgs);
            status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                                gCpswSwitchAppObj.coreId,
                                CPSW_ALE_IOCTL_REMOVE_ADDR,
                                &prms);
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
        memcpy(&setPolicerInArgs.policerMatch.srcMacAddr.addr.addr[0U],
               srcMacAddr,
               sizeof(setPolicerInArgs.policerMatch.srcMacAddr.addr.addr));
        setPolicerInArgs.policerMatch.srcMacAddr.addr.vlanId = 0;
        memcpy(&setPolicerInArgs.policerMatch.dstMacAddr.addr.addr[0U],
               dstMacAddr,
               sizeof(setPolicerInArgs.policerMatch.dstMacAddr.addr.addr));
        setPolicerInArgs.policerMatch.dstMacAddr.addr.vlanId = 0;
        setPolicerInArgs.threadIdEnable                        = FALSE;
        setPolicerInArgs.peakRateInBitsPerSec                  = rate;
        setPolicerInArgs.commitRateInBitsPerSec                = 5000000U;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setPolicerInArgs, &setPolicerOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                            gCpswSwitchAppObj.coreId,
                            CPSW_ALE_IOCTL_SET_POLICER,
                            &prms);
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
        memcpy(&getPolicerInArgs.srcMacAddr.addr.addr[0U],
               srcMacAddr,
               sizeof(getPolicerInArgs.srcMacAddr.addr.addr));
        getPolicerInArgs.srcMacAddr.addr.vlanId = 0U;
        memcpy(&getPolicerInArgs.dstMacAddr.addr.addr[0U],
               dstMacAddr,
               sizeof(getPolicerInArgs.dstMacAddr.addr.addr));
        getPolicerInArgs.dstMacAddr.addr.vlanId = 0U;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &getPolicerInArgs, &getPolicerOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                            gCpswSwitchAppObj.coreId,
                            CPSW_ALE_IOCTL_GET_POLICER,
                            &prms);
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
        memcpy(&getPolicerInArgs.srcMacAddr.addr.addr[0U],
               srcMacAddr,
               sizeof(getPolicerInArgs.srcMacAddr.addr.addr));
        getPolicerInArgs.srcMacAddr.addr.vlanId = 0U;
        memcpy(&getPolicerInArgs.dstMacAddr.addr.addr[0U],
               dstMacAddr,
               sizeof(getPolicerInArgs.dstMacAddr.addr.addr));
        getPolicerInArgs.dstMacAddr.addr.vlanId = 0U;

        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &getPolicerInArgs, &getPolicerOutArgs);

        status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                            gCpswSwitchAppObj.coreId,
                            CPSW_ALE_IOCTL_GET_POLICER,
                            &prms);
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
            memcpy(&delPolicerInArgs.policerMatch.srcMacAddr.addr.addr[0U],
                   srcMacAddr,
                   sizeof(delPolicerInArgs.policerMatch.srcMacAddr.addr));
            delPolicerInArgs.policerMatch.srcMacAddr.addr.vlanId = 0U;
            memcpy(&delPolicerInArgs.policerMatch.dstMacAddr.addr.addr[0U],
                   dstMacAddr,
                   sizeof(delPolicerInArgs.policerMatch.dstMacAddr.addr));
            delPolicerInArgs.policerMatch.dstMacAddr.addr.vlanId = 0U;
            delPolicerInArgs.delAleEntryMask = 0U; /* FIXME */
            CPSW_IOCTL_SET_IN_ARGS(&prms, &getPolicerInArgs);

            status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                                gCpswSwitchAppObj.coreId,
                                CPSW_ALE_IOCTL_DEL_POLICER,
                                &prms);
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

static Void CpswApp_cpuLoadTask(UArg a0, UArg a1)
{
    while (true)
    {
        //TODO Add task load and more granular load support
        CpswAppUtils_print("CPU Load: %u%%\n", Load_getCPULoad());
        Task_sleep(5000U);
    }
}

static Void CpswApp_uartMenuTskFxn(UArg a0, UArg a1)
{
    bool runflag = true;
    CpswAppIf_HandleInfo handleInfo;
    int32_t status = CPSW_EFAIL;

    /* Get CPSW & UDMA Drv Handle */
    CpswAppIf_getHandles(&handleInfo);

    gCpswSwitchAppObj.hCpsw = handleInfo.hCpsw;
    gCpswSwitchAppObj.hUdmaDrv = handleInfo.hUdmaDrv;
    gCpswSwitchAppObj.coreId = handleInfo.coreId;

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

    while(runflag)
    {
        uint8_t mcastAddr[ETH_MAC_ADDR_LEN];
        uint8_t srcMacAddr[ETH_MAC_ADDR_LEN];
        uint8_t dstMacAddr[ETH_MAC_ADDR_LEN];
        uint8_t ingressPortNum, egressPortNum;
        uint32_t vlanId, portmask, isEnable, rate;
        int32_t choice = 0U;

        CpswAppUtils_print("%s", gCpswAppSwitchMenu);
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

void CpswApp_createUartMenuTask(void)
{
    Task_Params params;
    Error_Block eb;
    static Task_Handle hUartMenuTask, hCpuLoadTask;

    Error_init(&eb);

    /* Initialize the task params. Set the task priority higher than the
     * default priority (1) */
    Task_Params_init(&params);
    params.priority  = 2U;
    params.stack     = gAppTskStackUart;
    params.stackSize = sizeof(gAppTskStackUart);
    hUartMenuTask = Task_create(CpswApp_uartMenuTskFxn, &params, &eb);
    if (hUartMenuTask == NULL)
    {
        BIOS_exit(0);
    }

    Task_Params_init(&params);
    params.instance->name = "CPU_LOAD";
    params.priority = 1U;
    hCpuLoadTask = Task_create(CpswApp_cpuLoadTask, &params, &eb);
    if (hCpuLoadTask == NULL)
    {
        BIOS_exit(0);
    }
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

    status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                        gCpswSwitchAppObj.coreId,
                        CPSW_ALE_IOCTL_ADD_UNICAST,
                        &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("addUnicastAddressEntry() CPSW_ALE_IOCTL_ADD_UNICAST failed: %d\n",
                           status);
    }
}

static void CpswApp_showAleTableAndPolicer(void)
{
    Cpsw_IoctlPrms prms;
    int32_t status;

    CPSW_IOCTL_SET_NO_ARGS(&prms);
    status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                        gCpswSwitchAppObj.coreId,
                        CPSW_ALE_IOCTL_DUMP_TABLE,
                        &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("showAleTableAndPolicer() CPSW_ALE_IOCTL_DUMP_TABLE failed: %d\n",
                           status);
    }

    CPSW_IOCTL_SET_NO_ARGS(&prms);
    status = Cpsw_ioctl(gCpswSwitchAppObj.hCpsw,
                        gCpswSwitchAppObj.coreId,
                        CPSW_ALE_IOCTL_DUMP_POLICER_ENTRIES,
                        &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("showAleTableAndPolicer() CPSW_ALE_IOCTL_DUMP_POLICER_ENTRIES failed: %d\n",
                           status);
    }
}


