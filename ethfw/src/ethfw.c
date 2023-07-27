/*
 *
 * Copyright (c) 2020 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 *
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
 * license under copyrights and patents it now or hereafter owns or controls to make,
 * have made, use, import, offer to sell and sell ("Utilize") this software subject to the
 * terms herein.  With respect to the foregoing patent license, such license is granted
 * solely to the extent that any such patent is necessary to Utilize the software alone.
 * The patent license shall not apply to any combinations which include this software,
 * other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source
 * code license limitations below) in the documentation and/or other materials provided
 * with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided
 * that the following conditions are met:
 *
 * *       No reverse engineering, decompilation, or disassembly of this software is
 * permitted with respect to any software provided in binary form.
 *
 * *       any redistribution and use are licensed by TI for use only with TI Devices.
 *
 * *       Nothing shall obligate TI to provide you with source code for the software
 * licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the
 * source code are permitted provided that the following conditions are met:
 *
 * *       any redistribution and use of the source code, including any resulting derivative
 * works, are licensed by TI for use only with TI Devices.
 *
 * *       any redistribution and use of any object code compiled from the source code
 * and any resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers
 *
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 *  \file ethfw.c
 *
 *  \brief Main file for Ethernet Firmware
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <stdint.h>

/* EthFw header files */
#include <ethfw/ethfw.h>

/* PDK Driver header files */
#include <ti/osal/osal.h>
#include <ti/drv/ipc/ipc.h>
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/enet/examples/utils/include/enet_apputils.h>
#include <ti/drv/enet/examples/utils/include/enet_mcm.h>

/* EthFw utils header files */
#include <utils/remote_service/include/app_remote_service.h>
#include <utils/perf_stats/include/app_perf_stats.h>
#include <utils/ethfw_stats/include/app_ethfw_stats_osal.h>
#include <utils/console_io/include/app_log.h>

/* EthFw remote configuration header files */
#include <ethremotecfg/server/include/ethremotecfg_server.h>
#include <ethremotecfg/server/include/cpsw_proxy_server.h>

#include <server-rtos/remote-device.h>

#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
#include <utils/ethfw_lwip/include/ethfw_lwip_utils.h>
#endif

#if defined(ETHFW_GPTP_SUPPORT)
/* Timesync header files */
#include <tsn_tilld_include.h>
#include <tsn_combase/combase.h>
#include <tsn_unibase/unibase_binding.h>
#include <tsn_gptp/gptp_config.h>
#include <tsn_gptp/gptpman.h>

#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/*! Month substring offset in date string */
#define ETHFW_VERSION_OFFSET_MONTH                    (0)

/*! Date substring offset in date string */
#define ETHFW_VERSION_OFFSET_DATE                     (4)

/*! Year substring offset in data string */
#define ETHFW_VERSION_OFFSET_YEAR                     (7)

/*! Month substring offset in date string */
#define ETHFW_VERSION_OFFSET_HOUR                     (0)

/*! Date substring offset in date string */
#define ETHFW_VERSION_OFFSET_MIN                      (3)

/*! Year substring offset in data string */
#define ETHFW_VERSION_OFFSET_SEC                      (6)

/*! Remote device endpoint number */
#define REMOTE_DEVICE_ENDPT                           (26U)

/*! AUTOSAR Eth driver endpoint number */
#define AUTOSAR_ETHDRIVER_DEVICE_ENDPT                (28U)

/*! AUTOSAR Eth driver endpoint-2 number */
#define AUTOSAR_ETHDRIVER_DEVICE_ENDPT2               (38U)

/*! Max VLAN id as per standard */
#define ETHFW_VLAN_ID_MAX                             (4094U)

/*! VLAN id used for host port */
#define ETHFW_HOST_PORT_VLAN_ID                       (1U)

/*! VLAN id used for all MAC ports in MAC-only mode */
#define ETHFW_MAC_ONLY_PORTS_VLAN_ID                  (2U)

/*! VLAN id used for all MAC ports in switch mode (non MAC-only mode) */
#define ETHFW_SWITCH_PORTS_VLAN_ID                    (3U)

/*! Max number of CPSW MAC ports supported */
#if defined(SOC_J721E) || defined(SOC_J784S4)
#define ETHFW_MAC_PORT_MAX                            (8U)
#else
#define ETHFW_MAC_PORT_MAX                            (4U)
#endif

#if defined(ETHFW_GPTP_SUPPORT)
/*! Logging task - very low priority so it doesn't interfere with gPTP stack */
#define ETHFW_LOGGER_TASK_PRIORITY                    (1U)

/*! gPTP stack priority - should be higher than other non-critical tasks which could interfere*/
#define ETHFW_GPTP_TASK_PRIORITY                      (7U)

/*! TSN stack size and alignment */
#define ETHFW_TSN_TASK_STACK_SIZE                     (16U * 1024U)
#define ETHFW_TSN_TASK_STACK_ALIGN                    ETHFW_TSN_TASK_STACK_SIZE

/*! Log task's buffer stack size and alignment */
#define ETHFW_TSN_LOGGER_TASK_STACK_SIZE              (2U * 1024U)
#define ETHFW_TSN_LOGGER_TASK_STACK_ALIGN             (32U)

/*! Size of the buffer used for storing logs before printing */
#define ETHFW_TSN_BUFFER_SIZE                         (5120U)

/*! Number of MAC ports supported by the gPTP stack */
#define ETHAPP_PTP_CFG_NUM_MAC_PORTS                  (1U)
#endif

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct EthFw_Port_s
{
    /*! MAC port number */
    Enet_MacPort macPort;

    /*! Port VLAN config */
    EnetPort_VlanCfg vlanCfg;
} EthFw_Port;

typedef struct EthFw_Obj_s
{
    /* Core Id */
    uint32_t coreId;

    /* Enet instance type */
    Enet_Type enetType;

    /* Enet instance id */
    uint32_t instId;

    /* CPSW configuration */
    Cpsw_Cfg cpswCfg;

    /* Firmware version */
    EthFw_Version version;

    /* Port mask of all enabled MAC ports */
    uint32_t enabledPortMask;

    /* Port mask of all MAC-only ports */
    uint32_t macOnlyPortMask;

    /* Port mask of all non MAC-only ports */
    uint32_t switchPortMask;

    /* MAC ports owned by EthFw */
    EthFw_Port ports[ENET_MAC_PORT_NUM];

    /* Number of MAC ports owned by EthFw */
    uint32_t numPorts;

    /* Virtual port configuration */
    EthFw_VirtPortCfg virtPortCfg[ETHFW_REMOTE_CLIENT_MAX];

    /* Number of valid virtual port configuration entries */
    uint32_t numVirtPorts;

    /* AUTOSAR virtual port configuration */
    EthFw_VirtPortCfg autosarVirtPortCfg[CPSWPROXYSERVER_AUTOSAR_REMOTE_CLIENT_MAX];

    /* Number of valid AUTOSAR virtual port configuration entries */
    uint32_t numAutosarVirtPorts;

    /*! Default VLAN id to be used for MAC ports configured in MAC-only mode */
    uint16_t dfltVlanIdMacOnlyPorts;

    /*! Default VLAN id to be used for MAC ports configured in switch mode (non MAC-only) */
    uint16_t dfltVlanIdSwitchPorts;

    /* Multiclient Manager (MCM) handle */
    EnetMcm_CmdIf mcmCmdIf;

    /* Shared multicast configuration */
    EthFw_SharedMcastCfg sharedMcastCfg;

    /* Reserved multicast configuration */
    EthFw_RsvdMcastCfg rsvdMcastCfg;

    /*! Callback function for application to set port link parameters */
    EthFw_setPortCfg setPortCfg;

#if defined(ETHFW_GPTP_SUPPORT)
    /* Whether TSN stack has been initialized or not */
    bool tsnInit;

    /* Whether gPTP has been started or not */
    bool ptpStarted;

    /* To run log Task in the loop*/
    bool logTaskrun;

    /* TSN stack netdevs */
    char netDevs[ETHAPP_PTP_CFG_NUM_MAC_PORTS][IFNAMSIZ];

    /* gPTP stack netdevs */
    char *gPtpNetDevs[ETHAPP_PTP_CFG_NUM_MAC_PORTS + 1];

    /* gPTP task handle */
    TaskP_Handle hPtpTask;

    /* gPTP task stack buffer */
    uint8_t gPtpStackBuf[ETHFW_TSN_TASK_STACK_SIZE] __attribute__ ((aligned(ETHFW_TSN_TASK_STACK_ALIGN)));

    /* Mutex object used for TSN stack logging */
    MutexP_Object logMutexObj;

    /* Mutex handle for logMutexObj */
    MutexP_Handle hLogMutex;

    /* TSN stack logging task handle */
    TaskP_Handle hLogTask;

    /* TSN logger task stack buffer */
    uint8_t logTaskStackBuf[ETHFW_TSN_LOGGER_TASK_STACK_SIZE] __attribute__ ((aligned(ETHFW_TSN_LOGGER_TASK_STACK_ALIGN)));

    /* Buffer used to accumulate the log messages given by stack until 'log_task' flushes them */
    uint8_t logBuf[ETHFW_TSN_BUFFER_SIZE];

    /* Buffer used to store string to be printed via app's print function */
    uint8_t printBuf[ETHFW_TSN_BUFFER_SIZE];

    /* gPTP config callback from app */
    EthFw_configPtpCb configPtpCb;

    /* gPTP config callback argument */
    void *configPtpCbArg;
#endif
} EthFw_Obj;

