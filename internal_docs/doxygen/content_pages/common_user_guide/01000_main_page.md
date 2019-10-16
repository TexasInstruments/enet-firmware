# Introduction {#ethfw_main}

[TOC]

# Integrated Switch {#ethfw_c_ug_switch}

SoC's, such as J721E, integrate an Ethernet Switch as an chip-in-chip. The
combined features of SOC and Switch IP (CPSW9G) can allow Ethernet switch to
function continuously enabling unaffected switching on external ports regardless
of the state of the rest of the device.

![](j7_switch_for_replacement_of_external_switch.png "J721E Switch as a replacement to external switch")

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Ethernet Firmware Software Stack  {#ethfw_c_ug_fw_architecture}

The Ethernet Firmware is TI RTOS based application for configuration of
Ethernet switch. The package contains remote configuration server, resource management
library, switch resident protocols, proxy layers to handle local and remote API calls
and demonstration applications (EthFw Demos).
The switch software uses PDK CPSW and other drivers for respective IP configuration.
Its is expected to be hosted on Cortex R5F in Main Domain.

![](switch_software_stack.png "Ethernet Switch Software Architecture")

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History {#ethfw_main_rev_history}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Revision | Date          | Author                 | Description
---------|---------------|------------------------|-------------------------------------
0.1      | 02 Apr 2019   | Prasad J, Misael Lopez | Added as per 0.8 Docs review meeting
0.2      | 14 Oct 2019   | Prasad J               | Updated stack diagram
