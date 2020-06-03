# Ethernet Firmware differentiating features demos {#demo_ethfw_combined_top}

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Introduction {#demo_ethfw_combined_intro}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

The applications that are part of this demo show Jacinto 7 integrated switch
differentiating features like interVLAN routing in hardware, firewall, packet
header based classification and rate limiting along with Layer-2 switching with
VLAN, multicast and software-based interVLAN routing among the ports.
The traffic forwarding process among the ports don't require CPU involvement
or DMA bandwidth as everything is completely handled by CPSW hardware.

The intention behind this demo which encompasses multiple sub-demos is to show
the switching capabilities of the J721E integrated Ethernet Switch (CPSW9G) as
well as the software developed which includes CPSW IP low-level driver (CPSW
LLD), TI NDK TCP/IP integration and Ethernet Switch Firmware (EthFw) application.

Below are top-level features demonstrated:

 - Basic L2 switching
 - Switching with VLAN
 - Multicast switching
 - HTTP server
 - Send/receive packets over TCP/UDP
 - Support for remote cores (Linux and TI RTOS)
 - Software-based interVLAN routing
 - Hardware-based interVLAN routing
 - IP next header filtering
 - MAC address based rate limiting
 - Time-synchronization using PTP
 - Multi-core time-synchronization with RTOS client

The Ethernet Firmware demo application is in charge of:

 - Opening the CPSW modules like ALE, MAC ports, host port and UDMA
 - Opening and configuring the MAC ports along with corresponding PHYs
   present in the GESI expansion board at RGMII/RMII 1Gbps mode
 - Initializing NDK stack
 - Configuring the HTTP and TCP/IP data servers

This application runs on the J721E EVM with GESI (Gateway/Ethernet
Switch/Industrial Expansion Board) board.  The demo requires two PCs running
Ubuntu connected to the GESI board in order to demonstrate the L2 switching
capabilities as well as to generate and monitor Ethernet traffic at different
stages of the demo.  The connection diagram is shown below.

![](demo_l2_switching_connections.png "EthFw demo connections diagram")

> **Note:** The IP addresses in above diagram can change based on your network
> configuration.

The demo application has a HTTP server hosting a web page which can be accessed
by any external device connected to the CPSW switch.

A GUI-based control interface to enable/disable/configure features like VLAN,
multicast, rate limiting, interVLAN routing and also to show the load of the
CPU is added in the release.

A video streaming application, like Plex or VLC, can be used to demonstrate
Ethernet packet switching functionality between multiple PCs. The media server
will run on one PC and the client(s) will run on other PC(s), all connected to
the switch via GESI board.

This demo uses [Plex media system](https://www.plex.tv/) for video streaming.
Plex clients can access media content via web interface, so any PC connected
to the switch can easily access it.

> **Note:** Please check licensing information and terms of usage of Plex TV
> media server and make sure it adheres to your organization's policy before
> using and configuring it.

A  Remote Client application for the Main R5F core 1 is also available as part
of this demo.  This application runs a local NDK stack on a virtual network
device which demonstrates the TI RTOS switch remote core integration.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Dependencies {#demo_ethfw_combined_depend}

This application depends on multiple components and are detailed in sections
below:
  -# TI RTOS: Uses **Task**, **Semaphore**, **Interrupt Handling HWI** and
     **Profiling Utility**.
  -# PDK
     - Board library: Required for the configuration of pin muxing, clocking, etc.
     - OSAL library: Provides the abstraction layer implementation for TI RTOS
     - UART driver: Required to print output messages to serial port
     - UDMA driver: Required for global level initialization of the UDMA driver
     - CPSW driver: Provides an interface for the application to configure the
       control path of the CPSW switch, as well as the interface to send and
       receive Ethernet frames to/from CPSW's host port

[Back To Top](@ref demo_ethfw_combined_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Compile Time Configurations {#demo_ethfw_combined_cfg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Not applicable.

[Back To Top](@ref demo_ethfw_combined_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Demo Setup {#demo_ethfw_combined_setup_cfg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Prerequisites {#demo_ethfw_combined_prerequisites}

### Plex server {#demo_ethfw_combined_prereq_plextv}

> **Note:** Plex server is required only in **PC 1**.

-# Install Plex Media Server. The Ubuntu/Windows installation executable
   and instructions can be found in their [website](https://www.plex.tv/).
   - It's recommended to disable Plex authentication on the local network
     because this demo is not connected to internet and will not be able
     to login otherwise. Follow the instructions in this
     [website](https://www.howtogeek.com/303282/how-to-use-plex-media-server-without-internet-access/).
-# Once setup, the media server will be started every time that the PC is
   powered on.
-# Add video samples to the Library as needed.

### packETH tool {#demo_ethfw_combined_prereq_packEth}

> **Note:** packETH tool is required only in **PC 1**.

Install packETH packet generator tool on the Linux PC. The Ubuntu installation
instructions can be found in their [website](http://packeth.sourceforge.net/packeth/Installation.html).

The packEth configurations used in this demo are included in the Ethernet Firmware package at
`<ETHFW_PATH>/docs/packeth_configurations/`

> **Note:** Please check licensing information and terms of usage of packETH
> tool and make sure it adheres to your organization's policy before using and
> configuring it.

### Python3 and Pip3 {#demo_ethfw_combined_prereq_python}

The CPSW Remote Configuration GUI tool is developed using Python3 and PyQt. Pip3
can be used to install additional Python modules required by the GUI tool.

> **Note:** The GUI tool can be executed from either **PC 1** or **PC 2**, so
> Python and its dependencies must be installed only on the selected PC.

Install Python3, PyQt, pip3 and other dependencies:

    sudo apt install python3-pip
    pip3 install --user pyqt5
    sudo apt-get install python3-pyqt5
    sudo apt-get install pyqt5-dev-tools
    sudo apt-get install qttools5-dev-tools
    pip3 install jsonschema pyserial serial xmodem

### Wireshark {#demo_ethfw_combined_prereq_wireshark}

> **Note:** Wireshark packet analyzer tool is required in both **PC 1** and
> **PC 2**.

Refer to the Wireshark installation instructions on Ubuntu in this
[website](https://linuxhint.com/install_wireshark_ubuntu/).

### iperf

> **Note:** iperf network performance measurement tool is required on either
> **PC 1** or **PC 2**.

Install iperf in the selected Ubuntu PC(s):

    sudo apt-get install iperf

### bmon

> **Note:** bmon is required only on **PC 2**.

bmon is a network bandwidth monitoring tool that will be used in this demo to
monitor the traffic received on **PC 2** during the interVLAN tests.

Install bmon in the Ubuntu PC as follows:

    sudo apt-get install bmon

### DHCP Server {#demo_ethfw_dhcp_server}

> **Note:** DHCP server is required only in **PC 1**.

A DHCP server is required to assign IPs dynamically to all internal cores
(A72, Main R5F core0, Main R5F core1) or external devices (PC 1, PC 2) in this
demo.

-# Refer to the DHCP installation and setup instructions on the Ubuntu
   [website](https://help.ubuntu.com/lts/serverguide/dhcp.html) for further
   details.

-# A possible configuration could be:

       subnet 192.168.1.0 netmask 255.255.255.0 {
           range 192.168.1.200 192.168.1.210;
           ...
       }

-# Set the **PC 1** IP to `192.168.1.<pc1>` and the restart the DHCP server.

-# **Optional** - If dynamic IP configuration is not possible, static IPs can
   be setup as follows:

   * For Linux,

         sudo ifconfig <ethDeviceName> 192.168.1.x netmask 255.255.255.0 up

   * For Windows, refer to the following
     [website](https://www.howtogeek.com/howto/19249/how-to-assign-a-static-ip-address-in-xp-vista-or-windows-7/)
     for suggested instructions about static IP configuration under a Windows
     environment.

    Device                              |  IP address
    ----------------------------------- | -------------
    PC 1 (Plex server)                  | 192.168.1.202
    J721E Main R5F core (running EthFw) | 192.168.1.203
    PC 2 (Plex client)                  | 192.168.1.204
    J721E A72 core (virtual net driver) | 192.168.1.205
    Default Gateway                     | 192.168.1.1
    Subnet Mask                         | 255.255.255.0

### PTP stack {#demo_ethfw_ptp_stack}

> **Note:** PTP stack is required only on **PC 2**.

PTP stack is required to run master clock and synchronize with the slave
running on EVM.

-# Check for hardware timestamping support,

 * In Ubuntu PC terminal, enter the command as follows:,
              
       ~]# ethtool -T eth3
       Time stamping parameters for eth3:
       Capabilities:
              hardware-transmit     (SOF_TIMESTAMPING_TX_HARDWARE)
              software-transmit     (SOF_TIMESTAMPING_TX_SOFTWARE)
              hardware-receive      (SOF_TIMESTAMPING_RX_HARDWARE)
              software-receive      (SOF_TIMESTAMPING_RX_SOFTWARE)
              software-system-clock (SOF_TIMESTAMPING_SOFTWARE)
              hardware-raw-clock    (SOF_TIMESTAMPING_RAW_HARDWARE)
       PTP Hardware Clock: 0
       Hardware Transmit Timestamp Modes:
              off                   (HWTSTAMP_TX_OFF)
              on                    (HWTSTAMP_TX_ON)
       Hardware Receive Filter Modes:
              none                  (HWTSTAMP_FILTER_NONE)
              all                   (HWTSTAMP_FILTER_ALL)
where eth3 is the interface you want to check.

 * For software time stamping support, the parameters list should include:

       SOF_TIMESTAMPING_SOFTWARE 

       SOF_TIMESTAMPING_TX_SOFTWARE 

       SOF_TIMESTAMPING_RX_SOFTWARE 

 * For hardware time stamping support, the parameters list should include:

       SOF_TIMESTAMPING_RAW_HARDWARE 

       SOF_TIMESTAMPING_TX_HARDWARE 

       SOF_TIMESTAMPING_RX_HARDWARE 

-# Install PTP stack in the Ubuntu PC as follows:

       sudo apt install linuxptp

-# Start PTP master: 

       sudo ptp4l -P -2 -S -i eth3 -m -q -p -l 7 /dev/ptp0 

Replace -S with -H if your NIC supports hardware timestamping.

[Back To Top](@ref demo_ethfw_combined_top)


## CCS Boot {#demo_ethfw_combined_CCS}

### Prerequisites {#demo_l2_switchin_CCS_prereqs}

Install Code Composer Studio and setup a <b>Target Configuration</b> for use
with J721E EVM. Refer to @ref ethfw_instal_ccs.

### Steps {#demo_ethfw_combined_CCS_steps}

-# Connect a micro USB cable to JTAG port of J721E_EVM. The XDS110 JTAG
   connector is labeled `XDS110` (J3).  Alternatively, XDS560v2 debugger can
   be connected to the JTAG connected labeled `JTAG MIPI` (J16).

-# Connect a micro USB cable to MAIN Domain UART port on J721E_EVM. It's
   labeled `UART` (J44).

-# Set EVM's DIP switches `SW8` and `SW9` for no-boot mode:
   * SW8 = 10001000
   * SW9 = 01110000

-# Open up a serial terminal for UART2 communication. This terminal will show
   logs from MCU2_0 core where the demo application runs.
   * Set serial parameters to: 115200 8N1.
   * Set hardware and software flow control to "No".
   * Below figure shows serial parameters set in Minicom.

   ![](demo_l2_switching_minicom.png "Serial Port Settings in Minicom")

-# Power on the J721E EVM board. Ensure that SD card is not present or QSPI
   flashed.

-# Connect the laptops/PCs as per demo connections diagram above.
   * **Important:** DHCP server (if required) must be connected to
     **MAC Port 1**.
   * **Note:** Do not connect any device to **MAC Port 0** if using J7 EVM alpha
     version as it may not be functional, please refer to the
     @ref ethfw_known_issues sections for further details

-# Load application binaries to Main R5F cores in the following sequence:
   * Load Main R5F core 0: app_remoteswitchcfg_server.xer5f
   * Load Main R5F core 1: app_remoteswitchcfg_client.xer5f
   * Run Main R5F core 1
   * Run Main R5F core 0
   * **Note:** For loading demo application binaries through CCS on J721E,
     please refer to CCS setup section in SDK top level documentation.

-# Start Runtime Object View (ROV) in CCS for the Main R5F core 1 and navigate
   to the SysMin component in order to see the MCU2_1 client application's logs.
   This application doesn't use an UART port for logging.

> **Note:** Linux running on A72 core is not compatible with CCS boot mode.

[Back To Top](@ref demo_ethfw_combined_top)


## SD Card Boot {#demo_ethfw_combined_sdcard}

### Steps {#demo_ethfw_combined_sdcard_steps}

-# Create a bootable SD card with Linux bootloader, kernel and file system.
   For details about SD card creation, refer to the Processor SDK Linux
   Automotive User's Guide.

-# Copy the demo application to the `firmware` directory of Linux file system
   in SD card:

       cp <SDK_INSTALL_PATH>/ethfw_xx_xx_xx/out/J721E/R5F/SYSBIOS/debug/app_remoteswitchcfg_server.xer5f <MOUNT>/rootfs/lib/firmware/

-# Update the soft-link `j7-main-r5f0_0-fw` to point to the demo application
   copied to SD card in the previous step:

       cd <MOUNT>/rootfs/lib/firmware/
       ln -sf app_remoteswitchcfg_server.xer5f j7-main-r5f0_0-fw

-# **Optional:** Copy the remote client application to the `firmware` directory
   of Linux filesystem in SD card and update soft-link:

       cp <SDK_INSTALL_PATH>/ethfw_xx_xx_xx/out/J721E/R5F/SYSBIOS/debug/app_remoteswitchcfg_client.xer5f <MOUNT>/rootfs/lib/firmware/
       cd <MOUNT>/rootfs/lib/firmware/
       ln -sf app_remoteswitchcfg_client.xer5f j7-main-r5f0_1-fw

-# Connect a micro USB cable to MAIN Domain UART port on J721E_EVM. It's
   labeled `UART` (J44).

-# Set EVM's DIP switches `SW8` and `SW9` for SD card boot:
   * SW8 = 10000010
   * SW9 = 00000000

-# Open up a serial terminal for UART0 communication. This terminal will show
   logs from Linux bootloader and kernel.
   * Set serial parameters to: 115200 8N1.

-# Open up a serial terminal for UART2 communication. This terminal will show
   logs from MCU2_0 core where the demo application runs.
   * Set serial parameters to: 115200 8N1.
   * Set hardware and software flow control to "No".
   * Below figure shows serial parameters set in Minicom.

   ![](demo_l2_switching_minicom.png "Serial Port Settings in Minicom")

-# Insert SD card into slot labeled `MICRO SD` and power on the J721E EVM board.

[Back To Top](@ref demo_ethfw_combined_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Running the Demo {#demo_ethfw_combined_execution}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Connecting External Devices {#ethfw_demo_connections}

-# Connect **PC 1** to MAC port 3 of GESI board. Refer to the
   [J721E EVM GESI Expansion Board](@ref ethfw_depend_evm_gesi_j721e) section to
   find the right RJ-45 connector.

-# Connect **PC 2** to MAC port 2 of GESI board.

> **Note:** The demo application in this release assumes that external devices,
> **PC 1** and **PC 2**, are connected prior to starting the demo.  It's a
> **mandatory step**.

The IPs assigned dynamically to Main R5F cores 0 and 1 will be printed in the
UART2 serial terminal.


## HTTP Server {#ethfw_http_client_page}

> **Note:** HTTP server support is removed from NDK_3_75_01_01, so currently HTTP server is not supported in ETHFW. From 7.1 release, NS tools will be used to enable HTTP server.

A HTTP server is also part of the demo application running in the Main R5F
core 0. The following is a snapshot of the webpage loaded when client accesses
the HTTP server on J721E EVM using a web browser: `http://192.168.1.<r5f_0>`.

![](tcpipdemopage.png "TCP/IP HTTP Server Landing Page")

Also, if Main R5F core 1 has been loaded with the remote client application,
then a second HTTP server running on that core can be access from either PC
connected to the switch using a web browser: `http://192.168.1.<r5f_1>`.

[Back To Top](@ref demo_ethfw_combined_top)


## Plex TV {#ethfw_plex_tv_usage}

### Plex TV Server {#ethfw_plex_tv_server_usage}

Plex TV server running on **PC 1** requires an initial setup covered in the
[Prerequisites](@ref demo_ethfw_combined_prereq_plextv) section. Note that
Plex server may required to be explicitly launched after PC has been booted.

### Plex TV Client {#ethfw_plex_tv_client_usage}

Run Plex client from **PC 2** by accessing the following address using your
favorite web browser: `http://192.168.1.<pc1>:32400/web/index.html`

![](PlexClient.png "Plex client interface")

[Back To Top](@ref demo_ethfw_combined_top)


## Virtual Net Driver on A72 {#ethfw_virt_net_driver}

Once the EVM is booted along with Linux on A72, the virtual net driver module
should be loaded and the `eth1` network device corresponding to CPSW9G should
be added.

-# Verify this by running `ifconfig -a` on Linux terminal console of the EVM.

-# Activate network interface on A72 core as follows:

       sudo ifconfig eth1 up

-# At this point, data transfer with other devices connected to the network
   should be possible. Ping the two PCs connected to the switch:

       ping 192.168.1.<pc1>
       ping 192.168.1.<pc2>

-# Similarly, ping the A72 core for either PC connected to the switch:

       ping 192.168.1.<a72>

[Back To Top](@ref demo_ethfw_combined_top)


## iperf {#ethfw_running_iperf}

The CPSW switch is capable of steering network traffic without CPU intervention
by classifying it based on its characteristics.  This can be demonstrated by
running iperf server on Linux running on the A72 core and iperf client on any of
the external devices, **PC 1** or **PC 2**.

-# Start iperf server on Linux running on A72.

       iperf -s

-# Run iperf client on the selected PC.  Set test duration with `-t` option as
   needed.

       iperf -c 192.168.1.<a72> -t 20 -i 1

-# Simultaneously, access the HTTP server at `http://192.168.1.<r5f_0>` from the
   same PC shows traffic being steered towards different processing cores (A72
   or R5F).

[Back To Top](@ref demo_ethfw_combined_top)


## GUI Configurator Tool {#ethfw_gui_tool_configuration}

-# After getting the IP address printed on the console, launch the GUI tool:

       cd <SDK_INSTALL_PATH>/pdk/packages/ti/drv/cpsw/tools/cpsw_configclient
       sudo python3 switchconfig_client.py

    You should be able to see a window opening up as shown below.

    ![](cpswconfigurationtool.png "CPSW Remote Configuration Tool")

-# Select the **SETTINGS** tab and enter the target IP `192.168.1.<r5f_0>` as
   shown below.

   ![](cpsw_cfgtool_ipset.png "CPSW Remote Configuration Tool")

   Once the IP is set, the **Main R5 Load** progress bar will get updated
   periodically.

-# Using the tool the Port statistics can be obtained using the **PORT
   STATISTICS** tab.

[Back To Top](@ref demo_ethfw_combined_top)


## InterVLAN Routing {#ethfw_intervlan_routing}

### Software InterVLAN Routing {#ethfw_sw_intervlan_routing}

-# Open the **CONFIGURATION FILE** tab of the GUI tool. Configuration files
   can be sent to the switch in order to enable or disable features of the
   CPSW9G.

-# To enable software-based interVLAN routing, click on the **Open** button
   and select the `sw_intervlan_routing_config.txt` file present in the
   `<SDK_INSTALL_PATH>/pdk/packages/ti/drv/cpsw/tools/cpsw_configclient/config_files`
   directory.
   * **Note:** The list of allowed commands and the configurations are present in
     the `schemas.py` file in the `cpsw_configclient/inc` directory.

-# Press **Send Config** button to send the configuration to the switch.

-# Now that the software-based interVLAN routing is enabled, the functionality
   can be verified by sending packets with VLAN ID using packETH tool.

-# In the packETH tool on the **PC 1**, which has IP address `192.168.1.<pc1>`,
   load the `swintervlanrouting` configuration file from `<ETHFW_PATH>/docs/packeth_configurations/` directory.

   The loaded configuration should match with the below picture.


   ![](packethswintervlan.png "packETH settings for software interVLAN routing")

-# packETH configuration for software interVLAN routing:
   * Destination MAC = `02:00:00:00:00:02`
   * Source MAC = `00:11:01:00:00:01`
   * VLAN ID = 0x64
   * Source IP = `192.168.1.202`
   * Destination IP = `192.168.1.204`
   * TTL = 255
   * Payload = 300 bytes

   Note that source and destination IP address don't have to match either
   **PC 1** or **PC 2** address.  They match the IP address in the
   `sw_intervlan_routing_config.txt` config file, so they must not be changed.

-# The packets sent with the above configuration will be routed to the **PC 2**
   with IP address `192.168.1.<pc2>` and the VLAN ID will be changed to 0xC8
   (200 in decimal). This can be verified using tools like Wireshark on the
   receiver PC.

-# The received packets should have the following header:
   * Destination MAC = `00:11:02:00:00:01`
   * Source MAC = `02:00:00:00:00:02`
   * VLAN ID = 0xC8
   * Source IP = `192.168.1.202`
   * Destination IP = `192.168.1.204`
   * TTL = 254
   * Payload = 300 bytes

-# Run bmon tool on **PC 2** to monitor the bandwidth of the traffic being
   received from the switch.

-# If packets are sent at a higher data rate, the CPU load will spike up.  This
   can be clearly seen from the GUI tool.

### Hardware InterVLAN Routing {#ethfw_hw_intervlan_routing}

-# Open the **CONFIGURATION FILE** tab of the GUI tool.

-# To enable hardware-based interVLAN routing, click on the **Open** button and
   select the `hw_intervlan_routing_config.txt` file present in the
   `<SDK_INSTALL_PATH>/pdk/packages/ti/drv/cpsw/tools/cpsw_configclient/config_files`
   directory.

-# Press **Send Config** button to send the configuration to the switch.

-# Now that the hardware-based interVLAN routing is enabled, the functionality
   can be verified by sending packets with VLAN ID using packETH tool.

-# Load the `hwintervlanrouting` configuration file from `<ETHFW_PATH>/docs/packeth_configurations/` directory.

   The loaded configuration should match with the below picture.

   ![](packethhwintervlan.png "packETH settings for hardware interVLAN routing")

-# packETH configuration for hardware interVLAN routing:
   * Destination MAC = `02:00:00:00:00:02`
   * Source MAC = `00:11:01:00:00:01`
   * VLAN ID = 0x64
   * Source IP = `192.168.1.201`
   * Destination IP = `192.168.1.204`
   * TTL = 255
   * Payload = 300 Bytes

   Note that source and destination IP address don't have to match either
   **PC 1** or **PC 2** address.  They match the IP address in the
   `hw_intervlan_routing_config.txt` config file, so they must not be changed.

-# The packets sent with the above configuration will be routed to the **PC 2**
   with IP address `192.168.1.<pc2>` and the VLAN ID will be changed to 0xC8
   (200 in decimal). This can be verified using tools like Wireshark on the
   receiver PC.

-# The received packets should have the following header:
   * Destination MAC = `00:11:02:00:00:01`
   * Source MAC = `02:00:00:00:00:02`
   * VLAN ID = 0xC8
   * Source IP = `192.168.1.201`
   * Destination IP = `192.168.1.204`
   * TTL = 254
   * Payload = 300 bytes

-# Run bmon tool on **PC 2** to monitor the bandwidth of the traffic being
   received from the switch.

-# Since the routing is now offloaded to hardware, there will be no impact on
   the CPU load even for data rates as high as 1Gbps.

[Back To Top](@ref demo_ethfw_combined_top)


## IP Next Header Filtering {#ethfw_ip_nxthdr_filtering}

CPSW9G supports whitelisting of up to four different IP protocols for a VLAN
group.  This demo white-lists TCP and UDP protocols and hence blocking packets
of other protocols in the VLAN network.

-# Add a VLAN entry with `vlanId: 0x2BC (700 in decimal)` with host port, MAC
   ports 2 and 3 as members of the VLAN group.

-# Open the **CONFIGURATION FILE** tab of the GUI tool.

-# To add the above mentioned VLAN entry, click on the **Open** button and
   select the `ip_nxt_hdr_whitelisting_config.txt` file present in the
   `<SDK_INSTALL_PATH>/pdk/packages/ti/drv/cpsw/tools/cpsw_configclient/config_files`
   directory.

-# Press **Send Config** button to send the configuration to the switch.

-# Load the `ipnxthdr_tcp` configuration file from
   `<ETHFW_PATH>/docs/packeth_configurations/` directory to the packEth tool and
   start sending packets.

-# Since TCP is whitelisted, the packets will be received at **PC 2**. This can
   be verified by using Wireshark in **PC 2** with `ip.addr eq 192.168.1.202 && vlan`
   filter.

-# Similarly, `ipnxthdr_udp` packETH configuration can be used to verify UDP.

-# Since the ICMP protocol is not whitelisted, packets sent using
   `ipnxthdr_icmp_echorequest` from packETH won't be received at **PC 2**.

[Back To Top](@ref demo_ethfw_combined_top)


## Rate Limiting {#ethfw_rate_limiting}

-# Rate Limiting can be enabled by adding a policer entry with parameters like
   Source and Destination MAC address of the traffic to be limited.  The rate at
   which the traffic is limited is based on the values of Peak Information Rate
   (PIR) and Committed Information Rate (CIR) both in bits per second (bps) set
   in the policer entry.

-# Open the **CONFIGURATION FILE** tab of the GUI tool.

-# To enable rate limiting, click on the **Open** button and select the
   `rate_limiting_config.txt` file present in the
   `<SDK_INSTALL_PATH>/pdk/packages/ti/drv/cpsw/tools/cpsw_configclient/config_files`
   directory.

-# Press **Send Config** button to send the configuration to the switch.

-# Load the `ratelimiting` configuration file from
   `<ETHFW_PATH>/docs/packeth_configurations/` directory to the packETH tool
   and stat sending packets at a rate more than 200 Mbps.

-# The packets received at the **PC 2** will not exceed the receive rate of
   200Mbps (~25MBps), since the PIR is set to 200 Mbps. This can be verified by
   checking the receive rate using `bmon` or `System Monitor` in **PC 2**.


[Back To Top](@ref demo_ethfw_combined_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Sample output {#demo_ethfw_combined_output}

Below is a sample log from the execution of this demo application.


### UART Console Logs (MCU2_0 Server Application) {#demo_ethfw_combined_logs_uart}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Enabling clocks for CPSW_9G!
=======================================================
            CPSW Ethernet Firmware                     
=======================================================
CPSW_9G Test on MAIN NAVSS
CpswPhy_bindDriver: PHY 12: OUI:080028 Model:23 Ver:01 <-> 'dp83867' : OK
CpswPhy_bindDriver: PHY 0: OUI:080028 Model:23 Ver:01 <-> 'dp83867' : OK
CpswPhy_bindDriver: PHY 3: OUI:080028 Model:23 Ver:01 <-> 'dp83867' : OK
CpswPhy_bindDriver: PHY 15: OUI:080028 Model:23 Ver:01 <-> 'dp83867' : OK
PHY 0 is alive
PHY 3 is alive
PHY 12 is alive
PHY 15 is alive
PHY 23 is alive

ETHFW Version   : 0.01.01
ETHFW Build Date: Jun  3, 2020
ETHFW Build Time: 17:05:18
ETHFW Commit SHA: 269c245e

Host MAC address: IPC_echo_test (core : mcu2_0) .....
70:ff:76:1d:92:c2
Remote demo device (core : mcu2_0) .....
Host MAC address: Function:CpswProxyServer_attachExtHandlerCb,HostId:4,CpswType:1
70:ff:76:1d:92:c2
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:5000d,InArgsLen:0, OutArgsLen:4 
[NIMU_NDK] CPSW has been started successfully
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:20000,InArgsLen:24, OutArgsLen:4 
Function:CpswProxyServer_registerMacHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, MacAddress:70:ff:76:1d:92:c3, FlowIdx:178, FlowIdxOffset:6
Cpsw_ioctlInternal: CPSW: Registered MAC address.ALE entry:11, Policer Entry:0Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Function:CpswProxyServer_ioctlHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, Cmd:10003,InArgsLen:1, OutArgsLen:1 
Cpsw_handleLinkUp: port 2: Link up: 1-Gbps Full-Duplex
Cpsw_handleLinkUp: port 3: Link up: 1-Gbps Full-Duplex
Function:CpswProxyServer_registerIpv4MacHandlerCb,HostId:4,Handle:a2cfbbf8,CoreKey:38acb976, MacAddress:70:ff:76:1d:92:c3 IPv4Addr:192.168.10.21

CPSW NIMU application, IP address I/F 1: 192.168.10.19


================LLI Table entries=========== 

Number of Static ARP Entries: 1 
Rx Flow for Software Inter-VLAN Routing is up

SNo.      IP Address         MAC Address  
------    -------------      --------------- 
1         192.168.10.21      70:FF:76:1D:92:C3
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


### SysMin Logs (MCU2_1 Client Application) {#demo_ethfw_combined_logs_sysmin}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Remote Device Framework Endpoint locate failed. Retrying !!!
Remote Device Framework Endpoint locate failed. Retrying !!!
Remote Device Framework Endpoint located. Remote Core Id:3, Remote End Point:26
Registered a device name = mcu_2_1_ethswitch-device-0, id = 0, type = 3
ETHFW Version: 0. 1. 1
ETHFW Build Date (YYYY/MMM/DD):2020/Jun/ 3
ETHFW Commit SHA:269c245e
ETHFW PermissionFlag:0x7ffffff, UART Connected:true,UART Id:2Function:CpswProxy_cmdHandler,Handle:@a2cfbbf8,CoreKey:38acb976, RxMtu:1518, TxMtu:2024:2024:2024:2024:2024:2024:2024:2024, TxCsumEnabled:1
[NIMU_NDK] Registration of the CPSW Successful
CPSW NIMU application, IP address I/F 1: 192.168.10.21

Current Synchronized time in Epoch format: ld
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


[Back To Top](@ref demo_ethfw_combined_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History {#demo_ethfw_combined_rev_history}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Revision | Date          | Author                 | Description
---------|---------------|------------------------|----------------------
0.1      | 01 Apr 2019   | Prasad J, Misael Lopez | Created for v.0.08.00
0.2      | 12 Jun 2019   | Prasad J               | Updates for EVM demo (.85 release)
0.3      | 17 Jul 2019   | Misael Lopez           | Updates for v.0.09.00
0.4      | 14 Oct 2019   | Santhana Bharathi N    | Updates for v.1.00.00
0.5      | 03 Jun 2020   | Santhana Bharathi N    | Updates for v.7.00.00 (Updated logs and added instructions for TimeSync)