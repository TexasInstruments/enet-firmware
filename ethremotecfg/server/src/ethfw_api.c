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

/*!
 * \file ethfw_api.c
 *
 * \brief Ethernet Firmware remote config server API.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* EthFwTrace id for this module, must be unique within ETHFW */
#define ETHFWTRACE_MOD_ID 0x101

#include <stdio.h>
#include <stdint.h>

/* PDK Driver header files */
#include <ti/osal/osal.h>
#include <ti/drv/ipc/ipc.h>
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/enet/examples/utils/include/enet_apputils.h>
#include <ti/drv/enet/examples/utils/include/enet_mcm.h>

#if defined(ETHFW_GPTP_SUPPORT)
/* Timesync header files */
#include <tsn_tilld_include.h>
#include <tsn_combase/combase.h>
#include <tsn_unibase/unibase_binding.h>
#include <tsn_gptp/gptp_config.h>
#include <tsn_gptp/gptpman.h>
#include <tsn_combase/tilld/lldenet.h>
#endif

/* EthFw header files */
#include <utils/ethfw_common/include/ethfw_utils.h>
#include <utils/ethfw_common/include/ethfw_trace.h>
#include <ethremotecfg/server/include/ethfw.h>
#include "cpsw_proxy_server.h"
#include "ethfw_mcast_priv.h"
#include "ethfw_vlan_priv.h"
#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
#include "ethfw_arp_priv.h"
#endif
#if defined(ETHFW_VEPA_SUPPORT)
#include "ethfw_vepa_priv.h"
#endif

/* EthFw utils header files */
#include <utils/console_io/include/app_log.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/*! Month substring offset in date string */
#define ETHFWVERSION_OFFSET_MONTH                    (0)

/*! Date substring offset in date string */
#define ETHFWVERSION_OFFSET_DATE                     (4)

/*! Year substring offset in data string */
#define ETHFWVERSION_OFFSET_YEAR                     (7)

/*! Month substring offset in date string */
#define ETHFWVERSION_OFFSET_HOUR                     (0)

/*! Date substring offset in date string */
#define ETHFWVERSION_OFFSET_MIN                      (3)

/*! Year substring offset in data string */
#define ETHFWVERSION_OFFSET_SEC                      (6)

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
#define ETHFW_MAC_ONLY_PORTS_VLAN_ID                  (0U)

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
#define ETHAPP_PTP_CFG_NUM_MAC_PORTS                  ETHFW_MAC_PORT_MAX
#endif

/* Compile time check for error value consistency with Enet LLD (and CSL) */
#define ETHFW_UTILS_COMPILETIME_ENET_CHECK(x)         ETHFW_UTILS_COMPILETIME_ASSERT(ETHFW_##x == ENET_##x)

#if defined(ETHFW_MONITOR_SUPPORT)
/*! Monitor Task priority */
#define ETHFW_MON_TASK_PRIORITY                       (2U)

/*! Monitor Task stack size and alignment */
#if defined(SAFERTOS)
#define ETHFW_MON_TASK_STACK_SIZE                     (16U * 1024U)
#define ETHFW_MON_TASK_STACK_ALIGN                    ETHFW_MON_TASK_STACK_SIZE
#else
#define ETHFW_MON_TASK_STACK_SIZE                     (16U * 1024U)
#define ETHFW_MON_TASK_STACK_ALIGN                    (32U)
#endif 

/*! Monitor task polling period */
#define ETHFW_MON_TASK_POLL_PERIOD_MS                 (100U)

/*! Remote client idle status check period */
#define ETHFW_MON_HWRECOVERY_IDLE_CHECK_PERIOD_MS     (100U)

/*! Number of client idle check retries before printing number of
 * clients that have idled.  Each retry iteration takes
 * ETHFW_MON_HWRECOVERY_IDLE_CHECK_PERIOD_MS. */
#define ETHFW_MON_HWRECOVERY_RETRY_LOG_ITER           (10U)

