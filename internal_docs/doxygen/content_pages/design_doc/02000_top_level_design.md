# Top-Level Design {#ethfw_tldesign_top}

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Top-Level Directory Structure {#ethfw_tldesign_dir_struct}

The directory structure of the Ethernet Firmware is shown below::

    ethfw                                   # Root folder for Ethernet Firmware
    ├── apps                                # Ethernet firmware applications
    │   ├── app_remoteswitchcfg_client      # Remote core client application
    │   │   └── mcu_2_1
    │   ├── app_remoteswitchcfg_server      # Ethernet Firmware server application
    │   │   └── mcu_2_0
    │   ├── common                          # Common MPU settings for J721E/J7200
    │   ├── bios_cfg                        # Common BIOS configuration (deprecated)
    │   ├── ipc_cfg                         # Common IPC configuration
    │   └── tap                             # Linux user-space demo app for intercore interface
    ├── concerto                            # Makefile-based build system common files
    │   └── compilers                       # Compiler specific common options
    ├── docs                                # User documentation
    │   ├── api_guide                       # Ethernet Firmware API guide
    │   │   └── html_
    │   ├── design_doc                      # Ethernet Firmware design document
    │   │   └── html_
    │   ├── misrac                          # MISRA-C reports
    │   ├── packeth_configurations          # packETH tool configurations (used for demo)
    │   ├── test_report                     # Test and traceability reports
    │   └── user_guide                      # Ethernet Firmware User's guide
    ├── ethremotecfg                        # EthSwitch Remote Device
    │   ├── client                          # Client side of the EthSwitch remote device
    │   │   ├── include
    │   │   └── src
    │   ├── protocol                        # EthSwitch API
    │   └── server                          # Server side of the EthSwitch remote device
    │       ├── include
    │       └── src
    ├── makerules                           # Helper makefiles for NDK, PDK
    ├── ethfw                               # Ethernet firmware library
    │   └── src
    └── utils                               # Helper utilities
        ├── ethfw_callbacks                 # Frequently used lwIP callbacks
        │   ├── include
        │   └── src
        ├── ethfw_lwip                      # Emulated Proxy ARP callbacks
        │   ├── include
        │   └── src
        ├── ethfw_stats                     # Networking statistics services
        │   ├── include
        │   └── src
        ├── intervlan                       # Hardware and software interVLAN demo utils
        │   ├── include
        │   └── src
        ├── console_io                      # Application logging utils
        │   ├── include
        │   └── src
        ├── ctoolslib
        └── mem                             # Memory allocation utils
            ├── include
            └── src

[Back To Top](@ref ethfw_tldesign_top)


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Top-Level Component Interaction

The *Ethernet Firmware* running on the *master core* interacts with the Enet LLD
running natively on the same core.  The Ethernet Firmware has access to all
functionality provided by Enet LLD public APIs for control path configuration
and Rx and Tx data transfers.  The Ethernet Firmware also configures and makes
use of the TCP/IP stack enabled by Enet LLD by the means of NDK/NIMU.

The *Ethernet Firmware* on the *master core* interacts with *remote cores*
running virtual network device interfaces (Linux or FreeRTOS) via *Ethernet
Switch Remote Device* which allows remote cores to communicate with the master
core via IPC.

The *remotes cores* also interact with the *Ethernet Firmware* to setup and
teardown data paths for Ethernet packet transfer towards CPSW. It's worth noting
that once the DMA channels/flows have been setup, no further interaction with
the Ethernet Firmware is required.

For further details on remote core integration in Linux and FreeRTOS, please
refer to the [Remote Core Integration](@ref ethfw_remotecore_top) section.

[Back To Top](@ref ethfw_tldesign_top)
