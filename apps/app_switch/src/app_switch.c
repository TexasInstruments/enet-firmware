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

#include <ti/drv/uart/UART_stdio.h>
#include <ti/csl/csl_cpswitch.h>

#include <ti/drv/udma/udma.h>
#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_memutils_cfg.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_memutils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpswapp_ethutils.h>

#include <ti/osal/osal.h>

#if !defined (QT_BUILD)
#include <ti/board/board.h>
#endif

#include "app_switch.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define TEST_LEN                    (500U)

#define TEST_NUM_LOOP               (1U)
#define TEST_PTK_NUM                (1U)
#define ENABLE_PRINTFRAME

#define CPSW_TYPE (CPSW_9G)

/* ========================================================================== */
/*                         Structures and Enums                               */
/* ========================================================================== */

typedef struct
{
    /* CPSW driver handle */
    Cpsw_Handle          hCpsw;
    Udma_DrvHandle       hUdmaDrv;

    CpswDma_RxFlowHandle hRxFlow;
    uint32_t             rxFlowIdx;
    CpswDma_PktInfoQ     rxFreeQ;
    CpswDma_PktInfoQ     rxReadyQ;

    CpswDma_TxChHandle   hTxCh;
    CpswDma_PktInfoQ     txFreePktInfoQ;

    /* TX Eth packet memory */
    uint8_t              txBufMem[CPSW_MEMUTILS_NUM_TX_PKTS][CPSWAPPUTILS_ALIGN(ETH_MAX_FRAME_LEN)]
                                __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
    /* TX DMA packet info memory */
    CpswDma_PktInfo      txFreePktInfo[CPSW_MEMUTILS_NUM_TX_PKTS];

    uint8_t              hostMacAddr[ETH_MAC_ADDR_LEN];

} CpswApp_Obj;


typedef struct 
{
    uint32_t portNum;
	uint8_t macAddr[ETH_MAC_ADDR_LEN];

} portMac_Obj;

/* ========================================================================== */
/*                 Internal Function Declarations                             */
/* ========================================================================== */

int32_t  CpswApp_openCpsw(void);
void     CpswApp_closeCpsw(void);
#if !defined (SIMULATOR)
void     CpswApp_showStats(void);
#endif
int32_t  CpswApp_openDma(void);
void     CpswApp_closeDma(void);
int32_t  CpswApp_pktRxTx(void);
uint32_t CpswApp_retrieveFreeTxPkts(void);
uint32_t CpswApp_receivePkts(void);
void     CpswApp_setRxflowPrms(CpswDma_OpenRxFlowPrms *pRxFlowPrms);
void     CpswApp_changeHostAleEntry(uint8_t macAddr[]);
void     CpswApp_addAleEntry(uint8_t macAddr[], uint32_t portNum);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
/* Broadcast address */
uint8_t                bcastAddr[ETH_MAC_ADDR_LEN] =
{
    0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU
};


/* CPSW configuration */
static CpswDma_OpenTxChPrms   cpswTxChCfg;
static CpswDma_OpenRxFlowPrms cpswRxFlowCfg;
static CpswApp_Obj gCpswLpbkAppObj;
static portMac_Obj J7PortMac_Obj[8];

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int32_t CpswApp_loopbackTest(CpswApp_ClkHandle clkhandle, uint32_t iteration)
{
    int32_t       status;

#if !defined (QT_BUILD)
    Board_initCfg boardCfg;

    boardCfg = BOARD_INIT_PINMUX_CONFIG |
               BOARD_INIT_MODULE_CLOCK |
               BOARD_INIT_UART_STDIO;
    Board_init(boardCfg);
#endif

	CpswAppUtils_setMacMode(CPSW_TYPE, MAC_CONN_TYPE_RGMII_FORCE_1000_FULL);
	
    CpswAppUtils_print("=================================\n");
    CpswAppUtils_print ("Cpsw Loopback app: Iteration %d\n", iteration);
    CpswAppUtils_print("=================================\n");

    /* Open the CPSW */
    status = CpswApp_openCpsw();
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("Failed to open CPSW: %d\n", status);
    }

    /* Open legacy DMA driver */
    if (status == CPSW_SOK)
    {
        status = CpswApp_openDma();
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Failed to open legacy DMA: %d\n", status);
        }
    }

    /* Enable host port */
    if (status == CPSW_SOK)
    {
        Cpsw_IoctlPrms prms;
        CpswAle_SetPortStateInArgs setPortStateInArgs;

        setPortStateInArgs.portNum   = CPSW_ALE_HOST_PORT_NUM;
        setPortStateInArgs.portState = CPSW_ALE_PORTSTATE_FORWARD;
        CPSW_IOCTL_SET_IN_ARGS(&prms, &setPortStateInArgs);
        prms.outArgs = NULL;
        status       = Cpsw_ioctl(gCpswLpbkAppObj.hCpsw, CPSW_ALE_IOCTL_SET_PORT_STATE,
                                  &prms);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print(
                "CpswApp_openCpsw() failed CPSW_ALE_IOCTL_SET_PORT_STATE: %d\n", status);
        }

        if (status == CPSW_SOK)
        {
            status = Cpsw_ioctl(gCpswLpbkAppObj.hCpsw, CPSW_HOSTPORT_IOCTL_ENABLE, NULL);
            if (status != CPSW_SOK)
            {
                CpswAppUtils_print("Failed to enable host port: %d\n", status);
            }
        }
    }


    /* Start timer */
    CpswApp_startClock(clkhandle);

    if (status == CPSW_SOK)
    {
        status = CpswApp_pktRxTx();
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Failed to enable host port: %d\n", status);
        }
    }

