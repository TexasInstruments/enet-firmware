# Ethernet Firmware differentiating features demos {#demo_ethfw_combined_top}

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Introduction {#ethfw_demo_intro}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

This application demonstrates basic Jacinto 7 integrated switch differentiating features
like InterVLAN Routing in Hardware, firewall, packet header based classification along with
Layer-2 switching with VLAN, multicast and Software based InterVLAN Routing among the ports.
The traffic forwarding process among the ports don't require CPU involved or DMA bandwidth as
everything is completely handled by CPSW hardware.

The intention behind this demo which encompasses multiple sub-demos is to show the switching
capabilities of the J721E integrated Ethernet Switch (CPSW9G) as well as the software developed
which includes CPSW LLD (low level driver for CPSW IP), TI NDK (TCP/IP)
integration and Ethernet Switch firmware(EthFw) application.

Below are top-level features demonstrated:

 - Basic L2 Switching
 - Switching with VLAN
 - Multicast switching
 - HTTP server
 - Send/Receive apps over TCP/UDP
 - Support for Remote Cores
 - Software based InterVLAN Routing
 - Hardware based InterVLAN Routing

The Ethernet Firmware demo application is in charge of:

 - Opening the CPSW modules like ALE, MAC ports, host port and UDMA
 - Opening & configuring the MAC ports along with corresponding PHYs
   present in the GESI expansion board at RGMII/RMII 1Gbps mode
 - Initializing NDK stack
 - Configuring the HTTP & TCP/IP data servers

This application runs on the J721E EVM with GESI (Gateway/Ethernet
Switch/Industrial Expansion Board) board. The demo application has a HTTP server
hosting a web page which can be accessed by any device connected to the CPSW
switch. A GUI based control interface to enable/disable/configure features like VLAN, multicast,
rate-limiting, InterVLAN Routing and also to show the load of the CPU is added in the release.

A video streaming application (Ex - Plex Media Server, VLC) can be used to
demonstrate Ethernet packet switching functionality between multiple PCs. The
media server will run on one PC and the client will be the accessed from another
PC which are connected via GESI board. The media server can be accessed via web
interface, so any laptop connected to the switch should be able to access it.

The switching demo uses www.plex.tv media server for showing video streaming.

> **Note:** Please check licensing information and terms of usage of Plex TV
> media server and make sure it adheres to your organization's policy before
> using and configuring it.

The IP address of J721E EVM is used to access the TCP/IP demo webpage from any
device connected to the CPSW switch.

![](demo_l2_switching_diagram.png "Layer-2 Switching Application Diagram")

Below diagram shows connections for video streaming connections.
![](demo_l2_switching_connections.png "Layer-2 Switching Demo connections diagram")

> **Note:** The IP addresses in above diagram can change based on your network
> configuration. Also use of Ubuntu laptop is not required if DHCP server is
> available in your network or if using static IP addresses.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Dependencies {#ethfw_demo_depend}

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
# Compile Time Configurations {#ethfw_demo_cfg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Not applicable.

[Back To Top](@ref demo_ethfw_combined_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Test Setup {#ethfw_demo_setup_cfg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Pre-requisites {#ethfw_demo_prerequisites}

### Plex server set up {#ethfw_demo_prereq_plextv}

-# Install Plex Media Server. The Ubuntu/Windows installation executable
   and instructions can be found in their [website](https://www.plex.tv/)
-# Once setup, the media server will be started every time that the PC is
   powered on.
-# Add video samples to the Library as needed.

### PackEth tool {#ethfw_demo_prereq_packEth}
-# Install PackEth packet generator tool on your Linux PC. The Ubuntu Installation
  instructions can be found in their [website](http://packeth.sourceforge.net/packeth/Installation.html)

> **Note:** Please check licensing information and terms of usage of PackEth tool
> and make sure it adheres to your organization's policy before
> using and configuring it.
### Python3 and Pip3 {#ethfw_demo_prereq_python}
  -# The GUI tool to send configurations is developed using
  Python3 and PyQt, and it requires various python modules. So install pip3 to get the modules installed in the easiest way.

        sudo apt install python3-pip
        pip3 install --user pyqt5
        sudo apt-get install python3-pyqt5
        sudo apt-get install pyqt5-dev-tools
        sudo apt-get install qttools5-dev-tools
        pip3 install jsonschema pyserial serial xmodem

### Setting static IPs {#ethfw_demo_static_ips}

-# As depicted in the previous connections diagram, the static IPs for all
   devices required in this demo can be set as follows:
    Device                                |  IP address
    ---------------------------------     | -------------
    PC running Plex server                | 192.168.1.202
    J721E when **enableStaticIP** = 1     | 192.168.1.203
    Laptop running Plex client            | 192.168.1.204
    J721E A72 Core with Virtual Net Driver| 192.168.1.205
    Default Gateway                       | 192.168.1.1
    Subnet Mask                           | 255.255.255.0

   * For Windows,

     Refer to the following
     [website](https://www.howtogeek.com/howto/19249/how-to-assign-a-static-ip-address-in-xp-vista-or-windows-7/)
     for suggested instructions about static IP configuration under a Windows
     environment.

   * For Linux,

         sudo ifconfig <ethDeviceName> 192.168.1.20x netmask 255.255.255.0 up

-# **Optional** - If static IP configuration is not possible, a local DHCP
   server can be setup in a Linux PC as shown in the connections diagram above.
   Otherwise, it's also possible to connect the **MAC Port 1** to a wider
   network running DHCP.


## CCS Boot {#ethfw_demo_CCS}

### Prerequisites {#demo_l2_switchin_CCS_prereqs}

Install Code Composer Studio and setup a <b>Target Configuration</b> for use
with J721E EVM. Refer to @ref ethfw_instal_ccs.

### Steps {#ethfw_demo_CCS_steps}

-# Connect a micro USB cable to JTAG port of J721E_EVM. The XDS110 JTAG
   connector is labeled `XDS110` (J3).

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
   * **Note:** Do not connect any device to **MAC Port 0** as it may not be
     functional, please refer to the @ref ethfw_known_issues sections for
     further details

-# For loading L2 Switching application binaries through CCS on J721E, please
   refer to @ref load_example_binaries_on_j7 section.


## SD Card Boot {#ethfw_demo_sdcard}

### Steps {#ethfw_demo_sdcard_steps}

-# Create a bootable SD card with Linux bootloader, kernel and filesystem.
   For details about SD card creation, refer to the Processor SDK Linux
   Automotive User's Guide.

-# Copy the demo application to the **firmware** directory of Linux filesystem in SD card:

       cp <SDK_INSTALL_PATH>/ethfw_xx_xx_xx/out/J721E/R5F/SYSBIOS/debug/_demo_.xer5f <MOUNT>/rootfs/lib/firmware/

-# Update the soft-link `j7-main-r5f0_0-fw` to point to the demo application
   copied to SD card in the previous step:

       cd <MOUNT>/rootfs/lib/firmware/
       ln -sf _demo_.xer5f j7-main-r5f0_0-fw

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


## HTTP Client Page (http://192.168.1.203) {#ethfw_http_client_page}

The following is a snapshot of webpage loaded when client accesses HTTP server
on J721E EVM.

![](tcpipdemopage.png "TCP/IP HTTP Server Landing Page")

[Back To Top](@ref demo_l2_switching_ndk_top)


## Plex TV Client {#ethfw_plex_tv_client_usage}

Once you see the IP address printed on console and all links detected by demo
application. Run Plex client by accessing the following address using your
favorite web browser: http://192.168.1.202:32400/web/index.html

![](PlexClient.png "Plex client interface")

## Virtual Net Driver on A72 {#ethfw_virt_net_driver}

Once the EVM is booted along with Linux on A72, the virtual net driver module should be loaded and the device "eth1" corresponding to CPSW9G should be added.

You can verify this by using "ifconfig -a" on Linux terminal console of the EVM.
Now set the static IP of A72 core as mentioned above. After this, you should be able
to transfer data with devices connected on the network.

## GUI Configurator tool {#ethfw_gui_tool_configuration}

After getting the IP address printed on the console, you can open the GUI tool
and connect to the J721E EVM.

    cd <SDK_INSTALL_PATH>/pdk/packages/ti/drv/cpsw/tools/cpsw_configclient
    sudo python3 switchconfig_client.py

You should be able to see a Window opening up as shown below.
![](cpswconfigurationtool.png "CPSW Remote Configuration Tool")

* Select the "SETTINGS" tab and enter your target IP as shown below.
![](cpsw_cfgtool_ipset.png "CPSW Remote Configuration Tool")

  Once the IP is set, the Main R5 Load progress bar will get updated periodically.

* Using the tool the Port statistics can be obtained using the "PORT STATISTICS" tab.

## Inter VLAN Routing {#ethfw_intervlan_routing}
### Test set up
![](demo_intervlan_routing_setup.png "Inter-VLAN test set up")

PC1 packEth configuration.

    * Pkt Header
    DST_MAC =02:00:00:00:00:02
    SRC MAC = 00:11:01:00:00:01
    VLAN ID=0x64
    SRC IP=192.168.106.128
    DST IP=192.168.108.128
    TTL = 255
    * Payload = 300 Bytes

Use Wireshark(R) in PC2 to capture and illustrate the received packet contents
- Verify that the SRC_MAC, DST_MAC, VLAN ID, TTL fields are all updated correctly
- Verify that the R5 CPU load is ~0, this is printed on the R5 console terminal (connect minicom to /dev/ttyUSB2
- Verify that packet receive rate using  bmon

    * Pkt Header
    DST_MAC = 00:11:02:00:00:01
    SRC MAC = 02:00:00:00:00:02
    VLAN ID=0x68
    SRC IP=192.168.108.128
    DST IP=192.168.106.128

    TTL = 255
    * Payload = 300 Bytes

The predefined packEth config files is available as part of docs.

### Software Inter VLAN Routing {#ethfw_sw_intervlan_routing}
  * Open the "CONFIGURATION FILE" tab of the GUI tool. Here you can load the pre-written
    configuration files and send them to the EVM to enable/disable features of the CPSW9G.
  * To enable, Software Inter VLAN Routing, click on the "Open" button and select the sw_intervlan_routing_config.txt file present in the "<SDK_INSTALL_PATH>/pdk/pdk/packages/ti/drv/cpsw/tools/cpsw_configclient/config_files" directory.
  * Always parse the configuration before sending to the EVM using "Parse" button to identify errors in the configuration.
  > **Note:** The list of allowed commands and the configurations are present in the "schemas.py" file in the "cpsw_configclient/inc/" directory.
  * Once the parsing succeeded, you can send the configuration using "Send Config" button.
  * Now that the Software based Inter VLAN routing is enabled, you can verify it by sending packets with VLAN ID tagged using packEth tool.
  * In the packEth tool on the PC with 192.168.1.202 IP address, enter the details as shown in the below picture.

  ![](packethswintervlan.png "PackEth settings for Software InterVLAN Routing")

  * The packets sent using packEth with IP address 192.168.1.202 and VLAN ID set to 0x64 (100 in decimal) will be routed to the PC with IP address 192.168.1.204 with VLAN ID changed to 0xC8 (200 in decimal). This can be verified using tools like WireShark on the receiver PC.
  * And also, if the packets are sent at a higher data rate, the CPU load will spike up, this can be clearly seen from the GUI tool.

### Hardware Inter VLAN Routing {#ethfw_hw_intervlan_routing}
  * Open the "CONFIGURATION FILE" tab of the GUI tool.
  * To enable, Hardware Inter VLAN Routing, click on the "Open" button and select the hw_intervlan_routing_config.txt file present in the "<SDK_INSTALL_PATH>/pdk/pdk/packages/ti/drv/cpsw/tools/cpsw_configclient/config_files" directory.
  * Always parse the configuration before sending to the EVM using "Parse" button to identify errors in the configuration.
  * Once the parsing succeeded, you can send the configuration using "Send Config" button.
  * Now that the Hardware based Inter VLAN routing is enabled, you can verify it by sending packets with VLAN ID tagged using packEth tool.
  * Change the configurations in PackEth tool as show below.

  ![](packethhwintervlan.png "PackEth settings for Hardware InterVLAN Routing")

  * The packets sent using packEth with IP address 192.168.1.201 and VLAN ID set to 0x64 (100 in decimal) will be routed to the PC with IP address 192.168.1.204 with VLAN ID changed to 0xC8 (200 in decimal). This can be verified using tools like WireShark on the receiver PC.
  * Since the routing is now offloaded to hardware, there will be no impact on the CPU load even if the data rate is increased to as high as 1Gbps.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Sample output {#ethfw_demo_output}

Below is a sample log from the execution of this demo application.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
### UART Console Logs

=======================================================
CPSW_9G Test on MAIN NAVSS
           CPSW L2 Switching APP
CpswPhy_bindDriver: PHY 0: OUI:080028 Model:23 Ver:01 <-> 'dp83867' : OK
=======================================================
CpswPhy_bindDriver: PHY 3: OUI:080028 Model:23 Ver:01 <-> 'dp83867' : OK
IPC_echo_test (core : mcu2_0) .....
Remote device (core : mcu2_1) .....
PHY 0 is alive
Remote demo device (core : mcu2_0) .....
PHY 3 is alive
PHY 12 is alive
PHY 15 is alive
PHY 16 is alive
PHY 17 is alive
PHY 18 is alive
PHY 23 is alive
Host MAC address: 70:ff:76:1d:87:8c
[NIMU_NDK] CPSW has been started successfully

CPSW NIMU application, IP address I/F 1: 192.168.1.203


 Rx Flow for Software Inter-VLAN Routing is up
Cpsw_handleLinkUp: port 3: Link up: 1-Gpbs Full-Duplex
Cpsw_handleLinkUp: port 2: Link up: 1-Gpbs Full-Duplex
Function:app_ethrdev_srv_cb_attach_ext_handler,HostId:0,CpswType:1
Function:app_ethrdev_srv_cb_register_mac_handler,HostId:0,Handle:a2b336c0,CoreKey:38acb7e60
Cpsw_ioctlInternal: CPSW: Registered MAC address.ALE entry:10, Policer Entry:0Function:app5

================LLI Table entries===========

Number of Static ARP Entries: 1

SNo.      IP Address         MAC Address
------    -------------      ---------------
0         192.168.1.205      70:FF:76:1D:87:8B

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[Back To Top](@ref demo_ethfw_combined_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History {#ethfw_demo_rev_history}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Revision | Date          | Author                 | Description
---------|---------------|------------------------|----------------------
0.1      | 01 Apr 2019   | Prasad J, Misael Lopez | Created for v.0.08.00
0.2      | 12 Jun 2019   | Prasad J               | Updates for EVM demo (.85 release)
0.3      | 17 Jul 2019   | Misael Lopez           | Updates for v.0.09.00
0.4      | 14 Oct 2019   | Santhana Bharathi N    | Updates for v.1.00.00