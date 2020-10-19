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

/* XDCtools header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Error.h>

/* BIOS header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/osal/SemaphoreP.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/utils/Load.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Mailbox.h>

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
#include <ti/drv/enet/examples/utils/include/enet_appboardutils.h>
#include <ti/drv/enet/examples/utils/include/enet_mcm.h>
#include <ti/drv/enet/examples/utils/include/enet_appsoc.h>
#include <ti/drv/enet/nimuenet/nimu_ndk.h>

/* EthFw utils header files */
#include <utils/remote_service/include/app_remote_service.h>
#include <utils/perf_stats/include/app_perf_stats.h>
#include <utils/ethfw_stats/include/app_ethfw_stats_sysbios.h>
#include <utils/console_io/include/app_log.h>

/* EthFw remote configuration header files */
#include <ethremotecfg/server/include/ethremotecfg_server.h>
#include <ethremotecfg/server/include/cpsw_proxy_server.h>

#include <server-rtos/remote-device.h>

#include <utils/profile/include/app_profile.h>

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

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct EthFw_Obj_s
{
    /* Core Id */
    uint32_t coreId;

    /* Enet instance type */
    Enet_Type enetType;

    /* Enet instance id */
    uint32_t instId;

    /*! CPSW configuration */
    Cpsw_Cfg cpswCfg;

    /*! Firmware version */
    EthFw_Version version;

    /*! MAC ports owned by EthFw */
    EthFw_Port ports[ENET_MAC_PORT_NUM];

    /*! Number of MAC ports owned by EthFw */
    uint32_t numPorts;

    /* Multiclient Manager (MCM) handle */
    EnetMcm_CmdIf mcmCmdIf;
} EthFw_Obj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

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

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/*! Ethernet Firmware object */
EthFw_Obj gEthFwObj;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

static void EthFw_initAleCfg(CpswAle_Cfg *aleCfg)
{
    /* ALE configuration */
    aleCfg->modeFlags = CPSW_ALE_CFG_MODULE_EN |
                        CPSW_ALE_CFG_UNKNOWN_UCAST_FLOOD2HOST;
    aleCfg->agingCfg.enableAutoAging = TRUE;
    aleCfg->agingCfg.agingPeriodInMs = 1000;
    aleCfg->nwSecCfg.enableVid0Mode = FALSE;
    aleCfg->vlanCfg.aleVlanAwareMode = TRUE;
    aleCfg->vlanCfg.cpswVlanAwareMode = FALSE;
    aleCfg->vlanCfg.unknownUnregMcastFloodMask = 0U;
    aleCfg->vlanCfg.unknownRegMcastFloodMask = 0U;
    aleCfg->vlanCfg.unknownVlanMemberListMask = CPSW_ALE_ALL_PORTS_MASK;
    aleCfg->vlanCfg.autoLearnWithVLAN = false;
    aleCfg->policerGlobalCfg.policingEnable = true;
    aleCfg->policerGlobalCfg.yellowDropEnable = false;
    aleCfg->policerGlobalCfg.redDropEnable = true;
    aleCfg->policerGlobalCfg.policerNoMatchMode = CPSW_ALE_POLICER_NOMATCH_MODE_GREEN;
    aleCfg->portCfg[0].learningCfg.noLearn = FALSE;
    aleCfg->portCfg[0].vlanCfg.dropUntagged = FALSE;
    aleCfg->portCfg[1].learningCfg.noLearn = FALSE;
    aleCfg->portCfg[1].vlanCfg.dropUntagged = FALSE;

    /* ALE policer configuration */
    aleCfg->policerGlobalCfg.policingEnable = true;
    aleCfg->policerGlobalCfg.yellowDropEnable = false;
    aleCfg->policerGlobalCfg.redDropEnable = true;
    aleCfg->policerGlobalCfg.policerNoMatchMode = CPSW_ALE_POLICER_NOMATCH_MODE_GREEN;
}

