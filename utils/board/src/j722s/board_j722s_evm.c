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
 * \file  board_j722s_evm.c
 *
 * \brief This file contains the implementation of the J722S SK-EVM board
 *        configuration functions.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* EthFwTrace id for this module, must be unique within ETHFW */
#define ETHFWTRACE_MOD_ID 0x406

#include <stdint.h>
#include <stdarg.h>

#include <enet_apputils.h>
#include <enet_appsoc.h>
#include <dp83867.h>
#include <mod/cpsw_macport.h>
#include <utils/board/include/ethfw_board_utils.h>
#include <utils/ethfw_common/include/ethfw_trace.h>
#include <j722s_csl_serdes3.h>
#include <csl_serdes.h>
#include <cslr_cpsgmii.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* SGMII Register Base Addresses and Offsets */
#define SGMII_SOFT_RESET_REG_OFFSET(num)     ((volatile uint32_t *)(0x08000104u + (0x100u * (num))))
#define SGMII_CONTROL_REG_OFFSET(num)        ((volatile uint32_t *)(0x08000110u + (0x100u * (num))))
#define SGMII_STATUS_REG_OFFSET(num)         ((volatile uint32_t *)(0x08000114u + (0x100u * (num))))
#define SGMII_ADV_ABILITY_REG_OFFSET(num)    ((volatile uint32_t *)(0x08000118u + (0x100u * (num))))
#define SGMII_LP_ADV_ABILITY_REG_OFFSET(num) ((volatile uint32_t *)(0x08000120u + (0x100u * (num))))

/* SGMII Mode definitions */
#define CSL_SGMII_MODE_FIBER            (0U)
#define CSL_SGMII_MODE_SGMII            (1U)

/* SGMII Duplex Mode definitions */
#define CSL_SGMII_HALF_DUPLEX           (0U)
#define CSL_SGMII_FULL_DUPLEX           (1U)

/* SGMII Link Speed definitions */
#define CSL_SGMII_10_MBPS               (0U)
#define CSL_SGMII_100_MBPS              (1U)
#define CSL_SGMII_1000_MBPS             (2U)

/* SGMII Advertisement Ability structure */
typedef struct {
    uint32_t duplexMode;  /* 0: Half, 1: Full */
    uint32_t linkSpeed;   /* 0: 10Mbps, 1: 100Mbps, 2: 1000Mbps */
    uint32_t sgmiiMode;   /* 0: Fiber, 1: SGMII */
    uint32_t bLinkUp;     /* 1: Link up */
} EthFwBoard_SGMII_AdvAbility;

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* EthFw board configuration params object */
typedef struct EthFwBoard_Obj_s
{
    /* UART configuration allowed */
    bool uartAllowed;

    /* I2C configuration allowed */
    bool i2cAllowed;

    /* GPIO configuration allowed *///ToDo: check this needs to be set
    bool gpioAllowed;

    /* SerDes configuration allowed */
    bool serdesAllowed;

    /* ENET bridge board to be enabled or not */
    bool enetBridgeEnabled;
} EthFwBoard_Obj;

/*!
 * \brief Board related configuration parameters of an Ethernet PHY.
 */
typedef struct EthFwBoard_PhyCfg_s
{
    /*! PHY device address */
    uint32_t phyAddr;

    /*! Interface type */
    EnetPhy_Mii mii;

    /*! Whether PHY is strapped or not */
    bool isStrapped;

    /*! Whether to skip PHY-specific extended configuration */
    bool skipExtendedCfg;

    /*! Extended PHY-specific configuration */
    const void *extendedCfg;

    /*! Size of the extended configuration */
    uint32_t extendedCfgSize;
} EthFwBoard_PhyCfg;

/*!
 * \brief Ethernet port configuration parameters.
 */
