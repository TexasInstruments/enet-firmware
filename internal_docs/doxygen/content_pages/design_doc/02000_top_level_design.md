# Top-Level Design {#ethfw_tldesign_top}

[TOC]

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Top-Level Directory Structure {#ethfw_tldesign_dir_struct}

The directory structure of the Ethernet Firmware is shown below::

    ethfw                                   # Root folder for Ethernet Firmware
    ├── apps                                # Ethernet firmware applications
    │   ├── app_remoteswitchcfg_client      # Remote core client application
    │   │   ├── j7200
    │   │   ├── j721e
    │   │   ├── j784s4
    │   ├── app_remoteswitchcfg_server      # Ethernet Firmware server application
    │   │   ├── j7200
    │   │   ├── j721e
    │   │   ├── j784s4
    │   ├── common                          # Common MPU settings for J721E/J7200
    │   ├── ipc_cfg                         # Common IPC configuration
    │   └── tap                             # Linux user-space demo app for intercore interface
    ├── concerto                            # Makefile-based build system common files
    │   └── compilers                       # Compiler specific common options
    ├── docs                                # User documentation
    │   ├── api_guide                       # Ethernet Firmware API guide
    │   ├── design_doc                      # Ethernet Firmware design document
    │   ├── misrac                          # MISRA-C reports
    │   ├── packeth_configurations          # packETH tool configurations (used for demo)
    │   ├── test_report                     # Test reports
    │   ├── traceability_report             # Traceability reports
    │   └── user_guide                      # Ethernet Firmware User's guide
    ├── ethremotecfg                        # EthSwitch Remote Device
    │   ├── client                          # Client side of the ethremotecfg framework
    │   │   ├── include
    │   │   └── src
    │   ├── protocol                        # Ethernet Firmware interface definition
    │   └── server                          # Server side of the ethremotecfg framework
    │       ├── include
    │       └── src                         # Server libraries for proxy arp, vlan, vepa and mcast
    ├── makerules                           # Helper makefiles for NDK, PDK
    ├── unit_test                           # Root folder for unit tests
    │   ├── ethfw_test_app                  
    │   │   ├── mcu_2_0                     # Unit test application for server
    │   │   └── mcu_2_1                     # Unit test application for client
    │   ├── ipc_cfg                         # Common IPC configuration for unit tests
    │   ├── test_cases                      # Test cases (for both server and client)
    │   └── unity                           # Framework used for unit test
    │       ├── include
    │       └── src
    └── utils
        ├── board
        │   ├── include
        │   └── src
        ├── console_io                      # Application logging utils
        │   ├── include
        │   └── src
        ├── ethfw_callbacks                 # Frequently used lwIP callbacks
        │   ├── include
        │   └── src
        ├── ethfw_common                    # Trace functionality and server/client common utils
        │   ├── include
        │   └── src
        ├── ethfw_estdemo                   # EST demo application
        │   ├── include
        │   └── src
        ├── ethfw_stats                     # Networking statistics services
        │   ├── include
        │   └── src
        ├── intervlan                       # Hardware and software interVLAN demo utils
        │   ├── include
        │   └── src
        ├── mem                             # Memory allocation utils
        │   ├── include
        │   └── src
        ├── perf_stats
        │   ├── include
        │   └── src
        └── remote_service
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
running virtual network device interfaces (Linux or FreeRTOS) via *ethremotecfg*
framework which allows remote cores to communicate with the master
core via IPC.

The *remotes cores* also interact with the *Ethernet Firmware* to setup and
teardown data paths for Ethernet packet transfer towards CPSW. It's worth noting
that once the DMA channels/flows have been setup, no further interaction with
the Ethernet Firmware is required.

For further details on remote core integration in Linux and FreeRTOS, please
refer to the [Remote Core Integration](@ref ethfw_remotecore_top) section.

[Back To Top](@ref ethfw_tldesign_top)