void EthFw_initConfigParams(Enet_Type enetType,
                            EthFw_Config *config)
{
    Cpsw_Cfg *cpswCfg = &config->cpswCfg;
    CpswAle_Cfg *aleCfg = &cpswCfg->aleCfg;
    cpswCfg->dmaCfg = NULL;
    Cpsw_VlanCfg *vlanCfg = &cpswCfg->vlanCfg;
    CpswHostPort_Cfg *hostPortCfg = &cpswCfg->hostPortCfg;
    EnetRm_ResCfg *resCfg = &cpswCfg->resCfg;

    /* MAC port ownership */
    config->ports = NULL;
    config->numPorts = 0U;

    /* Start with CPSW LLD's default configuration */
    // FIXME
    Enet_initCfg(enetType, 0U, cpswCfg, sizeof (*cpswCfg));
    EnetAppUtils_initResourceConfig(enetType, CpswAppSoc_getCoreId(), resCfg);

    /* VLAN configuration */
    vlanCfg->vlanAware = true;

    /* Host port configuration */
    hostPortCfg->removeCrc = true;
    hostPortCfg->padShortPacket = true;
    hostPortCfg->passCrcErrors = true;
    hostPortCfg->csumOffloadEn = true;
    hostPortCfg->rxMtu = 1522U;

    EthFw_initAleCfg(aleCfg);
}

EthFw_Handle EthFw_init(Enet_Type enetType,
                        const EthFw_Config *config)
{
    char *date = __DATE__;
    char *time = __TIME__;
    int32_t status;

    EnetAppUtils_assert(config != NULL);
    EnetAppUtils_assert(config->ports != NULL);
    EnetAppUtils_assert(config->numPorts <= ENET_MAC_PORT_NUM);
    EnetUdma_Cfg *udmaCfg = (EnetUdma_Cfg *)config->cpswCfg.dmaCfg;
    EnetAppUtils_assert(udmaCfg != NULL);
    EnetAppUtils_assert(udmaCfg->hUdmaDrv != NULL);

    memset(&gEthFwObj, 0, sizeof(gEthFwObj));

    /* Save config parameters */
    gEthFwObj.cpswCfg = config->cpswCfg;
    gEthFwObj.numPorts = config->numPorts;
    memcpy(&gEthFwObj.ports[0U],
           config->ports,
           gEthFwObj.numPorts * sizeof(EthFw_Port));

    gEthFwObj.coreId = CpswAppSoc_getCoreId();
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

    /* Initialize MCM */
    status = EthFw_initMcm();
    if (status != ENET_SOK)
    {
        appLogPrintf("ETHFW: failed to init CPSW MCM: %d\n", status);
    }
    EnetAppUtils_assert(status == ENET_SOK);

    /* Add ALE entry for broadcast MAC address. Note this is needed as the broadcast
     * is disabled via unknownRegMcastFloodMask and other flags in ALE init config.
     * In EthFw we need broadcast to handle ARP entries for clients */
    if (status == ENET_SOK)
    {
        EthFw_setAleBcastEntry();
    }

    return (status == ENET_SOK) ? &gEthFwObj : NULL;
}

void EthFw_deinit(EthFw_Handle hEthFw)
{
    EnetAppUtils_assert(hEthFw != NULL);

    /* De-initialize MCM */
    EthFw_deinitMcm();

    gEthFwObj.numPorts = 0U;
    memset(&gEthFwObj.cpswCfg, 0, sizeof(Cpsw_Cfg));
}