typedef struct EthFw_Autosar_EpId_s
{
    /*! Remote core id */
    uint32_t remoteCoreId;

    /*! Remote Endpoint id */
    uint32_t remoteEndptId;
} EthFw_Autosar_EpId;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static bool EthFw_isMacOnlyPort(Enet_MacPort macPort);

static int32_t EthFw_initMcm(void);

static void EthFw_deinitMcm(void);

static void EthFw_initLinkArgs(EnetPer_PortLinkCfg *linkArgs,
                               Enet_MacPort macPort);

static int32_t EthFw_setAleBcastEntry(void);

static void EthFw_getMcmCmdIfCb(Enet_Type enetType,
                                EnetMcm_CmdIf **pMcmCmdIfHandle);

static void EthFw_getDeviceData(uint32_t host_id,
                                struct rpmsg_kdrv_ethswitch_device_data *eth_dev_data);

static void EthFw_handleProfileInfoNotify(uint32_t host_id,
                                          Enet_Handle hEnet,
                                          Enet_Type enetType,
                                          uint32_t core_key,
                                          enum rpmsg_kdrv_ethswitch_client_notify_type notifyid,
                                          uint8_t *notify_info,
                                          uint32_t notify_info_len);

#if defined(ETHFW_GPTP_SUPPORT)
static void EthFw_tsnInit(void);

static void EthFw_tsnDeinit(void);
#endif

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/*! Ethernet Firmware object */
EthFw_Obj gEthFwObj;

/* EthFw RM: TX channel, RX flow and MAC address partitioning */
static EnetRm_ResPrms gEthFw_rmResPrms =
{
    .coreDmaResInfo =
    {
        [0] =
        {
            .coreId        = IPC_MPU1_0,
            .numTxCh       = 0U,
            .numRxFlows    = 0U,
            .numMacAddress = 0U,
        },
        [1] =
        {
            .coreId        = IPC_MCU1_0,
            .numTxCh       = 0U,
            .numRxFlows    = 0U,
            .numMacAddress = 0U,
        },
        [2] =
        {
            /* EthFw's RM usage:
             * TX chans: lwIP + PTP + SW interVLAN
             * RX flows: lwIP + proxy ARP + PTP + SW interVLAN + default flow
             * MAC addr: lwIP */
            .coreId        = IPC_MCU2_0,
            .numTxCh       = 3U,
            .numRxFlows    = 5U,
            .numMacAddress = 1U,
        },
        [3] =
        {
            .coreId        = IPC_MCU2_1,
            .numTxCh       = 0U,
            .numRxFlows    = 0U,
            .numMacAddress = 0U,
        },
#if defined(SOC_J721E) || defined(SOC_J784S4)
        [4] =
        {
            .coreId        = IPC_MCU3_0,
            .numTxCh       = 0U,
            .numRxFlows    = 0U,
            .numMacAddress = 0U,
        },
        [5] =
        {
            .coreId        = IPC_MCU3_1,
            .numTxCh       = 0U,
            .numRxFlows    = 0U,
            .numMacAddress = 0U,
        },
#endif
    },
#if defined(SOC_J721E) || defined(SOC_J784S4)
    .numCores = 6U,
#else
    .numCores = 4U,
#endif
};

/* EthFw IOCTLs: allow all on all cores */
static const EnetRm_IoctlPermissionTable gEthFw_rmIoctlPerm =
{
    .defaultPermittedCoreMask = (ENET_BIT(IPC_MPU1_0) |
                                 ENET_BIT(IPC_MCU2_0) |
                                 ENET_BIT(IPC_MCU2_1) |
#if defined(SOC_J721E) || defined(SOC_J784S4)
                                 ENET_BIT(IPC_MCU3_0) |
                                 ENET_BIT(IPC_MCU3_1) |
#endif
                                 ENET_BIT(IPC_MCU1_0)),
    .numEntries = 0,
};

/* IPC endpoints used for AUTOSAR virtual clients */
static EthFw_Autosar_EpId gEthFw_autosarEndptId[] =
{
    {
        .remoteCoreId  = IPC_MCU2_1,
        .remoteEndptId = AUTOSAR_ETHDRIVER_DEVICE_ENDPT,
    },
    {
        .remoteCoreId  = IPC_MCU1_0,
        .remoteEndptId = AUTOSAR_ETHDRIVER_DEVICE_ENDPT2,
    },
};

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

static void EthFw_initAleCfg(CpswAle_Cfg *aleCfg)
{
    /* ALE configuration */
    aleCfg->modeFlags = CPSW_ALE_CFG_MODULE_EN |
                        CPSW_ALE_CFG_UNKNOWN_UCAST_FLOOD2HOST;

    aleCfg->agingCfg.autoAgingEn = TRUE;
    aleCfg->agingCfg.agingPeriodInMs = 1000;

    aleCfg->nwSecCfg.vid0ModeEn = FALSE;

    aleCfg->vlanCfg.aleVlanAwareMode = TRUE;
    aleCfg->vlanCfg.cpswVlanAwareMode = TRUE;
    aleCfg->vlanCfg.unknownUnregMcastFloodMask = 0U;
    aleCfg->vlanCfg.unknownRegMcastFloodMask = 0U;
    aleCfg->vlanCfg.unknownVlanMemberListMask = CPSW_ALE_ALL_PORTS_MASK;
    aleCfg->vlanCfg.autoLearnWithVlan = false;

    aleCfg->policerGlobalCfg.policingEn = true;
    aleCfg->policerGlobalCfg.yellowDropEn = false;
    aleCfg->policerGlobalCfg.redDropEn = true;
    aleCfg->policerGlobalCfg.policerNoMatchMode = CPSW_ALE_POLICER_NOMATCH_MODE_GREEN;

    /* ALE policer configuration */
    aleCfg->policerGlobalCfg.policingEn = true;
    aleCfg->policerGlobalCfg.yellowDropEn = false;
    aleCfg->policerGlobalCfg.redDropEn = true;
    aleCfg->policerGlobalCfg.policerNoMatchMode = CPSW_ALE_POLICER_NOMATCH_MODE_GREEN;
}