#if !defined (SIMULATOR)
    /* Print CPSW statistics of all ports */
    if (status == CPSW_SOK)
    {
        CpswApp_showStats();
    }
#endif

    /* Disable host port */
    if (status == CPSW_SOK)
    {
        status = Cpsw_ioctl(gCpswLpbkAppObj.hCpsw, CPSW_HOSTPORT_IOCTL_DISABLE, NULL);
        if (status != CPSW_SOK)
        {
            CpswAppUtils_print("Failed to disable host port: %d\n", status);
        }
    }

    CpswApp_stopClock(clkhandle);

    if (status == CPSW_SOK)
    {
        /* Close legacy DMA driver */
        CpswApp_closeDma();
        /* Close CPSW */
        CpswApp_closeCpsw();
        /* Close UDMA driver */
        CpswAppUtils_udmaclose(gCpswLpbkAppObj.hUdmaDrv);

        CpswAppUtils_print("Loopback application completed\n");
    }
    else
    {
        CpswAppUtils_print("Loopback application failed to complete\n");
    }

    return 0;
}

static uint64_t CpswApp_virtToPhyFxn(const void *virtAddr,
                                     void       *appData)
{
    return ((uint64_t) virtAddr);
}

static void *CpswApp_phyToVirtFxn(uint64_t phyAddr,
                                  void    *appData)
{
#if defined (__aarch64__)
    uint64_t temp = phyAddr;
#else
    /* R5 is 32-bit machine, need to truncate to avoid void * typecast error */
    uint32_t temp = (uint32_t) phyAddr;
#endif
    return ((void *) temp);
}

void CpswApp_initAleConfig(CpswAle_Config *aleConfig)
{
    int32_t status;

    aleConfig->modeFlags =
        (CPSW_ALE_CONFIG_MASK_ALE_MODULE_ENABLE);
    aleConfig->agingConfig.enableAutoAging = TRUE;
    aleConfig->agingConfig.agingPeriodInMs = 1000;
    status = CpswSoc_getMacAddr(CPSW_TYPE, CPSW_MAC_PORT_0,
                                &aleConfig->macAddr[0][0]);

    CpswAppUtils_assert(status == CPSW_SOK);

    aleConfig->nwSecCfg.enableVid0Mode = TRUE;
    aleConfig->vlanConfig.aleVlanAwareMode = FALSE;
    aleConfig->vlanConfig.cpswVlanAwareMode = FALSE;
    aleConfig->vlanConfig.unknownUnregMcastFloodMask = CPSW_ALE_ALL_PORTS_MASK;
    aleConfig->vlanConfig.unknownRegMcastFloodMask = CPSW_ALE_ALL_PORTS_MASK;
    aleConfig->vlanConfig.unknownVlanMemberListMask = CPSW_ALE_ALL_PORTS_MASK;
}

uint32_t CpswApp_getNavSSInstanceId (Cpsw_Type cpswInstance)
{
    return (CPSW_2G == CPSW_TYPE) ? UDMA_INST_ID_MCU_0 :
                                    UDMA_INST_ID_MAIN_0;
}