typedef struct EthFwBoard_MacPortCfg_s
{
    /*! MAC port connected to */
    Enet_MacPort macPort;

    /*! MAC port interface */
    EnetMacPort_Interface mii;

    /*! PHY configuration parameters */
    EthFwBoard_PhyCfg phyCfg;

    /*! SGMII mode. Applicable only when port is used in Q/SGMII mode */
    EnetMac_SgmiiMode sgmiiMode;

    /*! Link configuration (speed and duplexity) */
    EnetMacPort_LinkCfg linkCfg;
} EthFwBoard_MacPortCfg;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static void EthFwBoard_enableMods(void);

static void EthFwBoard_configPinmux(void);

static void EthFwBoard_configCpswClocks(void);

static uint32_t EthFwBoard_getMacAddrPoolEeprom(uint8_t macAddr[][ENET_MAC_ADDR_LEN],
                                                uint32_t poolSize);

static uint32_t EthFwBoard_getMacAddrPoolStatic(uint8_t macAddr[][ENET_MAC_ADDR_LEN],
                                                uint32_t poolSize);

static void EthFwBoard_configSerdesBridge(uint32_t macPortNum);

/* SGMII Helper Functions */
static void EthFwBoard_SGMII_enableMasterMode(uint32_t portNum);

static void EthFwBoard_SGMII_disableAutoNegotiation(uint32_t portNum);

static void EthFwBoard_SGMII_setAdvAbility(uint32_t portNum,
                                           EthFwBoard_SGMII_AdvAbility* pSgmiiAdvAbility);

static void EthFwBoard_configSgmiiPort(uint32_t macPortNum);

static void EthFwBoard_SGMII_startRxTxSoftReset(uint32_t portNum);

static void EthFwBoard_SGMII_endRxTxSoftReset(uint32_t portNum);

static uint32_t EthFwBoard_SGMII_getRxTxSoftResetStatus(uint32_t portNum);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
static EthFwBoard_Obj gEthFwBoard;
/*!
 * \brief Common Processor Board (CPB) board's DP83867 PHY configuration.
 */

static const Dp83867_Cfg gEnetCpbBoard_dp83867PhyCfg =
{
/* The delay values are set based on trial and error and not tuned per port of the evm */
    .txClkShiftEn         = true,
    .rxClkShiftEn         = true,
    .txDelayInPs          = 250U,   /* 0.25 ns */
    .rxDelayInPs          = 2000U,  /* 2.00 ns */
    .txFifoDepth          = 4U,
    .impedanceInMilliOhms = 35000,  /* 35 ohms */
    .idleCntThresh        = 4U,     /* Improves short cable performance */
    .gpio0Mode            = DP83867_GPIO0_LED3,
    .gpio1Mode            = DP83867_GPIO1_COL, /* Unused */
    .ledMode              =
    {
        DP83867_LED_LINKED,         /* Unused */
        DP83867_LED_LINKED_100BTX,
        DP83867_LED_RXTXACT,
        DP83867_LED_LINKED_1000BT,
    },
};

/* 2 x ports: Port 1 (RGMII) + Port 2 (SGMII MAC-to-MAC) */
static EthFwBoard_MacPortCfg gEthFw_cpbMacPortCfg[] =
{
    {   /* MAC Port 1 - RGMII with DP83867 PHY */
        .macPort   = ENET_MAC_PORT_1,
        .mii       = { ENET_MAC_LAYER_GMII, ENET_MAC_SUBLAYER_REDUCED },
        .phyCfg    =
        {
            .phyAddr         = 0U,
            .isStrapped      = BFALSE,
            .skipExtendedCfg = BFALSE,
            .extendedCfg     = &gEnetCpbBoard_dp83867PhyCfg,
            .extendedCfgSize = sizeof(gEnetCpbBoard_dp83867PhyCfg),
        },
        .sgmiiMode = ENET_MAC_SGMIIMODE_INVALID,
        .linkCfg   = { ENET_SPEED_AUTO, ENET_DUPLEX_AUTO },
    },
};