static int32_t EthFw_getDfltVlanId(const EthFw_Config *config)
{
    int32_t status = ENET_SOK;

    if (config->dfltVlanIdMacOnlyPorts > ETHFW_VLAN_ID_MAX)
    {
        appLogPrintf("ETHFW: default VLAN id %u for MAC-only ports is out-of-range\n",
                     config->dfltVlanIdMacOnlyPorts);
        status = ENET_EINVALIDPARAMS;
    }

    if (config->dfltVlanIdSwitchPorts > ETHFW_VLAN_ID_MAX)
    {
        appLogPrintf("ETHFW: default VLAN id %u for switch ports is out-of-range\n",
                     config->dfltVlanIdSwitchPorts);
        status = ENET_EINVALIDPARAMS;
    }

    if (config->dfltVlanIdMacOnlyPorts == config->dfltVlanIdSwitchPorts)
    {
        appLogPrintf("ETHFW: default VLAN Id should not be same for MAC-only and switch ports (%u)\n",
                     config->dfltVlanIdSwitchPorts);
        status = ENET_EINVALIDPARAMS;
    }

    if (status == ENET_SOK)
    {
        gEthFwObj.dfltVlanIdMacOnlyPorts = config->dfltVlanIdMacOnlyPorts;
        gEthFwObj.dfltVlanIdSwitchPorts  = config->dfltVlanIdSwitchPorts;
    }

    return status;
}

static int32_t EthFw_getPortConfig(const EthFw_Config *config)
{
    EthRemoteCfg_VirtPort virtPort;
    Enet_MacPort macPort;
    uint32_t i;
    int32_t status = ENET_SOK;

    if (config->setPortCfg == NULL)
    {
        appLogPrintf("ETHFW: Invalid setPortCfg callback\n");
        status = ENET_EINVALIDPARAMS;
    }

    if (config->numPorts > ETHFW_MAC_PORT_MAX)
    {
        appLogPrintf("ETHFW: Too many MAC ports requested (%u), max is %u\n",
                     config->numPorts, ETHFW_MAC_PORT_MAX);
        status = ENET_EINVALIDPARAMS;
    }

    if (config->numVirtPorts > ETHFW_REMOTE_CLIENT_MAX)
    {
        appLogPrintf("ETHFW: Too many virtual ports requested (%u), max is %u\n",
                     config->numVirtPorts, ETHFW_REMOTE_CLIENT_MAX);
        status = ENET_EINVALIDPARAMS;
    }

    if (config->numAutosarVirtPorts > CPSWPROXYSERVER_AUTOSAR_REMOTE_CLIENT_MAX)
    {
        appLogPrintf("ETHFW: Too many AUTOSAR virtual ports requested (%u), max is %u\n",
                     config->numAutosarVirtPorts, CPSWPROXYSERVER_AUTOSAR_REMOTE_CLIENT_MAX);
        status = ENET_EINVALIDPARAMS;
    }

    if (status == ENET_SOK)
    {
        gEthFwObj.setPortCfg = config->setPortCfg;

        /* Get the port mask of all enabled MAC ports */
        gEthFwObj.numPorts = config->numPorts;
        for (i = 0U; i < gEthFwObj.numPorts; i++)
        {
            gEthFwObj.ports[i].macPort = config->ports[i];
            macPort = gEthFwObj.ports[i].macPort;

            gEthFwObj.enabledPortMask |= ENET_MACPORT_MASK(macPort);
        }

        /* Get the port mask of all ports in MAC-only mode */
        gEthFwObj.numVirtPorts = config->numVirtPorts;
        for (i = 0U; i < gEthFwObj.numVirtPorts; i++)
        {
            gEthFwObj.virtPortCfg[i] = config->virtPortCfg[i];
            virtPort = gEthFwObj.virtPortCfg[i].portId;

            if (EthRemoteCfg_isMacPort(virtPort))
            {
                macPort = EthRemoteCfg_getMacPort(virtPort);

                gEthFwObj.macOnlyPortMask |= ENET_MACPORT_MASK(macPort);
            }
        }

        /* Get the port mask of all AUTOSAR ports in MAC-only mode */
        gEthFwObj.numAutosarVirtPorts = config->numAutosarVirtPorts;
        for (i = 0U; i < gEthFwObj.numAutosarVirtPorts; i++)
        {
            gEthFwObj.autosarVirtPortCfg[i] = config->autosarVirtPortCfg[i];
            virtPort = gEthFwObj.autosarVirtPortCfg[i].portId;

            if (EthRemoteCfg_isMacPort(virtPort))
            {
                macPort = EthRemoteCfg_getMacPort(virtPort);

                gEthFwObj.macOnlyPortMask |= ENET_MACPORT_MASK(macPort);
            }
        }

        gEthFwObj.switchPortMask = (gEthFwObj.enabledPortMask &
                                    ~gEthFwObj.macOnlyPortMask);
    }

    return status;
}

static EthFw_Port *EthFw_getMacPortConfig(Enet_MacPort macPort)
{
    EthFw_Port *port = NULL;
    uint32_t i;

    for (i = 0U; i < gEthFwObj.numPorts; i++)
    {
        if (gEthFwObj.ports[i].macPort == macPort)
        {
            port = &gEthFwObj.ports[i];
            break;
        }
    }

    return port;
}

static void EthFw_setPortVlan(void)
{
    EthRemoteCfg_VirtPort virtPort;
    EthFw_Port *ethFwPort;
    Enet_MacPort macPort;
    EnetPort_VlanCfg *vlanCfg;
    uint32_t i;

    for (i = 0U; i < gEthFwObj.numPorts; i++)
    {
        macPort = gEthFwObj.ports[i].macPort;

        ethFwPort = EthFw_getMacPortConfig(macPort);
        if (ethFwPort != NULL)
        {
            vlanCfg = &ethFwPort->vlanCfg;

            if (EthFw_isMacOnlyPort(macPort))
            {
                vlanCfg->portVID = gEthFwObj.dfltVlanIdMacOnlyPorts;
            }
            else
            {
                vlanCfg->portVID = gEthFwObj.dfltVlanIdSwitchPorts;
            }
            vlanCfg->portPri = 0U;
            vlanCfg->portCfi = 0U;
        }
    }
}

static bool EthFw_isMacOnlyPort(Enet_MacPort macPort)
{
    bool isMacOnly = false;

    if ((gEthFwObj.macOnlyPortMask & ENET_MACPORT_MASK(macPort)) != 0U)
    {
        isMacOnly = true;
    }

    return isMacOnly;
}

static void EthFw_setPortMode(void)
{
    Enet_MacPort macPort;
    CpswAle_Cfg *aleCfg = &gEthFwObj.cpswCfg.aleCfg;
    CpswAle_PortVlanCfg *pvidCfg;
    CpswAle_PortMacModeCfg *macModeCfg;
    CpswAle_PortLearningSecurityCfg *learningCfg;
    CpswAle_PortVlanSecurityCfg *vlanSecCfg;
    uint32_t aleSwitchPortMask = 0U;
    uint32_t aleMacOnlyPortMask = 0U;
    uint32_t alePort;
    uint32_t i;

    aleSwitchPortMask  = (gEthFwObj.switchPortMask << 1U);
    aleMacOnlyPortMask = (gEthFwObj.macOnlyPortMask << 1U);

    /* Reset MAC-only and learning config for all enabled ports. It will be
     * overwritten as needed right after */
    for (i = 0U; i < gEthFwObj.numPorts; i++)
    {
        macPort = gEthFwObj.ports[i].macPort;
        alePort = CPSW_ALE_MACPORT_TO_ALEPORT(macPort);

        macModeCfg  = &aleCfg->portCfg[alePort].macModeCfg;
        learningCfg = &aleCfg->portCfg[alePort].learningCfg;
        pvidCfg     = &aleCfg->portCfg[alePort].pvidCfg;
        vlanSecCfg  = &aleCfg->portCfg[alePort].vlanCfg;

        if (EthFw_isMacOnlyPort(macPort))
        {
            macModeCfg->macOnlyCafEn = false;
            macModeCfg->macOnlyEn = true;
            learningCfg->noLearn = true;
            vlanSecCfg->dropUntagged = FALSE;

            pvidCfg->vlanIdInfo.tagType  = ENET_VLAN_TAG_TYPE_INNER;
            pvidCfg->vlanIdInfo.vlanId   = gEthFwObj.dfltVlanIdMacOnlyPorts;
            pvidCfg->vlanMemberList      = aleMacOnlyPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->regMcastFloodMask   = aleMacOnlyPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->unregMcastFloodMask = aleMacOnlyPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->forceUntaggedEgressMask = aleMacOnlyPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->noLearnMask     = 0U;
            pvidCfg->vidIngressCheck = 0U;
            pvidCfg->limitIPNxtHdr   = false;
            pvidCfg->disallowIPFrag  = false;
        }
        else
        {
            macModeCfg->macOnlyCafEn = false;
            macModeCfg->macOnlyEn = false;
            learningCfg->noLearn = false;
            vlanSecCfg->dropUntagged = FALSE;

            pvidCfg->vlanIdInfo.tagType  = ENET_VLAN_TAG_TYPE_INNER;
            pvidCfg->vlanIdInfo.vlanId   = gEthFwObj.dfltVlanIdSwitchPorts;
            pvidCfg->vlanMemberList      = aleSwitchPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->regMcastFloodMask   = aleSwitchPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->unregMcastFloodMask = 0U;
            pvidCfg->forceUntaggedEgressMask = aleSwitchPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->noLearnMask     = 0U;
            pvidCfg->vidIngressCheck = 0U;
            pvidCfg->limitIPNxtHdr   = false;
            pvidCfg->disallowIPFrag  = false;
        }
    }

    pvidCfg = &aleCfg->portCfg[0].pvidCfg;
    pvidCfg->vlanIdInfo.tagType  = ENET_VLAN_TAG_TYPE_INNER;
    pvidCfg->vlanIdInfo.vlanId   = ETHFW_HOST_PORT_VLAN_ID;
    pvidCfg->vlanMemberList      = CPSW_ALE_ALL_PORTS_MASK;
    pvidCfg->regMcastFloodMask   = CPSW_ALE_ALL_PORTS_MASK;
    pvidCfg->unregMcastFloodMask = 0U;
    pvidCfg->forceUntaggedEgressMask = CPSW_ALE_ALL_PORTS_MASK;
    pvidCfg->noLearnMask     = 0U;
    pvidCfg->vidIngressCheck = 0U;
    pvidCfg->limitIPNxtHdr   = false;
    pvidCfg->disallowIPFrag  = false;

    learningCfg = &aleCfg->portCfg[0].learningCfg;
    learningCfg->noLearn = false;

    vlanSecCfg = &aleCfg->portCfg[0].vlanCfg;
    vlanSecCfg->dropUntagged = FALSE;
}