int32_t CpswApp_openCpsw(void)
{
    Cpsw_Config    cpswCfg;
    CpswOsal_Prms  osalPrms;
    CpswUtils_Prms utilsPrms;
    Cpsw_IoctlPrms prms;
    int32_t        status = CPSW_SOK;
	Cpsw_MacPort i;

    /* Set configuration parameters */
    Cpsw_initParams(&cpswCfg);
    cpswCfg.vlanConfig.vlanAware          = false;
    cpswCfg.hostPortConfig.removeCrc      = true;
    cpswCfg.hostPortConfig.padShortPacket = true;
    cpswCfg.hostPortConfig.passCrcErrors  = true;
    CpswApp_initAleConfig(&cpswCfg.aleConfig);


    if (CPSW_9G == CPSW_TYPE)
    {
        CpswAppUtils_print("CPSW_9G Test on MAIN NAVSS\n");
    }
    else if (CPSW_2G == CPSW_TYPE)
    {
        CpswAppUtils_print("CPSW_2G Test on MCU NAVSS\n");
    }

    cpswCfg.dmaConfig.rxChInitPrms.dmaPriority = UDMA_DEFAULT_RX_CH_DMA_PRIORITY;

    /* Application should open UDMA first as CPSW DMA needs handle to opened UDMA
     * to initialize CPSW RX channel */
    Udma_DrvHandle hUdmaDrv = CpswAppUtils_udmaOpen(CPSW_TYPE,
                                                    CpswApp_getNavSSInstanceId(CPSW_TYPE));
    if (NULL == hUdmaDrv)
    {
        CpswAppUtils_print("UDMA_open failed, asserting.. \n");
        CpswAppUtils_assert(NULL == hUdmaDrv);
    }
    else
    {
        cpswCfg.dmaConfig.hUdmaDrv = hUdmaDrv;
        gCpswLpbkAppObj.hUdmaDrv = hUdmaDrv;
    }

    status = CpswSoc_getMacAddr(CPSW_TYPE, CPSW_MAC_PORT_0,
                             &cpswCfg.aleConfig.macAddr[0][0]);

    CpswAppUtils_assert(status == CPSW_SOK);

    memcpy (&gCpswLpbkAppObj.hostMacAddr[0U], &cpswCfg.aleConfig.macAddr[0][0],
                ETH_MAC_ADDR_LEN);
    CpswAppUtils_print("Host MAC address: ");
    CpswAppUtils_printMacAddr(&cpswCfg.aleConfig.macAddr[0][0]);

    /* Initialize CPSW driver with default OSAL, utils */
    utilsPrms.printFxn     = UART_printf;
    utilsPrms.phyToVirtFxn = &CpswApp_phyToVirtFxn;
    utilsPrms.virtToPhyFxn = &CpswApp_virtToPhyFxn;

    Cpsw_initOsalPrms(&osalPrms);

    Cpsw_init(CPSW_TYPE, &osalPrms, &utilsPrms);

    /* Open the CPSW driver */
    gCpswLpbkAppObj.hCpsw = Cpsw_open(CPSW_TYPE, &cpswCfg);
    if (gCpswLpbkAppObj.hCpsw == NULL)
    {
        CpswAppUtils_print("CpswApp_openCpsw() failed to open: %d\n", status);
        status = CPSW_EFAIL;
    }

    /* memutils open should happen after Cpsw is opened as it uses CpswUtils_Q
     * functions */
    CpswMemUtils_init();

    if (status == CPSW_SOK)
	{
		for (i=CPSW_MAC_PORT_0; i < CPSW_MAC_PORT_NUM; i++)
		{
			Cpsw_OpenPortLinkInArgs linkArgs = {
				.portNum =  i,
			};
			
			CpswMacPort_Config     *macConfig = &linkArgs.macConfig;
			CpswMacPort_LinkConfig *linkConfig = &linkArgs.linkConfig;
			CpswPhy_Config         *phyConfig = &linkArgs.phyConfig;

			Cpsw_initMacPortParams(macConfig);

			CpswMacPort_Interface  *interface = &linkArgs.interface;

			interface->layerType    = CPSW_MAC_LAYER_GMII;
			interface->sublayerType = CPSW_MAC_SUBLAYER_REDUCED;

	#if defined (SIMULATOR)
			/*J721e VLAB model doesn't support mac loopback */
			macConfig->enableLoopback = false;

			/* Set phyAddr as CPSW_PHY_INVALID_PHYADDR to indicate NOPHY mode */
			phyConfig->phyAddr      = CPSW_PHY_INVALID_PHYADDR;
			interface->variantType  = CPSW_MAC_VARIANT_FORCED;
			linkConfig->speed       = CPSW_SPEED_1GBIT;
			linkConfig->duplexity   = CPSW_DUPLEX_FULL;
	#else
			macConfig->enableLoopback = true;

			interface->variantType  = CPSW_MAC_VARIANT_NONE;
			linkConfig->speed       = CPSW_SPEED_AUTO;
			linkConfig->duplexity   = CPSW_DUPLEX_AUTO;

			phyConfig->phyAddr               = 0U;
			phyConfig->mdixEnable            = true;
			phyConfig->resetEnable           = true;
			phyConfig->linkIntEnable         = false;
			phyConfig->isLoopback            = false;
			phyConfig->txClkShiftEnable      = false;
			phyConfig->rxClkShiftEnable      = false;
			phyConfig->masterMode            = false;
			phyConfig->extClkSource          = 0U; /* TODO: Unused */
			phyConfig->txDelay               = 0U;
			phyConfig->rxDelay               = 0U;
			phyConfig->txFifoDepth           = 0U;
			phyConfig->rxFifoDepth           = 0U;
			phyConfig->CpswPhyLedModeType[0] = CPSWPHY_LED_LINKOK;
			phyConfig->CpswPhyLedModeType[1] = CPSWPHY_LED_LINKOKTXACT;
			phyConfig->CpswPhyLedModeType[2] = CPSWPHY_LED_LINKOKRXACT;
	#endif

			if (macConfig->enableLoopback)
			{
				/* For mac loopback disable secure flag for the host port entry */
				CpswApp_changeHostAleEntry(&gCpswLpbkAppObj.hostMacAddr[0U]);
			}
			
			if (CPSW_9G == CPSW_TYPE)
			{	
				J7PortMac_Obj[i].portNum = i+1;
				memset(&J7PortMac_Obj[i].macAddr[0],0,sizeof(J7PortMac_Obj[i].macAddr));
				J7PortMac_Obj[i].macAddr[0] = (i+1) * 2;
				CpswAppUtils_print ("port: %d Mac address: ", i+1);
				CpswAppUtils_printMacAddr(&J7PortMac_Obj[i].macAddr[0]);
				CpswApp_addAleEntry (&J7PortMac_Obj[i].macAddr[0], J7PortMac_Obj[i].portNum);
				
			}

			CPSW_IOCTL_SET_IN_ARGS(&prms, &linkArgs);
			status = Cpsw_ioctl(gCpswLpbkAppObj.hCpsw, CPSW_IOCTL_OPEN_PORT_LINK, &prms);
			if (status != CPSW_SOK)
			{
				CpswAppUtils_print("CpswApp_openCpsw() failed to open MAC port: %d\n",
								   status);
			}
		}
	}

    return status;
}