/*! Value of seconds in nanoseconds. Useful for calculations */
#define ETHFW_TIME_SEC_TO_NS                          (1000000000U)
#endif

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct EthFw_Port_s
{
    /* MAC port number */
    Enet_MacPort macPort;

    /* Port VLAN config */
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

    /* Default VLAN id to be used for MAC ports configured in MAC-only mode */
    uint16_t dfltVlanIdMacOnlyPorts;

    /* Default VLAN id to be used for MAC ports configured in switch mode (non MAC-only) */
    uint16_t dfltVlanIdSwitchPorts;

    /* Multiclient Manager (MCM) handle */
    EnetMcm_CmdIf mcmCmdIf;

    /* Callback function for application to set port link parameters */
    EthFw_setPortCfg setPortCfg;

    /* Remote client alloc object */
    EthFw_AllocCfg clientAllocCfg[ETHFW_REMOTE_CLIENT_ALLOC_MAX];

    /* Number of remote clients with resource allocation */
    uint32_t numClients;

#if defined(ETHFW_MONITOR_SUPPORT)
    /* Whether recovery is enabled or not */
    bool recoveryEn;

    /* Monitor and recovery callbacks */
    EthFw_MonitorCfg monitor;

    /* Saved statistics counters */
    EthFw_MonStats monStats[ETHFW_MAC_PORT_MAX + 1U];

    /* CPSW statistics block */
    CpswStats_PortStats cpswStats;

    /*! Clock handle for Monitor Task */
    ClockP_Handle hMonitorClock;

    /*! Semaphore handle for Monitor Task */
    SemaphoreP_Handle hMonitorSem;

    /*! Task handle for Monitor Task */
    TaskP_Handle hMonitorTask;

    /* To run Monitor Task */
    bool monitorTaskRun;

    /* Monitor Task stack buffer */
    uint8_t monTaskStackBuf[ETHFW_MON_TASK_STACK_SIZE] __attribute__ ((aligned(ETHFW_MON_TASK_STACK_ALIGN)));
#endif

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

    /* Number of active netdevs */
    uint32_t numNetDevs;

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

static void EthFw_getDeviceData(EthRemoteCfg_DeviceData *ethdevData);

static void EthFw_handleProfileInfoNotify(uint32_t host_id,
                                          Enet_Handle hEnet,
                                          Enet_Type enetType,
                                          uint32_t core_key,
                                          EthRemoteCfg_NotifyType notifyid,
                                          uint8_t *notify_info,
                                          uint32_t notify_info_len);

#if defined(ETHFW_MONITOR_SUPPORT)
static int32_t EthFw_startMonitorTask(void);

static void EthFw_stopMonitorTask(void);

static void EthFw_monitorTask(void *a0,
                              void *a1);

static int32_t EthFw_resetHandler(void);
#endif

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

/* Policer table partition into chunks of different size each having different
 * priority CPSW_ALE_POLICER_PARTITION_DEFAULT having lowest and
 * CPSW_ALE_POLICER_PARTITION_LEVEL_1 having the highest priority */
static uint32_t gEthFw_policerTablePartSize[CPSW_ALE_POLICER_TABLE_PART_MAX] =
{
    [CPSW_ALE_POLICER_PARTITION_LEVEL_1] = 10U,
    [CPSW_ALE_POLICER_PARTITION_LEVEL_2] = 20U,
    [CPSW_ALE_POLICER_PARTITION_LEVEL_3] = 0U,
    [CPSW_ALE_POLICER_PARTITION_LEVEL_4] = 0U,
    /* Give the unused/default partition size to be 0
     * Partitions with size 0 are clubbed to default partition */
    [CPSW_ALE_POLICER_PARTITION_DEFAULT] = 0U
};
/* Note: Sum of partition sizes must be <= Total number of policer entries available */

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

static void EthFw_compileTimeChecks(void)
{
    /* Verify that ETHFW error types are consistent with Enet LLD error types */
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(SOK);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(SINPROGRESS);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(EFAIL);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(EBADARGS);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(EINVALIDPARAMS);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(ETIMEOUT);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(EALLOC);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(EUNEXPECTED);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(EBUSY);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(EALREADYOPEN);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(EPERM);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(ENOTSUPPORTED);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(ENOTFOUND);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(EUNKNOWNIOCTL);
    ETHFW_UTILS_COMPILETIME_ENET_CHECK(EMALFORMEDIOCTL);
}

static void EthFw_initAleCfg(CpswAle_Cfg *aleCfg)
{
    /* ALE configuration */
    aleCfg->modeFlags = CPSW_ALE_CFG_MODULE_EN;
#if defined(ETHFW_VEPA_SUPPORT)
    aleCfg->modeFlags |= CPSW_ALE_CFG_MULTIHOST;
#endif

    aleCfg->agingCfg.autoAgingEn     = BTRUE;
    aleCfg->agingCfg.agingPeriodInMs = 1000;

    aleCfg->nwSecCfg.vid0ModeEn = BFALSE;

    aleCfg->vlanCfg.aleVlanAwareMode           = BTRUE;
    aleCfg->vlanCfg.cpswVlanAwareMode          = BTRUE;
    aleCfg->vlanCfg.unknownUnregMcastFloodMask = 0U;
    aleCfg->vlanCfg.unknownRegMcastFloodMask   = 0U;
    aleCfg->vlanCfg.unknownVlanMemberListMask  = CPSW_ALE_ALL_PORTS_MASK;
    aleCfg->vlanCfg.autoLearnWithVlan          = BFALSE;

    /* ALE policer configuration */
    aleCfg->policerGlobalCfg.policingEn         = BTRUE;
    aleCfg->policerGlobalCfg.yellowDropEn       = BFALSE;
    aleCfg->policerGlobalCfg.redDropEn          = BTRUE;
    aleCfg->policerGlobalCfg.policerNoMatchMode = CPSW_ALE_POLICER_NOMATCH_MODE_GREEN;

    /* ALE policer partition configuration */
    memcpy(aleCfg->policerTablePartSize, gEthFw_policerTablePartSize, sizeof(gEthFw_policerTablePartSize));
}

static int32_t EthFw_getDfltVlanId(const EthFw_Config *config)
{
    int32_t status = ENET_SOK;

    if (config->dfltVlanIdMacOnlyPorts > ETHFW_VLAN_ID_MAX)
    {
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "Default VLAN id %u for MAC-only ports is out-of-range",
                     config->dfltVlanIdMacOnlyPorts);
    }

    if (config->dfltVlanIdSwitchPorts > ETHFW_VLAN_ID_MAX)
    {
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "Default VLAN id %u for switch ports is out-of-range",
                     config->dfltVlanIdSwitchPorts);
    }

    if (config->dfltVlanIdMacOnlyPorts == config->dfltVlanIdSwitchPorts)
    {
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "Default VLAN Id should not be same for MAC-only and switch ports (%u)",
                     config->dfltVlanIdSwitchPorts);
    }

    ETHFWTRACE_WARN_IF((config->dfltVlanIdMacOnlyPorts != 0U),
                       "Default VLAN of MAC-only ports is %u, "
                       "promiscuous mode will not be functional if VLAN is not 0",
                       config->dfltVlanIdMacOnlyPorts);

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
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "Invalid setPortCfg callback");
    }

    if (config->numPorts > ETHFW_MAC_PORT_MAX)
    {
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "Too many MAC ports requested (%u), max is %u",
                     config->numPorts, ETHFW_MAC_PORT_MAX);
    }

    if (config->numVirtPorts > ETHFW_REMOTE_CLIENT_MAX)
    {
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "Too many virtual ports requested (%u), max is %u",
                       config->numVirtPorts, ETHFW_REMOTE_CLIENT_MAX);
    }

    if (config->numAutosarVirtPorts > CPSWPROXYSERVER_AUTOSAR_REMOTE_CLIENT_MAX)
    {
        status = ENET_EINVALIDPARAMS;
        ETHFWTRACE_ERR(status, "Too many AUTOSAR virtual ports requested (%u), max is %u",
                       config->numAutosarVirtPorts, CPSWPROXYSERVER_AUTOSAR_REMOTE_CLIENT_MAX);
    }

    for (i = 0U; i <= ENET_MAC_PORT_NUM; i++)
    {
        /* Default VLAN of all ports is set to ETHFW_VLAN_ID_MAX, as 0U is reserved for MAC only ports */
        gEthFwObj.cpswCfg.aleCfg.portCfg[i].pvidCfg.vlanIdInfo.vlanId = ETHFW_VLAN_ID_MAX;
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

static int32_t EthFw_setupVlan(const EthFw_Config *config)
{
    Enet_Handle hEnet;
    EthFwVlan_Cfg vlanCfg;
    int32_t status = ENET_SOK;

    vlanCfg.vlanCfg  = config->vlanCfg;
    vlanCfg.numVlans = config->numVlans;
    vlanCfg.dfltVlanIdSwitchPorts  = gEthFwObj.dfltVlanIdSwitchPorts;
    vlanCfg.dfltVlanIdMacOnlyPorts = gEthFwObj.dfltVlanIdMacOnlyPorts;
    vlanCfg.switchPortMask  = gEthFwObj.switchPortMask;
    vlanCfg.macOnlyPortMask = gEthFwObj.macOnlyPortMask;

    hEnet = Enet_getHandle(gEthFwObj.enetType, 0U /* instId */);
    if (hEnet != NULL)
    {
        status = EthFwVlan_init(hEnet, &vlanCfg);
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Incorrect VLAN configuration");
    }
    else
    {
        status = ENET_EFAIL;
        ETHFWTRACE_ERR(status, "Failed to get Enet handle");
    }

    return status;
}

static bool EthFw_isMacOnlyPort(Enet_MacPort macPort)
{
    bool isMacOnly = BFALSE;

    if ((gEthFwObj.macOnlyPortMask & ENET_MACPORT_MASK(macPort)) != 0U)
    {
        isMacOnly = BTRUE;
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
            macModeCfg->macOnlyCafEn = BFALSE;
            macModeCfg->macOnlyEn    = BTRUE;
            learningCfg->noLearn     = BTRUE;
            vlanSecCfg->dropUntagged = BFALSE;

            pvidCfg->vlanIdInfo.tagType  = ENET_VLAN_TAG_TYPE_INNER;
            pvidCfg->vlanIdInfo.vlanId   = gEthFwObj.dfltVlanIdMacOnlyPorts;
            pvidCfg->vlanMemberList      = aleMacOnlyPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->regMcastFloodMask   = aleMacOnlyPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->unregMcastFloodMask = aleMacOnlyPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->forceUntaggedEgressMask = aleMacOnlyPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->noLearnMask     = 0U;
            pvidCfg->vidIngressCheck = BFALSE;
            pvidCfg->limitIPNxtHdr   = BFALSE;
            pvidCfg->disallowIPFrag  = BFALSE;
        }
        else
        {
            macModeCfg->macOnlyCafEn = BFALSE;
            macModeCfg->macOnlyEn    = BFALSE;
            learningCfg->noLearn     = BFALSE;
            vlanSecCfg->dropUntagged = BFALSE;

            pvidCfg->vlanIdInfo.tagType  = ENET_VLAN_TAG_TYPE_INNER;
            pvidCfg->vlanIdInfo.vlanId   = gEthFwObj.dfltVlanIdSwitchPorts;
            pvidCfg->vlanMemberList      = aleSwitchPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->regMcastFloodMask   = aleSwitchPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->unregMcastFloodMask = 0U;
            pvidCfg->forceUntaggedEgressMask = aleSwitchPortMask | CPSW_ALE_HOST_PORT_MASK;
            pvidCfg->noLearnMask     = 0U;
            pvidCfg->vidIngressCheck = BFALSE;
            pvidCfg->limitIPNxtHdr   = BFALSE;
            pvidCfg->disallowIPFrag  = BFALSE;
        }
    }

    /* ALE host port configuration */
    pvidCfg = &aleCfg->portCfg[0].pvidCfg;
    pvidCfg->vlanIdInfo.tagType  = ENET_VLAN_TAG_TYPE_INNER;
    pvidCfg->vlanIdInfo.vlanId   = ETHFW_HOST_PORT_VLAN_ID;
    pvidCfg->vlanMemberList      = CPSW_ALE_ALL_PORTS_MASK;
    pvidCfg->regMcastFloodMask   = CPSW_ALE_ALL_PORTS_MASK;
    pvidCfg->unregMcastFloodMask = 0U;
    pvidCfg->forceUntaggedEgressMask = CPSW_ALE_ALL_PORTS_MASK;
    pvidCfg->noLearnMask     = 0U;
    pvidCfg->vidIngressCheck = BFALSE;
    pvidCfg->limitIPNxtHdr   = BFALSE;
    pvidCfg->disallowIPFrag  = BFALSE;

    learningCfg = &aleCfg->portCfg[0].learningCfg;
    learningCfg->noLearn      = BFALSE;
#if defined(ETHFW_VEPA_SUPPORT)
    learningCfg->noSaUpdateEn = BTRUE;
#endif

    vlanSecCfg = &aleCfg->portCfg[0].vlanCfg;
    vlanSecCfg->dropUntagged   = BFALSE;
#if defined(ETHFW_VEPA_SUPPORT)
    vlanSecCfg->dropDoubleVlan = BFALSE;
    vlanSecCfg->dropDualVlan   = BTRUE;
#endif
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
        ETHFWTRACE_ERR(ENET_EALLOC, "Empty MAC address pool");
        EnetAppUtils_assert(BFALSE);
    }
    else if (resCfg->macList.numMacAddress < req)
    {
        ETHFWTRACE_WARN("MAC address pool size is too small (req=%u alloc=%u),"
                        "may not be sufficient depending on concurrent usage",
                        req, resCfg->macList.numMacAddress);
    }
}