/* 1 x SGMII port in MAC-to-MAC mode using (SGMII) ENET bridge expansion board */
static EthFwBoard_MacPortCfg gEthFw_enetBridgeMacPortCfg =
{
    .macPort   = ENET_MAC_PORT_2,
    .mii       = { ENET_MAC_LAYER_GMII, ENET_MAC_SUBLAYER_SERIAL },
    .phyCfg    =
    {
        .phyAddr         = ENETPHY_INVALID_PHYADDR,
        .isStrapped      = BFALSE,
        .skipExtendedCfg = BFALSE,
        .extendedCfg     = NULL,
        .extendedCfgSize = 0U,
    },
    .sgmiiMode = ENET_MAC_SGMIIMODE_SGMII_FORCEDLINK,
    .linkCfg   = { ENET_SPEED_1GBIT, ENET_DUPLEX_FULL },
};

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int32_t EthFwBoard_init(uint32_t flags)
{
    /* Save the functionality requested by app */
    gEthFwBoard.enetBridgeEnabled = ENET_NOT_ZERO(flags & ETHFW_BOARD_ENET_BRIDGE_ENABLE);
    gEthFwBoard.serdesAllowed = ENET_NOT_ZERO(flags & ETHFW_BOARD_SERDES_CONFIG);
    int32_t boardStatus = ENET_SOK;

    if (gEthFwBoard.enetBridgeEnabled)
    {
        EthFwBoard_configSerdesBridge(ENET_MAC_PORT_2);
        /*The below API is called as a part of enableMacport and hence commenting out here*/
        //EthFwBoard_configSgmiiPort(ENET_MAC_PORT_2);
    }

    EnetAppUtils_enableClocks(ENET_CPSW_3G, 0U);

    return boardStatus;
}

int32_t EthFwBoard_validateMacPorts(Enet_MacPort* enabledMacPortList,
                                    uint8_t numEnabledMacPortList,
                                    Enet_MacPort* gptpEnabledPortList,
                                    uint8_t numGptpEnabledPortList
                                    )
{
    /* TODO: Add MAC port validation logic later */
    int32_t status = ENET_SOK;
    return status;
}

uint32_t EthFwBoard_getMacPorts(Enet_MacPort macPorts[ENET_MAC_PORT_NUM])
{
    uint32_t num = 0U;
    uint32_t req;
    uint32_t i;

    memset(macPorts, 0, sizeof(*macPorts));

    req = EnetUtils_min(ENET_MAC_PORT_NUM, ENET_ARRAYSIZE(gEthFw_cpbMacPortCfg));
    for (i = 0U; i < req; i++)
    {
        macPorts[num++] = gEthFw_cpbMacPortCfg[i].macPort;
    }

    if (gEthFwBoard.enetBridgeEnabled)
    {
        if (num < ENET_MAC_PORT_NUM)
        {
            macPorts[num++] = gEthFw_enetBridgeMacPortCfg.macPort;
        }
    }

    return num;
}

static const EthFwBoard_MacPortCfg *EthFwBoard_findPortCfg(Enet_MacPort macPort)
{
    const EthFwBoard_MacPortCfg *portCfg = NULL;
    uint32_t i;

    for (i = 0U; i < ENET_ARRAYSIZE(gEthFw_cpbMacPortCfg); i++)
    {
        if (gEthFw_cpbMacPortCfg[i].macPort == macPort)
        {
            portCfg = &gEthFw_cpbMacPortCfg[i];
            break;
        }
    }

    if ((portCfg == NULL) && gEthFwBoard.enetBridgeEnabled)
    {
        if (gEthFw_enetBridgeMacPortCfg.macPort == macPort)
        {
            portCfg = &gEthFw_enetBridgeMacPortCfg;
        }
    }

    return portCfg;
}