static EnetRm_ResourceInfo *EthFw_getRmInfo(uint32_t coreId)
{
    EnetRm_ResourceInfo *rmInfo = NULL;
    uint32_t i;

    for (i = 0U; i < gEthFw_rmResPrms.numCores; i++)
    {
        if (gEthFw_rmResPrms.coreDmaResInfo[i].coreId == coreId)
        {
            rmInfo = &gEthFw_rmResPrms.coreDmaResInfo[i];
            break;
        }
    }

    return rmInfo;
}

static void EthFw_updateEnetRm(void)
{
    EnetRm_ResCfg *resCfg = &gEthFwObj.cpswCfg.resCfg;
    EnetRm_ResPrms *rmPrms = &resCfg->resPartInfo;
    EnetRm_ResourceInfo *rmInfo;
    uint32_t req = 0U;
    uint32_t coreId;
    uint32_t i;
    uint32_t rdevVirtPorts[IPC_MAX_PROCS];
    uint32_t autosarVirtPorts[IPC_MAX_PROCS];
    uint32_t virtPortCnt = 0U;

    memset(rdevVirtPorts, 0, sizeof(rdevVirtPorts));
    memset(autosarVirtPorts, 0, sizeof(autosarVirtPorts));

    /* Count the number of remote_device-based virtual ports */
    for (i = 0U; i < gEthFwObj.numVirtPorts; i++)
    {
        coreId = gEthFwObj.virtPortCfg[i].remoteCoreId;
        rdevVirtPorts[coreId]++;
    }

    /* Count the number of AUTOSAR virtual ports */
    for (i = 0U; i < gEthFwObj.numAutosarVirtPorts; i++)
    {
        coreId = gEthFwObj.autosarVirtPortCfg[i].remoteCoreId;
        autosarVirtPorts[coreId]++;
    }

    /* Add RM needed by virtual ports, each one needs:
     * - 1 x TX channel
     * - 1 x RX flow
     * - 1 x MAC address from ETHFW pool
     */
    for (i = 0U; i < IPC_MAX_PROCS; i++)
    {
        virtPortCnt = EnetUtils_max(rdevVirtPorts[i], autosarVirtPorts[i]);
        if (virtPortCnt > 0U)
        {
            rmInfo = EthFw_getRmInfo(i);
            if (rmInfo != NULL)
            {
                rmInfo->numTxCh += virtPortCnt;
                rmInfo->numRxFlows += virtPortCnt;
                rmInfo->numMacAddress += virtPortCnt;
            }
        }
    }

    /* Overwriting RM with our own */
    resCfg->resPartInfo = gEthFw_rmResPrms;

    /* Compute the MAC address pool size for the virtual port allocation */
    for (i = 0U; i < rmPrms->numCores; i++)
    {
        req += rmPrms->coreDmaResInfo[i].numMacAddress;
    }

    /* Limit pool size to the size of MAC address array */
    if (resCfg->macList.numMacAddress > ENET_ARRAYSIZE(resCfg->macList.macAddress))
    {
        resCfg->macList.numMacAddress = ENET_ARRAYSIZE(resCfg->macList.macAddress);
    }

    /* Pool size provided by application is too small, warn user about it */
    if (resCfg->macList.numMacAddress == 0U)
    {
        appLogPrintf("ETHFW: Empty MAC address pool\n");
        EnetAppUtils_assert(false);
    }
    else if (resCfg->macList.numMacAddress < req)
    {
        appLogPrintf("ETHFW: MAC address pool size is too small (req=%u alloc=%u)\n"
                     "       may not be sufficient depending on concurrent usage\n",
                     req, resCfg->macList.numMacAddress);
    }
}

static int32_t EthFw_getSharedMcast(const EthFw_Config *config)
{
    const EthFw_SharedMcastCfg *mcastCfg = &config->sharedMcastCfg;
    const uint8_t *macAddr;
    uint32_t i;
    int32_t status = ENET_SOK;

    if (mcastCfg->numMacAddr > ETHFW_SHARED_MCAST_LIST_LEN)
    {
        appLogPrintf("ETHFW: Invalid number of multicast addresses (%u), must be <= %u\n",
                     mcastCfg->numMacAddr, ETHFW_SHARED_MCAST_LIST_LEN);
        status = ENET_EINVALIDPARAMS;
    }

    if (status == ENET_SOK)
    {
        memset(&gEthFwObj.sharedMcastCfg.macAddrList[0U], 0, sizeof(gEthFwObj.sharedMcastCfg.macAddrList));

        appLogPrintf("ETHFW: Shared multicasts (software fanout):\n");

        for (i = 0U; i < mcastCfg->numMacAddr; i++)
        {
            macAddr = &mcastCfg->macAddrList[i][0U];
            if (EnetUtils_isMcastAddr(macAddr))
            {
                appLogPrintf("  %02x:%02x:%02x:%02x:%02x:%02x\n",
                             macAddr[0U], macAddr[1U], macAddr[2U],
                             macAddr[3U], macAddr[4U], macAddr[5U]);
                EnetUtils_copyMacAddr(&gEthFwObj.sharedMcastCfg.macAddrList[i][0U], macAddr);
            }
            else
            {
                appLogPrintf("ETHFW: Addr %02x:%02x:%02x:%02x:%02x:%02x is not mcast\n",
                             macAddr[0U], macAddr[1U], macAddr[2U],
                             macAddr[3U], macAddr[4U], macAddr[5U]);
                status = ENET_EINVALIDPARAMS;
                break;
            }
        }

        gEthFwObj.sharedMcastCfg.numMacAddr = mcastCfg->numMacAddr;
        gEthFwObj.sharedMcastCfg.filterAddMacSharedCb = mcastCfg->filterAddMacSharedCb;
        gEthFwObj.sharedMcastCfg.filterDelMacSharedCb = mcastCfg->filterDelMacSharedCb;
    }

    return status;
}