void EthFw_initConfigParams(Enet_Type enetType,
                            EthFw_Config *config)
{
#if defined(ETHFW_MONITOR_SUPPORT)
    EthFw_MonitorCfg *monCfg = &config->monitorCfg;
#endif
    Cpsw_Cfg *cpswCfg = &config->cpswCfg;
    CpswAle_Cfg *aleCfg = &cpswCfg->aleCfg;
    Cpsw_VlanCfg *vlanCfg = &cpswCfg->vlanCfg;
    CpswHostPort_Cfg *hostPortCfg = &cpswCfg->hostPortCfg;
    CpswCpts_Cfg *cptsCfg = &cpswCfg->cptsCfg;
    EnetRm_ResCfg *resCfg = &cpswCfg->resCfg;
    uint32_t instId = 0U;

    memset(config, 0, sizeof(*config));

    /* MAC port ownership */
    config->ports = NULL;
    config->numPorts = 0U;

    /* Virtual ports (remote_device based) */
    config->virtPortCfg = NULL;
    config->numVirtPorts = 0U;

    /* VLAN configuration */
    config->vlanCfg = NULL;
    config->numVlans = 0U;

    /* Multicast configuration */
    config->mcastCfg.sharedMcastCfg.filterAddMacSharedCb = NULL;
    config->mcastCfg.sharedMcastCfg.filterDelMacSharedCb = NULL;
    config->mcastCfg.sharedMcastCfg.mcastCfg = NULL;
    config->mcastCfg.sharedMcastCfg.numMcast = 0U;
    config->mcastCfg.rsvdMcastCfg.mcastCfg = NULL;
    config->mcastCfg.rsvdMcastCfg.numMcast = 0U;

    /* Virtual ports (bare IPC, AUTOSAR) */
    config->autosarVirtPortCfg = NULL;
    config->numAutosarVirtPorts = 0U;

    /* Default VLAN ids */
    config->dfltVlanIdMacOnlyPorts = ETHFW_MAC_ONLY_PORTS_VLAN_ID;
    config->dfltVlanIdSwitchPorts  = ETHFW_SWITCH_PORTS_VLAN_ID;

#if defined(ETHFW_VEPA_SUPPORT)
    /* Initialize VEPA config params */
    EthFwVepa_initCfg(&config->vepaCfg);
#endif

    /* Start with CPSW LLD's default configuration */
    Enet_initCfg(enetType, instId, cpswCfg, sizeof (*cpswCfg));
    cpswCfg->dmaCfg = NULL;
    resCfg->ioctlPermissionInfo = gEthFw_rmIoctlPerm;
    resCfg->selfCoreId = EnetSoc_getCoreId();
    resCfg->macList.numMacAddress = 0U;

    /* Disable CPTS host receive timestamping as it can cause
     * MAC port lockup in packets with corrupted SFD */
    cptsCfg->hostRxTsEn = BFALSE;

    /* VLAN configuration */
    vlanCfg->vlanAware = BTRUE;

#if defined(ETHFW_VEPA_SUPPORT)
    /* CPSW switching using INNER VLAN tag
     * VLAN_LTYPE_SEL value is selected by the S_CN_SWITCH
     * i.e. VLAN processing uses inner_vlan_ltype
     * vlanCfg->vlanSwitch = ENET_VLAN_TAG_TYPE_INNER
     * As VLAN_LTYPE_OUTER is same as VLAN_LTYPE_INNER
     * double tagged packet will be looked as single tagged by ALE */
    vlanCfg->innerVlan = ENET_ETHERTYPE_CUSTOMER_VLAN;
    vlanCfg->outerVlan = ENET_ETHERTYPE_CUSTOMER_VLAN;
#endif
    /* Host port configuration */
    hostPortCfg->removeCrc       = BTRUE;
    hostPortCfg->padShortPacket  = BTRUE;
    hostPortCfg->passCrcErrors   = BTRUE;
    hostPortCfg->rxMtu           = 1522U;
    hostPortCfg->vlanCfg.portVID = ETHFW_HOST_PORT_VLAN_ID;
#if defined(ETHFW_CPSW_MULTIHOST_CHECKSUM_ERRATA)
    hostPortCfg->csumOffloadEn   = BFALSE;
#else
    hostPortCfg->csumOffloadEn   = BTRUE;
#endif

    EthFw_initAleCfg(aleCfg);

#if defined(ETHFW_MONITOR_SUPPORT)
    monCfg->periodInMsecs     = ETHFW_MON_TASK_POLL_PERIOD_MS;
    monCfg->openLwipDmaCb     = NULL;
    monCfg->closeLwipDmaCb    = NULL;
    monCfg->lwipDmaCbArg      = NULL;
    monCfg->statsMonHostEvtCb = NULL;
    monCfg->statsMonMacEvtCb  = NULL;
    monCfg->statsMonCbArg     = NULL;
#endif
}