int32_t EthFw_initRemoteConfig(EthFw_Handle hEthFw)
{
    CpswProxyServer_Config_t cfg;
    int32_t status;

    EnetAppUtils_assert(hEthFw != NULL);

    /* Initialize Proxy Server */
    memset(&cfg, 0, sizeof(cfg));
    cfg.getMcmCmdIfCb = &EthFw_getMcmCmdIfCb;
    cfg.initEthfwDeviceDataCb = &EthFw_getDeviceData;
    cfg.notifyCb = &EthFw_handleProfileInfoNotify;
    cfg.rpmsgEndPointId = REMOTE_DEVICE_ENDPT;

    /* Remote cores: mcu2_1, mpu1_0 */
    cfg.numRemoteCores = 2;
    cfg.remoteCoreCfg[0].remoteCoreId = IPC_MCU2_1;
    snprintf(cfg.remoteCoreCfg[0].serverName, ETHREMOTECFG_SERVER_MAX_NAME_LEN, ETHREMOTEDEVICE_DEVICE_NAME_MCU_2_1);
    cfg.remoteCoreCfg[1].remoteCoreId = IPC_MPU1_0;
    snprintf(cfg.remoteCoreCfg[1].serverName, ETHREMOTECFG_SERVER_MAX_NAME_LEN, ETHREMOTEDEVICE_DEVICE_NAME_MPU_1_0);

    /* AUTOSAR core: mcu2_1 */
    cfg.autosarEthDriverRemoteCoreId = IPC_MCU2_1;
    cfg.autosarEthDeviceEndPointId = AUTOSAR_ETHDRIVER_DEVICE_ENDPT;

    /* Enable server-to-client notify service */
    cfg.notifyServiceCpswType = gEthFwObj.enetType;
    cfg.notifyServiceRemoteCoreId = IPC_MCU2_1;

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
    EnetMcm_InitConfig cpswMcmCfg;
    EnetMcm_HandleInfo handleInfo;
    uint32_t i;
    int32_t status;

    /* Initialize CPSW MCM */
    cpswMcmCfg.cpswCfg = &gEthFwObj.cpswCfg;
    cpswMcmCfg.enetType = gEthFwObj.enetType;
    cpswMcmCfg.instId = gEthFwObj.instId;
    cpswMcmCfg.setPortLinkCfg = EthFw_initLinkArgs;
    cpswMcmCfg.numMacPorts = gEthFwObj.numPorts;
    cpswMcmCfg.periodicTaskPeriod = ENETPHY_FSM_TICK_PERIOD_MS;
    cpswMcmCfg.print = appLogPrintf;

    for (i = 0U; i < gEthFwObj.numPorts; i++)
    {
        cpswMcmCfg.macPortList[i] = gEthFwObj.ports[i].portNum;
    }

    status = EnetMcm_init(&cpswMcmCfg);
    EnetAppUtils_assert(status == ENET_SOK);

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

    CpswMacPort_initCfg(macCfg);
    EnetPhy_initCfg(phyCfg);

    /* PHY parameters from board specific code */
    CpswAppBoardUtils_setPhyConfig(gEthFwObj.enetType,
                                   macPort,
                                   macCfg,
                                   mii,
                                   phyCfg);

    if (phyCfg->phyAddr == ENETPHY_INVALID_PHYADDR)
    {
        linkCfg->speed = ENET_SPEED_1GBIT;
        linkCfg->duplexity = ENET_DUPLEX_FULL;
    }
    else
    {
        linkCfg->speed = ENET_SPEED_AUTO;
        linkCfg->duplexity = ENET_DUPLEX_AUTO;
    }

    /* Use VLAN config from parameters given to EthFw */
    for (i = 0U; i < gEthFwObj.numPorts; i++)
    {
        if (gEthFwObj.ports[i].portNum == macPort)
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
    eth_dev_data->uart_id = CPSW_UTILS_MCU2_0_UART_INSTANCE;
}

static void EthFw_handleProfileInfoNotify(uint32_t host_id,
                                          Enet_Handle hEnet,
                                          Enet_Type enetType,
                                          uint32_t core_key,
                                          enum rpmsg_kdrv_ethswitch_client_notify_type notifyid,
                                          uint8_t *notify_info,
                                          uint32_t notify_info_len)
{
    appProfileAvgLoadInfo *info = (appProfileAvgLoadInfo *)notify_info;
    uint32_t i;

    EnetAppUtils_assert(Enet_getHandle(enetType, 0U) == hEnet);
    EnetAppUtils_assert(notifyid == RPMSG_KDRV_TP_ETHSWITCH_CLIENTNOTIFY_CUSTOM);
    EnetAppUtils_assert(notify_info_len == sizeof(appProfileAvgLoadInfo));

    appLogPrintf("\n***********************************\n");
    appLogPrintf(" CPU Load         : %d\n", info->cpuLoad);
    appLogPrintf(" Packet count     : %d\n", info->packetCount);
    appLogPrintf(" ISR              : %d\n", info->isr);
    appLogPrintf(" SWI              : %d\n", info->swi);
    appLogPrintf(" Total task count : %d\n", info->totalTaskCount);
    appLogPrintf(" Active task count: %d\n", info->activeTaskCount);

    for (i = 0U; i < info->activeTaskCount; i++)
    {
        appLogPrintf(" Task: %s: %d %%\n", info->tskLoad[i].tskName, info->tskLoad[i].load);
    }

    appLogPrintf("***********************************\n");
}