static int32_t EthFw_getRsvdMcast(const EthFw_Config *config)
{
    const EthFw_RsvdMcastCfg *mcastCfg = &config->rsvdMcastCfg;
    const uint8_t *macAddr;
    uint32_t i;
    int32_t status = ENET_SOK;

    if (mcastCfg->numMacAddr > ETHFW_RSVD_MCAST_LIST_LEN)
    {
        appLogPrintf("ETHFW: Invalid number of multicast addresses (%u), must be <= %u\n",
                     mcastCfg->numMacAddr, ETHFW_RSVD_MCAST_LIST_LEN);
        status = ENET_EINVALIDPARAMS;
    }

    if ((status == ENET_SOK) &&
        (mcastCfg->numMacAddr > 0U))
    {
        memset(&gEthFwObj.rsvdMcastCfg.macAddrList[0U], 0, sizeof(gEthFwObj.rsvdMcastCfg.macAddrList));

        appLogPrintf("ETHFW: Reserved multicasts:\n");

        for (i = 0U; i < mcastCfg->numMacAddr; i++)
        {
            macAddr = &mcastCfg->macAddrList[i][0U];
            if (EnetUtils_isMcastAddr(macAddr))
            {
                appLogPrintf("  %02x:%02x:%02x:%02x:%02x:%02x\n",
                             macAddr[0U], macAddr[1U], macAddr[2U],
                             macAddr[3U], macAddr[4U], macAddr[5U]);
                EnetUtils_copyMacAddr(&gEthFwObj.rsvdMcastCfg.macAddrList[i][0U], macAddr);
            }
            else
            {
                appLogPrintf("ETHFW: Addr %02x:%02x:%02x:%02x:%02x:%02x is not mcast\n",
                             macAddr[0U], macAddr[1U], macAddr[2U],
                             macAddr[3U], macAddr[4U], macAddr[5U]);
                status = ENET_EINVALIDPARAMS;
                break;
            }
        }

        gEthFwObj.rsvdMcastCfg.numMacAddr = mcastCfg->numMacAddr;
    }

    return status;
}

void EthFw_initConfigParams(Enet_Type enetType,
                            EthFw_Config *config)
{
    Cpsw_Cfg *cpswCfg = &config->cpswCfg;
    CpswAle_Cfg *aleCfg = &cpswCfg->aleCfg;
    Cpsw_VlanCfg *vlanCfg = &cpswCfg->vlanCfg;
    CpswHostPort_Cfg *hostPortCfg = &cpswCfg->hostPortCfg;
    EnetRm_ResCfg *resCfg = &cpswCfg->resCfg;
    uint32_t instId = 0U;

    memset(config, 0, sizeof(*config));

    /* MAC port ownership */
    config->ports = NULL;
    config->numPorts = 0U;

    /* Virtual ports (remote_device based) */
    config->virtPortCfg = NULL;
    config->numVirtPorts = 0U;

    /* Virtual ports (bare IPC, AUTOSAR) */
    config->autosarVirtPortCfg = NULL;
    config->numAutosarVirtPorts = 0U;

    /* Shared multicast config */
    config->sharedMcastCfg.numMacAddr = 0U;
    config->sharedMcastCfg.filterAddMacSharedCb = NULL;
    config->sharedMcastCfg.filterDelMacSharedCb = NULL;

    /* Reserved multicast config */
    config->rsvdMcastCfg.numMacAddr = 0U;

    /* Default VLAN ids */
    config->dfltVlanIdMacOnlyPorts = ETHFW_MAC_ONLY_PORTS_VLAN_ID;
    config->dfltVlanIdSwitchPorts  = ETHFW_SWITCH_PORTS_VLAN_ID;

    /* Start with CPSW LLD's default configuration */
    Enet_initCfg(enetType, instId, cpswCfg, sizeof (*cpswCfg));
    cpswCfg->dmaCfg = NULL;
    resCfg->ioctlPermissionInfo = gEthFw_rmIoctlPerm;
    resCfg->selfCoreId = EnetSoc_getCoreId();
    resCfg->macList.numMacAddress = 0U;

    /* VLAN configuration */
    vlanCfg->vlanAware = true;

    /* Host port configuration */
    hostPortCfg->removeCrc = true;
    hostPortCfg->padShortPacket = true;
    hostPortCfg->passCrcErrors = true;
    hostPortCfg->csumOffloadEn = true;
    hostPortCfg->rxMtu = 1522U;
    hostPortCfg->vlanCfg.portVID = ETHFW_HOST_PORT_VLAN_ID;

    EthFw_initAleCfg(aleCfg);
}

EthFw_Handle EthFw_init(Enet_Type enetType,
                        const EthFw_Config *config)
{
    EnetUdma_Cfg *udmaCfg;
    char *date = __DATE__;
    char *time = __TIME__;
    int32_t status = ENET_SOK;

    EnetAppUtils_assert(config != NULL);
    EnetAppUtils_assert(config->ports != NULL);
    EnetAppUtils_assert(config->numPorts <= ENET_MAC_PORT_NUM);

    udmaCfg = (EnetUdma_Cfg *)config->cpswCfg.dmaCfg;
    EnetAppUtils_assert(udmaCfg != NULL);
    EnetAppUtils_assert(udmaCfg->hUdmaDrv != NULL);

    memset(&gEthFwObj, 0, sizeof(gEthFwObj));

    /* Save config parameters */
    gEthFwObj.cpswCfg = config->cpswCfg;

#if defined(ETHFW_GPTP_SUPPORT)
    /* Save gPTP stack config callback */
    gEthFwObj.configPtpCb    = config->configPtpCb;
    gEthFwObj.configPtpCbArg = config->configPtpCbArg;
#endif

    /* Get default VLAN ids for MAC-only and switch ports */
    status = EthFw_getDfltVlanId(config);

    /* Save hardware and virtual port configuration */
    if (status == ENET_SOK)
    {
        status = EthFw_getPortConfig(config);
        if (status != ENET_SOK)
        {
            appLogPrintf("ETHFW: incorrect port configuration: %d\n", status);
        }
    }

    /* Set MAC port's default VLAN id */
    if (status == ENET_SOK)
    {
        EthFw_setPortVlan();
    }

    if (status == ENET_SOK)
    {
        /* Set EthFw port mode: switch or MAC-only */
        EthFw_setPortMode();

        /* Update Enet RM according to the virtual port configuration */
        EthFw_updateEnetRm();
    }

    /* Get shared multicast table from app's config */
    if (status == ENET_SOK)
    {
        status = EthFw_getSharedMcast(config);
        if (status != ENET_SOK)
        {
            appLogPrintf("ETHFW: incorrect shared mcast configuration: %d\n", status);
        }
    }

    /* Get reserved multicast table from app's config */
    if (status == ENET_SOK)
    {
        status = EthFw_getRsvdMcast(config);
        if (status != ENET_SOK)
        {
            appLogPrintf("ETHFW: incorrect reserved mcast configuration: %d\n", status);
        }
    }

    if (status == ENET_SOK)
    {
        gEthFwObj.coreId = EnetSoc_getCoreId();
        gEthFwObj.enetType = enetType;
        gEthFwObj.instId = 0U;

        /* Populate EthFw version */
        gEthFwObj.version.major = RPMSG_KDRV_TP_ETHSWITCH_VERSION_MAJOR;
        gEthFwObj.version.minor = RPMSG_KDRV_TP_ETHSWITCH_VERSION_MINOR;
        gEthFwObj.version.rev = RPMSG_KDRV_TP_ETHSWITCH_VERSION_REVISION;

        /* __DATE__ is a string constant that contains eleven characters and
         * looks like "Feb 12 1996". If the day of the month is less than
         * 10, it is padded with a space on the left */
        memcpy(&gEthFwObj.version.month[0U],
               &date[ETHFW_VERSION_OFFSET_MONTH],
               ETHFW_VERSION_MONTHLEN);
        memcpy(&gEthFwObj.version.date[0U],
               &date[ETHFW_VERSION_OFFSET_DATE],
               ETHFW_VERSION_DATELEN);
        memcpy(&gEthFwObj.version.year[0U],
               &date[ETHFW_VERSION_OFFSET_YEAR],
               ETHFW_VERSION_YEARLEN);

        /* __TIME__ is a string in 24 hour time format */
        memcpy(&gEthFwObj.version.hour[0U],
               &time[ETHFW_VERSION_OFFSET_HOUR],
               ETHFW_VERSION_HOURLEN);
        memcpy(&gEthFwObj.version.min[0U],
               &time[ETHFW_VERSION_OFFSET_MIN],
               ETHFW_VERSION_MINLEN);
        memcpy(&gEthFwObj.version.sec[0U],
               &time[ETHFW_VERSION_OFFSET_SEC],
               ETHFW_VERSION_SECLEN);

        /* RPMSG_KDRV_TP_ETHSWITCH_VERSION_LAST_COMMIT is defined by the build system */
        memcpy(&gEthFwObj.version.commitHash[0U],
               RPMSG_KDRV_TP_ETHSWITCH_VERSION_LAST_COMMIT,
               ETHFW_VERSION_COMMITSHALEN);

        gEthFwObj.version.month[ETHFW_VERSION_MONTHLEN] = '\0';
        gEthFwObj.version.date[ETHFW_VERSION_DATELEN] = '\0';
        gEthFwObj.version.year[ETHFW_VERSION_YEARLEN] = '\0';
        gEthFwObj.version.hour[ETHFW_VERSION_HOURLEN] = '\0';
        gEthFwObj.version.min[ETHFW_VERSION_MINLEN] = '\0';
        gEthFwObj.version.sec[ETHFW_VERSION_SECLEN] = '\0';
        gEthFwObj.version.commitHash[ETHFW_VERSION_COMMITSHALEN] = '\0';
    }

#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
    /* Initialize lwIP ARP helper */
    if (status == ENET_SOK)
    {
        status = EthFwArpUtils_init();
        if (status != ETHFW_LWIP_UTILS_SOK)
        {
            appLogPrintf("ETHFW: failed to init ARP utils: %d\n", status);
        }
    }
#endif

    /* Initialize MCM */
    if (status == ENET_SOK)
    {
        status = EthFw_initMcm();
        if (status != ENET_SOK)
        {
            appLogPrintf("ETHFW: failed to init CPSW MCM: %d\n", status);
        }
        EnetAppUtils_assert(status == ENET_SOK);
    }

    /* Add ALE entry for broadcast MAC address. Note this is needed as the broadcast
     * is disabled via unknownRegMcastFloodMask and other flags in ALE init config.
     * In EthFw we need broadcast to handle ARP entries for clients */
    if (status == ENET_SOK)
    {
        status = EthFw_setAleBcastEntry();
    }

#if defined(ETHFW_GPTP_SUPPORT)
    /* Initializes tsn stack before calling gPTP task */
    if (status == ENET_SOK)
    {
        EthFw_tsnInit();
    }
#endif

    return (status == ENET_SOK) ? &gEthFwObj : NULL;
}