EthFw_Handle EthFw_init(Enet_Type enetType,
                        const EthFw_Config *config)
{
    EnetUdma_Cfg *udmaCfg;
    char *date = __DATE__;
    char *time = __TIME__;
    uint32_t i;
    int32_t status = ENET_SOK;

    EthFw_compileTimeChecks();

    EnetAppUtils_assert(config != NULL);
    EnetAppUtils_assert(config->ports != NULL);
    EnetAppUtils_assert(config->numPorts <= ENET_MAC_PORT_NUM);

    udmaCfg = (EnetUdma_Cfg *)config->cpswCfg.dmaCfg;
    EnetAppUtils_assert(udmaCfg != NULL);
    EnetAppUtils_assert(udmaCfg->hUdmaDrv != NULL);

    memset(&gEthFwObj, 0, sizeof(gEthFwObj));

    /* Get the allocated resources for all remote clients */
    gEthFwObj.numClients = config->numAlloc;
    for (i = 0U; i < gEthFwObj.numClients; i++)
    {
        gEthFwObj.clientAllocCfg[i] = config->allocCfg[i];
    }

    /* Save config parameters */
    gEthFwObj.cpswCfg = config->cpswCfg;

#if defined(ETHFW_MONITOR_SUPPORT)
    gEthFwObj.monitor = config->monitorCfg;
    gEthFwObj.recoveryEn = ((gEthFwObj.monitor.openLwipDmaCb != NULL) &&
                            (gEthFwObj.monitor.closeLwipDmaCb != NULL));
    ETHFWTRACE_INFO_IF(!gEthFwObj.recoveryEn, "CPSW recovery is not enabled");
#endif

#if defined(ETHFW_GPTP_SUPPORT)
    /* Save gPTP stack config callback */
    gEthFwObj.configPtpCb    = config->configPtpCb;
    gEthFwObj.configPtpCbArg = config->configPtpCbArg;
#endif

    /* Get default VLAN ids for MAC-only and switch ports */
    status = EthFw_getDfltVlanId(config);
    ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to get default VLAN ids");

    /* Save hardware and virtual port configuration */
    if (status == ENET_SOK)
    {
        status = EthFw_getPortConfig(config);
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Incorrect port configuration");
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

    if (status == ENET_SOK)
    {
        gEthFwObj.coreId = EnetSoc_getCoreId();
        gEthFwObj.enetType = enetType;
        gEthFwObj.instId = 0U;

        /* Populate EthFw version */
        gEthFwObj.version.major = ETHREMOTECFG_FW_ETHSWITCH_VERSION_MAJOR;
        gEthFwObj.version.minor = ETHREMOTECFG_FW_ETHSWITCH_VERSION_MINOR;
        gEthFwObj.version.rev = ETHREMOTECFG_FW_ETHSWITCH_VERSION_REVISION;

        /* __DATE__ is a string constant that contains eleven characters and
         * looks like "Feb 12 1996". If the day of the month is less than
         * 10, it is padded with a space on the left */
        memcpy(&gEthFwObj.version.month[0U],
               &date[ETHFWVERSION_OFFSET_MONTH],
               ETHFW_VERSION_MONTHLEN);
        memcpy(&gEthFwObj.version.date[0U],
               &date[ETHFWVERSION_OFFSET_DATE],
               ETHFW_VERSION_DATELEN);
        memcpy(&gEthFwObj.version.year[0U],
               &date[ETHFWVERSION_OFFSET_YEAR],
               ETHFW_VERSION_YEARLEN);


        /* __TIME__ is a string in 24 hour time format */
        memcpy(&gEthFwObj.version.hour[0U],
               &time[ETHFWVERSION_OFFSET_HOUR],
               ETHFW_VERSION_HOURLEN);
        memcpy(&gEthFwObj.version.min[0U],
               &time[ETHFWVERSION_OFFSET_MIN],
               ETHFW_VERSION_MINLEN);
        memcpy(&gEthFwObj.version.sec[0U],
               &time[ETHFWVERSION_OFFSET_SEC],
               ETHFW_VERSION_SECLEN);

        /* ETHRPC_ETHSWITCH_VERSION_LAST_COMMIT is defined by the build system */
        memcpy(&gEthFwObj.version.commitHash[0U],
               ETHREMOTECFG_ETHSWITCH_VERSION_LAST_COMMIT,
               ETHFW_VERSION_COMMITSHALEN);

        gEthFwObj.version.month[ETHFW_VERSION_MONTHLEN] = '\0';
        gEthFwObj.version.date[ETHFW_VERSION_DATELEN] = '\0';
        gEthFwObj.version.year[ETHFW_VERSION_YEARLEN] = '\0';
        gEthFwObj.version.hour[ETHFW_VERSION_HOURLEN] = '\0';
        gEthFwObj.version.min[ETHFW_VERSION_MINLEN] = '\0';
        gEthFwObj.version.sec[ETHFW_VERSION_SECLEN] = '\0';
        gEthFwObj.version.commitHash[ETHFW_VERSION_COMMITSHALEN] = '\0';
    }

    /* Initialize multicast support */
    if (status == ENET_SOK)
    {
        status = EthFwMcast_init(&config->mcastCfg,
                                 gEthFwObj.switchPortMask,
                                 gEthFwObj.macOnlyPortMask);
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Incorrect shared mcast configuration");
    }

#if (defined(FREERTOS) || defined(SAFERTOS)) && defined(ETHFW_PROXY_ARP_HANDLING)
    /* Initialize lwIP ARP helper */
    if (status == ENET_SOK)
    {
        status = EthFwArp_init();
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to init ARP utils");
    }
#endif

#if defined(ETHFW_VEPA_SUPPORT)
    status = EthFwVepa_init(&config->vepaCfg);
    ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to init VEPA utils");
#endif

    /* Initialize MCM */
    if (status == ENET_SOK)
    {
        status = EthFw_initMcm();
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to init CPSW MCM");
        EnetAppUtils_assert(status == ENET_SOK);
    }

    /* Add ALE entry for broadcast MAC address. Note this is needed as the broadcast
     * is disabled via unknownRegMcastFloodMask and other flags in ALE init config.
     * In EthFw we need broadcast to handle ARP entries for clients */
    if (status == ENET_SOK)
    {
        status = EthFw_setAleBcastEntry();
    }

    /* Setup static VLANs */
    if (status == ENET_SOK)
    {
        status = EthFw_setupVlan(config);
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to setup static VLANs");
    }

#if defined(ETHFW_GPTP_SUPPORT)
    /* Initializes tsn stack before calling gPTP task */
    if (status == ENET_SOK)
    {
        EthFw_tsnInit();
    }
#endif

#if defined(ETHFW_MONITOR_SUPPORT)
    /* Start the Monitor Task */
    if (status == ENET_SOK)
    {
        status = EthFw_startMonitorTask();
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to start monitor task");
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
    EthFwArp_deinit();
#endif

#if defined(ETHFW_VEPA_SUPPORT)
    /* De-initialize VEPA table */
    EthFwVepa_deinit();
#endif

    EthFwMcast_deinit();

#if defined(ETHFW_MONITOR_SUPPORT)
    /* Stop the Monitor Task */
    EthFw_stopMonitorTask();
#endif

    /* De-initialize MCM */
    EthFw_deinitMcm();

    gEthFwObj.numPorts = 0U;
    memset(&gEthFwObj.cpswCfg, 0, sizeof(Cpsw_Cfg));
}

uint32_t EthFw_getRemoteEndptId(uint32_t coreId)
{
    uint32_t i;
    bool foundCoreId = BFALSE;
    uint32_t remoteEndptId;

    /* match the coreID the number of remote_device-based virtual ports */
    for (i = 0U; i < ENET_ARRAYSIZE(gEthFw_autosarEndptId); i++)
    {
        if(coreId == gEthFw_autosarEndptId[i].remoteCoreId)
        {
            foundCoreId = BTRUE;
            remoteEndptId = gEthFw_autosarEndptId[i].remoteEndptId;
            break;
        }
    }
    EnetAppUtils_assert(BTRUE == foundCoreId);
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
    cfg.instId = gEthFwObj.instId;
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
        cfg.autosarPortCfg[i].remoteCoreId = gEthFwObj.autosarVirtPortCfg[i].remoteCoreId;
        cfg.autosarPortCfg[i].portId    = gEthFwObj.autosarVirtPortCfg[i].portId;
        cfg.autosarEthDeviceEndPointId[i]   = EthFw_getRemoteEndptId(cfg.autosarPortCfg[i].remoteCoreId);
    }

    /* Alloc resources for the remote clients */
    EnetAppUtils_assert(gEthFwObj.numClients <= CPSWPROXYSERVER_REMOTE_CLIENT_ALLOC_MAX);

    cfg.numAllocObj = gEthFwObj.numClients;
    for (i = 0U; i < cfg.numAllocObj; i++)
    {
        cfg.allocObj[i].clientId = gEthFwObj.clientAllocCfg[i].clientId;
        cfg.allocObj[i].remoteProcId = gEthFwObj.clientAllocCfg[i].remoteProcId;
        cfg.allocObj[i].virtMacPortMask = gEthFwObj.clientAllocCfg[i].virtMacPortMask;
        cfg.allocObj[i].virtSwitchPortMask = gEthFwObj.clientAllocCfg[i].virtSwitchPortMask;
    }

    cfg.dfltVlanIdMacOnlyPorts = gEthFwObj.dfltVlanIdMacOnlyPorts;
    cfg.dfltVlanIdSwitchPorts  = gEthFwObj.dfltVlanIdSwitchPorts;

    cfg.enabledPortMask = gEthFwObj.enabledPortMask;
    cfg.macOnlyPortMask = gEthFwObj.macOnlyPortMask;

    status = CpswProxyServer_init(&cfg);
    ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "Failed to init CPSW Proxy");

    return status;
}

