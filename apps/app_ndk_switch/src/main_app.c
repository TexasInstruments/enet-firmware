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

/**
 *  \file main_tirtos.c
 *
 *  \brief Main file for TI-RTOS build
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/utils/Load.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Mailbox.h>

/* OSAL Header file */
#include <ti/osal/osal.h>

/* PDK Driver Header files */
#include <ti/drv/sciclient/sciclient.h>
#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils_cfg.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appmemutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_appboardutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_multiclientmanager.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpswapp_ethutils.h>
#include <ti/drv/cpsw/nimucpsw/nimu_ndk.h>
#include <ti/drv/cpsw/nimucpsw/ndk2cpsw_appif.h>

/* NDK headers */
#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/ndk/inc/socket.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/tools/servers.h>
#include <ti/ndk/inc/tools/console.h>

#include "app_switch.h"
#include "webpage.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* Test application stack size */
#define APP_TSK_STACK_MAIN              (10U * 1024U)

#define ENABLE_NDKSERVERS

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct
{
    /* CPSW instance type */
    Cpsw_Type cpswType;

    /* MAC ports */
    Cpsw_MacPort *macPorts;

    /* Number of MAC ports */
    uint32_t numMacPorts;

    /* Multiclient manager handles */
    CpswMcm_Handle hMcm[CPSW_COUNT];

    /* UDMA driver handle */
    Udma_DrvHandle hUdmaDrv;

    /* Host port MAC address */
    uint8_t hostMacAddr[ETH_MAC_ADDR_LEN];
} CpswMain_AppObj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static void  CpswApp_createSecondFlowTask(void);
static Void secondFlowTaskFxn(UArg a0, UArg a1);
static uint32_t CpswApp_getNavSSInstanceId(Cpsw_Type cpswType);
static void CpswApp_setAleConfig(CpswAle_Config *aleConfig);
static void CpswApp_initLinkArgs(Cpsw_OpenPortLinkInArgs *linkArgs,
                                 Cpsw_MacPort macPort);
static int32_t CpswApp_init(void);

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

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

char *VerStr = "NIMU CPSW Example";

/* Test application stack */
static uint8_t gAppTskStackMain[APP_TSK_STACK_MAIN]
                                __attribute__((aligned(32)));

static Task_Handle task;

static Cpsw_MacPort gCpswMainAppMacPorts[] = {
#if defined(SOC_AM65XX)
    CPSW_MAC_PORT_0,
#elif defined(SOC_J721E)
    CPSW_MAC_PORT_1,
    CPSW_MAC_PORT_0,
    CPSW_MAC_PORT_2,
    CPSW_MAC_PORT_3,
#endif
};

static CpswMain_AppObj gCpswMainAppObj = {
#if defined(SOC_AM65XX)
    .cpswType = CPSW_2G,
#elif defined(SOC_J721E)
    .cpswType = CPSW_9G,
#endif
    .macPorts = gCpswMainAppMacPorts,
    .numMacPorts = CPSWAPPUTILS_ARRAY_SIZE(gCpswMainAppMacPorts),
};

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int main(void)
{
    int32_t status;

    CpswAppBoardUtils_init();
    CpswAppUtils_print("Board_init success\n");

    CpswAppUtils_setMacMode(gCpswMainAppObj.cpswType,
                            MAC_CONN_TYPE_RGMII_FORCE_1000_FULL);

    CpswAppUtils_print("=======================================================\n");
    CpswAppUtils_print ("           CPSW MULTI RX-FLOW & MULTI TX APP          \n");
    CpswAppUtils_print("=======================================================\n");

    status = CpswApp_init();

    CpswApp_createSecondFlowTask();

    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("Failed to open CPSW: %d\n", status);
        CpswAppUtils_assert(status == CPSW_SOK);
    }

    /* does not return */
    BIOS_start();

    return(0);
}

