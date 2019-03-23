# Release Notes 00.08.00 {#rel_notes_mainpage_00_08_00}

# Introduction {#rel_intro_00_08_00}

This release notes provides important information that will assist you in using the EthFw package.
This document provides the product information and known issues w.r.t EthFw software package.
The EthFw package consists of firmware for Ethernet switch, profiling tools and
demos/applications for **J7 family of devices**
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# What's New {#rel_new_in_this_release_00_08_00}
- Basic Switching Demo Application
  + Sample Example Configuring switch for switching across external ports & host port
- NDK Integration
  + Sample Example Application using TI NDK
  + Showcases TCP/IP stack usage in the Ethernet firmware

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Upgrade and Compatibility {#rel_upgrade_00_08_00}

NA. This is a pre-silicon release.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Device_Support {#rel_supported_devices_00_08_00}

SoC     |  HOST (OS)  | Target (OS) | Test Platform
--------|-------------|-------------|----------------
J721E   | Windows / Linux | TI RTOS  | J721E VLAB Simulator

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Validation Information {#rel_validation_info_00_08_00}

This release was built and validated using the following tools

Refer [EthFw User Guide](../user_guide/ethfw_c_ug_top.html) for additional instructions
to install and setup the dependancies.
* Build Tools (included in Processor SDK):
    1. ARM R5F Code Generation Tools version: 16.9.9
    2. TI Network Development Kit: 3_40_01_01
    3. TI RTOS: 6_76_00_01_eng
    4. TI XDC Tools: 3_51_01_18_core
* The VLAB version used to validate this package is 2.4.6 and VLAB toolbox version
is 0.14.5. For details on the validated examples refer the test report.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Dependencies {#rel_dependencies_00_08_00}
* Included in EthFw
* Not included in EthFw
 - EthFw is an add-on package and depends on components of SDK, please refer
   [EthFw User Guide](../user_guide/ethfw_c_ug_top.html) for details.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Fixed Issues {#rel_fixed_issues_00_08_00}
Not applicable

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Known Issues {#rel_known_issues_00_08_00}
None

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Limitations {#rel_limitations_00_08_00}

None
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Technical Support {#rel_tech_help_00_08_00}

For further information or to report any problems, contact http://e2e.ti.com or http://community.ti.com or http://support.ti.com.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Package Versioning

<table rules="all" frame="box" cellspacing="4" cellpadding="4" frame="void" width="80%" border="1">
<tr valign="top" bgcolor="#cccccc">
<th width="10%">Digit  </th><th width="20%">Meaning  </th><th width="70%">Description   </th></tr>
<tr valign="top">
<td>1&nbsp;(<b>M</b>=Major) </td><td>Major revision </td><td>Incremented when the new version is substantially different from the previous For example, a new module added or an existing module's algorithm significantly altered.   </td></tr>
<tr valign="top">
<td>2&nbsp;(<b>m</b>=minor) </td><td>Minor revision </td><td>Incremented when the new version has changed but not in a major way. For example, some minor changes in the API or feature set.   </td></tr>
<tr valign="top">
<td>3&nbsp;(<b>p</b>=patch) </td><td>Patch number </td><td>Incremented for all other source code changes. This include any packaging support code.   </td></tr>
<tr valign="top">
<td>4&nbsp;(<b>b</b>=build) </td><td>Build number </td><td>Incremented for each release delivery to CM. Reset for any change to M, m or p  </td></tr>
</table>
<hr size="1"><small>
Copyright  2019, Texas Instruments Incorporated</small>