int32_t EthFw_lateAnnounce(EthFw_Handle hEthFw,
                           uint32_t procId)
{
    int32_t status;

    EnetAppUtils_assert(hEthFw != NULL);

    /* Late announcement of server's endpoint to remote processor */
    status = CpswProxyServer_lateAnnounce(procId);
    ETHFWTRACE_ERR_IF((status != IPC_SOK), status, "Late announcement to proc %u failed", procId);

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
    mcmCfg.print = EthFwTrace_print;
    mcmCfg.traceTsFunc = NULL;
    mcmCfg.extTraceFunc = NULL;

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
        ETHFWTRACE_ERR(status, "Failed to set MAC port %u config", ENET_MACPORT_ID(macPort));
        EnetAppUtils_assert(BFALSE);
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
    setMcastInArgs.addr.vlanId     = 0U;
    setMcastInArgs.info.super      = BFALSE;
    setMcastInArgs.info.fwdState   = CPSW_ALE_FWDSTLVL_FWD;
    setMcastInArgs.info.portMask   = CPSW_ALE_ALL_PORTS_MASK;
    setMcastInArgs.info.numIgnBits = 0U;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &setMcastInArgs, &setMcastOutArgs);

    status = Enet_ioctl(hEnet,
                        gEthFwObj.coreId,
                        CPSW_ALE_IOCTL_ADD_MCAST,
                        &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to add bcast ALE entry");

    return status;
}

/* Proxy Server callbacks */

static void EthFw_getMcmCmdIfCb(Enet_Type enetType,
                                EnetMcm_CmdIf **pMcmCmdIfHandle)
{
    *pMcmCmdIfHandle = &gEthFwObj.mcmCmdIf;
}

static void EthFw_getDeviceData(EthRemoteCfg_DeviceData *ethdevData)
{
    ethdevData->fwVer.major = gEthFwObj.version.major;
    ethdevData->fwVer.minor = gEthFwObj.version.minor;
    ethdevData->fwVer.rev = gEthFwObj.version.rev;

    memcpy(ethdevData->fwVer.month,
           &gEthFwObj.version.month[0U],
           sizeof(ethdevData->fwVer.month));

    memcpy(ethdevData->fwVer.date,
           &gEthFwObj.version.date[0U],
           sizeof(ethdevData->fwVer.date));

    memcpy(ethdevData->fwVer.year,
           &gEthFwObj.version.year[0U],
           sizeof(ethdevData->fwVer.year));

    memcpy(ethdevData->fwVer.commitHash,
           &gEthFwObj.version.commitHash[0U],
           sizeof(ethdevData->fwVer.commitHash));

    /* Enable permission for all ETHDEV remote commands without consideration of cores.
     * This should be changed based on trusted cores */
    ethdevData->permissionFlags = ((1ULL << ETHREMOTECFG_CMD_TYPE_LAST) - 1);
    ethdevData->uartConnected = BTRUE;
    ethdevData->uartId = ENET_UTILS_MCU2_0_UART_INSTANCE;
}

static void EthFw_handleProfileInfoNotify(uint32_t hostId,
                                          Enet_Handle hEnet,
                                          Enet_Type enetType,
                                          uint32_t coreKey,
                                          EthRemoteCfg_NotifyType notifyid,
                                          uint8_t *notifyInfo,
                                          uint32_t notifyInfoLen)
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
            EthFwTrace_print("%s", gEthFwObj.printBuf);
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
        ETHFWTRACE_ERR(ENET_EFAIL, "Failed to create log task");
        EnetAppUtils_assert(BFALSE);
    }
}

static void EthFw_gptpTask(void *a0,
                           void *a1)
{
    char **netdevs = (char **)a0;
    uint32_t numNetDevs = (uint32_t)a1;
    int32_t i;
    int32_t status;

    /* This function start gPTP, it has a true loop inside */
    status = gptpman_run(netdevs, numNetDevs, 1, NULL);
    ETHFWTRACE_ERR_IF((status < 0), status, "gptpman_run() failed");
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
        gEthFwObj.logTaskrun = BTRUE;

        EthFw_startLogTask();

        unibase_init(&params);
        ubb_memory_out_init(NULL, 0);
        gEthFwObj.tsnInit = BTRUE;
    }
}

static void EthFw_tsnDeinit(void)
{
    unibase_close();

    gEthFwObj.logTaskrun = BFALSE;

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

    gEthFwObj.tsnInit    = BFALSE;
    gEthFwObj.ptpStarted = BFALSE;
    gEthFwObj.logTaskrun = BFALSE;
}

