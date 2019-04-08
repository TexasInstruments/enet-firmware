# Layer-2 Switching {#demo_l2_switching_top}

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Introduction {#demo_l2_switching_intro}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

This application demonstrates basic Layer-2 switching among the ports in the
CPSW switch. The traffic forwarding process among the ports don't require
CPU involved or DMA bandwidth as everything is completely handled by
CPSW hardware.

This application runs on the VLAB simulator for J721E CPSW 9G peripheral.
A Python-based script is provided to generate Ethernet frames which are
injected to the external ports of the CPSW switch. The source and
destination ports are randomly chosen in order to guarantee that all ports
of the switch are properly verified for frame transmission and reception.

![](demo_l2_switching_diagram.png "Layer-2 Switching Application Diagram")

The following diagram shows interaction of J721E VLAB model, the VLAB test
script and the Layer-2 switching application.

![](demo_l2_switching_diagram2.png "Layer-2 Switching Application in VLAB")

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

[Back To Top](@ref demo_l2_switching_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Flow Chart {#demo_l2_switching_flowchart}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

![](demo_l2_switching_flowchart.png "Layer-2 Switching Flow Chart")

[Back To Top](@ref demo_l2_switching_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Compile Time Configurations {#demo_l2_switching_demo_cfg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Not applicable.

[Back To Top](@ref demo_l2_switching_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Test Setup {#demo_l2_switching_setup_cfg}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Steps to run {#demo_l2_switching_steps_top}

### Pre-requisites {#demo_l2_switching_steps_prerequisites}

-# Locate the Python-based scripts provided along with the source code of this
   application. It can be found at `./scripts/vlab_switch_test/switch_logic_9g.py`
-# Add scripts to VLAB PATH. Refer to VLAB user guide for further details on
   adding to VLAB PATH. Following are some ways to add scripts path to VLAB_PATH.

       set_preferences(vlab_path=["<ethfw_xx_yy_zz_bb>/scripts/vlab_switch_test"])

       vlab.path+=["<ethfw_xx_yy_zz_bb>/scripts/vlab_switch_test"]

-# You can copy the scripts into VLAB launch directory in case don't want to
   update VLAB path.
-# Install Code Composer Studio and setup a <b>Target Configuration</b> for
   use with VLAB


### Steps {#demo_l2_switching_steps}

-# Start VLAB using the following command

       load("<ethfw_xx_yy_zz_bb>/scripts/vlab_switch_test/run_switch_logic_9g.py")

-# Verify that the status of all cores in the <b>Status</b> panel are in loaded
   state as shown in picture below:

   ![](demo_l2_switching_steps_1.png "VLAB Status: all cores in loaded state")

-# Enable the UART terminal in order to get application messages sent to the
   UART port. Run below command in the VLAB terminal:

       vlab.display_terminal(vlab.terminal.mcu_island_usart0);

-# Open Code Composer Studio and launch the Target Configuration previously
   setup (refer to @ref demo_l2_switching_steps_prerequisites)

   ![](demo_l2_switching_steps_2.png "Launch CCS Target Configuration")

-# In Code Composer Studio, select <b>pulsar0_cr5f_0_proxy</b> from the list of
   cores in the <b>Debug</b> panel

   ![](demo_l2_switching_steps_3.png "pulsar0_cr5f_0_proxy")

-# Go to the <b>Run</b> menu and then select <b>Load</b> -> <b>Load Program</b>

-# In the <b>Load Program</b> window, browse the EthFw Layer-2 switching application binary.
   It can be found at ethfw_xx_xx_xx/out/J721E/R5F/SYSBIOS/debug/ethfw_app_switch_tirtos_mcu_2_0.xer5f

   ![](demo_l2_switching_steps_4.png "Loading the demo application binary")

-# Resume all other cores by selecting each of them from the list of cores in
   CCS' <b>Debug</b> panel, and then <b>Run</b> -> <b>Resume</b> or just hitting F8

   ![](demo_l2_switching_steps_5.png "Resume all other cores")

-# In VLAB, run the following command from VLAB terminal:

       run()

-# In Code Composer Studio, resume <b>pulsar0_cr5f_0_proxy</b> core by selecting it
   from the list of cores in the <b>Debug</b> panel, and then select <b>Run</b> ->
   <b>Resume</b> or just hitting F8

   ![](demo_l2_switching_steps_6.png "Resume MCU2_0 core")

-# In VLAB, verify that VLAB's <b>mcu_island_uart0</b> terminal displays all ports
   with MAC addresses as follows:

   ![](demo_l2_switching_steps_7.png "VLAB UART0 terminal displaying ports and MAC addresses")

-# In VLAB, run the Python-based demo script from the VLAB terminal. This script
   will generate and inject Ethernet frames in the external ports of the
   CPSW switch

       load("switch_logic_9g.py")

-# Please refer to the @ref demo_l2_switching_output section for sample VLAB test logs

[Back To Top](@ref demo_l2_switching_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Sample output {#demo_l2_switching_output}

Below is a sample log from the execution of this demo application.

### UART Console Logs

    Cpsw Loopback app: Iteration 0
    =================================
    CPSW_9G Test on MAIN NAVSS
    Host MAC address: 00:10:20:30:40:50
    port: 1 Mac address: 02:00:00:00:00:00
    port: 2 Mac address: 04:00:00:00:00:00
    port: 3 Mac address: 06:00:00:00:00:00
    port: 4 Mac address: 08:00:00:00:00:00
    port: 5 Mac address: 0a:00:00:00:00:00
    port: 6 Mac address: 0c:00:00:00:00:00
    port: 7 Mac address: 0e:00:00:00:00:00
    port: 8 Mac address: 10:00:00:00:00:00
    initQs() txFreePktInfoQ initialized with 128 pkts

### VLAB Console Logs

    VLAB 2.4.6
    Copyright (c) VLAB Works Pty Ltd, 2008-2018. All Rights Reserved.
    VLAB> load(u"/home/a0132233/vlab-works/utilities/simulator/astc/run_switch_logic_9g.py")

    Keystone Virtual Platform Toolbox (J7ES) 0.14.5-snapshot17


    Fast Models [11.2.37 (Dec 11 2017)]
    Copyright 2000-2017 ARM Limited.
    All Rights Reserved.

    C7X model based on Loki ver. 04.7.373 (clock: 50.000000 MHz)
    Debug server (CCS) started on port 23456
    Simulation paused at 0 s (delta 3)
    WARNING: break point will only be enabled when 'pulsar0_cr5f_0_proxy' completes its current execution block
    WARNING: break point requested by external debugger will only be enabled when 'pulsar0_cr5f_0_proxy' completes its current execution block
    VLAB> run()
    ARM Core Model: WARNING - Simulation code-translation cache failed to gain DMI for PC=0x00000000. Simulation performance will be reduced.
    VLAB> load("switch_logic_9g.py")
    tx_status: True
    receive_flushed: True
    Port1 received: EthernetFrame(0xffffffffffff, 0x1020304050, 518) :-)
    Port2 received: EthernetFrame(0xffffffffffff, 0x1020304050, 518) :-)
    Port3 received: EthernetFrame(0xffffffffffff, 0x1020304050, 518) :-)
    Port4 received: EthernetFrame(0xffffffffffff, 0x1020304050, 518) :-)
    Port5 received: EthernetFrame(0xffffffffffff, 0x1020304050, 518) :-)
    Port6 received: EthernetFrame(0xffffffffffff, 0x1020304050, 518) :-)
    Port7 received: EthernetFrame(0xffffffffffff, 0x1020304050, 518) :-)
    Port8 received: EthernetFrame(0xffffffffffff, 0x1020304050, 518) :-)
    receive flush done
    Port4 -> Port5: EthernetFrame(0xa0000000000, 0x80000000000, 200)
    Port5 received: EthernetFrame(0xa0000000000, 0x80000000000, 200) :-)
    Port5 -> Port2: EthernetFrame(0x40000000000, 0xa0000000000, 200)
    Port2 received: EthernetFrame(0x40000000000, 0xa0000000000, 200) :-)
    Port7 -> Port2: EthernetFrame(0x40000000000, 0xe0000000000, 200)
    Port2 received: EthernetFrame(0x40000000000, 0xe0000000000, 200) :-)
    Port4 -> Port1: EthernetFrame(0x20000000000, 0x80000000000, 200)
    Port1 received: EthernetFrame(0x20000000000, 0x80000000000, 200) :-)
    Port6 -> Port4: EthernetFrame(0x80000000000, 0xc0000000000, 200)
    Port4 received: EthernetFrame(0x80000000000, 0xc0000000000, 200) :-)
    Port5 -> Port2: EthernetFrame(0x40000000000, 0xa0000000000, 200)
    Port2 received: EthernetFrame(0x40000000000, 0xa0000000000, 200) :-)
    Port2 -> Port3: EthernetFrame(0x60000000000, 0x40000000000, 200)
    Port3 received: EthernetFrame(0x60000000000, 0x40000000000, 200) :-)
    Port2 -> Port3: EthernetFrame(0x60000000000, 0x40000000000, 200)
    Port3 received: EthernetFrame(0x60000000000, 0x40000000000, 200) :-)
    Port2 -> Port4: EthernetFrame(0x80000000000, 0x40000000000, 200)
    Port4 received: EthernetFrame(0x80000000000, 0x40000000000, 200) :-)
    Port3 -> Port4: EthernetFrame(0x80000000000, 0x60000000000, 200)
    Port4 received: EthernetFrame(0x80000000000, 0x60000000000, 200) :-)
    Port4 -> Port2: EthernetFrame(0x40000000000, 0x80000000000, 200)
    Port2 received: EthernetFrame(0x40000000000, 0x80000000000, 200) :-)
    Port7 -> Port1: EthernetFrame(0x20000000000, 0xe0000000000, 200)
    Port1 received: EthernetFrame(0x20000000000, 0xe0000000000, 200) :-)
    Port7 -> Port3: EthernetFrame(0x60000000000, 0xe0000000000, 200)
    Port3 received: EthernetFrame(0x60000000000, 0xe0000000000, 200) :-)
    Port2 -> Port1: EthernetFrame(0x20000000000, 0x40000000000, 200)
    Port1 received: EthernetFrame(0x20000000000, 0x40000000000, 200) :-)
    Port5 -> Port1: EthernetFrame(0x20000000000, 0xa0000000000, 200)
    Port1 received: EthernetFrame(0x20000000000, 0xa0000000000, 200) :-)
    Port5 -> Port2: EthernetFrame(0x40000000000, 0xa0000000000, 200)
    Port2 received: EthernetFrame(0x40000000000, 0xa0000000000, 200) :-)
    Port7 -> Port6: EthernetFrame(0xc0000000000, 0xe0000000000, 200)
    Port6 received: EthernetFrame(0xc0000000000, 0xe0000000000, 200) :-)
    Port2 -> Port5: EthernetFrame(0xa0000000000, 0x40000000000, 200)
    Port5 received: EthernetFrame(0xa0000000000, 0x40000000000, 200) :-)
    Port1 -> Port7: EthernetFrame(0xe0000000000, 0x20000000000, 200)
    Transmit Done
    Port7 received: EthernetFrame(0xe0000000000, 0x20000000000, 200) :-)
    No frame to receive
    Receive Done
    Done


Ethernet frames in the tests are generated randomly, so the output may be different
in reader's test logs.

[Back To Top](@ref demo_l2_switching_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History {#demo_l2_switching_rev_history}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Revision | Date          | Author                 | Description
---------|---------------|------------------------|----------------------
0.1      | 01 Apr 2019   | Prasad J, Misael Lopez | Created for v.0.08.00
