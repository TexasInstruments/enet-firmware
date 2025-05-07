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
| Layer-2 switching active       | 141.80 ms | 136.10 ms | 114.85 ms |
| Host port ready for RX/TX      |  43.46 ms | 54.280 ms |  29.43 ms |
| TCP/IP stack initialized       | 52.620 ms | 59.140 ms |  30.64 ms |
| gPTP stack initilized          | 52.910 ms | 67.340 ms |  36.67 ms |
| CPSW Proxy Server initialized  | 66.430 ms | 73.490 ms |  41.35 ms |

This table doesn't take into account the time between power-on reset (POR) and the 
Firmware image loaded and made ready to run, as it will be bootloader dependent.

In a MAC-to-PHY scenario, the *Layer-2 switching active* time is heavily determined by the
time taken by the Ethernet PHYs to establish a link with the remote partner.  The total
*Layer-2 switching active* time must take into account the *link time* corresponding to the
PHY configuration being used.

**Note:** Boot time measurements are not done for J742S2 due to EVM limitation.


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


## EthFw Performance Numbers(Standalone) {#ethfw_standalone_numbers}

### TCP Performance
| Test              | Bandwidth (Mbps)       | CPU Load (%)|
|:------------------|:----------------------:|:-----------:|
| TCP RX            | 275                    |    93       |
| TCP TX            | 13.4                   |    92       |
| TCP Bidirectional | RX = 160, TX= 179      |    76       |

### UDP Performance

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
<td align="center">0.5</td>
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
<td align="center">1</td>
<td align="center">19</td>
<td align="center">0.18</td>
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
<td align="center">2</td>
<td align="center">21</td>
<td align="center">0.62</td>
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
<td align="center">3.5</td>
<td align="center">22</td>
<td align="center">0.88</td>
<td align="center">9</td>
<td align="center">22</td>
<td align="center">0.68</td>
<td align="center">20</td>
<td align="center">23</td>
<td align="center">0.71</td>
<td align="center">100</td>
<td align="center">26</td>
<td align="center">0.57</td>
</tr>
<tr class="odd">
<td>UDP TX (Max)</td>
<td align="center">15.5</td>
<td align="center">90</td>
<td align="center">0.0038</td>
<td align="center">40</td>
<td align="center">82</td>
<td align="center">0.01</td>
<td align="center">80</td>
<td align="center">78</td>
<td align="center">0.003</td>
<td align="center">231</td>
<td align="center">80</td>
<td align="center">0.0023</td>
</tr>
</table>

## Shared Memory transport {#ethfw_shared_mem_transport_datasheet}

Inter-core network interface allows EthFw to communicate with another core using standard TCP/IP protocol suite.
Tap user-space application serves as a medium to facilitate the exchange of Ethernet frames between 
the A72 Linux and R5_0 (MCU2_0) master core.


### Test Setup {#ethfw_shared_mem_test_setup}

![](Intercore_SharedMem_Performance_Setup.png "Intercore using shared memory transport")

### Performance Numbers {#ethfw_shared_mem_numbers}

#### TCP Performance

| Test              | Bandwidth (Mbps)       | CPU Load (%)|
|:------------------|:----------------------:|:-----------:|
| TCP RX            | 12.7                   |    28       |
| TCP TX            | 13.4                   |    24       |
| TCP Bidirectional | RX = 5.71, TX= 6.29    |    34       |


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
<td align="center">0.25</td>
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
<td align="center">0.43</td>
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
<td align="center">0.52</td>
<td align="center">21</td>
<td align="center">0.62</td>
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
<td align="center">0.53</td>
<td align="center">22</td>
<td align="center">0.29</td>
<td align="center">2.1</td>
<td align="center">22</td>
<td align="center">0.31</td>
<td align="center">5</td>
<td align="center">24</td>
<td align="center">0.73</td>
<td align="center">12.5</td>
<td align="center">27</td>
<td align="center">0.57</td>
</tr>
<tr class="odd">
<td>UDP TX (Max)</td>
<td align="center">15.5</td>
<td align="center">90</td>
<td align="center">0.0038</td>
<td align="center">36</td>
<td align="center">82</td>
<td align="center">0.01</td>
<td align="center">43</td>
<td align="center">78</td>
<td align="center">0.003</td>
<td align="center">54</td>
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
| TCP RX            | 245                |    95       |
| TCP TX            | 249                |    93       |
| TCP Bidirectional | RX=92, TX=112      |    74       |


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
<td align="center">25.5</td>
<td align="center">32</td>
<td align="center">0.00</td>
<td align="center">25.5</td>
<td align="center">24</td>
<td align="center">0.00</td>
<td align="center">25.5</td>
<td align="center">16</td>
<td align="center">0.00</td>
</tr>
<tr class="even">
<td align="center">10.0</td>
<td align="center">46</td>
<td align="center">0.00</td>
<td align="center">51.2</td>
<td align="center">74</td>
<td align="center">0.056</td>
<td align="center">51.2</td>
<td align="center">37</td>
<td align="center">0.004</td>
<td align="center">51.2</td>
<td align="center">26</td>
<td align="center">0.00</td>
</tr>
<tr class="odd">
<td align="center">15.0</td>
<td align="center">90</td>
<td align="center">0.082</td>
<td align="center"></td>
<td align="center"></td>
<td align="center"></td>
<td align="center">102</td>
<td align="center">82</td>
<td align="center">0.045</td>
<td align="center">112</td>
<td align="center">46</td>
<td align="center">0.00</td>
</tr>
<tr class="even">
<td>UDP RX (Max)</td>
<td align="center">20.5</td>
<td align="center">91</td>
<td align="center">0.75</td>
<td align="center">70.6</td>
<td align="center">85</td>
<td align="center">0.82</td>
<td align="center">120</td>
<td align="center">85</td>
<td align="center">0.21</td>
<td align="center">203</td>
<td align="center">91</td>
<td align="center">0.10</td>
</tr>
<tr class="odd">
<td>UDP TX (Max)</td>
<td align="center">30.1</td>
<td align="center">100</td>
<td align="center">0.003</td>
<td align="center">101</td>
<td align="center">100</td>
<td align="center">0.02</td>
<td align="center">174</td>
<td align="center">100</td>
<td align="center">0.001</td>
<td align="center">339</td>
<td align="center">100</td>
<td align="center">0.001</td>
</tr>
</table>

<BR>

**Note:** VEPA intercore performance numbers for J742S2 are not published in this release. Will come in later releases.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History  {#ethfw_datasheet_rev_hist}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Revision | Date          | Author         | Description            | Status
---------|---------------|----------------|------------------------|----------------
1.1      | 28 Nov 2023   | Misael Lopez   | Added SDK 9.1 results  | Approved
1.2      | 25 Mar 2024   | Misael Lopez   | Added SDK 9.2 results  | Approved
1.3      | 26 Jul 2024   | Vaibhav Jindal | Added SDK 10.0 results | Approved