static void EthFw_gptpStart(char *netdevs[],
                            uint32_t numNetDevs)
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
        params.arg1      = (void *)numNetDevs;

        gEthFwObj.hPtpTask = TaskP_create(&EthFw_gptpTask, &params);
        if (NULL == gEthFwObj.hPtpTask)
        {
            ETHFWTRACE_ERR(ETHFW_EFAIL, "Failed to create gptp task");
            EnetAppUtils_assert(BFALSE);
        }
        else
        {
            gEthFwObj.ptpStarted = BTRUE;
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
            snprintf(&gEthFwObj.netDevs[j][0], IFNAMSIZ, "tilld%d", i + 1);
            gEthFwObj.gPtpNetDevs[j] = &gEthFwObj.netDevs[j][0];
            ethdevs[j].netdev  = gEthFwObj.netDevs[j];
            ethdevs[j].macport = macPort;
            memcpy(&ethdevs[j].srcmac, hostMacAddr, ENET_MAC_ADDR_LEN);

            ETHFWTRACE_INFO("Enable gPTP on MAC port %u (%s)",
                            ENET_MACPORT_ID(macPort), gEthFwObj.gPtpNetDevs[j]);
            j++;
        }
    }

    gEthFwObj.numNetDevs = j;

    /* Filling netdev table where each entry consists of an interface,
     * its MAC port and mac addr (if any) */
    if (status == ENET_SOK)
    {
        status  = cb_lld_init_devs_table(ethdevs, gEthFwObj.numNetDevs,
                                         gEthFwObj.enetType, gEthFwObj.instId);
        ETHFWTRACE_ERR_IF((status < 0), status, "Failed to int devs table");
    }

    /* CPSW has a single clock for all the ports */
    gptpconf_set_item(CONF_SINGLE_CLOCK_MODE, &singleClk);

    /* Let app overwrite any gPTP configuration parameters */
    if (gEthFwObj.configPtpCb != NULL)
    {
        gEthFwObj.configPtpCb(gEthFwObj.configPtpCbArg);
    }

    EthFw_gptpStart(gEthFwObj.gPtpNetDevs, gEthFwObj.numNetDevs);

    ETHFWTRACE_INFO("TimeSync PTP enabled");

    return status;
#else
    ETHFWTRACE_WARN("TimeSync is not supported");

    return ENET_SOK;
#endif
}

#if defined(ETHFW_MONITOR_SUPPORT)
static void EthFw_monitorClockCb(void *arg)
{
    /* Post semaphore to Monitor Task */
    SemaphoreP_post(gEthFwObj.hMonitorSem);
}

static int32_t EthFw_startMonitorTask(void)
{
    SemaphoreP_Params semParams;
    ClockP_Params clkParams;
    TaskP_Params params;
    int32_t status = ENET_SOK;

    SemaphoreP_Params_init(&semParams);
    semParams.mode = SemaphoreP_Mode_BINARY;
    gEthFwObj.hMonitorSem = SemaphoreP_create(1U, &semParams);

    if (gEthFwObj.hMonitorSem == NULL)
    {
        status = ETHFW_EALLOC;
        ETHFWTRACE_ERR(ETHFW_EALLOC, "Unable to create monitor clock semaphore");
        EnetAppUtils_assert(BFALSE);
    }

    if (status == ENET_SOK)
    {
        /* Create Monitor Task to monitor and detect EthFw's failure. */
        TaskP_Params_init(&params);
        params.priority  = ETHFW_MON_TASK_PRIORITY;
        params.stack     = &gEthFwObj.monTaskStackBuf[0];
        params.stacksize = sizeof(gEthFwObj.monTaskStackBuf);
        params.name      = "ETHFW Monitor Task";
        gEthFwObj.monitorTaskRun = BTRUE;

        gEthFwObj.hMonitorTask = TaskP_create(&EthFw_monitorTask, &params);
        
        if (gEthFwObj.hMonitorTask == NULL)
        {
            status = ETHFW_EALLOC;
            ETHFWTRACE_ERR(ETHFW_EALLOC, "Unable to create monitor task");
            EnetAppUtils_assert(BFALSE);
        }
    }

    if (status == ENET_SOK)
    {
        ClockP_Params_init(&clkParams);
        clkParams.startMode = ClockP_StartMode_AUTO;
        clkParams.period    = gEthFwObj.monitor.periodInMsecs;
        clkParams.runMode   = ClockP_RunMode_CONTINUOUS;

        /* Creating clock and setting clock callback function */
        gEthFwObj.hMonitorClock = ClockP_create((void*)&EthFw_monitorClockCb, &clkParams);
        if (gEthFwObj.hMonitorClock == NULL)
        {
            status = ETHFW_EALLOC;
            ETHFWTRACE_ERR(ETHFW_EALLOC, "Unable to create monitor clock");
            EnetAppUtils_assert(BFALSE);
        }
    }
    return status;
}

static void EthFw_stopMonitorTask(void)
{

    gEthFwObj.monitorTaskRun = BFALSE;

    if (gEthFwObj.hMonitorTask != NULL)
    {
        TaskP_delete(gEthFwObj.hMonitorTask);
        gEthFwObj.hMonitorTask = NULL;
    }

    /* Delete semaphore */
    SemaphoreP_delete(gEthFwObj.hMonitorSem);

    /* Stop and delete the clock */
    ClockP_stop(gEthFwObj.hMonitorClock);
    ClockP_delete(gEthFwObj.hMonitorClock);
}

static void EthFw_saveStats(void)
{
    Enet_Handle hEnet = Enet_getHandle(gEthFwObj.enetType, 0U /* instId */);
    const CpswStats_HostPort_Ng *hostStats = (const CpswStats_HostPort_Ng *)&gEthFwObj.cpswStats;
    const CpswStats_MacPort_Ng *macStats = (const CpswStats_MacPort_Ng *)&gEthFwObj.cpswStats;
    EthFw_MonStats *monStats;
    Enet_IoctlPrms prms;
    Enet_MacPort macPort;
    uint32_t portNum;
    uint32_t i;
    uint32_t j;
    int32_t status = ENET_SOK;

    /* Get host port stats counters */
    monStats = &gEthFwObj.monStats[0U];
    ENET_IOCTL_SET_OUT_ARGS(&prms, &gEthFwObj.cpswStats);
    status = Enet_ioctl(hEnet, gEthFwObj.coreId, ENET_STATS_IOCTL_GET_HOSTPORT_STATS, &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to get host port stats");

    if (status == ENET_SOK)
    {
        monStats->rxBottomOfFifoDrop = hostStats->rxBottomOfFifoDrop;
        monStats->rxTopOfFifoDrop    = hostStats->rxTopOfFifoDrop;
        for (j = 0U; j < ENET_PRI_NUM; j++)
        {
            monStats->txPriDrop[j] = hostStats->txPriDrop[j];
        }
    }

    /* Get MAC port stats counters */
    for (i = 0U; i < gEthFwObj.numPorts; i++)
    {
        macPort = ENET_MACPORT_DENORM(i);
        portNum = ENET_MACPORT_NORM(macPort);
        monStats = &gEthFwObj.monStats[portNum + 1U];

        ENET_IOCTL_SET_INOUT_ARGS(&prms, &macPort, &gEthFwObj.cpswStats);
        status = Enet_ioctl(hEnet, gEthFwObj.coreId, ENET_STATS_IOCTL_GET_MACPORT_STATS, &prms);
        ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                          "Failed to get MAC port %u stats", ENET_MACPORT_ID(macPort));

        if (status == ENET_SOK)
        {
            monStats->rxBottomOfFifoDrop = macStats->rxBottomOfFifoDrop;
            monStats->rxTopOfFifoDrop    = macStats->rxTopOfFifoDrop;
            for (j = 0U; j < ENET_PRI_NUM; j++)
            {
                monStats->txPriDrop[j] = macStats->txPriDrop[j];
            }
        }
    }
}