void EthFw_deinit(EthFw_Handle hEthFw)
{
    EnetAppUtils_assert(hEthFw != NULL);

#if defined(ETHFW_GPTP_SUPPORT)
    EthFw_tsnDeinit();
#endif

#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
    /* De-initialize lwIP ARP helper */
    EthFwArpUtils_deinit();
#endif

    /* De-initialize MCM */
    EthFw_deinitMcm();

    gEthFwObj.numPorts = 0U;
    memset(&gEthFwObj.cpswCfg, 0, sizeof(Cpsw_Cfg));
}

uint32_t EthFw_getRemoteEndptId(uint32_t coreId)
{
    uint32_t i;
    bool foundCoreId = false;
    uint32_t remoteEndptId;

    /* match the coreID the number of remote_device-based virtual ports */
    for (i = 0U; i < ENET_ARRAYSIZE(gEthFw_autosarEndptId); i++)
    {
        if(coreId == gEthFw_autosarEndptId[i].remoteCoreId)
        {
            foundCoreId = true;
            remoteEndptId = gEthFw_autosarEndptId[i].remoteEndptId;
            break;
        }
    }
    EnetAppUtils_assert(true == foundCoreId);
    return remoteEndptId;
}

int32_t EthFw_initRemoteConfig(EthFw_Handle hEthFw)
{
    CpswProxyServer_Config_t cfg;
    int32_t status;
    uint32_t i;

    EnetAppUtils_assert(hEthFw != NULL);

    /* Initialize Proxy Server */
    memset(&cfg, 0, sizeof(cfg));
    cfg.getMcmCmdIfCb = &EthFw_getMcmCmdIfCb;
    cfg.initEthfwDeviceDataCb = &EthFw_getDeviceData;
    cfg.notifyCb = &EthFw_handleProfileInfoNotify;
    cfg.rpmsgEndPointId = REMOTE_DEVICE_ENDPT;

    /* Enable MAC ports */
    cfg.numMacPorts = gEthFwObj.numPorts;
    for (i = 0U; i < cfg.numMacPorts; i++)
    {
        cfg.macPort[i] = gEthFwObj.ports[i].macPort;
    }

    /* Remote cores which use remote_device framework */
    cfg.numVirtPorts = gEthFwObj.numVirtPorts;
    for (i = 0U; i < cfg.numVirtPorts; i++)
    {
        cfg.virtPortCfg[i].remoteCoreId = gEthFwObj.virtPortCfg[i].remoteCoreId;
        cfg.virtPortCfg[i].portId       = gEthFwObj.virtPortCfg[i].portId;
        cfg.notifyServiceRemoteCoreId[i] = gEthFwObj.virtPortCfg[i].remoteCoreId;
    }

    /* AUTOSAR virtual clients */
    EnetAppUtils_assert(gEthFwObj.numAutosarVirtPorts <= CPSWPROXYSERVER_AUTOSAR_REMOTE_CLIENT_MAX);

    cfg.autosarEthVirtPortNum = gEthFwObj.numAutosarVirtPorts;
    for (i = 0U; i < cfg.autosarEthVirtPortNum; i++)
    {
        cfg.autosarEthDriverRemoteCoreId[i] = gEthFwObj.autosarVirtPortCfg[i].remoteCoreId;
        cfg.autosarEthDriverVirtPort[i]     = gEthFwObj.autosarVirtPortCfg[i].portId;
        cfg.autosarEthDeviceEndPointId[i]   = EthFw_getRemoteEndptId(cfg.autosarEthDriverRemoteCoreId[i]);
    }

    /* Enable server-to-client notify service */
    cfg.notifyServiceCpswType = gEthFwObj.enetType;

    /* Set CPSW Proxy shared multicast config.
     * All parameters have already been checked. */
    cfg.sharedMcastCfg.filterAddMacSharedCb = (CpswProxyServer_FilterAddMacSharedCb)gEthFwObj.sharedMcastCfg.filterAddMacSharedCb;
    cfg.sharedMcastCfg.filterDelMacSharedCb = (CpswProxyServer_FilterDelMacSharedCb)gEthFwObj.sharedMcastCfg.filterDelMacSharedCb;
    cfg.sharedMcastCfg.numMacAddr = gEthFwObj.sharedMcastCfg.numMacAddr;
    memcpy(&cfg.sharedMcastCfg.macAddrList[0U][0U],
           &gEthFwObj.sharedMcastCfg.macAddrList[0U][0U],
           cfg.sharedMcastCfg.numMacAddr * ENET_MAC_ADDR_LEN);

    /* Set CPSW Proxy reserved multicast config */
    cfg.rsvdMcastCfg.numMacAddr = gEthFwObj.rsvdMcastCfg.numMacAddr;
    memcpy(&cfg.rsvdMcastCfg.macAddrList[0U][0U],
           &gEthFwObj.rsvdMcastCfg.macAddrList[0U][0U],
           cfg.rsvdMcastCfg.numMacAddr * ENET_MAC_ADDR_LEN);

    cfg.dfltVlanIdMacOnlyPorts = gEthFwObj.dfltVlanIdMacOnlyPorts;
    cfg.dfltVlanIdSwitchPorts  = gEthFwObj.dfltVlanIdSwitchPorts;

    status = CpswProxyServer_init(&cfg);
    if (status != ENET_SOK)
    {
        appLogPrintf("EthFw_initRemoteConfig() failed to init CPSW Proxy: %d\n", status);
    }

    /* Start Proxy Server */
    if (status == ENET_SOK)
    {
        status = CpswProxyServer_start();
        if (status != ENET_SOK)
        {
            appLogPrintf("EthFw_initRemoteConfig() failed to start CPSW Proxy: %d\n", status);
        }
    }

    return status;
}

