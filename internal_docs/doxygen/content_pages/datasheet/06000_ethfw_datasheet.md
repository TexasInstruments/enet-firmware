# EthFw Datasheet {#ethfw_datasheet}


[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Boot Time {#ethfw_datasheet_boot_time}

The Ethernet Firmware boot time measurements in the table below show the current
status in the TI Processor SDK for J721E.  The test setup is:
 - Hardware: TI J721E EVM with GESI daughter board.
 - Software: Ethernet Firmware running on Main R5F 0 core 0 at 1 GHz.

**Note:** It is worth noting that the reported boot time below is not optimized.

<table >
  <tr>
    <th>Function
    <th>Description
    <th>Total Time
  </tr>
  <tr>
    <td>main()</td>
    <td>
        Starting time is when Ethernet Firmware application is ready to run in main().

        Using main() as starting point decouples these measurements from the EthFw binary
        loading mechanism.
    </td>
    <td>0 ms</td>
  </tr>
  <tr>
    <td>Layer-2 switching active</td>
    <td>
        Time elapsed from main() till L2 switching is active.
        - Board and clocks initialization.
        - CPSW has been initialized.
        - MAC ports have been opened.
        - ALE has been configured to route packets at Layer-2.
        - Two MAC ports are linked.

        The time reported here is for an equivalent of a MAC-to-MAC link, where PHY driver
        state machine is bypassed.
    </td>
    <td>535 ms</td>
  </tr>
  <tr>
    <td>Host port ready for RX/TX</td>
    <td>
        Time elapsed from main() till host port is ready for packet transmission and reception.
        - UDMA RX flow has been opened and host port is ready to receive packets.
        - UDMA TX channel has been opened and host port is ready to transmit packets.
    </td>
    <td>725 ms</td>
  </tr>
</table>

This table doesn't take into account the time between power-on reset (POR) and the 
Firmware image loaded and made ready to run, as it will be bootloader dependent.

In a MAC-to-PHY scenario, the *Layer-2 switching active* time is heavily determined by the
time taken by the Ethernet PHYs to establish a link with the remote partner.  The total
*Layer-2 switching active* time must take into account the *link time* corresponding to the
PHY configuration being used.

The table below shows the time taken to get link up for different PHY configurations.

<table >
  <tr>
    <th>PHY Configuration
    <th>Link Time
  </tr>
  <tr>
    <td>1 Gbps, auto-negotiation</td>
    <td>3,965 ms</td>
  </tr>
  <tr>
    <td>100 Mbps, auto-negotiation</td>
    <td>2,182 ms</td>
  </tr>
  <tr>
    <td>100 Mbps, manual mode</td>
    <td>760 ms</td>
  </tr>
</table>

The PHY tick period was reduced periodic tick to 5 msecs to improve link up time. The default tick
period in Processor SDK is 100 msecs.

<BR>

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History  {#ethfw_datasheet_rev_hist}

Revision | Date          | Author        | Description         | Status
---------|---------------|---------------|---------------------|----------------
0.1      | 09 Mar 2021   | Misael Lopez  | First version       | Pending Review