void CpswApp_closeCpsw(void)
{
    Cpsw_IoctlPrms prms;
    Cpsw_GenericPortLinkInArgs args = {
        .portNum = CPSW_MAC_PORT_0,
    };
    int32_t status;

    CPSW_IOCTL_SET_IN_ARGS(&prms, &args);
    status = Cpsw_ioctl(gCpswLpbkAppObj.hCpsw, CPSW_IOCTL_CLOSE_PORT_LINK, &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print("close() failed to close MAC port: %d\n", status);
    }

    Cpsw_close(gCpswLpbkAppObj.hCpsw);

    Cpsw_deinit(CPSW_TYPE);
}

void CpswApp_showStats(void)
{
    Cpsw_IoctlPrms prms;
    CpswStats_GenericMacPortInArgs inArgs;
    CpswStats_PortStats portStats;
    int32_t status = CPSW_SOK;

    CPSW_IOCTL_SET_OUT_ARGS(&prms, &portStats);
    status = Cpsw_ioctl(gCpswLpbkAppObj.hCpsw, CPSW_STATS_IOCTL_GET_HOSTPORT_STATS,
                        &prms);
    if (status == CPSW_SOK)
    {
        CpswAppUtils_print("\n Port 0 Statistics\n");
        CpswAppUtils_print("-----------------------------------------\n");
        CpswAppUtils_printHostPortStats((CpswStats_HostPort_2g *)&portStats);
        CpswAppUtils_print("\n");
    }
    else
    {
        CpswAppUtils_print("CpswApp_showStats() failed to get host stats: %d\n",
                           status);
    }

    if (status == CPSW_SOK)
    {
        inArgs.portNum    = CPSW_MAC_PORT_0;
        CPSW_IOCTL_SET_INOUT_ARGS(&prms, &inArgs, &portStats);
        status = Cpsw_ioctl(gCpswLpbkAppObj.hCpsw, CPSW_STATS_IOCTL_GET_MACPORT_STATS,
                            &prms);
        if (status == CPSW_SOK)
        {
            CpswAppUtils_print("\n Port 1 Statistics\n");
            CpswAppUtils_print("-----------------------------------------\n");
            CpswAppUtils_printMacPortStats((CpswStats_MacPort_2g *)&portStats);
            CpswAppUtils_print("\n");
        }
        else
        {
            CpswAppUtils_print("CpswApp_showStats() failed to get MAC stats: %d\n",
                               status);
        }
    }
}