int32_t EthFw_lateAnnounce(EthFw_Handle hEthFw,
                           uint32_t procId)
{
    int32_t status;

    EnetAppUtils_assert(hEthFw != NULL);

    /* Late announcement of server's endpoint to remote processor */
    status = appRemoteDeviceLateAnnounce(procId);
    if (status != IPC_SOK)
    {
        appLogPrintf("EthFw_lateAnnounce: late announcement to proc %u failed: %d\n", procId, status);
    }

    return status;
}

void EthFw_getVersion(EthFw_Handle hEthFw,
                      EthFw_Version *version)
{
    EnetAppUtils_assert(hEthFw != NULL);

    *version = gEthFwObj.version;
}

static int32_t EthFw_initMcm(void)
{
    EnetMcm_InitConfig mcmCfg;
    EnetMcm_HandleInfo handleInfo;
    uint32_t i;
    int32_t status = ENET_SOK;

    /* Initialize CPSW MCM */
    mcmCfg.perCfg = (void *)&gEthFwObj.cpswCfg;
    mcmCfg.enetType = gEthFwObj.enetType;
    mcmCfg.instId = gEthFwObj.instId;
    mcmCfg.setPortLinkCfg = EthFw_initLinkArgs;
    mcmCfg.numMacPorts = gEthFwObj.numPorts;
    mcmCfg.periodicTaskPeriod = ENETPHY_FSM_TICK_PERIOD_MS;
    mcmCfg.print = appLogPrintf;

    for (i = 0U; i < gEthFwObj.numPorts; i++)
    {
        mcmCfg.macPortList[i] = gEthFwObj.ports[i].macPort;
    }

    if ((mcmCfg.enetType != ENET_CPSW_5G) &&
        (mcmCfg.enetType != ENET_CPSW_9G))
    {
        status = ENET_ENOTSUPPORTED;
    }

    if (status == ENET_SOK)
    {
        status = EnetMcm_init(&mcmCfg);
        EnetAppUtils_assert(status == ENET_SOK);
    }

    /* Get MCM command interface */
    if (status == ENET_SOK)
    {
        EnetMcm_getCmdIf(gEthFwObj.enetType, &gEthFwObj.mcmCmdIf);
        EnetAppUtils_assert(gEthFwObj.mcmCmdIf.hMboxCmd != NULL);
        EnetAppUtils_assert(gEthFwObj.mcmCmdIf.hMboxResponse != NULL);
    }

    /* Get MCM handle - CPSW driver should be open as a consequence */
    if (status == ENET_SOK)
    {
        EnetMcm_acquireHandleInfo(&gEthFwObj.mcmCmdIf, &handleInfo);
    }

    return status;
}

static void EthFw_deinitMcm(void)
{
    /* Release MCM handle - CPSW should close if we're last client */
    EnetMcm_releaseHandleInfo(&gEthFwObj.mcmCmdIf);
    EnetMcm_releaseCmdIf(gEthFwObj.enetType, &gEthFwObj.mcmCmdIf);

    /* De-initialize CPSW MCM */
    EnetMcm_deInit(gEthFwObj.enetType);
}

static void EthFw_initLinkArgs(EnetPer_PortLinkCfg *linkArgs,
                               Enet_MacPort macPort)
{
    EnetPhy_Cfg *phyCfg = &linkArgs->phyCfg;
    CpswMacPort_Cfg *macCfg = (CpswMacPort_Cfg *)linkArgs->macCfg;
    EnetMacPort_LinkCfg *linkCfg = &linkArgs->linkCfg;
    EnetMacPort_Interface *mii = &linkArgs->mii;
    uint32_t i;
    int32_t status;

    /* Port link config is set by app */
    status = gEthFwObj.setPortCfg(macPort, macCfg, mii, phyCfg, linkCfg);
    if (status != ENET_SOK)
    {
        appLogPrintf("EthFw_initLinkArgs() Failed to set MAC port %u config: %d\n",
                     ENET_MACPORT_ID(macPort), status);
        EnetAppUtils_assert(false);
    }

    for (i = 0U; i < gEthFwObj.numPorts; i++)
    {
        if (gEthFwObj.ports[i].macPort == macPort)
        {
            macCfg->vlanCfg = gEthFwObj.ports[i].vlanCfg;
        }
    }
}

static int32_t EthFw_setAleBcastEntry(void)
{
    Enet_Handle hEnet = Enet_getHandle(gEthFwObj.enetType, 0U /* instId */);
    Enet_IoctlPrms prms;
    uint32_t setMcastOutArgs;
    CpswAle_SetMcastEntryInArgs setMcastInArgs;
    uint8_t bCastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    int32_t status;

    memcpy(&setMcastInArgs.addr.addr[0], &bCastAddr[0U], sizeof(setMcastInArgs.addr.addr));
    setMcastInArgs.addr.vlanId = 0U;
    setMcastInArgs.info.super  = false;
    setMcastInArgs.info.fwdState   = CPSW_ALE_FWDSTLVL_FWD;
    setMcastInArgs.info.portMask   = CPSW_ALE_ALL_PORTS_MASK;
    setMcastInArgs.info.numIgnBits = 0U;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &setMcastInArgs, &setMcastOutArgs);

    status = Enet_ioctl(hEnet,
                        gEthFwObj.coreId,
                        CPSW_ALE_IOCTL_ADD_MCAST,
                        &prms);
    if (status != ENET_SOK)
    {
        appLogPrintf("EthFw_setAleBcastEntry() ADD_MULTICAST ioctl failed: %d\n", status);
    }

    return status;
}

/* Proxy Server callbacks */

static void EthFw_getMcmCmdIfCb(Enet_Type enetType,
                                EnetMcm_CmdIf **pMcmCmdIfHandle)
{
    *pMcmCmdIfHandle = &gEthFwObj.mcmCmdIf;
}

static void EthFw_getDeviceData(uint32_t host_id,
                                struct rpmsg_kdrv_ethswitch_device_data *eth_dev_data)
{
    eth_dev_data->fw_ver.major = gEthFwObj.version.major;
    eth_dev_data->fw_ver.minor = gEthFwObj.version.minor;
    eth_dev_data->fw_ver.rev = gEthFwObj.version.rev;

    memcpy(eth_dev_data->fw_ver.month,
           &gEthFwObj.version.month[0U],
           sizeof(eth_dev_data->fw_ver.month));

    memcpy(eth_dev_data->fw_ver.date,
           &gEthFwObj.version.date[0U],
           sizeof(eth_dev_data->fw_ver.date));

    memcpy(eth_dev_data->fw_ver.year,
           &gEthFwObj.version.year[0U],
           sizeof(eth_dev_data->fw_ver.year));

    memcpy(eth_dev_data->fw_ver.commit_hash,
           &gEthFwObj.version.commitHash[0U],
           sizeof(eth_dev_data->fw_ver.commit_hash));

    /* Enable permission for all ETHDEV remote commands without consideration of cores.
     * This should be changed based on trusted cores */
    eth_dev_data->permission_flags = ((1 << RPMSG_KDRV_TP_ETHSWITCH_MAX) - 1);
    eth_dev_data->uart_connected = true;
    eth_dev_data->uart_id = ENET_UTILS_MCU2_0_UART_INSTANCE;
}

static void EthFw_handleProfileInfoNotify(uint32_t host_id,
                                          Enet_Handle hEnet,
                                          Enet_Type enetType,
                                          uint32_t core_key,
                                          enum rpmsg_kdrv_ethswitch_client_notify_type notifyid,
                                          uint8_t *notify_info,
                                          uint32_t notify_info_len)
{
    /* Nothing to do */
}

