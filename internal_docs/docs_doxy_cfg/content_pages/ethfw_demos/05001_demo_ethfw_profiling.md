# CAN Profiling Application {#demo_can_profile_top}

[TOC]

# Introduction {#demo_can_profile_intro}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This application measures the CPU cycles required for transmission of N number
of packets for a given baud-rate and CAN configurations.

Simulates transmission of N number of CAN messages (64 bytes, CAN FD message)
the received messages (when in loopback mode) is copied to local variable
simulating a copy of the message by communication stack.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Dependencies {#demo_can_profile_depend}
This application depends on multiple components and are detailed in sections below
    -# TI RTOS : Uses <b>Task</b>, <b>semaphore</b>, <b>Interrupt Handling HWI</b> and <b>Profiling Utility</b>.
    -# MCAL
        - CAN Driver
        - Dio Driver to setup pins required for TX/RX
    -# MCAL BSW Stubs
        Stubs at (SDK Install Directory)/mcusw_xx.yy.xx.bb/mcuss_demos/Bsw_Stubs
        are used. Functions CanIf_Init (), CanIf_TxConfirmation () and CanIf_RxIndication () are required.
    -# MCAL Configurations
        CAN & DIO Configurations at (SDK Install Directory)/mcusw_xx.yy.xx.bb/mcuss_demos/mcal_config are used.

[Back To Top](@ref demo_can_profile_top)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Flow Chart {#demo_can_profile_flowchart}

![](demo_can_profile_flowchart.png "CAN Profiling Application")

[Back To Top](@ref demo_can_profile_top)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Compile Time Configurations {#demo_can_profile_demo_cfg}

-# APP_NUM_MSG_PER_ITERATION
    Controls the number of messages that would be sent per iteration
-# APP_NUM_ITERATION
    Number of iterations, total can messages sent would be APP_NUM_MSG_PER_ITERATION * APP_NUM_ITERATION
-# APP_INSTANCE_1_INST_IN_CFG_ONLY
    Use first instance of CAN peripheral configured. Useful when operating in
    TX only mode with an external CAN utility / CAN bus.

-# Enable transmit only mode
   In transmit only mode,this application measures the CPU cycles required for transmission of N number
   of packets for a given baud-rate and CAN configurations.These messages can be received if external
   CAN utility like CANOE,PEAK are connected to the IDK application board which has 2 CAN ports.
   Below is the configuration.
   - APP_INSTANCE_1_INST_IN_CFG_ONLY
    Use first instance of CAN peripheral configured. This macro should be enabled for tx only mode.
   - CAN_LOOPBACK_ENABLE
    Disable internal loopback  mode.
    CAN Configuration is present at (SDK Install Directory)/mcusw_xx.yy.xx.bb/mcuss_demos/mcal_config/Can_Demo_Cfg.

[Back To Top](@ref demo_can_profile_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Test Setup / Configurations used {#demo_can_profile_setup_cfg}

## Loopback mode
    -# Supported EVM / SoC
![](Can_Profiling_CanLoopbackMode.png "Loopback Setup")
## TX Only mode
   -# CAN_HIGH of all the nodes on the bus shall be connected together.
      Similary CAN_LOW of all the nodes on the bus shall be connected together.
![](Can_Profiling_CanTxOnlySetup.png "TxOnly Setup")

[Back To Top](@ref demo_can_profile_top)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## Steps to run {#demo_can_profile_steps_2_run}

-# Build the demo application as detailed in [User Guide] (@ref mcusw_build_all_demos)
-# Steps to run is detailed in [User Guide] (@ref mcusw_run_eg)

[Back To Top](@ref demo_can_profile_top)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Sample output Loop back mode {#demo_can_profile_output_loopback}
Below is the log from one of the runs, when CAN is configured in loop-back mode.

    CAN Profile App:Variant - Pre Compile being used !!!
    CAN Profile App: Successfully Enabled CAN Transceiver
    CAN Profile App:Will Transmit & Receive10000 Messages, 5 times
    CAN Profile App:NOTE : Operating in internal loop-back mode
    CAN Profile App:Transmit & Receive 10000 packets 5 times
    CAN Profile App:Average of 10242 packets in 1 second with CPU Load 14%
    CAN Profile App:Measured Load: Total CPU: 17%, HWI: 10%, SWI:0% TSK: 4%
    CAN Profile App:Message Id Received 800000c0 Message Length is 64
    CAN Profile App:Test completed for 0 instance

    CAN Profile App:Will Transmit & Receive10000 Messages, 5 times
    CAN Profile App:NOTE : Operating in internal loop-back mode
    CAN Profile App:Transmit & Receive 10000 packets 5 times
    CAN Profile App:Average of 10198 packets in 1 second with CPU Load 15%
    CAN Profile App:Measured Load: Total CPU: 17%, HWI: 10%, SWI:0% TSK: 5%
    CAN Profile App:Message Id Received 800000b0 Message Length is 64
    CAN Profile App:Test completed for 1 instance

    CAN Profile App: 8192 bytes used for stack
    CAN Profile App:Profiling completes!!!

[Back To Top](@ref demo_can_profile_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
## Sample output transmit only mode {#demo_can_profile_output_tx_only}
Below is the log from one of the runs, when CAN is transmitting only

    CAN Profile App:Variant - Pre Compile being used !!!
    CAN Profile App: Successfully Enabled CAN Transceiver
    CAN Profile App:Will Transmit10000 Messages, 5 times
    CAN Profile App:Transmit 10000 packets 5 times
    CAN Profile App:Average of 5663 packets in 1 second with CPU Load 6%
    CAN Profile App:Measured Load: Total CPU: 8%, HWI: 2%, SWI:0% TSK: 4%
    CAN Profile App: 8192 bytes used for stack
    CAN Profile App:Profiling completes!!!

[Back To Top](@ref demo_can_profile_top)

- - - - - - - - - - - - - - - - - - - - - - - - - - 

# Document Revision History {#demo_can_profile_rev_history}

Revision | Date          | Author     | Description         | Status
---------|---------------|------------|---------------------|-------
0.1      | 24 Dec 2018   | Sujith S   | Initial Version   | Under Review
0.2      | 13 Jan 2019   | Sunil M S  | Updated logs and setup picture (MCAL-2661) | Approved