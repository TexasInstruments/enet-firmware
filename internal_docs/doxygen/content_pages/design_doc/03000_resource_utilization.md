# Resource Utilization {#ethfw_resource_top}

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Peripherals {#ethfw_resource_peripherals}

<table>
<tr>
    <th>Peripheral
    <th>Instance
    <th>Usage
<tr>
    <td rowspan="2">I2C
    <td>Wakeup I2C0
    <td>Read EEPROM to get MAC addresses of the board
<tr>
    <td>Main I2C0
    <td>Board I/O expander configuration for RMII and MDIO on GESI board
<tr>
    <td rowspan="2">DMTimer
    <td>-
    <td>FreeRTOS tick
<tr>
    <td>Abstracted by PDK OSAL
    <td>CPSW interrupt pacing
<tr>
    <td>UART
    <td>Main UART2
    <td>Logging
<tr>
    <td>Mailbox
    <td>Abstracted by RPMSG
    <td>IPC with remote cores
<tr>
    <td>SerDes
    <td>Sierra SerDes 0 Lane 1<br>Torrent SerDes 0 Lane 2
    <td>QSGMII (J721E)<br>QSGMII (J7200)
<tr>
    <td rowspan="3">Control Module
    <td>ENET_CTRL
    <td>MAC interface type, RGMII delay
<tr>
    <td>CLKOUT_CTRL
    <td>RMII clock out
<tr>
    <td>CPSW_CLKSEL
    <td>CPTS reference clock selection
</table>

[Back To Top](@ref ethfw_resource_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# DMA {#ethfw_resource_dma}

<table>
<tr>
    <th>Data Direction
    <th>Resource
    <th>Count
<tr>
    <td rowspan="2">Transmit
    <td>Channels
    <td>3 (1 for EthFw/lwIP, 1 for PTP, 1 for software interVLAN)
<tr>
    <td>Ring Acc
    <td>9 (rings per channel: 1 for FQ, 1 for CQ, 1 for TDCQ)
<tr>
    <td rowspan="4">Receive
    <td>Channels
    <td>1
<tr>
    <td>Flows
    <td>5 (1 for EthFw/lwIP, 1 for Proxy ARP, 1 for PTP, 1 for software interVLAN, 1 default flow (drop flow))
<tr>
    <td>Ring Acc
    <td>20 (rings per flow: 1 for FQ, 1 for CQ, 1 for TDCQ, 1 for dropRing)
<tr>
    <td>Proxy Rings
    <td>4 (1 for EthFw/lwIP, 1 for Proxy ARP, 1 for PTP, 1 for software interVLAN)
</table>

[Back To Top](@ref ethfw_resource_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Interrupts {#ethfw_resource_irqs}

| Interrupt      | Description                                                                               |
| -------------- | ----------------------------------------------------------------------------------------- |
| MDIO_PEND      | MDIO pending interrupt (combined MDIO_LINKINT and MDIO_USERINT events)                    |
| STAT_PEND      | Statistics pending interrupt (half roll-over)                                             |
| EVNT_PEND      | Event pending interrupt (event is pushed into CPTS Event FIFO)                            |
| UDMA Tx Completion | UDMA Transmit completion (abstracted by UDMA LLD, event notifications given in EthFw) |
| UDMA Rx Completion | UDMA Receive completion (abstracted by UDMA LLD, event notifications given in EthFw)  |

[Back To Top](@ref ethfw_resource_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Clocks {#ethfw_resource_clocks}

| Clock             | SYSFW ID                          | Description                                                                |
| ----------------- | --------------------------------- | -------------------------------------------------------------------------- |
| CPPI_ICLK         | TISCI_DEV_CPSW0_CPPI_CLK_CLK      | CPPI packet streaming interface clock. Main clock for CPSW0                |
| RGMII_MHZ_250_CLK | TISCI_DEV_CPSW0_RGMII_MHZ_250_CLK | 250-MHz RGMII reference clock                                              |
| RGMII_MHZ_50_CLK  | TISCI_DEV_CPSW0_RGMII_MHZ_50_CLK  | 50-MHz RGMII reference clock                                               |
| RGMII_MHZ_5_CLK   | TISCI_DEV_CPSW0_RGMII_MHZ_5_CLK   | 5-MHz RGMII reference clock                                                |
| RMII_MHZ_50_CLK   | -                                 | 50-MHz RMII reference clock (internal (CLKOUT) or external (RMII_REF_CLK)) |
| MAIN_SYSCLK0      | -                                 | 500-MHz CPTS reference clock                                               |

[Back To Top](@ref ethfw_resource_top)