int32_t EthFwBoard_setPortCfg(Enet_MacPort macPort,
                              CpswMacPort_Cfg *macCfg,
                              EnetMacPort_Interface *mii,
                              EnetPhy_Cfg *phyCfg,
                              EnetMacPort_LinkCfg *linkCfg)
{
    const EthFwBoard_MacPortCfg *portCfg;
    int32_t status = ENET_ENOTFOUND;

    CpswMacPort_initCfg(macCfg);
    EnetPhy_initCfg(phyCfg);

    portCfg = EthFwBoard_findPortCfg(macPort);
    if (portCfg != NULL)
    {
        /* Set MII configuration: RGMII or Q/SGMII */
        *mii = portCfg->mii;
        mii->variantType = ENET_MAC_VARIANT_FORCED;

        /* Set PHY configuration parameters */
        phyCfg->phyAddr         = portCfg->phyCfg.phyAddr;
        phyCfg->isStrapped      = portCfg->phyCfg.isStrapped;
        phyCfg->loopbackEn      = BFALSE;
        phyCfg->skipExtendedCfg = portCfg->phyCfg.skipExtendedCfg;
        phyCfg->extendedCfgSize = portCfg->phyCfg.extendedCfgSize;
        memcpy(phyCfg->extendedCfg, portCfg->phyCfg.extendedCfg, portCfg->phyCfg.extendedCfgSize);

        /* Set link configuration: speed and duplex */
        *linkCfg = portCfg->linkCfg;

        /* Set SGMII mode (applicable for Q/SGMII ports only) */
        macCfg->sgmiiMode = portCfg->sgmiiMode;

        status = ENET_SOK;
    }
    else
    {
        ETHFWTRACE_ERR(status, "Port %u params not found", ENET_MACPORT_ID(macPort));
    }

    return status;
}

static void EthFwBoard_enableMods(void)
{

}

static void EthFwBoard_configSerdesBridge(uint32_t macPortNum)
{

    uint32_t boardStatus;
    uint32_t serdesInstance;

    ETHFWTRACE_INFO("EthFwBoard_configSerdesBridge: Configuring SerDes for MAC port %u", macPortNum);

    if (gEthFwBoard.serdesAllowed)
    {

        if (macPortNum == ENET_MAC_PORT_1)
        {
            serdesInstance = CSL_TORRENT_SERDES1;
            ETHFWTRACE_INFO("Using SERDES1 for MAC port 1");
        }
        else
        {
            serdesInstance = CSL_TORRENT_SERDES0;
            ETHFWTRACE_INFO("Using SERDES0 for MAC port 2");
        }

        ETHFWTRACE_INFO("Enabling SerDes instance %u", serdesInstance);
        boardStatus = BoardUtils_enableSerDes(serdesInstance);
        ETHFWTRACE_ERR_IF((boardStatus != ETHFW_SOK), boardStatus,
                          "Failed to enable SerDes ");

        /* Configure SerDes for SGMII functionality */
        ETHFWTRACE_INFO("Configuring SerDes for SGMII (this should lock PLL)");
        boardStatus = Board_serdesCfgSgmii(serdesInstance);
        ETHFWTRACE_ERR_IF((boardStatus != ETHFW_SOK), boardStatus,
                          "Failed to configure SerDes for SGMII");
        ETHFWTRACE_INFO("SerDes SGMII configuration complete");
        EnetAppUtils_assert(boardStatus == ETHFW_SOK);
    }
    else
    {
        ETHFWTRACE_ERR(-1, "SerDes configuration not allowed");
    }
}

static void EthFwBoard_configCpswClocks(void)
{
    /* Enable CPPI_CLK_CLK and set RGMII_MHZ_[5,50,250]_CLK clock rate */
    EnetAppUtils_enableClocks(ENET_CPSW_3G, 0U);
}

uint32_t EthFwBoard_getMacAddrPool(uint8_t macAddr[][ENET_MAC_ADDR_LEN],
                                   uint32_t poolSize)
{
    uint32_t allocCnt = 0U;
    uint32_t staticCnt = 0U;

    allocCnt = EthFwBoard_getMacAddrPoolEeprom(macAddr, poolSize);

    if (allocCnt < poolSize)
    {
        staticCnt = EthFwBoard_getMacAddrPoolStatic(&macAddr[allocCnt], poolSize - allocCnt);
        allocCnt += staticCnt;
    }

    return allocCnt;
}

