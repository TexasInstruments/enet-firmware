# Layer-2 Switching & TCP/IP Apps{#demo_l2_switching_ndk_top}

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Introduction {#demo_l2_switching_intro}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

This application demonstrates basic Layer-2 switching with VLAN, multicast among the ports in the
Ethernet switch (CPSW9G) in the J721E device. The traffic forwarding process among the
ports don't require CPU involved or DMA bandwidth as everything is completely handled by
CPSW hardware.

The intention behind this demo is to show the switching capabilities of the J721E
integrated Ethernet Switch (CPSW9G) as well as the software developed which includes
CPSW LLD (low level driver for CPSW IP), TI NDK (TCP/IP) integration and Ethernet Switch firmware application.

Below are top-level features of demonstrated -

 - Basic L2 Switching
 - Switching with VLAN
 - Multicast switching
 - HTTP server
 - Send/Receive apps over TCP/UDP

The Ethernet Firmware demo application is in charge of -

 - Opening the CPSW modules like ALE, MAC ports, host port and UDMA
 - Opening & configuring the 4 x MAC ports along with corresponding PHYs present in the GESI expansion board at RGMII 1Gbps mode.
 - Initializing NDK stack.
 - Configuring the HTTP & TCP/IP data servers.

This application runs on the J721E EVM with GESI (Gateway/Ethernet Switch/Industrial Expansion Board) board.
The demo application has a HTTP server hosting a web page which can be accessed by any device connected to the
CPSW switch. The demo application also supports a basic serial terminal-based control interface to
enable/disable/configure features like VLAN, multicast, rate-limiting.

A video streaming application (Ex - Plex Media Server, VLC) can be used to demonstrate Ethernet packet switching functionality between multiple PCs. The media server will run on one PC and the client will be the accessed from another PC which are connected via GESI board. The media server can be accessed via web interface, so any laptop connected to the switch should be able to access it.

The switching demo uses www.plex.tv media server for showing video streaming.

    Note: Please check licensing information & terms of usage of plex.tv media server and make sure it
    adheres your organizations policy before using and configuring it.

The IP address of J721E EVM is used to access the TCP/IP demo webpage from any device connected to the CPSW switch.

![](demo_l2_switching_diagram.png "Layer-2 Switching Application Diagram")

Below diagram shows connections for video streaming connections.
![](demo_l2_switching_connections.png "Layer-2 Switching Demo connections diagram")

    Note: The IP addresses in above diagram can change based on your network configuration.
    Also use of Ubuntu laptop is not needed if DHCP server is available in your network.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Dependencies {#demo_l2_switching_depend}

This application depends on multiple components and are detailed in sections below:
    -# TI RTOS: Uses **Task**, **Semaphore**, **Interrupt Handling HWI** and **Profiling Utility**.
    -# PDK
        - Board library: Required for the configuration of pin muxing, clocking, etc.
        - OSAL library: Provides the abstraction layer implementation for TI RTOS
        - UART driver: Required to print output messages to serial port
        - UDMA driver: Required for global level initialization of the UDMA driver
        - CPSW driver: Provides an interface for the application to configure the
          control path of the CPSW switch, as well as the interface to send and
          receive Ethernet frames to/from CPSW's host port

[Back To Top](@ref demo_l2_switching_ndk_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Compile Time Configurations {#demo_l2_switching_demo_cfg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Not applicable.

[Back To Top](@ref demo_l2_switching_ndk_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Test Setup {#demo_l2_switching_setup_cfg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Steps to run {#demo_l2_switching_steps_top}

### Pre-requisites {#demo_l2_switching_steps_prerequisites}

 # PC Set up
-# Install Code Composer Studio and setup a <b>Target Configuration</b> for
   use with J721E EVM.(refer to @ref ethfw_instal_ccs)
-# Connect the emulator to the PC.

### PC Set up {#demo_pc_setup_steps}
-# Plex server set up
    1. Install Plex Media Server. The Ubuntu/Windows installation executable and instructions
       can be found in their webpage.
    2. Once setup, the media server will be started every time that the PC is powered on.
    3. Add video samples to the Library as needed.

-# Connect your laptops/PCs as per connection diagram shown above. Make sure you
   don't use PORT0 as it is not functional on GESI board.
-# Configure each laptop for static IP configuration. Configure laptop running plex server at
  192.168.1.101 and other laptop in 192.168.1.102. The J721E is configured for static IP
  192.168.1.100
  For configuring the static IP below can be refereed.
  https://www.howtogeek.com/howto/19249/how-to-assign-a-static-ip-address-in-xp-vista-or-windows-7/

### Steps {#demo_l2_switching_steps}

-# Open Code Composer Studio and launch the Target Configuration previously
   setup (refer to @ref demo_l2_switching_steps_prerequisites)

   ![](demo_l2_switching_steps_2.png "Launch CCS Target Configuration")

-# Go to the <b>View</b> menu and then select <b>Scripting Console </b>
-# Run the launch.js script provided in ethfw_xx_xx_xx/tools/ on scripting console to load and execute DMSC firmware binary. This step can take considerable time as it configures PLL etc. in the SOC via GEL files and configures DDR.

   ![](launch_dss_script.png "Launch script")

   After script completes execution you should see below in Debug window

   ![](launch_dss_script_complete.png "Launch script Complete")

-# In Code Composer Studio, select <b>MAIN_Cortex_R5_0_0</b> from the list of
   cores in the <b>Debug</b> panel

   ![](demo_l2_switching_steps_3.png "MAIN_Cortex_R5_0_0")

-# Go to the <b>Run</b> menu and then select <b>Load</b> -> <b>Load Program</b>

-# In the <b>Load Program</b> window, browse the EthFw Layer-2 switching application binary.
   It can be found at ethfw_xx_xx_xx/out/J721E/R5F/SYSBIOS/debug/ethfw_app_ndk_switch_tirtos_mcu_2_0.xer5f

   ![](demo_l2_switching_steps_4.png "Loading the demo application binary")

-# Go to the <b>Run</b> menu and then select <b>Resume</b> to start executing demo binary.

-# Once you see the IP address printed on console and all links detected by demo application. Run Plex client using below command

    http://192.168.1.101:32400/web

    ![](PlexClient.png "PlexClient.PNG")

### HTTP Client Page (http://192.168.1.100)
The following is snapshot of webpage loaded when client accesses HTTP server on J721E EVM.
![](tcpipdemopage.png "TCP/IP HTTP Server Landing Page")

-# Please refer to the @ref demo_l2_switching_output section for sample EVM test logs

[Back To Top](@ref demo_l2_switching_ndk_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Sample output {#demo_l2_switching_output}

Below is a sample log from the execution of this demo application.

### UART Console Logs

[Back To Top](@ref demo_l2_switching_ndk_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History {#demo_l2_switching_rev_history}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Revision | Date          | Author                 | Description
---------|---------------|------------------------|----------------------
0.1      | 01 Apr 2019   | Prasad J, Misael Lopez | Created for v.0.08.00
0.2      | 12 June 2019  | Prasad J               | Updates for EVM demo (.85 release)