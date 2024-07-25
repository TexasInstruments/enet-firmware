# EthFw Datasheet {#ethfw_datasheet}


[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Boot Time {#ethfw_datasheet_boot_time}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Intercore Performance Numbers {#ethfw_performance_numbers}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Configuration

| Hardware Configuration    | Value                    |
|:--------------------------|:------------------------:|
| Processing Core           | R5F0 and A72             |
| Core Frequency            | 1 GHz                    |
| Packet buffer memory      | DDR                      |
| Hardware checksum offload | Yes                      |
| Scatter-gather TX         | Yes                      |
| Scatter-gather RX         | No                       |


| Software Configuration    | Value                    |
|:--------------------------|:------------------------:|
| RTOS                      | FreeRTOS                 |
| RTOS application          | EthFw applicaton         |
| TCP/IP stack              | lwIP 2.2.0               |
| Linux client tool version | iperf v2.0.5             |


## Shared Memory transport {#ethfw_shared_mem_transport_datasheet}

Inter-core network interface allows EthFw to communicate with another core using standard TCP/IP protocol suite.
Tap user-space application serves as a medium to facilitate the exchange of Ethernet frames between 
the A72 Linux and R5_0 (MCU2_0) master core.


### Test Setup {#ethfw_shared_mem_test_setup}

![](Intercore_SharedMem_Performance_Setup.png "Intercore using shared memory transport")

### Performance Numbers {#ethfw_shared_mem_numbers}

#### TCP Performance

| Test              | Bandwidth (Mbps)   | CPU Load (%)|
|:------------------|:------------------:|:-----------:|
| TCP RX            | 11.8               |    27       |
| TCP TX            | 11.7               |    26       |
| TCP Bidirectional | RX=5.83, TX=5.98   |    32       |


#### UDP Performance


<table style="width:96%;" rules="all" frame="box">
<tr class="header">
<th rowspan="2">Test</th>
<th colspan="3">Datagram Length = 64B</th>
<th colspan="3">Datagram Length = 256B</th>
<th colspan="3">Datagram Length = 512B</th>
<th colspan="3">Datagram Length = 1470B</th>
</tr>
<tr class="odd"><th><p>Bandwidth (Mbps)</p></th>
<th><p>CPU Load (%)</p></th><th>
<p>Packet Loss (%)</p></th>
<th><p>Bandwidth (Mbps)</p></th>
<th><p>CPU Load (%)</p></th>
<th><p>Packet Loss (%)</p></th>
<th><p>Bandwidth (Mbps)</p></th>
<th><p>CPU Load (%)</p></th>
<th><p>Packet Loss (%)</p></th>
<th><p>Bandwidth (Mbps)</p></th>
<th><p>CPU Load (%)</p></th>
<th><p>Packet Loss (%)</p></th>
</tr>
<tr class="odd">
<td rowspan="3">UDP RX</td>
<td align="center">0.205</td>
<td align="center">14</td>
<td align="center">0.00</td>
<td align="center">1.00</td>
<td align="center">16</td>
<td align="center">0.00</td>
<td align="center">2.0</td>
<td align="center">16</td>
<td align="center">0.00</td>
<td align="center">5.00</td>
<td align="center">18</td>
<td align="center">0.00</td>
</tr>
<tr class="even">
<td align="center">0.410</td>
<td align="center">19</td>
<td align="center">0.00</td>
<td align="center">2.00</td>
<td align="center">21</td>
<td align="center">0.00</td>
<td align="center">4.00</td>
<td align="center">22</td>
<td align="center">0.00</td>
<td align="center">8.00</td>
<td align="center">21</td>
<td align="center">0.00</td>
</tr>
<tr class="odd">
<td align="center">0.511</td>
<td align="center">21</td>
<td align="center">0.00</td>
<td align="center"></td>
<td align="center"></td>
<td align="center"></td>
<td align="center"></td>
<td align="center"></td>
<td align="center"></td>
<td align="center">10.00</td>
<td align="center">24</td>
<td align="center">0.00</td>
</tr>
<tr class="even">
<td>UDP RX (Max)</td>
<td align="center">0.512</td>
<td align="center">22</td>
<td align="center">0.32</td>
<td align="center">2.05</td>
<td align="center">22</td>
<td align="center">0.34</td>
<td align="center">4.09</td>
<td align="center">23</td>
<td align="center">0.71</td>
<td align="center">11.8</td>
<td align="center">26</td>
<td align="center">0.57</td>
</tr>
<tr class="odd">
<td>UDP TX (Max)</td>
<td align="center">15.5</td>
<td align="center">90</td>
<td align="center">0.0038</td>
<td align="center">32.0</td>
<td align="center">82</td>
<td align="center">0.0064</td>
<td align="center">41.4</td>
<td align="center">78</td>
<td align="center">0.003</td>
<td align="center">51.3</td>
<td align="center">80</td>
<td align="center">0.0023</td>
</tr>
</table>


## VEPA intercore (only on J784S4) {#ethfw_vepa_datasheet}

EthFw provides support to enable VEPA (Virtual Ethernet Port Aggregator) functionality with CPSW capable of multihost data flow. Multihost is a CPSW ALE feature that enables packets to be sent and received on host port.

VEPA or hairpin mode allows the traffic to return to the same port (host port in this case) at which it ingressed on. It enables to forward packets directly to clients via host port increasing intercore virtual ethernet communication performance.

### Test Setup {#ethfw_vepa_test_setup}

![](Intercore_VEPA_Performance_Setup.png "Intercore using shared memory transport")

### Performance Numbers {#ethfw_vepa_numbers}

#### TCP Performance

| Test              | Bandwidth (Mbps)   | CPU Load (%)|
|:------------------|:------------------:|:-----------:|
| TCP RX            | 238                |    93       |
| TCP TX            | 237                |    92       |
| TCP Bidirectional | RX=91, TX=106      |    76       |


#### UDP Performance


<table style="width:96%;" rules="all" frame="box">
<tr class="header">
<th rowspan="2">Test</th>
<th colspan="3">Datagram Length = 64B</th>
<th colspan="3">Datagram Length = 256B</th>
<th colspan="3">Datagram Length = 512B</th>
<th colspan="3">Datagram Length = 1470B</th>
</tr>
<tr class="odd"><th><p>Bandwidth (Mbps)</p></th>
<th><p>CPU Load (%)</p></th><th>
<p>Packet Loss (%)</p></th>
<th><p>Bandwidth (Mbps)</p></th>
<th><p>CPU Load (%)</p></th>
<th><p>Packet Loss (%)</p></th>
<th><p>Bandwidth (Mbps)</p></th>
<th><p>CPU Load (%)</p></th>
<th><p>Packet Loss (%)</p></th>
<th><p>Bandwidth (Mbps)</p></th>
<th><p>CPU Load (%)</p></th>
<th><p>Packet Loss (%)</p></th>
</tr>
<tr class="odd">
<td rowspan="3">UDP RX</td>
<td align="center">5.02</td>
<td align="center">27</td>
<td align="center">0.00</td>
<td align="center">25.3</td>
<td align="center">37</td>
<td align="center">0.00</td>
<td align="center">25.1</td>
<td align="center">27</td>
<td align="center">0.00</td>
<td align="center">25.0</td>
<td align="center">20</td>
<td align="center">0.00</td>
</tr>
<tr class="even">
<td align="center">10.0</td>
<td align="center">46</td>
<td align="center">0.00</td>
<td align="center">51.1</td>
<td align="center">79</td>
<td align="center">0.13</td>
<td align="center">50.6</td>
<td align="center">43</td>
<td align="center">0.006</td>
<td align="center">50.0</td>
<td align="center">30</td>
<td align="center">0.00</td>
</tr>
<tr class="odd">
<td align="center">15.0</td>
<td align="center">90</td>
<td align="center">0.096</td>
<td align="center"></td>
<td align="center"></td>
<td align="center"></td>
<td align="center">102</td>
<td align="center">87</td>
<td align="center">0.16</td>
<td align="center">101</td>
<td align="center">51</td>
<td align="center">0.00</td>
</tr>
<tr class="even">
<td>UDP RX (Max)</td>
<td align="center">20.1</td>
<td align="center">91</td>
<td align="center">0.71</td>
<td align="center">67.7</td>
<td align="center">91</td>
<td align="center">0.88</td>
<td align="center">110</td>
<td align="center">90</td>
<td align="center">0.25</td>
<td align="center">193</td>
<td align="center">93</td>
<td align="center">0.11</td>
</tr>
<tr class="odd">
<td>UDP TX (Max)</td>
<td align="center">28.6</td>
<td align="center">100</td>
<td align="center">0.003</td>
<td align="center">98.3</td>
<td align="center">100</td>
<td align="center">0.02</td>
<td align="center">169</td>
<td align="center">100</td>
<td align="center">0.001</td>
<td align="center">327</td>
<td align="center">100</td>
<td align="center">0.001</td>
</tr>
</table>

<BR>

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History  {#ethfw_datasheet_rev_hist}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Revision | Date          | Author         | Description            | Status
---------|---------------|----------------|------------------------|----------------
1.1      | 28 Nov 2023   | Misael Lopez   | Added SDK 9.1 results  | Approved
1.2      | 25 Mar 2024   | Misael Lopez   | Added SDK 9.2 results  | Approved
1.3      | 26 Jul 2024   | Vaibhav Jindal | Added SDK 10.0 results | Approved