static void CpswApp_createSecondFlowTask(void)
{
    Task_Params params;
    Error_Block eb;

    Error_init(&eb);

    /* Initialize the task params. Set the task priority higher than the
     * default priority (1) */
    Task_Params_init(&params);
    params.priority  = 2U;
    params.stack     = gAppTskStackMain;
    params.stackSize = sizeof(gAppTskStackMain);
    task = Task_create(secondFlowTaskFxn, &params, &eb);
    if (task == NULL)
    {
        BIOS_exit(0);
    }
}

static Void secondFlowTaskFxn(UArg a0, UArg a1)
{
    uint8_t i = 0;

    /* Run the Second Flow test */
    CpswApp_secondFlowTest(i);
}

static uint32_t CpswApp_getNavSSInstanceId(Cpsw_Type cpswType)
{
    return (CPSW_2G == cpswType) ? UDMA_INST_ID_MCU_0 : UDMA_INST_ID_MAIN_0;
}

static void CpswApp_setAleConfig(CpswAle_Config *aleConfig)
{
    aleConfig->modeFlags = CPSW_ALE_CONFIG_MASK_ALE_MODULE_ENABLE |
                           CPSW_ALE_CONFIG_MASK_UNKNOWN_UNICAST_FLOOD2HOST;

    aleConfig->agingConfig.enableAutoAging = TRUE;
    aleConfig->agingConfig.agingPeriodInMs = 1000;

    aleConfig->nwSecCfg.enableVid0Mode = FALSE;

    aleConfig->vlanConfig.aleVlanAwareMode = TRUE;
    aleConfig->vlanConfig.cpswVlanAwareMode = FALSE;
    aleConfig->vlanConfig.unknownUnregMcastFloodMask = 0U;
    aleConfig->vlanConfig.unknownRegMcastFloodMask = 0U;
    aleConfig->vlanConfig.unknownVlanMemberListMask = CPSW_ALE_ALL_PORTS_MASK;
    aleConfig->vlanConfig.autoLearnWithVLAN = TRUE;

    aleConfig->policerGlobalConfig.policingEnable = true;
    aleConfig->policerGlobalConfig.yellowDropEnable = false;
    aleConfig->policerGlobalConfig.redDropEnable = true;
    aleConfig->policerGlobalConfig.policerNoMatchMode = CPSW_ALE_POLICER_NOMATCH_MODE_GREEN;

    aleConfig->portCfg[0].learningCfg.noLearn = FALSE;
    aleConfig->portCfg[0].vlanCfg.dropUntagged = FALSE;

    aleConfig->portCfg[1].learningCfg.noLearn = FALSE;
    aleConfig->portCfg[1].vlanCfg.dropUntagged = FALSE;
}

static void CpswApp_initLinkArgs(Cpsw_OpenPortLinkInArgs *linkArgs,
                                 Cpsw_MacPort macPort)
{
    CpswMacPort_Config     *macConfig = &linkArgs->macConfig;
    CpswMacPort_LinkConfig *linkConfig = &linkArgs->linkConfig;
    CpswMacPort_Interface  *interface = &linkArgs->interface;
    CpswPhy_Config         *phyConfig = &linkArgs->phyConfig;

    linkArgs->portNum = macPort;

    Cpsw_initMacPortParams(macConfig);

    CpswAppBoardUtils_setPhyConfig(gCpswMainAppObj.cpswType,
                                   macPort,
                                   interface,
                                   phyConfig);

    if (phyConfig->phyAddr == CPSW_PHY_INVALID_PHYADDR)
    {
        linkConfig->speed     = CPSW_SPEED_1GBIT;
        linkConfig->duplexity = CPSW_DUPLEX_FULL;
    }
    else
    {
        linkConfig->speed     = CPSW_SPEED_AUTO;
        linkConfig->duplexity = CPSW_DUPLEX_AUTO;
    }
}