static uint32_t EthFwBoard_getMacAddrPoolEeprom(uint8_t macAddr[][ENET_MAC_ADDR_LEN],
                                                uint32_t poolSize)
{
    int32_t boardStatus;
    uint32_t allocCnt = 0U;

    /* Read number of MAC addresses in EFuse and EEPROM */
    boardStatus = EnetAppSoc_fillMacAddrList(macAddr, poolSize, &allocCnt);
    EnetAppUtils_assert(boardStatus == ENET_SOK);

    if (allocCnt == 0U)
    {
        ETHFWTRACE_ERR(ETHFW_EALLOC, "No MAC addresses read from GESI and/or QENET boards");
    }

    return allocCnt;
}

static uint32_t EthFwBoard_getMacAddrPoolStatic(uint8_t macAddr[][ENET_MAC_ADDR_LEN],
                                                uint32_t poolSize)
{
    uint32_t macAddrCnt;
    uint32_t allocCnt = 0U;

    /*
     * Workaround for EthFw/u-boot I2C conflicts:
     * EthFw reads MAC addresses from GESI and QUAD_ETH boards during EthFw
     * initialization which are stored in EEPROM memories and are read via
     * I2C.  These I2C accesses tend to occur around the same u-boot is also
     * performing I2C accesses, causing transactions to timeout or other
     * similar symptoms.
     *
     * I2C sharing is a known limitation but no current solution exists at
     * this time.  As a temporary workaround, EthFw will use fixed MAC
     * addresses in Linux builds. For RTOS build, MAC addresses will still
     * be read from EEPROM as such I2C contention isn't an problem.
     */
    uint8_t macAddrBuf[][ENET_MAC_ADDR_LEN] =
    {
        { 0x70U, 0xFFU, 0x76U, 0x1DU, 0x92U, 0xC1U },
        { 0x70U, 0xFFU, 0x76U, 0x1DU, 0x92U, 0xC2U },
        { 0x70U, 0xFFU, 0x76U, 0x1DU, 0x92U, 0xC3U },
        { 0x70U, 0xFFU, 0x76U, 0x1DU, 0x92U, 0xC4U },
        { 0x70U, 0xFFU, 0x76U, 0x1DU, 0x92U, 0xC5U },
        { 0x70U, 0xFFU, 0x76U, 0x1DU, 0X8BU, 0xC4U },
        { 0x70U, 0xFFU, 0x76U, 0x1DU, 0X8BU, 0xC5U },
        { 0x70U, 0xFFU, 0x76U, 0x1DU, 0X8BU, 0xC6U },
        { 0x70U, 0xFFU, 0x76U, 0x1DU, 0X8BU, 0xC7U },
    };

    macAddrCnt = ENET_ARRAYSIZE(macAddrBuf);

    /* Save only those required to meet the max number of MAC entries */
    allocCnt = EnetUtils_min(macAddrCnt, poolSize);
    memcpy(&macAddr[0U][0U], &macAddrBuf[0U][0U], allocCnt * ENET_MAC_ADDR_LEN);

    return allocCnt;
}

/* ========================================================================== */
/*                      SGMII Helper Functions                               */
/* ========================================================================== */

static void EthFwBoard_SGMII_startRxTxSoftReset(uint32_t portNum)
{
    CSL_FINS(*(volatile uint32_t *)SGMII_SOFT_RESET_REG_OFFSET(portNum),
             CPSGMII_SOFT_RESET_REG_RT_SOFT_RESET, 1);
    return;
}

static void EthFwBoard_SGMII_endRxTxSoftReset(uint32_t portNum)
{
    CSL_FINS(*(volatile uint32_t *)SGMII_SOFT_RESET_REG_OFFSET(portNum),
             CPSGMII_SOFT_RESET_REG_RT_SOFT_RESET, 0);
    return;
}

static uint32_t EthFwBoard_SGMII_getRxTxSoftResetStatus(uint32_t portNum)
{
    return CSL_FEXT(*(volatile uint32_t *)SGMII_SOFT_RESET_REG_OFFSET(portNum),
                    CPSGMII_SOFT_RESET_REG_RT_SOFT_RESET);
}