void CpswApp_rxIsrFxn(Udma_EventHandle eventHandle,
              uint32_t         eventType,
              void            *appData)
{}

void CpswApp_txIsrFxn(Udma_EventHandle eventHandle,
              uint32_t         eventType,
              void            *appData)
{}

void CpswApp_initTxFreePktQ(void)
{
    CpswDma_PktInfo *pktInfo;
    uint32_t         i;

    /* Initialize all queues */
    CpswUtils_initQ(&gCpswLpbkAppObj.txFreePktInfoQ);

    /* Initialize TX EthPkts and queue them to txFreePktInfoQ */
    for (i = 0U; i < CPSWAPPUTILS_ARRAY_SIZE(gCpswLpbkAppObj.txFreePktInfo); i++)
    {
        pktInfo = &gCpswLpbkAppObj.txFreePktInfo[i];

        CpswDma_pktInfoInit(pktInfo);

        memset (&pktInfo->node, 0U, sizeof(pktInfo->node));
        pktInfo->bufPtr     = (uint8_t *) &gCpswLpbkAppObj.txBufMem[i][0U];
        pktInfo->orgBufLen  = ETH_MAX_FRAME_LEN;
        pktInfo->userBufLen = ETH_MAX_FRAME_LEN;
        pktInfo->appPriv    = NULL;

        CpswUtils_enQ(&gCpswLpbkAppObj.txFreePktInfoQ, &pktInfo->node);
    }

    CpswAppUtils_print("initQs() txFreePktInfoQ initialized with %d pkts\n",
                       CpswUtils_getQCount(&gCpswLpbkAppObj.txFreePktInfoQ));
}

void CpswApp_setTxChPrms(CpswDma_OpenTxChPrms *pTxChPrms)
{
    pTxChPrms->cpswInstance = CPSW_TYPE;

    /* TODO this should be taken from CPSW RM */
    pTxChPrms->chNum               = CPSW_DMA_TX_CH_NUM(0);

    pTxChPrms->ringMemAllocFxn     = &CpswMemUtils_allocRingMemFxn;
    pTxChPrms->ringMemFreeFxn      = &CpswMemUtils_freeRingMemFxn;

    pTxChPrms->numTxPkts           = CPSW_MEMUTILS_NUM_TX_PKTS;
    pTxChPrms->disableCacheOpsFlag = false;

    pTxChPrms->dmaDescAllocFxn     = &CpswMemUtils_allocDmaDescFxn;
    pTxChPrms->dmaDescFreeFxn      = &CpswMemUtils_freeDmaDescFxn;

    pTxChPrms->udmaEvtCfg.eventCb     = &CpswApp_txIsrFxn;
    pTxChPrms->udmaEvtCfg.hEventCbArg = &gCpswLpbkAppObj.hTxCh;

    pTxChPrms->hCallbackArg        = &gCpswLpbkAppObj.hTxCh;
}

void CpswApp_setRxflowPrms(CpswDma_OpenRxFlowPrms *pRxFlowPrms)
{
    pRxFlowPrms->ringMemAllocFxn        = &CpswMemUtils_allocRingMemFxn;
    pRxFlowPrms->ringMemFreeFxn         = &CpswMemUtils_freeRingMemFxn;

    pRxFlowPrms->udmaEvtCfg.eventCb     = &CpswApp_rxIsrFxn;
    pRxFlowPrms->udmaEvtCfg.hEventCbArg = &gCpswLpbkAppObj.hRxFlow;

    pRxFlowPrms->numRxPkts              = CPSW_MEMUTILS_NUM_RX_PKTS;
    pRxFlowPrms->maxPktLength           = CPSW_MEMUTILS_LARGE_POOL_PKT_SIZE;

    pRxFlowPrms->disableCacheOpsFlag    = false;
    pRxFlowPrms->dmaDescAllocFxn        = &CpswMemUtils_allocDmaDescFxn;
    pRxFlowPrms->dmaDescFreeFxn         = &CpswMemUtils_freeDmaDescFxn;

    pRxFlowPrms->hCallbackArg           = &gCpswLpbkAppObj.hRxFlow;

}

