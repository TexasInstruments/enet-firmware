# Porting Guide {#ethfw_c_porting_top}

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Introduction {#ethfw_porting_intro}

The default implementation of Ethernet Firmware provided in this software package
enables CPSW switch support on TI EVMs, currently supporting J721E, J7200, J784S4 
and J742S2 TI EVMs.  So Ethernet Firmware's default implementation is board specific,
and porting is required when enabling a new platform.

The main porting steps will be explained throughout this document, and they can
be summarized into three distintive operations: **board initialization**,
**MAC address pool initialization** and **MAC port and PHY configuration**.


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Board initialization {#ethfw_porting_board_init}

This is a big category that encompasses any board level initialization outside
of CPSW peripheral, either internal or external to the Jacinto 7 SoC.
Ethernet Firmware configures CPSW peripheral using Enet LLD.

The configuration steps described in the following sections is not an exhaustive
list, but rather the ones most commonly used.


## Ethernet port interface type selection {#ethfw_porting_board_init_mii}

The MII interface type is configurable per Ethernet port: RMII, RGMII, SGMII,
QSGMII, etc.  The supported interface types is SoC dependent, please refer to
the TRM to find out which  MII interface types are supported in a given SoC.

The interface type is configured Control Module's `CTRLMMR_ENET<n>_CTRL` register,
where `<n>` is the port number.  Note that the RGMII internal transmit delay
is also configured in the same register.

For TI EVMs, the Ethernet port interface type is configured via PDK board library
using `BOARD_INIT_ENETCTRL_CPSW9G` init flag, as follows:

```C
Board_initCfg boardCfg = 0U;
Board_STATUS boardStatus;

/* Set other board config flags */
...

/* Request board library to configure MII interface type in ENETCTRL register */
boardCfg |= BOARD_INIT_ENETCTRL_CPSW9G;

/* Initialize board library */
boardStatus = Board_init(boardCfg);
```

Alternatively, MII interface type could also be configured by bootloader.


## Enabling modules via SciClient {#ethfw_porting_board_init_sciclient}

The SoC *modules* (such as CPSW, SerDes, UART, Timers) which are needed by
Ethernet Firmware need to be explicitly turned on via *sciclient* before using
them.  Note that some of these modules might have already been turned on by
the bootloader, so this step could be skipped for such modules.

The code snippet below shows the SerDes module being turned on:

```C
uint32_t moduleId = TISCI_DEV_SERDES_16G0;

status = Sciclient_pmSetModuleState(moduleId,
                                    TISCI_MSG_VALUE_DEVICE_SW_STATE_ON,
                                    TISCI_MSG_FLAG_AOP | TISCI_MSG_FLAG_DEVICE_RESET_ISO,
                                    SCICLIENT_SERVICE_WAIT_FOREVER);
```

This sciclient API can be used for other modules as well, passing the
appropriate `moduleId`. Please refer to the Sciclient PM API Interface
documentation in the PDK API Guide for further information.

`EnetAppUtils_setDeviceState()` helper function provided as part of Enet
LLD apputils can also be used to turn on and reset modules.  Note that
this helper function asserts that the sciclient calls return no errors.

```C
uint32_t moduleId = TISCI_DEV_SERDES_16G0;

EnetAppUtils_setDeviceState(moduleId, TISCI_MSG_VALUE_DEVICE_SW_STATE_ON, 0U);
```

## Pinmux settings {#ethfw_porting_board_init_pinmux}

The bootloader will initialize a set of base pins needed for its initial operation,
but may not necessarily configure all pins required for Ethernet.  Those pins have
to be explicitly setup by Ethernet Firmware RTOS app.

It's highly recommended to use TI's [SYSCONFIG](https://www.ti.com/tool/SYSCONFIG)
tool for pinmux configuration.  SYSCONFIG can generate a C file output with the
requested pinmux settings.  The settings in this file can be directly passed to
`Board_pinmuxUpdate()`, which is PDK board library's API used to update the mux
configuration of a set of pins:

```C
    /* Pinmux configuration of pins used by Ethernet Firmware. */
    Board_pinmuxUpdate(gEthFwPinmuxData, BOARD_SOC_DOMAIN_MAIN);
```

It's recommended that pinmux settings done in this step is limited to the pins
used for Ethernet related functionality in order to avoid overwriting the pinmux
settings done by others (i.e. bootloader, other processing cores).


## Ethernet PHY initialization {#ethfw_porting_board_init_phy}

During the initialization stage of Ethernet Firmware, the Ethernet ports are
*opened*, which triggers a PHY state machine in Enet LLD that takes care of
configuring the Ethernet PHYs via MDIO.  However, this excludes any board
level setting, such as taking the PHY out of reset.

Often times PHYs are taken out of reset at board level at power-on reset (POR).
When not, explicit action must taken - usually by driving a GPIO line.  This
can be done by bootloader or at this stage in Ethernet Firmware's board init.


## SerDes configuration {#ethfw_porting_board_init_serdes}

SerDes has to be configured in advance for the Ethernet ports in Q/SGMII mode.
For multi-link case, where lanes of the same SerDes are configured for different
functionality  (i.e. Ethernet, PCIe, USB), this is typically done by the bootloader.
In single-link or when all lanes are for Ethernet related functionality, the bootloader
may not configure SerDes and Ethernet Firmware must do it via CSL Serdes.

### Clock configuration {#ethfw_porting_board_init_serdes_clks}

The SerDes reference clock(s) rate must be set consistently with the clock
requirement of the intended SerDes function as indicated via `refClk`
argument of `CSL_serdesRefclkSel()` function.

The code snippet below shows how the SerDes reference clocks are being configured
to 100MHz for J721E SerDes used for Q/SGMII operation.  It's worth noting that
the clock rate set in `params.refClock` and `PMLIBClkRateSet()` are consistently
set to 100MHz.

```C
void Board_CfgQsgmii(void)
{
    CSL_SerdesLaneEnableParams params;
    CSL_SerdesResult result;

    /* QSGMII Config */
    params.serdesInstance    = (CSL_SerdesInstance)SGMII_SERDES_INSTANCE;
    params.baseAddr          = CSL_SERDES_16G0_BASE;
    params.refClock          = CSL_SERDES_REF_CLOCK_100M;
    params.refClkSrc         = CSL_SERDES_REF_CLOCK_INT;
    params.linkRate          = CSL_SERDES_LINK_RATE_5G;
    params.numLanes          = 1;
    params.laneMask          = 0x2;
    params.phyType           = CSL_SERDES_PHY_TYPE_QSGMII;
    ...

    CSL_serdesPorReset(params.baseAddr);

    /* Select the IP type, IP instance num, Serdes Lane Number */
    CSL_serdesIPSelect(CSL_CTRL_MMR0_CFG0_BASE,
                       params.phyType,
                       params.phyInstanceNum,
                       params.serdesInstance,
                       SGMII_LANE_NUM);

    /* Set SerDes reference clock configuration */
    result = CSL_serdesRefclkSel(CSL_CTRL_MMR0_CFG0_BASE,
                                 params.baseAddr,
                                 params.refClock,
                                 params.refClkSrc,
                                 params.serdesInstance,
                                 params.phyType);

    ...
}

void EthFwBoard_configureSerdesClock(void)
{
    uint32_t moduleId = TISCI_DEV_SERDES_16G0;
    uint32_t clkId;
    uint32_t clkRateHz;

    /* Set rate of SerDes CORE_REFCLK1 to 100M */
    clkId     = TISCI_DEV_SERDES_16G0_CORE_REF1_CLK;
    clkRateHz = 100000000U;
    PMLIBClkRateSet(moduleId, clkId, clkRateHz);

    /* Set rate of SerDes CORE_REFCLK to 100M */
    clkId     = TISCI_DEV_SERDES_16G0_CORE_REF_CLK;
    clkRateHz = 100000000U;
    PMLIBClkRateSet(moduleId, clkId, clkRateHz);

    /* Turn on SerDes module, take it out of reset */
    EnetAppUtils_setDeviceState(moduleId, TISCI_MSG_VALUE_DEVICE_SW_STATE_ON, 0U);
}
```

Alternatively, user may choose to use Enet LLD's `EnetAppUtils_clkRateSet()` helper
function to set the clocks.  This is a simple wrapper on top of PDK PM library APIs.


### SerDes module configuration {#ethfw_porting_board_init_serdes_mod}

SerDes module configuration is done using CSL SerDes APIs.  For TI EVM support,
PDK Board library provides helper functions, such as `Board_serdesCfgSgmii()` and
`Board_serdesCfgQsgmii()` to configure SerDes according to TI EVM capabilities.
Ethernet Firmware uses these helper functions.  Users may choose to implement their
own SerDes configuration functions using CSL.

The CSL SerDes library support can be found at the following locations:
 - J721E: `<pdk>/packages/ti/csl/src/ip/serdes_cd/V0`
 - J7200: `<pdk>/packages/ti/csl/src/ip/serdes_cd/V1`
 - J721S2: `<pdk>/packages/ti/csl/src/ip/serdes_cd/V2`
 - J784S4: `<pdk>/packages/ti/csl/src/ip/serdes_cd/V3`
 - J742S2: `<pdk>/packages/ti/csl/src/ip/serdes_cd/V3`


## CPSW clocks configuration {#ethfw_porting_board_init_cpsw}

Similar to any other SoC module, CPSW clocks must be configured and the module must
be explicitly turned on via Sciclient.

The following clocks must be configured for proper operation of CPSW: `CPPI_CLK_CLK`,
`RGMII_MHZ_5_CLK`, `RGMII_MHZ_CLK`, and `RGMII_MHZ_250_CLK`.  Optionally, when using
time synchronization, CPTS source clock must also be configured, the source is
selected from a list of possible sources which is SoC dependent.  Please refer to
TRM of the specific SoC being used.

```C
uint32_t moduleId = TISCI_DEV_CPSW0;
uint32_t appFlags = TISCI_MSG_FLAG_DEVICE_EXCLUSIVE;
uint64_t cppiClkFreqHz = 0U;

/* Set rate of CPPI_CLK */
cppiClkFreqHz = EnetSoc_getClkFreq(enetType, instId, CPSW_CPPI_CLK);
PMLIBClkRateSet(moduleId, TISCI_DEV_CPSW0_CPPI_CLK_CLK, clkRateHz);

/* Set rate of RGMII clocks */
PMLIBClkRateSet(moduleId, TISCI_DEV_CPSW0_RGMII_MHZ_250_CLK, 250000000U);
PMLIBClkRateSet(moduleId, TISCI_DEV_CPSW0_RGMII_MHZ_50_CLK, 50000000U);
PMLIBClkRateSet(moduleId, TISCI_DEV_CPSW0_RGMII_MHZ_5_CLK, 5000000U);

/* Turn on CPSW module, take it out of reset */
EnetAppUtils_setDeviceState(moduleId, TISCI_MSG_VALUE_DEVICE_SW_STATE_ON, appFlags);
```

Alternatively, user may choose to use Enet LLD's helper functions as shown below:

```C
/* CPTS clock source */
#define ETHFW_CPSW_CPTS_RFT_CLK                 (ENET_CPSW0_CPTS_CLKSEL_MAIN_SYSCLK0)

void EthFwBoard_configCpswClocks(void)
{
    /* Enable CPPI_CLK_CLK and set RGMII_MHZ_[5,50,250]_CLK clock rate */
    EnetAppUtils_enableClocks(ENET_CPSW_9G, 0U);

    /* Select CPTS source clock (CPTS_RFT_CLK) in CLKSEL mux */
    EnetAppUtils_selectCptsClock(ENET_CPSW_9G, 0U, ETHFW_CPSW_CPTS_RFT_CLK);
}
```

[Back To Top](@ref ethfw_c_porting_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# MAC address pool {#ethfw_porting_mac_pool}

Ethernet Firmware requires a pool of MAC addresses to be used during its lifecycle,
for its own usage and for the virtual MAC clients.  A MAC address is given to each
virtual client as they attach to Ethernet Firmware at runtime.

The pool size requirement is as follows:
 - One MAC address for Ethernet Firmware. This is used by the Ethernet Firmware
   lwIP stack running on Main R5F.
 - One MAC address for each virtual network interface for Linux, QNX or RTOS clients.
    - Processing cores that enable multiple virtual interfaces require as many MAC addresses.
    - For instance, Linux and RTOS clients enable \ref ethfw_virtual_switch_port and
      \ref ethfw_virtual_mac_port, hence they require two MAC addresses.
 - One MAC address for AUTOSAR virtual MAC.

It's worth noting that the required MAC address pool can exceed the number of
physical Ethernet ports, as the pool size also depends on the number of
virtual clients being enabled.

In the default implementation of Ethernet Firmware for TI EVMs, a total of 6 x
MAC addresses are required for Linux and RTOS SDK releases.

| Count | Owner                 | Core   | Usage
|:-----:|:---------------------:|:------:|:-----------------------------------------------
| 1     | Ethernet Firmware     | mcu2_0 |lwIP stack
| 2     | Linux virtual clients | mpu1_0 | 1 x virtual switch port (eth1)
| ^     | ^                     | ^      | 1 x virtual MAC port (eth2)|
| 2     | RTOS virtual clients  | mcu2_1 | 1 x virtual switch port (netif1)
| ^     | ^                     | ^      | 1 x virtual MAC port (netif2)
| 1     | AUTOSAR virtual MAC   | mcu2_1 | 1 x virtual switch port

For QNX SDK releases, a total of 4 x MAC addresses are required as MAC-only feature
is not enabled which reduces the overall count.

| Count | Owner                 | Core   | Usage
|:-----:|:---------------------:|:------:|:-----------------------------------------------
| 1     | Ethernet Firmware     | mcu2_0 |lwIP stack
| 1     | QNX virtual client    | mpu1_0 | 1 x virtual switch port
| 1     | RTOS virtual clients  | mcu2_1 | 1 x virtual switch port
| 1     | AUTOSAR virtual MAC   | mcu2_1 | 1 x virtual switch port

The pool of MAC addresses is usually provisioned in an EEPROM memory, so the Ethernet
Firmware RTOS application must read them from that memory and populate the
pool which is passed to Ethernet Firmware at open time via
[<b>EthFw_Config</b>](../api_guide/structEthFw__Config.html).

The code snippet below shows the configuration parameters where the MAC address
pool must be populated:

```C
#define ETHAPP_MAC_ADDR_POOL_SIZE               (6U)

int32_t EthApp_initEthFw(void)
{
    EthFw_Config ethFwCfg;
    Cpsw_Cfg *cpswCfg = &ethFwCfg.cpswCfg;
    EnetRm_MacAddressPool *pool = &cpswCfg->resCfg.macList;
    uint32_t poolSize;

    ...

    /* Populate MAC address pool */
    poolSize = EnetUtils_min(ENET_ARRAYSIZE(pool->macAddress), ETHAPP_MAC_ADDR_POOL_SIZE);
    pool->numMacAddress = EthFwBoard_getMacAddrPool(pool->macAddress, poolSize);

    ...

    /* Initialize EthFw */
    gEthAppObj.hEthFw = EthFw_init(gEthAppObj.enetType, &ethFwCfg);

    ...
}

uint32_t EthFwBoard_getMacAddrPool(uint8_t macAddr[][ENET_MAC_ADDR_LEN],
                                   uint32_t poolSize)
{
    uint32_t allocCnt;

    /* Read MAC addresses from EEPROM */
    allocCnt = EthFwBoard_getMacAddrPoolEeprom(macAddr, poolSize);

    return allocCnt;
}
```

In TI Jacinto 7 EVMs, the EEPROMs in the expansion boards are flashed with a number
of MAC addresses:
 - EEPROM in GESI expansion board has 5 MAC addresses.
 - EEPROM in QpENet expansion board (QSGMII) has 4 MAC addreses.

**Note:** In TI EVMs, there is a potential conflict when Ethernet Firmware tries
to read EEPROM memories via I2C at the same time that another processing core
(i.e. A72) is also using the same I2C bus (see [ETHFW-1580](https://sir.ext.ti.com/jira/browse/EXT_EP-10020)).
Due to this limitation, Ethernet Firmware uses a static MAC address pool for
Linux/QNX SDKs as a temporary workaround. **The static MAC address pool must not be
used in production code.**

[Back To Top](@ref ethfw_c_porting_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# MAC port and PHY configuration {#ethfw_porting_phy_cfg}

The RTOS application passes to Ethernet Firmware the list of MAC ports intended
to be used.  Ethernet Firmware then enables each of them during its initialization
stage when [<b>EthFw_init()</b>](../api_guide/group__ETHFW__LIB__API.html#ga41135bde04b458fc86b70b2448c0e5a3)
is called.  At that time, Ethernet Firmware will call the
[<b>EthFw_Config.setPortCfg()</b>](../api_guide/structEthFw__Config.html#a25336535f220df239805c80dd3e5d738)
callback for each MAC port.

It's expected that the implementation of [<b>EthFw_Config.setPortCfg()</b>](../api_guide/structEthFw__Config.html#a25336535f220df239805c80dd3e5d738) callback will:
 - Set the MII interface type: RMII, RGMII, Q/SGMII, etc.
 - Set the link configuration: auto-negotiation or fixed (speed/duplexity)
 - Set the PHY configuration: PHY address, PHY specific parameters, strap, etc.
     - For MAC-to-MAC connection, PHY address must be set to `ENETPHY_INVALID_PHYADDR`.
 - Set SGMII mode (for Q/SGMII ports only).

The following code snippet shows the relevant functions and configuration parameters
involved in MAC and PHY configuration:

```C
Enet_MacPort gEthAppPorts[] =
{
    ENET_MAC_PORT_1, /* RGMII */
    ENET_MAC_PORT_2, /* QSGMII main */
    ENET_MAC_PORT_3, /* RGMII */
    ENET_MAC_PORT_4, /* RGMII */
    ENET_MAC_PORT_5, /* QSGMII sub */
    ENET_MAC_PORT_6, /* QSGMII sub */
    ENET_MAC_PORT_7, /* QSGMII sub */
    ENET_MAC_PORT_8, /* RGMII */
};

int32_t EthApp_initEthFw(void)
{
    EthFw_Config ethFwCfg;

    ...

    /* Set hardware port configuration parameters */
    ethFwCfg.ports    = &gEthAppPorts[0];
    ethFwCfg.numPorts = ARRAY_SIZE(gEthAppPorts);
    ethFwCfg.setPortCfg = EthFwBoard_setPortCfg;

    ...

    /* Initialize EthFw */
    gEthAppObj.hEthFw = EthFw_init(gEthAppObj.enetType, &ethFwCfg);

    ...
}

int32_t EthFwBoard_setPortCfg(Enet_MacPort macPort,
                              CpswMacPort_Cfg *macCfg,
                              EnetMacPort_Interface *mii,
                              EnetPhy_Cfg *phyCfg,
                              EnetMacPort_LinkCfg *linkCfg)
{
    if (macPort == ENET_MAC_PORT_1)
    {
        /* Set MII type to RGMII */
        mii->layerType    = ENET_MAC_LAYER_GMII;
        mii->subLayerType = ENET_MAC_SUBLAYER_REDUCED;
        mii->variantType  = ENET_MAC_VARIANT_FORCED;

        /* Set link configuration to auto-negotiation */
        linkCfg->speed     = ENET_SPEED_AUTO;
        linkCfg->duplexity = ENET_DUPLEX_AUTO;

        /* Set the RGMII PHY configuration parameters */
        phyCfg->phyAddr         = 12U,
        phyCfg->isStrapped      = false,
        phyCfg->skipExtendedCfg = false;
        phyCfg->extendedCfg     = &gEnetGesiBoard_dp83867PhyCfg,
        phyCfg->extendedCfgSize = sizeof(gEnetGesiBoard_dp83867PhyCfg),

        /* SGMII mode is don't care for RGMII ports */
        macCfg->sgmiiMode = ENET_MAC_SGMIIMODE_INVALID;
    }
    else if (macPort == ENET_MAC_PORT_2)
    {
        /* Set QSGMII main port configuration parameters */
    }
    ...

    return ENET_SOK;
}
```

The implementation of `EthFwBoard_setPortCfg()` for TI EVMs provided in the SDK
is slightly different than the one shown in the code snippet above.  Instead,
it's organized based on the expansion boards supported in the different TI EVMs.
This is an implementation decision, so user should choose the approach that
works best for their system.

[Back To Top](@ref ethfw_c_porting_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# TI EVM Board Support {#ethfw_porting_tievm}

Starting with SDK 8.2.1, board initialization has been moved from Enet LLD to
Ethernet Firmware repository:
 - Old location is `<pdk>/packages/ti/drv/enet/examples/utils/V1/`.
 - New location is `<ethfw>/utils/board/`.

This implementation is specific to TI EVMs (Ethernet port configuration, PHYs being
used, etc), and can be used as reference when porting Ethernet Firmware to a new platform.

It has been made a separate library because it's required in two scenarios:
 - When Ethernet Firmware is built as a standalone RTOS application (i.e. when Main R5F0
   core 0 is dedicated for Ethernet Firmware), and/or
 - When Ethernet Firmware is integrated in a larger RTOS application (i.e. when integrated
   in Vision Apps for J721E/J784S4).

There EthFw board library is composed of four main board initialization functions:

- [<b>EthFwBoard_init()</b>](../api_guide/group__ETHFW__BOARD__UTILS.html#ga8625aa9f3642ecc8b5122560fc6ac626).
  Implements the board initialization steps described in \ref ethfw_porting_board_init section.
- [<b>EthFwBoard_getMacPorts()</b>](../api_guide/group__ETHFW__BOARD__UTILS.html#ga6b188c815f14dc8e1ab58ebbfc4fdad7). Returns the list of MAC ports supported by the EthFw board library.
- [<b>EthFwBoard_setPortCfg()</b>](../api_guide/group__ETHFW__BOARD__UTILS.html#gacf09b70f8441308445d6fb8906d187dc). Implements the MAC/PHY configuration callback described in \ref ethfw_porting_phy_cfg.
- [<b>EthFwBoard_getMacAddrPool()</b>](../api_guide/group__ETHFW__BOARD__UTILS.html#gae0ae6187fa1ab8991ff25250f8248822). Populates the MAC address pool with values read from EEPROM or static defined
  (TI EVM workaround).

[Back To Top](@ref ethfw_c_porting_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History {#ethfw_porting_guide_rev_history}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


Revision | Date          | Author                 | Description
---------|---------------|------------------------|----------------------
0.1      | 25 Aug 2022   | Misael Lopez           | Initial version of porting guide

[Back To Top](@ref ethfw_c_porting_top)
