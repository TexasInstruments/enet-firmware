# CCS setup {#ccs_setup_top}

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Download and Install CCS {#ethfw_download_instal_ccs}

Code Composer Studio is an integrated development environment (IDE) that supports
TI's Micro controller and Embedded Processors portfolio. It provides useful tools
to develop and debug embedded applications.

Please visit Code Composer Studio product [page](http://www.ti.com/tool/ccstudio)
for more information or visit the CCS Downloads
[page](http://software-dl.ti.com/ccs/esd/documents/ccs_downloads.html).

Supported CCS version for J721E EVM is 9.0.1.00004 as shown below.

   ![](ccs_version.png "Code Composer Studio version")

[Back To Top](@ref ccs_setup_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Setup for J721E EVM {#ethfw_instal_ccs_gel_setup}

-# Install the CCS Emulation Packs for J721E. After installation the
   following versions should be listed

   ![](ccs_emulation_packs.png "Code Composer Studio Emulation Packs")



-# Download and untar CCS J721E device support pack in **C:\\ti\\ccs_version\\ccs\\ccs_base** folder.

   * Please contact your local FAE for chip support package link details.


[Back To Top](@ref ccs_setup_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Creating a Target Configuration File {#ethfw_target_config}

-# In Code Composer Studio, go to the **File** menu and select **New** -> **Target Configuration File**
-# Name the Target Configuration file as **J7ES_EVM.ccxml** and click **Finish**
-# Select the Connection to JTAG emulator which you have connected to J721E EVM.
   For on board emulator use connection as "Texas Instruments XDS110 USB Debug Probe"
-# Set Board or Device to **JACINTO721E**.

![](ccs_target_configuration.png "Creating CCS Target Configuration File")

-# Open launch.js script located at below path [SDK_INSTALL_PATH]\pdk_xx_xx_xx\packages\ti\drv\sciclient\tools\ccsLoadDmsc\j721e\ and change
   - **gelFilePath** to your GEL files in the CCS installation.
   - **pdkPath** to [SDK_INSTALL_PATH]\pdk_xx_xx_xx\packages

[Back To Top](@ref ccs_setup_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Load Example Binaries on J721E {#load_example_binaries_on_j7}

  Note: Please configure EVM in NOBOOT mode for connecting and loading binaries
        via CCS. As EthFw demos are run via CCS, please configure J721E EVM in
        NOBOOT mode.

-# Open Code Composer Studio and launch the Target Configuration previously setup

   ![](demo_l2_switching_steps_2.png "Launch CCS Target Configuration")

-# Go to the <b>View</b> Target configurations and launch a target configuration
   is done by right clicking on it.
-# Go to the <b>View</b> menu and then select <b>Scripting Console </b>
-# Run the launch.js script located in the pdk_xx_xx_xx\packages\ti\drv\sciclient\tools\ccsLoadDmsc\j721e\
   on scripting console to load and execute DMSC firmware binary.
   This step can take considerable time as it configures PLL etc. in the SOC via GEL files and configures DDR.

   ![](launch_dss_script.png "Launch script")

   After script completes execution you should see below in Debug window

   ![](launch_dss_script_complete.png "Launch script Complete")

-# In Code Composer Studio, select <b>MAIN_Cortex_R5_0_0</b> from the list of
   cores in the <b>Debug</b> panel

   ![](demo_l2_switching_steps_3.png "MAIN_Cortex_R5_0_0")

-# Go to the <b>Run</b> menu and then select <b>Load</b> -> <b>Select Program to Load</b>

-# In the <b>Load Program</b> window, browse the EthFw Layer-2 switching application binary.

   ![](demo_l2_switching_steps_4.png "Loading the demo application binary")

-# Go to the <b>Run</b> menu and then select <b>Resume</b> to start executing demo binary.


[Back To Top](@ref ccs_setup_top)