/* PTP related functions */
#if defined(ETHFW_GPTP_SUPPORT)
static void EthFw_logTask(void *a0, void *a1)
{
    int32_t len;

    while (gEthFwObj.logTaskrun)
    {
        MutexP_lock(gEthFwObj.hLogMutex, MutexP_WAIT_FOREVER);
        len = strlen((char *)gEthFwObj.logBuf);
        if (len > 0)
        {
            memcpy(gEthFwObj.printBuf, gEthFwObj.logBuf, len);
            gEthFwObj.logBuf[0] = 0;
            gEthFwObj.printBuf[len] = 0;
        }
        MutexP_unlock(gEthFwObj.hLogMutex);

        if (len > 0)
        {
            /* The print function will take a long time, we should not
             * call it inside the mutex lock. */
            appLogPrintf("%s", gEthFwObj.printBuf);
        }

        TaskP_sleep(1000);
    }
}

static int32_t EthFw_logBuffer(bool flush, const char *str)
{
    int32_t usedLen;
    int32_t bufSizeLeft;
    int32_t loglen = strlen(str);
    int32_t status = ENET_SOK;

    MutexP_lock(gEthFwObj.hLogMutex, MutexP_WAIT_FOREVER);
    usedLen = strlen((char *)gEthFwObj.logBuf);
    bufSizeLeft = sizeof(gEthFwObj.logBuf)-usedLen;
    if (bufSizeLeft > loglen)
    {
        snprintf((char *)&gEthFwObj.logBuf[usedLen], bufSizeLeft, "%s", str);
    }
    else
    {
        snprintf((char *)&gEthFwObj.logBuf[0], sizeof(gEthFwObj.logBuf), "log overflow!\n");
    }
    MutexP_unlock(gEthFwObj.hLogMutex);

    return status;
}

static void EthFw_startLogTask(void)
{
    TaskP_Params params;

    gEthFwObj.hLogMutex = MutexP_create(&gEthFwObj.logMutexObj);

    /* Create logging task for gPTP stack */
    TaskP_Params_init(&params);
    params.priority  = ETHFW_LOGGER_TASK_PRIORITY;
    params.stack     = &gEthFwObj.logTaskStackBuf[0];
    params.stacksize = sizeof(gEthFwObj.logTaskStackBuf);
    params.name      = "ETHFW Log Task";

    gEthFwObj.hLogTask = TaskP_create(&EthFw_logTask, &params);
    if (NULL == gEthFwObj.hLogTask)
    {
        appLogPrintf("ETHFW: Failed to create log task!\n");
        EnetAppUtils_assert(false);
    }
}

static void EthFw_gptpTask(void *a0,
                           void *a1)
{
    char **netdevs = (char **)a0;
    int32_t i;

    /* Count the number of netdevs */
    for (i = 0; netdevs[i]; i++)
    {
        /* Do nothing */
    }

    /* This function start gPTP, it has a true loop inside */
    if (gptpman_run(netdevs, i, 1, NULL) < 0)
    {
        appLogPrintf("ETHFW: gptpman_run() error\n");
    }
}

static void EthFw_tsnInit(void)
{
    unibase_init_para_t params;

    if (!gEthFwObj.tsnInit)
    {
        /*refer to 'ub_logging.h for logging levels*/
        ubb_default_initpara(&params);
        params.ub_log_initstr    = "5,ubase:5,cbase:5,gptp:4";
        params.cbset.gettime64   = cb_lld_gettime64;
        params.cbset.console_out = EthFw_logBuffer;
        gEthFwObj.logTaskrun = true;

        EthFw_startLogTask();

        unibase_init(&params);
        ubb_memory_out_init(NULL, 0);
        gEthFwObj.tsnInit = true;
    }
}

static void EthFw_tsnDeinit(void)
{
    unibase_close();

    gEthFwObj.logTaskrun = false;

    if (gEthFwObj.hLogTask != NULL)
    {
        TaskP_delete(gEthFwObj.hLogTask);
        gEthFwObj.hLogTask = NULL;
    }
    if (gEthFwObj.hPtpTask != NULL)
    {
        TaskP_delete(gEthFwObj.hPtpTask);
        gEthFwObj.hPtpTask = NULL;
    }
    if (gEthFwObj.hLogMutex != NULL)
    {
        MutexP_delete(&gEthFwObj.hLogMutex);
        gEthFwObj.hLogMutex = NULL;
    }

    unibase_close();

    gEthFwObj.tsnInit = false;
    gEthFwObj.ptpStarted = false;
    gEthFwObj.logTaskrun = false;
}

static void EthFw_gptpStart(char *netdevs[])
{
    TaskP_Params params;

    if (!gEthFwObj.ptpStarted)
    {
        /* gPTP Task Init */
        TaskP_Params_init(&params);
        params.priority  = ETHFW_GPTP_TASK_PRIORITY;
        params.stack     = &gEthFwObj.gPtpStackBuf[0];
        params.stacksize = sizeof(gEthFwObj.gPtpStackBuf);
        params.name      = "ETHFW gPTP Task";
        params.arg0      = netdevs;

        gEthFwObj.hPtpTask = TaskP_create(&EthFw_gptpTask, &params);
        if (NULL == gEthFwObj.hPtpTask)
        {
            appLogPrintf("ETHFW: Failed to create gptp task!\n");
            EnetAppUtils_assert(false);
        }
        else
        {
            gEthFwObj.ptpStarted = true;
        }
    }
}
#endif

int32_t EthFw_initTimeSyncPtp(const uint8_t *hostMacAddr,
                              uint32_t portMask)
{
#if defined(ETHFW_GPTP_SUPPORT)
    lld_ethdev_t ethdevs[MAX_NUMBER_ENET_DEVS] = {0};
    Enet_MacPort macPort;
    int32_t status = ENET_SOK;
    int32_t singleClk = 1;
    int32_t i;
    int32_t j = 0;

    for (i = 0; i < ETHFW_MAC_PORT_MAX; i++)
    {
        if (ENET_IS_BIT_SET(portMask, i))
        {
            macPort = ENET_MACPORT_DENORM(i);

            /* Linking each MAC port with an interface name */
            if (j < ETHAPP_PTP_CFG_NUM_MAC_PORTS)
            {
                snprintf(&gEthFwObj.netDevs[j][0], IFNAMSIZ, "tilld%d", i + 1);
                gEthFwObj.gPtpNetDevs[j] = &gEthFwObj.netDevs[j][0];
                ethdevs[j].netdev  = gEthFwObj.netDevs[j];
                ethdevs[j].macport = macPort;
                memcpy(&ethdevs[j].srcmac, hostMacAddr, ENET_MAC_ADDR_LEN);

                appLogPrintf("ETHFW: Enable gPTP on MAC port %u (%s)\n",
                             ENET_MACPORT_ID(macPort), gEthFwObj.gPtpNetDevs[j]);

                j++;
            }
            else
            {
                appLogPrintf("ETHFW: Can't enable gPTP on MAC port %u, exceeded max number of ports (%u)\n",
                             ENET_MACPORT_ID(macPort), ETHAPP_PTP_CFG_NUM_MAC_PORTS);
                status = ENET_EINVALIDPARAMS;
                EnetAppUtils_assert(false);

            }
        }
    }

    /* Filling netdev table where each entry consists of an interface,
     * its MAC port and mac addr (if any) */
    if (status == ENET_SOK)
    {
        if (cb_lld_init_devs_table(ethdevs, j, gEthFwObj.enetType, gEthFwObj.instId) < 0)
        {
            appLogPrintf("ETHFW: Failed to int devs table\n");
            status = ENET_EFAIL;
        }
    }

    /* CPSW has a single clock for all the ports */
    gptpconf_set_item(CONF_SINGLE_CLOCK_MODE, &singleClk);

    /* Let app overwrite any gPTP configuration parameters */
    if (gEthFwObj.configPtpCb != NULL)
    {
        gEthFwObj.configPtpCb(gEthFwObj.configPtpCbArg);
    }

    EthFw_gptpStart(gEthFwObj.gPtpNetDevs);

    appLogPrintf("ETHFW: TimeSync PTP enabled\n");

    return status;
#else
    appLogPrintf("ETHFW: Warning - TimeSync is not supported\n");
    return ENET_SOK;
#endif
}