static int32_t CpswApp_init(void)
{
    CpswMcm_InitConfig cpswMcmCfg;
    Cpsw_Config cpswCfg;
    int32_t status = CPSW_SOK;

    CpswAppUtils_assert(gCpswMainAppObj.numMacPorts <= CPSW_MAC_PORT_NUM);

    /* Set configuration parameters */
    Cpsw_initParams(&cpswCfg);
    cpswCfg.vlanConfig.vlanAware          = false;
    cpswCfg.hostPortConfig.removeCrc      = true;
    cpswCfg.hostPortConfig.padShortPacket = true;
    cpswCfg.hostPortConfig.passCrcErrors  = true;
    CpswApp_setAleConfig(&cpswCfg.aleConfig);

    cpswCfg.dmaConfig.rxChInitPrms.dmaPriority = UDMA_DEFAULT_RX_CH_DMA_PRIORITY;

    /* Open UDMA */
    gCpswMainAppObj.hUdmaDrv = CpswAppUtils_udmaOpen(gCpswMainAppObj.cpswType,
                                                     CpswApp_getNavSSInstanceId(gCpswMainAppObj.cpswType));

    cpswMcmCfg.pCpswCfg = &cpswCfg;
    cpswMcmCfg.cpswType = gCpswMainAppObj.cpswType;
    cpswMcmCfg.hUdmaDrv = gCpswMainAppObj.hUdmaDrv;
    cpswMcmCfg.setMacConfig = CpswApp_initLinkArgs;
    cpswMcmCfg.numMacPorts = gCpswMainAppObj.numMacPorts;

    memcpy(&cpswMcmCfg.macPortList[0U],
           gCpswMainAppObj.macPorts,
           gCpswMainAppObj.numMacPorts);

    /* First MAC port in the array gives the host address */
    status = CpswSoc_getMacAddr(gCpswMainAppObj.cpswType,
                                gCpswMainAppObj.macPorts[0],
                                &cpswCfg.aleConfig.macAddr[0][0]);

    memcpy(&gCpswMainAppObj.hostMacAddr[0U],
           &cpswCfg.aleConfig.macAddr[0][0],
           ETH_MAC_ADDR_LEN);

    CpswAppUtils_print("Host MAC address: ");
    CpswAppUtils_printMacAddr(&gCpswMainAppObj.hostMacAddr[0U]);

    gCpswMainAppObj.hMcm[gCpswMainAppObj.cpswType] = CpswMcm_init(&cpswMcmCfg);

    return status;
}

void CpswApp_deInit(void)
{
    while (Task_getMode(task) != Task_Mode_TERMINATED)
    {
        Task_sleep(10);
    }

    if (Task_deleteTerminatedTasks == FALSE)
    {
        Task_delete(&task);
    }

    CpswAppUtils_udmaclose(gCpswMainAppObj.hUdmaDrv);

    memset(&gCpswMainAppObj, 0U, sizeof(CpswMain_AppObj));
}

void CpswAppIf_getHandles(CpswAppIf_HandleInfo *handleInfo)
{
    CpswMcm_HandleInfo McmHandleInfo;
    int32_t status;

    if (gCpswMainAppObj.hMcm[gCpswMainAppObj.cpswType] == NULL)
    {
        status = CpswApp_init();
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("CpswAppIf_getHandles() failed to init app: %d", status);
            CpswAppUtils_assert(status == CPSW_SOK);
        }
    }

    CpswMcm_getHandle(gCpswMainAppObj.hMcm[gCpswMainAppObj.cpswType],
                      &McmHandleInfo);

    handleInfo->hCpsw = McmHandleInfo.hCpsw;
    handleInfo->hUdmaDrv = McmHandleInfo.hUdmaDrv;
    handleInfo->cpswType = gCpswMainAppObj.cpswType;
}

void CpswAppIf_releaseHandles(Cpsw_Type cpswType)
{
    CpswMcm_releaseHandle(gCpswMainAppObj.hMcm[cpswType]);
}

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
    snprintf(ipAddr, 17, "%d.%d.%d.%d\n",
             (uint8_t)(ipAddrHex>>24)&0xFF,
             (uint8_t)(ipAddrHex>>16)&0xFF,
             (uint8_t)(ipAddrHex>>8)&0xFF,
             (uint8_t)ipAddrHex&0xFF);

    CpswAppUtils_print("\nCPSW NIMU application, IP address I/F 1: %s\n\r", ipAddr);
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