void CpswApp_initRxReadyPktQ()
{
    CpswDma_PktInfoQ rxReadyQ;
    int32_t          status;
    uint32_t         i;
    CpswDma_PktInfo *pPktInfo;

    CpswUtils_initQ(&gCpswLpbkAppObj.rxFreeQ);
    CpswUtils_initQ(&rxReadyQ);

    for (i=0U; i<CPSW_MEMUTILS_NUM_RX_PKTS; i++)
    {
        pPktInfo = CpswMemUtils_allocEthPktFxn(&gCpswLpbkAppObj,
                                          CPSW_MEMUTILS_LARGE_POOL_PKT_SIZE);
        CpswAppUtils_assert(pPktInfo != NULL);
        CpswUtils_enQ(&gCpswLpbkAppObj.rxFreeQ, &pPktInfo->node);
    }

    /* Retrieve any CPSW packets which are ready */
    status = CpswDma_retrieveRxPackets(gCpswLpbkAppObj.hRxFlow, &rxReadyQ);
    CpswAppUtils_assert ( status == CPSW_SOK );
    /* There should not be any packet with DMA during init */
    CpswAppUtils_assert ( CpswUtils_getQCount(&rxReadyQ) == 0U );

    CpswAppUtils_submitRxPackets(gCpswLpbkAppObj.hRxFlow,
                                &gCpswLpbkAppObj.rxFreeQ);
    /* Assert here as during init no. of DMA descriptors should be equal to
     * no. of free Ethernet buffers available with app */

    CpswAppUtils_assert( 0U == CpswUtils_getQCount(&gCpswLpbkAppObj.rxFreeQ));

}

int32_t CpswApp_openDma(void)
{
    int32_t status = CPSW_SOK;

    /* Open the CPSW TX channel  */
    if (status == CPSW_SOK)
    {
        CpswApp_initTxFreePktQ();

        /* Set configuration parameters */
        CpswDma_initTxChParams(&cpswTxChCfg);
        CpswApp_setTxChPrms(&cpswTxChCfg);
        gCpswLpbkAppObj.hTxCh = CpswDma_openTxCh(&cpswTxChCfg);
        if (NULL != gCpswLpbkAppObj.hTxCh)
        {
            status = CpswDma_enableTxEvent(gCpswLpbkAppObj.hTxCh);
            if (CPSW_SOK != status)
            {
                CpswAppUtils_print("CpswDma_startTxCh() failed: %d\n", status);
                status = CPSW_EFAIL;
            }
        }
        else
        {
            CpswAppUtils_print("CpswDma_openTxCh() failed to open: %d\n",
                               status);
            status = CPSW_EFAIL;
        }
    }

    /* Open the CPSW RX flow  */
    if (status == CPSW_SOK)
    {
        CpswDma_initRxFlowParams(&cpswRxFlowCfg);

        CpswApp_setRxflowPrms(&cpswRxFlowCfg);

        cpswRxFlowCfg.cpswInstance = CPSW_TYPE;
        cpswRxFlowCfg.flowIdx = Cpsw_rmAllocFlowIndex(gCpswLpbkAppObj.hCpsw);

        gCpswLpbkAppObj.hRxFlow = CpswDma_openRxFlow (&cpswRxFlowCfg);
        if (NULL == gCpswLpbkAppObj.hRxFlow)
        {
            CpswAppUtils_print("CpswDma_openRxFlow() failed to open: %d\n",
                               status);
            Cpsw_rmFreeFlowIndex(gCpswLpbkAppObj.hCpsw,
                                 cpswRxFlowCfg.flowIdx);
            CpswAppUtils_assert (NULL != gCpswLpbkAppObj.hRxFlow);
        }
        else
        {
            /* save flow idx */
            gCpswLpbkAppObj.rxFlowIdx = cpswRxFlowCfg.flowIdx;
            /* Submit all ready RX buffers to DMA.*/
            CpswApp_initRxReadyPktQ();
        }

    }

    return status;
}

void CpswApp_closeDma(void)
{
    CpswDma_PktInfoQ rxReadyQ;
    CpswDma_PktInfo *pktInfo;
    int32_t          status;
    uint32_t         rxReadyCnt, i;

    /* Retrieve any CPSW packets which are ready */
    status = CpswDma_retrieveRxPackets(gCpswLpbkAppObj.hRxFlow, &rxReadyQ);
    CpswAppUtils_assert ( status == CPSW_SOK );

    rxReadyCnt = CpswUtils_getQCount(&rxReadyQ) ;

    /* Free all retrieved packets from DMA */
    /* TODO - How do we free packets in teardown ring? */
    for (i=0U; i<rxReadyCnt; i++)
    {
        pktInfo = (CpswDma_PktInfo *)CpswUtils_deQ(&rxReadyQ);
        CpswMemUtils_freeEthPktFxn(pktInfo);
    }

    /* Close RX channel */
    status = CpswDma_closeRxFlow(gCpswLpbkAppObj.hRxFlow);
    Cpsw_rmFreeFlowIndex(gCpswLpbkAppObj.hCpsw, gCpswLpbkAppObj.rxFlowIdx);

    /* Close TX channel */
    status += CpswDma_disableTxEvent(gCpswLpbkAppObj.hTxCh);
    status += CpswDma_closeTxCh(gCpswLpbkAppObj.hTxCh);

    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
            "CpswApp_closeDma() failed: %d\n", status);
    }

    CpswMemUtils_deInit();
}

