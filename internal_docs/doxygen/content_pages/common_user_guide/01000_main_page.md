# Introduction {#ethfw_main}

[TOC]

# Integrated Switch {#ethfw_c_ug_switch}

SoC's, such as J7200 or J721E, integrate an Ethernet Switch as an chip-in-chip. The
combined features of SoC and Switch IP (CPSW5G/CPSW9G) can allow Ethernet switch to
function continuously enabling unaffected switching on external ports regardless
of the state of the rest of the device.

The diagram below shows the hardware capabilities of the integrated Ethernet Switch
in J721E SoC that enable it to act as a replacement to an external switch in the
traditional automotive Ethernet gateway system.

![](j7_switch_for_replacement_of_external_switch.png "J721E Switch as a replacement to external switch")

The integrated switch in J721E offers 8 MAC ports, while the switch in J7200 offers
4 MAC ports.  In both cases, an internal host port provides connectivity to the SoC
which enables the different processing cores to exchange packets with the switch.

It's worth noting that the previous diagram shows internal data paths that are
available in the SoC but not necessarily enabled in the Ethernet Firmware component.
Please refer to the \ref ethfw_remote_clients section for further information
about the processing cores where virtual Ethernet functionality has been enabled.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Ethernet Firmware Software Stack  {#ethfw_c_ug_fw_architecture}

The Ethernet Firmware is a FreeRTOS based application for configuration of Ethernet switch,
hosted on the Cortex R5F 0 core 0 in Main domain of J721E and J7200 SoCs.

Ethernet Firmware package contains remote configuration server, resource management
library, switch resident protocols, proxy layers to handle local and remote API calls
and demonstration applications (EthFw Demos). The switch software uses PDK Enet and other
drivers for respective IP configuration.  The main building blocks of the Ethernet Firmware
are shown in the following diagram.

![](switch_software_stack.png "Ethernet Switch Software Architecture")


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Document Revision History {#ethfw_main_rev_history}
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Revision | Date          | Author                 | Description
---------|---------------|------------------------|-------------------------------------
0.1      | 02 Apr 2019   | Prasad J, Misael Lopez | Added as per 0.8 Docs review meeting
0.2      | 14 Oct 2019   | Prasad J               | Updated stack diagram
1.0      | 02 Nov 2020   | Misael Lopez           | Updated for Enet LLD migration
1.1      | 08 Jul 2021   | Misael Lopez           | Updates for v.8.00.00
1.2      | 07 Dec 2021   | Misael Lopez           | Updates for v.8.01.00
1.3      | 25 Feb 2022   | Misael Lopez           | Updates for v.8.02.00