static bool EthFw_analyzeHostStats(void)
{
    Enet_Handle hEnet = Enet_getHandle(gEthFwObj.enetType, 0U /* instId */);
    const CpswStats_HostPort_Ng *stats = (const CpswStats_HostPort_Ng *)&gEthFwObj.cpswStats;
    EthFw_MonStats *monStats;
    EthFw_MonStats diffStats;
    Enet_IoctlPrms prms;
    bool needsRecovery = BFALSE;
    uint32_t evt = 0U;
    uint32_t i;
    int32_t status = ENET_SOK;

    monStats = &gEthFwObj.monStats[0U];

    /* Get host port stats counters */
    ENET_IOCTL_SET_OUT_ARGS(&prms, &gEthFwObj.cpswStats);
    status = Enet_ioctl(hEnet, gEthFwObj.coreId, ENET_STATS_IOCTL_GET_HOSTPORT_STATS, &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Failed to get host port stats");

    /* Check stats counters we are monitoring */
    if (status == ENET_SOK)
    {
        /* Monitor port statistics to detect and CPSW peripheral failure.
         * Current EthFw Monitor task looks for rxBottomOfFifoDrop value.
         * if rxBottomOfFifoDrop > 0, the CPSW has gone into unrecoverable state,
         * so resetting the Enet Peripheral. */
        if (stats->rxBottomOfFifoDrop > monStats->rxBottomOfFifoDrop)
        {
            evt |= ETHFW_STATSMON_RXBOTTOMOFFIFODROP;
            needsRecovery = BTRUE;
        }

        if (stats->rxTopOfFifoDrop > monStats->rxTopOfFifoDrop)
        {
            evt |= ETHFW_STATSMON_RXTOPOFFIFODROP;
        }

        for (i = 0U; i < ENET_PRI_NUM; i++)
        {
            if (stats->txPriDrop[i] > monStats->txPriDrop[i])
            {
                evt |= ETHFW_STATSMON_TXPRIDROP;
                break;
            }
        }
    }

    /* Call application callback if one has been provided */
    if ((status == ETHFW_SOK) &&
        (evt != 0U) &&
        (gEthFwObj.monitor.statsMonHostEvtCb != NULL))
    {
        diffStats.rxBottomOfFifoDrop = stats->rxBottomOfFifoDrop - monStats->rxBottomOfFifoDrop;
        diffStats.rxTopOfFifoDrop    = stats->rxTopOfFifoDrop - monStats->rxTopOfFifoDrop;
        for (i = 0U; i < ENET_PRI_NUM; i++)
        {
            diffStats.txPriDrop[i] = stats->txPriDrop[i] - monStats->txPriDrop[i];
        }

        gEthFwObj.monitor.statsMonHostEvtCb(evt, &diffStats, stats,
                                            gEthFwObj.monitor.statsMonCbArg);

        monStats->rxBottomOfFifoDrop = stats->rxBottomOfFifoDrop;
        monStats->rxTopOfFifoDrop    = stats->rxTopOfFifoDrop;
        memcpy(monStats->txPriDrop, stats->txPriDrop, sizeof(monStats->txPriDrop));
    }

    return needsRecovery;
}

static bool EthFw_analyzePortStats(Enet_MacPort macPort)
{
    Enet_Handle hEnet = Enet_getHandle(gEthFwObj.enetType, 0U /* instId */);
    const CpswStats_MacPort_Ng *stats = (const CpswStats_MacPort_Ng *)&gEthFwObj.cpswStats;
    EthFw_MonStats *monStats;
    EthFw_MonStats diffStats;
    Enet_IoctlPrms prms;
    bool needsRecovery = BFALSE;
    uint32_t evt = 0U;
    uint32_t portNum = ENET_MACPORT_NORM(macPort);
    uint32_t i;
    int32_t status = ENET_SOK;

    monStats = &gEthFwObj.monStats[portNum + 1U];

    /* Get MAC port stats counters */
    ENET_IOCTL_SET_INOUT_ARGS(&prms, &macPort, &gEthFwObj.cpswStats);
    status = Enet_ioctl(hEnet, gEthFwObj.coreId, ENET_STATS_IOCTL_GET_MACPORT_STATS, &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status,
                      "Failed to get MAC port %u stats", ENET_MACPORT_ID(macPort));

    /* Check stats counters we are monitoring */
    if (status == ENET_SOK)
    {
        /* Monitor port statistics to detect and CPSW peripheral failure.
         * Current EthFw Monitor task looks for rxBottomOfFifoDrop value.
         * if rxBottomOfFifoDrop > 0, the CPSW has gone into unrecoverable state,
         * so resetting the Enet Peripheral. */
        if (stats->rxBottomOfFifoDrop > monStats->rxBottomOfFifoDrop)
        {
            evt |= ETHFW_STATSMON_RXBOTTOMOFFIFODROP;
            needsRecovery = BTRUE;
        }

        if (stats->rxTopOfFifoDrop > monStats->rxTopOfFifoDrop)
        {
            evt |= ETHFW_STATSMON_RXTOPOFFIFODROP;
        }

        for (i = 0U; i < ENET_PRI_NUM; i++)
        {
            if (stats->txPriDrop[i] > monStats->txPriDrop[i])
            {
                evt |= ETHFW_STATSMON_TXPRIDROP;
                break;
            }
        }
    }

    /* Call application callback if one has been provided */
    if ((status == ETHFW_SOK) &&
        (evt != 0U) &&
        (gEthFwObj.monitor.statsMonMacEvtCb != NULL))
    {
        diffStats.rxBottomOfFifoDrop = stats->rxBottomOfFifoDrop - monStats->rxBottomOfFifoDrop;
        diffStats.rxTopOfFifoDrop    = stats->rxTopOfFifoDrop - monStats->rxTopOfFifoDrop;
        for (i = 0U; i < ENET_PRI_NUM; i++)
        {
            diffStats.txPriDrop[i] = stats->txPriDrop[i] - monStats->txPriDrop[i];
        }

        gEthFwObj.monitor.statsMonMacEvtCb(macPort, evt, &diffStats, stats,
                                           gEthFwObj.monitor.statsMonCbArg);

        monStats->rxBottomOfFifoDrop = stats->rxBottomOfFifoDrop;
        monStats->rxTopOfFifoDrop    = stats->rxTopOfFifoDrop;
        memcpy(monStats->txPriDrop, stats->txPriDrop, sizeof(monStats->txPriDrop));
    }

    return needsRecovery;
}

static void EthFw_monitorTask(void *a0,
                              void *a1)
{
    Enet_Handle hEnet = Enet_getHandle(gEthFwObj.enetType, 0U /* instId */);
    Enet_MacPort macPort;
    Enet_MacPort recoveryMacPort;
    bool needsRecovery = BFALSE;
    uint32_t i;
    int32_t status = ENET_SOK;
    bool isTeardownComplete = BFALSE;
    bool isrecoveryPortLinked = BFALSE;
    bool noPendingReq = BFALSE;
    uint32_t numTotalClients  = 0U;
    uint32_t numActiveClients  = 0U;
    uint32_t numIdleClients = 0U;
    uint32_t teardownLoopCnt = 0U;

    while (gEthFwObj.monitorTaskRun)
    {
        SemaphoreP_pend(gEthFwObj.hMonitorSem, SemaphoreP_WAIT_FOREVER);

        needsRecovery = EthFw_analyzeHostStats();

        for (i = 0U; (i < gEthFwObj.numPorts) && !needsRecovery; i++)
        {
            macPort = ENET_MACPORT_DENORM(i);
            if (EnetAppUtils_isPortLinkUp(hEnet, gEthFwObj.coreId, macPort) == BTRUE)
            {
                needsRecovery = EthFw_analyzePortStats(macPort);
                if (needsRecovery)
                {
                    recoveryMacPort = macPort;
                }
            }
        }

        if (needsRecovery && gEthFwObj.recoveryEn)
        {
            /* Stop the clock during reset recovery handling */
            ClockP_stop(gEthFwObj.hMonitorClock);

            /* Stop periodic ticks to Mcm */
            EnetMcm_stopPeriodicTick(&gEthFwObj.mcmCmdIf);

            /* Get the clients status */
            CpswProxyServer_getIdleClientCnt(&numTotalClients,&numIdleClients);

            ETHFWTRACE_INFO("%u clients attached to be reset", numTotalClients);

            if (numTotalClients != 0U)
            {
                /* Notify the clients about the HW error */
                CpswProxyServer_bcastNotify(ETHREMOTECFG_NOTIFY_HWERROR);

                /* Wait for clients to complete their DMA teardown */
                while (!isTeardownComplete)
                {
                    /* get the client status */
                    CpswProxyServer_getIdleClientCnt(&numActiveClients,&numIdleClients);

                    if (numIdleClients == numTotalClients)
                    {
                        isTeardownComplete = BTRUE;
                    }

                    TaskP_sleep(ETHFW_MON_HWRECOVERY_IDLE_CHECK_PERIOD_MS);

                    teardownLoopCnt++;
                    if ((teardownLoopCnt % ETHFW_MON_HWRECOVERY_RETRY_LOG_ITER) == 0U)
                    {
                        ETHFWTRACE_INFO("%u of %u clients have completed DMA tear-down",
                                        numIdleClients, numActiveClients);
                        teardownLoopCnt = 0U;
                    }
                }

                /* Wait untill all ARP table entries are free. */
                while (!noPendingReq)
                {
#if defined(ETHFW_PROXY_ARP_HANDLING)
                    if (EthFwArp_getUseCnt() == 0U)
                    {
                        noPendingReq = BTRUE;
                    }
#endif

#if defined(ETHFW_VEPA_SUPPORT)
                    if (EthFwVepa_getUseCnt() == 0U)
                    {
                        noPendingReq = BTRUE;
                    }
#endif
                    TaskP_sleep(ETHFW_MON_HWRECOVERY_IDLE_CHECK_PERIOD_MS);
                }
            }

            /* Call the EthFw reset handler */
            ETHFWTRACE_INFO("CPSW recovery is about to take place");
            status = EthFw_resetHandler();

            if (status != ENET_SOK)
            {
                ETHFWTRACE_ERR(status, "Failed to recover CPSW");
                EnetAppUtils_assert(status == ENET_SOK);
            }
            else
            {
                /* Send a notification to clients for HW recovery completion only when Port link is up*/
                while(!isrecoveryPortLinked)
                {
                    if (EnetAppUtils_isPortLinkUp(hEnet, gEthFwObj.coreId, recoveryMacPort) == BTRUE)
                    {
                        isrecoveryPortLinked = BTRUE;
                    }

                    TaskP_sleep(ETHFW_MON_HWRECOVERY_IDLE_CHECK_PERIOD_MS);
                }

                /* Notify the clients about the HW error recovery completion */
                CpswProxyServer_bcastNotify(ETHREMOTECFG_NOTIFY_HWRECOVERY_COMPLETE);
            }

            /* Set teardown flag to false for next iteration */
            isTeardownComplete = BFALSE;
            isrecoveryPortLinked = BFALSE;
            noPendingReq = BFALSE;
            ClockP_start(gEthFwObj.hMonitorClock);
        }
    }
}

uint64_t EthFw_getCurrentTime(uint32_t *nanoSeconds,
                          uint64_t *seconds)
{
    Enet_Handle hEnet = Enet_getHandle(gEthFwObj.enetType, 0U /* instId */);
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;
    uint64_t tsVal = 0U;

    /* Software Time stamp Push event */
    ENET_IOCTL_SET_OUT_ARGS(&prms, &tsVal);
    status = Enet_ioctl(hEnet,
                        gEthFwObj.coreId,
                        ENET_TIMESYNC_IOCTL_GET_CURRENT_TIMESTAMP,
                        &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Error in getting the current CPTS time");

    *nanoSeconds = (uint32_t)(tsVal % (uint64_t)ETHFW_TIME_SEC_TO_NS);
    *seconds = tsVal / (uint64_t)ETHFW_TIME_SEC_TO_NS;

    tsVal = (uint64_t)(((uint64_t)*seconds * (uint64_t)ETHFW_TIME_SEC_TO_NS) + *nanoSeconds);

    return tsVal;
}

void EthFw_setCurrentTime(uint64_t *time)
{
    Enet_Handle hEnet = Enet_getHandle(gEthFwObj.enetType, 0U /* instId */);
    int32_t status = ENET_SOK;
    Enet_IoctlPrms prms;

    /* Update the CPTS time */
    ENET_IOCTL_SET_IN_ARGS(&prms, time);

    status = Enet_ioctl(hEnet,
                        gEthFwObj.coreId,
                        ENET_TIMESYNC_IOCTL_SET_TIMESTAMP,
                        &prms);
    ETHFWTRACE_ERR_IF((status != ENET_SOK), status, "Error in setting the updated CPTS time");
}

static int32_t EthFw_resetHandler(void)
{
    Enet_Handle hEnet = Enet_getHandle(gEthFwObj.enetType, 0U /* instId */);
    uint32_t nanoSeconds = 0U;
    uint64_t seconds = 0LLU;
    uint64_t preResetTime;
    uint64_t postResetTime;
    uint64_t currentTime;
    uint64_t updatedTime;
    int32_t status = ENET_SOK;
    uint32_t i;

    /* Get current CPTS time */
    currentTime = EthFw_getCurrentTime(&nanoSeconds, &seconds);

    /* Get OS time before triggering a reset */
    preResetTime = TimerP_getTimeInUsecs();

    /* Close MAC Ports */
    status = EnetMcm_closeMacPorts(&gEthFwObj.mcmCmdIf);
    EnetAppUtils_assert(status == ENET_SOK);

    /* Call App callback to close the Lwip Dma channels */
    gEthFwObj.monitor.closeLwipDmaCb(gEthFwObj.monitor.lwipDmaCbArg);

#if defined(ETHFW_GPTP_SUPPORT)
    /* Close the gPTP DMA channels */
    LLDEnetDmaClose();
#endif

    /* Save the context */
    EnetMcm_saveCtxt(&gEthFwObj.mcmCmdIf);

    /* Reset the CPSW */
    EnetAppUtils_turnCpswOff();
    EnetAppUtils_delayInUsec(5000U);
    EnetAppUtils_turnCpswOn();

    /* Clear local stats counters */
    memset(gEthFwObj.monStats, 0, sizeof(gEthFwObj.monStats));

    /* Workaround: CPSW software stats are currently not cleared during save/restore context.
     * In order to prevent that the recovery mechanism runs in infinite loop, save the last
     * software stats so they become the starting point going forward */
    EthFw_saveStats();

    /* Restore the context */
    status = EnetMcm_restoreCtxt(&gEthFwObj.mcmCmdIf);
    EnetAppUtils_assert(status == ENET_SOK);

    /* Call App callback to open the Lwip Dma channels */
    gEthFwObj.monitor.openLwipDmaCb(gEthFwObj.monitor.lwipDmaCbArg);

#if defined(ETHFW_GPTP_SUPPORT)
    /* start the gPTP DMA channels */
    LLDEnetDmaOpen();
#endif

    /* Open MAC Ports */
    status = EnetMcm_openMacPorts(&gEthFwObj.mcmCmdIf);
    EnetAppUtils_assert(status == ENET_SOK);

    /* Get OS time post reset is done */
    postResetTime = TimerP_getTimeInUsecs();

    /* Calculate the updated time( current time + (time taken by during reset)*1000U (convert to nanoseconds))
    * in nanoseconds to be set into CPTS */
    updatedTime = currentTime + (postResetTime - preResetTime)*1000U;

    /* Update CPTS time with the time taken by reset recovery */
    EthFw_setCurrentTime(&updatedTime);
    
    /* Start periodic ticks to Mcm */
    EnetMcm_startPeriodicTick(&gEthFwObj.mcmCmdIf);

    return status;
}
#endif