void CpswApp_addAleEntry(uint8_t macAddr[], uint32_t portNum)
{
    int32_t          status;
    Cpsw_IoctlPrms prms;
    CpswAle_AddEntryOutArgs setUcastOutArgs;
    CpswAle_SetUnicastEntryInArgs setUcastInArgs;

    memcpy (&setUcastInArgs.addr.addr[0U], macAddr, sizeof (setUcastInArgs.addr.addr));
    setUcastInArgs.addr.vlanId = 0U;
    setUcastInArgs.info.portNum = portNum;
    setUcastInArgs.info.blocked = false;
    setUcastInArgs.info.secure = false;
    setUcastInArgs.info.super = 0U;
    setUcastInArgs.info.ageable = false;


    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setUcastInArgs, &setUcastOutArgs);

    status       = Cpsw_ioctl(gCpswLpbkAppObj.hCpsw, CPSW_ALE_IOCTL_ADD_UNICAST,
                              &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
            "CpswApp_openCpsw() failed CPSW_ALE_IOCTL_SET_PORT_STATE: %d\n", status);
    }
}


void CpswApp_changeHostAleEntry(uint8_t macAddr[])
{
    int32_t          status;
    Cpsw_IoctlPrms prms;
    CpswAle_AddEntryOutArgs setUcastOutArgs;
    CpswAle_SetUnicastEntryInArgs setUcastInArgs;

    memcpy (&setUcastInArgs.addr.addr[0U], macAddr, sizeof (setUcastInArgs.addr.addr));
    setUcastInArgs.addr.vlanId = 0U;
    setUcastInArgs.info.portNum = 0U;
    setUcastInArgs.info.blocked = false;
    setUcastInArgs.info.secure = false;
    setUcastInArgs.info.super = 0U;
    setUcastInArgs.info.ageable = false;


    CPSW_IOCTL_SET_INOUT_ARGS(&prms, &setUcastInArgs, &setUcastOutArgs);

    status       = Cpsw_ioctl(gCpswLpbkAppObj.hCpsw, CPSW_ALE_IOCTL_ADD_UNICAST,
                              &prms);
    if (status != CPSW_SOK)
    {
        CpswAppUtils_print(
            "CpswApp_openCpsw() failed CPSW_ALE_IOCTL_SET_PORT_STATE: %d\n", status);
    }
}

