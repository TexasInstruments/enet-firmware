# EthFw Datasheet {#ethfw_datasheet}


[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Boot Time {#ethfw_datasheet_boot_time}

The Ethernet Firmware boot time measurements in the table below show the current
status in the TI Processor SDK for J721E/J7200/J784S4.  The test setup is:
 - Hardware: TI Jacinto EVM with MAC-to-MAC connection to a second TI EVM.
 - Software: Ethernet Firmware running on Main R5F 0 core 0 at 1 GHz.

**Note:** It is worth noting that the reported boot time below is not optimized.

<table >
  <tr>
    <th>Function
    <th>Description
  </tr>
  <tr>
    <td>main()</td>
    <td>
        Starting time is when Ethernet Firmware application is ready to run in main().

        Using main() as starting point decouples these measurements from the EthFw binary
        loading mechanism.
    </td>
  </tr>
  <tr>
    <td>Layer-2 switching active</td>
    <td>
        Time elapsed from main() till L2 switching is active.
        - Board and clocks initialization.
        - CPSW has been initialized.
        - One MAC port have been opened (MAC-to-MAC connection).
        - ALE has been configured to route packets at Layer-2.
        - Two MAC ports are linked (MAC-to-MAC connection).
    </td>
  </tr>
  <tr>
    <td>Host port ready for RX/TX</td>
    <td>
        Time elapsed from main() till host port is ready for packet transmission and reception.
        - UDMA RX flow has been opened and host port is ready to receive packets.
        - UDMA TX channel has been opened and host port is ready to transmit packets.
    </td>
  </tr>
  <tr>
    <td>TCP/IP stack initialized</td>
    <td>
        Time elapsed from main() till TCP/IP stack is initialized.
        - TCP/IP lwIP stack's *netif up* status callback reports (static) IP address.
    </td>
  </tr>
  <tr>
    <td>gPTP stack initilized</td>
    <td>
        Time elapsed from main() till gPTP stack is initialized.
        - gPTP initialization routine is called.
        - This is not the convergence time to achieve time synchronization.
    </td>
  </tr>
  <tr>
    <td>CPSW Proxy Server initialized</td>
    <td>
        Time elapsed from main() till CPSW Proxy Server is initialized.
        - ETHFW is ready to receive remote commands from virtual clients.
        - Excludes MPU1_0 late init, late announcement.
    </td>
  </tr>
</table>

The time taken to reach each of the ETHFW boot stages described above is summarized
in the following table.

| Boot stage                     |  J721E    |  J7200    | J784S4    |
|:-------------------------------|:---------:|:---------:|:---------:|
| main()                         |      0 ms |      0 ms |      0 ms |
| Layer-2 switching active       | 143.94 ms | 188.96 ms | 112.97 ms |
| Host port ready for RX/TX      |  58.54 ms | 101.22 ms |  29.91 ms |
| TCP/IP stack initialized       |  90.71 ms | 127.87 ms |  30.75 ms |
| gPTP stack initilized          | 230.21 ms | 190.56 ms |  94.39 ms |
| CPSW Proxy Server initialized  | 307.27 ms | 222.49 ms | 132.32 ms |

This table doesn't take into account the time between power-on reset (POR) and the 
Firmware image loaded and made ready to run, as it will be bootloader dependent.

In a MAC-to-PHY scenario, the *Layer-2 switching active* time is heavily determined by the
time taken by the Ethernet PHYs to establish a link with the remote partner.  The total
*Layer-2 switching active* time must take into account the *link time* corresponding to the
PHY configuration being used.

<BR>

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History  {#ethfw_datasheet_rev_hist}

Revision | Date          | Author        | Description           | Status
---------|---------------|---------------|-----------------------|----------------
1.1      | 28 Nov 2023   | Misael Lopez  | Added SDK 9.1 results | Approved
1.2      | 25 Mar 2024   | Misael Lopez  | Added SDK 9.2 results | Approved