static void EthFwBoard_SGMII_enableMasterMode(uint32_t portNum)
{
    volatile uint32_t * SgmiiCtrlReg = (volatile uint32_t *)SGMII_CONTROL_REG_OFFSET(portNum);
    volatile uint32_t SgmiiCtrl = *SgmiiCtrlReg;
    CSL_FINS(SgmiiCtrl, CPSGMII_CONTROL_REG_MASTER, 1);
    *SgmiiCtrlReg = SgmiiCtrl;
    return;
}

static void EthFwBoard_SGMII_disableAutoNegotiation(uint32_t portNum)
{
    volatile uint32_t * SgmiiCtrlReg = (volatile uint32_t *)SGMII_CONTROL_REG_OFFSET(portNum);
    volatile uint32_t SgmiiCtrl = *SgmiiCtrlReg;
    CSL_FINS(SgmiiCtrl, CPSGMII_CONTROL_REG_MR_AN_ENABLE, 0);
    *SgmiiCtrlReg = SgmiiCtrl;
    return;
}

static void EthFwBoard_SGMII_setAdvAbility(uint32_t portNum,
                                           EthFwBoard_SGMII_AdvAbility* pSgmiiAdvAbility)
{
    volatile uint32_t * MRAdvAblityReg = (volatile uint32_t *)SGMII_ADV_ABILITY_REG_OFFSET(portNum);
    volatile uint32_t MRAdvAblity = *MRAdvAblityReg;

    CSL_FINSR(MRAdvAblity, 11, 10, pSgmiiAdvAbility->linkSpeed);
    CSL_FINSR(MRAdvAblity, 12, 12, pSgmiiAdvAbility->duplexMode);
    CSL_FINSR(MRAdvAblity, 15, 15, pSgmiiAdvAbility->bLinkUp);

    /* tx_config_reg[15:0] bit zero is 0 for Fiber mode and 1 for SGMII mode */
    if (CSL_SGMII_MODE_FIBER == pSgmiiAdvAbility->sgmiiMode)
    {
        CSL_FINSR(MRAdvAblity, 0, 0, 0U);
    }
    else
    {
        CSL_FINSR(MRAdvAblity, 0, 0, 1U);
    }
    *MRAdvAblityReg = MRAdvAblity;
    return;
}

static void EthFwBoard_configSgmiiPort(uint32_t macPortNum)
{
    EthFwBoard_SGMII_AdvAbility  sgmiiAdvAbility;
    uint32_t portNum = macPortNum - ENET_MAC_PORT_FIRST;

    ETHFWTRACE_INFO("Configuring SGMII Port %u", macPortNum);

    /* Perform soft reset */
    EthFwBoard_SGMII_startRxTxSoftReset(portNum);
    EthFwBoard_SGMII_endRxTxSoftReset(portNum);

    /* Wait till software reset is complete */
    while (EthFwBoard_SGMII_getRxTxSoftResetStatus(portNum) != 0U)
    {
        EnetAppUtils_delayInUsec(1U);
    }

    /* Configure for MAC-to-MAC mode:
     * - Master mode enabled (this is the MAC side of the connection)
     * - Auto-negotiation disabled (forced link)
     * - Fixed 1Gbps, Full Duplex
     */
    EthFwBoard_SGMII_enableMasterMode(portNum);
    EthFwBoard_SGMII_disableAutoNegotiation(portNum);
    EnetAppUtils_delayInUsec(1U);

    /* Set advertisement ability for 1Gbps, Full Duplex, Link Up */
    sgmiiAdvAbility.duplexMode = CSL_SGMII_FULL_DUPLEX;
    sgmiiAdvAbility.linkSpeed = CSL_SGMII_1000_MBPS;
    sgmiiAdvAbility.sgmiiMode = CSL_SGMII_MODE_SGMII;  /* SGMII mode (not Fiber) */
    sgmiiAdvAbility.bLinkUp = 1;

    EthFwBoard_SGMII_setAdvAbility(portNum, &sgmiiAdvAbility);
    EnetAppUtils_delayInUsec(5);

    ETHFWTRACE_INFO("SGMII Port %u configuration complete", macPortNum);

    return;
}