int32_t CpswApp_pktRxTx(void)
{
    int32_t          status = CPSW_SOK;
    uint32_t         loopCntr;
    CpswDma_PktInfoQ txSubmitQ;
    CpswDma_PktInfo *pktInfo;
    EthFrame        *frame;
    uint32_t         txRetrievePktCnt = 0U;
    uint32_t         rxReadyCnt;

	/* Transmit a single packet */
	CpswUtils_initQ(&txSubmitQ);
	
	/* send one packet from host to all external ports */

	pktInfo = (CpswDma_PktInfo *) CpswUtils_deQ(&gCpswLpbkAppObj.txFreePktInfoQ);
	if ( NULL != pktInfo )
	{
		/* Fill the TX Eth frame with test content */
		frame = (EthFrame *) pktInfo->bufPtr;
		memcpy(frame->hdr.dstMac, bcastAddr, ETH_MAC_ADDR_LEN);
		memcpy(frame->hdr.srcMac, &gCpswLpbkAppObj.hostMacAddr[0U], ETH_MAC_ADDR_LEN);
		frame->hdr.etherType = htons(ETHERTYPE_EXPERIMENTAL1);
		memset(&frame->payload[0U], (uint8_t) (0xA5), TEST_LEN);
		pktInfo->userBufLen = TEST_LEN + sizeof (EthFrameHeader);
		pktInfo->appPriv    = &gCpswLpbkAppObj;

		/* Enqueue the packet for later transmission */
		CpswUtils_enQ(&txSubmitQ, &pktInfo->node);

		status = CpswAppUtils_submitTxPackets(gCpswLpbkAppObj.hTxCh,
											  &txSubmitQ);
		/* Retrieve TX free packets */
		if (status == CPSW_SOK)
		{
			while (txRetrievePktCnt != 1U)
			{
				//TODO this is not failure as HW is busy sending packets, we
				// need to wait and again call retrieve packets
				CpswAppUtils_wait(1);
				txRetrievePktCnt += CpswApp_retrieveFreeTxPkts();
				#if DEBUG
				CpswAppUtils_print( "Failed to retrieve consumed transmit packets: %d\n",
					status);
				#endif
			}
		}
		else
		{
			CpswAppUtils_print (" Error!- submit Tx packet failed");
		}
	}
	else
	{
		CpswAppUtils_print (" Error!- No free TX pkt info!");
	}
	
	loopCntr = 1U;
    while (loopCntr)
    {
		/* Get the packets received so far */
		rxReadyCnt = CpswApp_receivePkts();
		if (rxReadyCnt > 0U)
		{
			/* Consume the received packets and release them */
			pktInfo = (CpswDma_PktInfo *) CpswUtils_deQ(
				&gCpswLpbkAppObj.rxReadyQ);
			while (NULL != pktInfo)
			{
				/* Consume the packet by just printing its content */
				frame = (EthFrame *) pktInfo->bufPtr;
#ifdef ENABLE_PRINTFRAME
				CpswAppUtils_printFrame
					( frame, (pktInfo->userBufLen - sizeof (EthFrameHeader)) );
#endif
				/* Release the received packet */
				CpswUtils_enQ(&gCpswLpbkAppObj.rxFreeQ, &pktInfo->node);
				pktInfo = (CpswDma_PktInfo *) CpswUtils_deQ(
					&gCpswLpbkAppObj.rxReadyQ);
			}

			/*Submit now processed buffers */
			if (status == CPSW_SOK)
			{
				CpswAppUtils_submitRxPackets(gCpswLpbkAppObj.hRxFlow,
											 &gCpswLpbkAppObj.rxFreeQ);
			}

			CpswAppUtils_print("Received %d packets\n", rxReadyCnt);
		}

    }

	return status;

}

uint32_t CpswApp_retrieveFreeTxPkts(void)
{
    CpswDma_PktInfoQ txFreeQ;
    CpswDma_PktInfo *pktInfo;
    int32_t          status;
    uint32_t         txFreeQCnt = 0U;

    CpswUtils_initQ(&txFreeQ);

    /* Retrieve any CPSW packets that may be free now */
    status = CpswDma_retrieveTxDonePackets(gCpswLpbkAppObj.hTxCh, &txFreeQ);
    if (status == CPSW_SOK)
    {
        txFreeQCnt = CpswUtils_getQCount(&txFreeQ);

        pktInfo = (CpswDma_PktInfo *) CpswUtils_deQ(&txFreeQ);
        while (NULL != pktInfo)
        {
            CpswUtils_enQ(&gCpswLpbkAppObj.txFreePktInfoQ, &pktInfo->node);

            pktInfo = (CpswDma_PktInfo *) CpswUtils_deQ(&txFreeQ);
        }
    }
    else
    {
        CpswAppUtils_print("retrieveFreeTxPkts() failed to retrieve pkts: %d\n",
                           status);
    }

    return txFreeQCnt;
}

uint32_t CpswApp_receivePkts(void)
{
    CpswDma_PktInfoQ rxReadyQ;
    CpswDma_PktInfo *pktInfo;
    int32_t          status;
    uint32_t         rxReadyCnt = 0U;

    CpswUtils_initQ(&rxReadyQ);

    /* Retrieve any CPSW packets which are ready */
    status = CpswDma_retrieveRxPackets(gCpswLpbkAppObj.hRxFlow, &rxReadyQ);
    if (status == CPSW_SOK)
    {
        rxReadyCnt = CpswUtils_getQCount(&rxReadyQ);
        /* Queue the received packet to rxReadyQ and pass new ones from rxFreeQ
        **/
        pktInfo = (CpswDma_PktInfo *) CpswUtils_deQ(&rxReadyQ);
        while (pktInfo != NULL)
        {
            CpswUtils_enQ(&gCpswLpbkAppObj.rxReadyQ, &pktInfo->node);
            pktInfo = (CpswDma_PktInfo *) CpswUtils_deQ(&rxReadyQ);
        }
    }
    else
    {
        CpswAppUtils_print("receivePkts() failed to retrieve pkts: %d\n",
                           status);
    }

    return rxReadyCnt;
}

Cpsw_Handle CpswApp_getCpswHandle(void)
{
    return gCpswLpbkAppObj.hCpsw;
}

/* end of file */
