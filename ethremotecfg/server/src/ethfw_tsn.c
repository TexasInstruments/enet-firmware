/*
 *
 * Copyright (c) 2024 Texas Instruments Incorporated
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
 *  \file ethfw_tsn.c
 *
 *  \brief This file contains the TSN and gPTP implementation of ETHFW
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* EthFwTrace id for this module, must be unique within ETHFW */
#define ETHFWTRACE_MOD_ID 0x108

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* TSN header files */
#include <tsn_buildconf/jacinto_buildconf.h>
#include <tsn_gptp/tilld/lld_gptp_private.h>
#include <tsn_uniconf/yangs/yang_db_runtime.h>
#include <tsn_uniconf/yangs/yang_modules.h>
#include <tsn_uniconf/uc_dbal.h>
#include <tsn_gptp/gptpconf/gptpgcfg.h>
#include <tsn_uniconf/yangs/ieee1588-ptp-tt_access.h>
#include <tsn_gptp/gptpconf/xl4-extmod-xl4gptp.h>
#include <tsn_uniconf/uc_dbal.h>
#include <tsn_uniconf/ucman.h>
#include <tsn_gptp/gptpman.h>
#include <tsn_unibase/unibase_binding.h>

/* PDK driver header files */
#include <ti/osal/MutexP.h>
#include <ti/osal/osal.h>
#include <ti/osal/SemaphoreP.h>
#include <ti/osal/TaskP.h>

/* Enet LLD header files */
#include <ti/drv/enet/enet.h>
#include <ti/drv/enet/include/per/cpsw.h>
#include <ti/drv/enet/examples/utils/include/enet_apputils.h>

/* EthFw header files */
#include <utils/ethfw_common/include/ethfw_utils.h>
#include <utils/ethfw_common/include/ethfw_trace.h>
#include <ethremotecfg/server/include/ethfw_tsn.h>


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/*! Logging task - very low priority so it doesn't interfere with gPTP stack */
#define ETHFW_TSN_LOGGER_TASK_PRIORITY                              (1U)

/*! gPTP stack priority - should be higher than other non-critical tasks which could interfere */
#define ETHFW_TSN_GPTP_TASK_PRIORITY                                (2U)

/*! gPTP Task name */
#define ETHFW_TSN_GPTP_TASK_NAME                                    "gPTP Task"

/*! TSN stack size and alignment */
#define ETHFW_TSN_TASK_STACK_SIZE                                   (16U * 1024U)
#define ETHFW_TSN_TASK_STACK_ALIGN                                  (32U)

/*! Uniconf stack priority */
#define ETHFW_TSN_UC_TASK_PRIORITY                                  (2U)

/*! Uniconf Task name */
#define ETHFW_TSN_UC_TASK_NAME                                      "Uniconf Task"

/*! Uniconf stack size and alignment */
#define ETHFW_TSN_UC_TASK_STACK_SIZE                                (16U * 1024U)
#define ETHFW_TSN_UC_TASK_STACK_ALIGN                               (32U)

/*! Log task's buffer stack size and alignment */
#define ETHFW_TSN_LOGGER_TASK_STACK_SIZE                            (2U * 1024U)
#define ETHFW_TSN_LOGGER_TASK_STACK_ALIGN                           (32U)

/*! Size of the buffer used for storing logs before printing */
#define ETHFW_TSN_BUFFER_SIZE                                       (8960U)

/*! Max size of the TSN log print buffer length */
#define ETHFW_TSN_TRACE_MAX_BUFFER_SIZE                             (250U)

/*! Number of MAC ports supported by the gPTP stack */
#if defined(SOC_J721E) || defined(SOC_J784S4)
#define ETHFW_TSN_CFG_NUM_MAC_PORTS                                 (8U)
#else
#define ETHFW_TSN_CFG_NUM_MAC_PORTS                                 (4U)
#endif

#define MAX_KEY_SIZE                                                (32U)
#define MAX_BUFFER_SIZE                                             (256U)

#define ETHFW_TSN_UC_CONF_FILE_NUM                                  (0U)
#define ETHFW_TSN_INTERFACE_CONFFILE_PATH                           (NULL)
#define ETHFW_TSN_UC_DBFILE_PATH                                    (NULL)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* Container structure for database name and value pairs for yang configurations */
typedef struct EthFwTsn_DbNameVal_s
{
    char *name;
    char *val;
} EthFwTsn_DbNameVal;

/* Container structure for database name and value pairs, for non-yang configurations */
typedef struct EthFwTsn_DbIntVal_s
{
    char *name;
    int item;
    int val;
} EthFwTsn_DbIntVal;

typedef struct EthFwTsn_dbArgs_s
{
    uc_dbald *dbald;
    yang_db_runtime_dataq_t *ydrd;
} EthFwTsn_dbArgs;

typedef int32_t (*EthFwTsn_OnModuleDBInit)(EthFwTsn_dbArgs *dbArgs);

typedef void* (*EthFwTsn_OnModuleStart)(void *arg1);

typedef struct EthFwTsn_GptpOpt_s
{
    char *devlist;
    const char **confFiles;
    uint32_t domainNum;
    uint32_t domains[GPTP_MAX_DOMAINS];
    uint32_t instNum;
    uint32_t numConf;
} EthFwTsn_GptpOpt;

/* Container structure for Uniconf config params */
typedef struct EthFwTsn_UniconfCfg_s
{
    char *dbName;
    ucman_data_t ucData;
    UC_NOTICE_SIG_T ucReadySem;
    bool dbInitFlag;
} EthFwTsn_UniconfCfg;

/* Container structure for TSN Modules config params */
typedef struct EthFwTsn_ModuleCfg_s
{
    bool stopFlag;
    int taskPriority;
    CB_THREAD_T hTaskHandle;
    const char *taskName;
    uint8_t *stackBuffer;
    uint32_t stackSize;
    EthFwTsn_OnModuleDBInit onModuleDBInit;
    EthFwTsn_OnModuleStart onModuleRunner;
    bool enable;
} EthFwTsn_ModuleCfg;

/*
 * \brief Structure holding gPTP and TSN configs
 */
typedef struct EthFwTsn_Obj_s
{
    /* Enet instance type */
    uint32_t enetType;

    /* Enet instance id */
    uint32_t instId;

    /* Whether TSN stack has been initialized or not */
    bool tsnInit;

    /* To run log Task in the loop*/
    bool logTaskrun;

    /* TSN stack netdevs */
    char netDevs[ETHFW_TSN_CFG_NUM_MAC_PORTS][IFNAMSIZ];

    /* gPTP stack netdevs */
    char *gPtpNetDevs[ETHFW_TSN_CFG_NUM_MAC_PORTS + 1];

    /* Number of active netdevs */
    uint32_t numNetDevs;

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

    /* Trace max buffer */
    char traceBuf[ETHFW_TSN_TRACE_MAX_BUFFER_SIZE];

    /* gPTP config callback from app */
    EthFwTsn_configPtpCb configPtpCb;

    /* gPTP config callback argument */
    void *configPtpCbArg;

    /* Uniconf Module Config */
    EthFwTsn_UniconfCfg ucCfg;
} EthFwTsn_Obj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static int32_t EthFwTsn_logBuffer(bool flush, const char *str);

static void EthFwTsn_startLogTask(void);

/* Can be read from cfg files, or in case of no db file is specified, init runtime config */
static int32_t EthFwTsn_initDb(EthFwTsn_UniconfCfg *ucCfg);

static int32_t EthFwTsn_startModTask(EthFwTsn_ModuleCfg *modCfg, uint32_t moduleIdx);

static int32_t EthFwTsn_startMod(void);

static int32_t EthFwTsn_uniconfInit(EthFwTsn_dbArgs *dbArgs);

static int32_t EthFwTsn_gptpDbInit(EthFwTsn_dbArgs *dbArgs);

static void *EthFwTsn_gptpTask(void *args);

static void *EthFwTsn_uniconfTask(void *args);

static int32_t EthFwTsn_gptpNonYangConfig(uint8_t instance);

static int32_t EthFwTsn_gptpYangConfig(yang_db_runtime_dataq_t *ydrd,
                                       uint32_t instance,
                                       uint32_t domain);

static void EthFwTsn_cfgGptpDefaultDs(yang_db_runtime_dataq_t *ydrd,
                                      uint32_t instance,
                                      uint32_t domain,
                                      bool dbInitFlag);

static void EthFwTsn_cfgGptpPortDs(yang_db_runtime_dataq_t *ydrd,
                                   uint32_t instance,
                                   uint32_t domain,
                                   int portIndex,
                                   bool dbInitFlag);

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

EthFwTsn_Obj gEthFwTsnObj;

/* Uniconf Stack buffer */
uint8_t gUniconfStackBuf[ETHFW_TSN_UC_TASK_STACK_SIZE] __attribute__ ((aligned(ETHFW_TSN_UC_TASK_STACK_ALIGN)));

/* gPTP task stack buffer */
uint8_t gPtpStackBuf[ETHFW_TSN_TASK_STACK_SIZE] __attribute__ ((aligned(ETHFW_TSN_TASK_STACK_ALIGN)));

/* Default values for gptp port data set */
static EthFwTsn_DbNameVal gGptpPortDsRw[] =
{
    {"port-enable", "true"},
    {"log-announce-interval", "0"},
    {"gptp-cap-receipt-timeout", "3"},
    {"announce-receipt-timeout", "3"},
    {"initial-log-announce-interval", "0"},
    {"initial-log-sync-interval", "-3"},
    {"sync-receipt-timeout", "3"},
    {"initial-log-pdelay-req-interval", "0"},
    {"allowed-lost-responses", "9"},
    {"allowed-faults", "9"},
    {"mean-link-delay-thresh", "0x27100000"},
};

static EthFwTsn_DbNameVal gGptpPortDsRo[] =
{
    {"log-sync-interval", "-3"},
    {"minor-version-number", "1"},
    {"current-log-sync-interval", "-3"},
    {"current-log-gptp-cap-interval", "3"},
    {"current-log-pdelay-req-interval", "0"},
    {"initial-one-step-tx-oper", "1"},
    {"current-one-step-tx-oper", "1"},
    {"use-mgt-one-step-tx-oper", "false"},
    {"mgt-one-step-tx-oper", "1"},
};

/* Default values for gptp default data set */
static EthFwTsn_DbNameVal gGptpDefaultDsRw[] =
{
    {"priority1", "248"},
    {"priority2", "248"},
    {"external-port-config-enable", "false"},
    {"clock-quality/clock-class", "cc-default"},
    {"clock-quality/clock-accuracy", "ca-time-accurate-to-250-ns"},
    {"clock-quality/offset-scaled-log-variance", "0x436a"}
};

static EthFwTsn_DbNameVal gGptpDefaultDsRo[] =
{
    {"time-source", "internal-oscillator"},
    {"ptp-timescale", "true"},
};

/* Default values for gptp non-yang data set */
static EthFwTsn_DbIntVal gGptpNonYangDs[] =
{
    {"SINGLE_CLOCK_MODE", XL4_EXTMOD_XL4GPTP_SINGLE_CLOCK_MODE, 1},
    {"USE_HW_PHASE_ADJUSTMENT", XL4_EXTMOD_XL4GPTP_USE_HW_PHASE_ADJUSTMENT, 1},
    {"CLOCK_COMPUTE_INTERVAL_MSEC", XL4_EXTMOD_XL4GPTP_CLOCK_COMPUTE_INTERVAL_MSEC, 100},
    {"FREQ_OFFSET_IIR_ALPHA_START_VALUE", XL4_EXTMOD_XL4GPTP_FREQ_OFFSET_IIR_ALPHA_START_VALUE, 1},
    {"FREQ_OFFSET_IIR_ALPHA_STABLE_VALUE", XL4_EXTMOD_XL4GPTP_FREQ_OFFSET_IIR_ALPHA_STABLE_VALUE, 4},
    {"PHASE_OFFSET_IIR_ALPHA_START_VALUE", XL4_EXTMOD_XL4GPTP_PHASE_OFFSET_IIR_ALPHA_START_VALUE, 1},
    {"PHASE_OFFSET_IIR_ALPHA_STABLE_VALUE", XL4_EXTMOD_XL4GPTP_PHASE_OFFSET_IIR_ALPHA_STABLE_VALUE, 4},
    {"MAX_DOMAIN_NUMBER", XL4_EXTMOD_XL4GPTP_MAX_DOMAIN_NUMBER, GPTP_MAX_DOMAINS},
#if GPTP_MAX_DOMAINS == 2
    {"CMLDS_MODE", XL4_EXTMOD_XL4GPTP_CMLDS_MODE, 1},
    {"SECOND_DOMAIN_THIS_CLOCK", XL4_EXTMOD_XL4GPTP_SECOND_DOMAIN_THIS_CLOCK, 1}
#endif
};

EthFwTsn_ModuleCfg gModCfgTable[ETHFWTSN_MAX_TASK_IDX] =
{
        [ETHFWTSN_GPTP_TASK_IDX] =
        {
            .enable = BFALSE,
            .stopFlag = BTRUE,
            .taskPriority = ETHFW_TSN_GPTP_TASK_PRIORITY,
            .taskName = ETHFW_TSN_GPTP_TASK_NAME,
            .stackBuffer = gPtpStackBuf,
            .stackSize = sizeof(gPtpStackBuf),
            .onModuleDBInit = EthFwTsn_gptpDbInit,
            .onModuleRunner = EthFwTsn_gptpTask,
        },
        [ETHFWTSN_UNICONF_TASK_IDX] =
        {
            .enable = BFALSE,
            .stopFlag = BTRUE,
            .taskPriority = ETHFW_TSN_UC_TASK_PRIORITY,
            .taskName = ETHFW_TSN_UC_TASK_NAME,
            .stackBuffer = gUniconfStackBuf,
            .stackSize = sizeof(gUniconfStackBuf),
            .onModuleDBInit = EthFwTsn_uniconfInit,
            .onModuleRunner = EthFwTsn_uniconfTask,
        },
};

/* Max domains supported is defined as GPTP_MAX_DOMAINS in jacinto_buildconf.h.
   Please refer to the same for more details */
static EthFwTsn_GptpOpt gGptpOpt =
{
    .confFiles = NULL,
    .domainNum = GPTP_MAX_DOMAINS,
#if GPTP_MAX_DOMAINS == 1U
    .domains = {0},
#elif GPTP_MAX_DOMAINS == 2U
    .domains = {0, 1},
#else
    #error "Only support 2 domains"
#endif
    .instNum = 0U,
    .numConf = 0U,
};


/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

static void EthFwTsn_logTask(void *a0, void *a1)
{
    int32_t len;
    int32_t maxLen = sizeof(gEthFwTsnObj.traceBuf);
    int32_t retLen = 0U;
    int32_t i;

    while (gEthFwTsnObj.logTaskrun)
    {
        MutexP_lock(gEthFwTsnObj.hLogMutex, MutexP_WAIT_FOREVER);
        len = strlen((char *)gEthFwTsnObj.logBuf);
        if (len > 0U)
        {
            memcpy(gEthFwTsnObj.printBuf, gEthFwTsnObj.logBuf, len);
            gEthFwTsnObj.logBuf[0U] = 0U;
            gEthFwTsnObj.printBuf[len] = 0U;
        }
        MutexP_unlock(gEthFwTsnObj.hLogMutex);

        if (len > 0U)
        {
            i = 0U;
            /* The print function will take a long time, we should not
             * call it inside the mutex lock. */
            do
            {
                retLen = snprintf((char *)&gEthFwTsnObj.traceBuf, maxLen, "%s", (char *)&gEthFwTsnObj.printBuf[i]);
                EthFwTrace_print("%s", gEthFwTsnObj.traceBuf);
                i += maxLen;
            }
            while (retLen > 0U && i <= len);
        }

        TaskP_sleepInMsecs(100U);
    }
}

static int32_t EthFwTsn_logBuffer(bool flush, const char *str)
{
    int32_t usedLen;
    int32_t bufSizeLeft;
    int32_t loglen = strlen(str);
    int32_t status = ENET_SOK;

    MutexP_lock(gEthFwTsnObj.hLogMutex, MutexP_WAIT_FOREVER);
    usedLen = strlen((char *)gEthFwTsnObj.logBuf);
    bufSizeLeft = sizeof(gEthFwTsnObj.logBuf)-usedLen;
    if (bufSizeLeft > loglen)
    {
        snprintf((char *)&gEthFwTsnObj.logBuf[usedLen], bufSizeLeft, "%s", str);
    }
    else
    {
        snprintf((char *)&gEthFwTsnObj.logBuf[0], sizeof(gEthFwTsnObj.logBuf), "log overflow!\n");
    }
    MutexP_unlock(gEthFwTsnObj.hLogMutex);

    return status;
}

static void EthFwTsn_startLogTask(void)
{
    TaskP_Params params;

    gEthFwTsnObj.hLogMutex = MutexP_create(&gEthFwTsnObj.logMutexObj);

    /* Create logging task for gPTP stack */
    TaskP_Params_init(&params);
    params.priority  = ETHFW_TSN_LOGGER_TASK_PRIORITY;
    params.stack     = &gEthFwTsnObj.logTaskStackBuf[0];
    params.stacksize = sizeof(gEthFwTsnObj.logTaskStackBuf);
    params.name      = "ETHFW Log Task";

    gEthFwTsnObj.hLogTask = TaskP_create(&EthFwTsn_logTask, &params);
    if (NULL == gEthFwTsnObj.hLogTask)
    {
        ETHFWTRACE_ERR(ENET_EFAIL, "Failed to create log task");
        EnetAppUtils_assert(BFALSE);
    }
}

static void *EthFwTsn_gptpTask(void *args)
{
    int32_t i;
    int32_t status;
    const char *netdevs[IFNAMSIZ];
    EthFwTsn_ModuleCfg *mod = &gModCfgTable[ETHFWTSN_GPTP_TASK_IDX];
    EthFwTsn_UniconfCfg *ucCfg = &gEthFwTsnObj.ucCfg;

    /* Let app overwrite any gPTP configuration parameters */
    if (gEthFwTsnObj.configPtpCb != NULL)
    {
        gEthFwTsnObj.configPtpCb(gEthFwTsnObj.configPtpCbArg);
    }

    for (i = 0U; i < gEthFwTsnObj.numNetDevs; i++)
    {
        netdevs[i] = gEthFwTsnObj.netDevs[i];
    }

    status = gptpgcfg_init(ucCfg->dbName, gGptpOpt.confFiles, gGptpOpt.instNum, true,
                           EthFwTsn_gptpNonYangConfig);

    ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                      "gptpgcfg_init() error in %s", __func__);

    if (ETHFW_SOK == status)
    {
        /* This function start gPTP, it has a true loop inside */
        status = gptpman_run(gGptpOpt.instNum, netdevs, gEthFwTsnObj.numNetDevs,
                             NULL, &mod->stopFlag);
        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, "gptpman_run() failed");
    }

    gptpgcfg_close(gGptpOpt.instNum);
    return NULL;
}

static void *EthFwTsn_uniconfTask(void *args)
{
    EthFwTsn_ModuleCfg *mod = &gModCfgTable[ETHFWTSN_UNICONF_TASK_IDX];
    EthFwTsn_UniconfCfg *uniConfCfg = &gEthFwTsnObj.ucCfg;
    const char *configFiles[2] = {ETHFW_TSN_INTERFACE_CONFFILE_PATH, NULL};

    uniConfCfg->ucData.ucmode      = UC_CALLMODE_THREAD|UC_CALLMODE_UNICONF;
    uniConfCfg->ucData.stoprun     = &mod->stopFlag;
    uniConfCfg->ucData.hwmod       = "";
    uniConfCfg->ucData.ucmanstart  = &uniConfCfg->ucReadySem;
    uniConfCfg->ucData.dbname      = uniConfCfg->dbName;
    uniConfCfg->ucData.configfiles = configFiles;
    uniConfCfg->ucData.numconfigfile = ETHFW_TSN_UC_CONF_FILE_NUM;

    return uniconf_main(&uniConfCfg->ucData);
}

void EthFwTsn_init(EthFwTsn_Config *tsnCfg)
{
    unibase_init_para_t params;

    if (!gEthFwTsnObj.tsnInit)
    {
        /*refer to 'ub_logging.h for logging levels*/
        ubb_default_initpara(&params);
        params.ub_log_initstr    = "4,ubase:45,cbase:45,uconf:34,gptp:45";
        params.cbset.gettime64   = cb_lld_gettime64;
        params.cbset.console_out = EthFwTsn_logBuffer;
        gEthFwTsnObj.logTaskrun = BTRUE;

        EthFwTsn_startLogTask();

        unibase_init(&params);
        ubb_memory_out_init(NULL, 0);
        gEthFwTsnObj.ucCfg.dbName = ETHFW_TSN_UC_DBFILE_PATH;
        gEthFwTsnObj.ucCfg.dbInitFlag = BFALSE;
        gEthFwTsnObj.tsnInit = BTRUE;
    }

    gEthFwTsnObj.enetType       = tsnCfg->enetType;
    gEthFwTsnObj.instId         = tsnCfg->instId;
    gEthFwTsnObj.configPtpCb    = tsnCfg->configPtpCb;
    gEthFwTsnObj.configPtpCbArg = tsnCfg->configPtpCbArg;
}

void EthFwTsn_deInit(void)
{
    EthFwTsn_ModuleCfg *mod;
    uint32_t i;

    gEthFwTsnObj.logTaskrun = BFALSE;

    if (gEthFwTsnObj.hLogTask != NULL)
    {
        TaskP_delete(gEthFwTsnObj.hLogTask);
        gEthFwTsnObj.hLogTask = NULL;
    }
    if (gEthFwTsnObj.hLogMutex != NULL)
    {
        MutexP_delete(&gEthFwTsnObj.hLogMutex);
        gEthFwTsnObj.hLogMutex = NULL;
    }
    if (gEthFwTsnObj.ucCfg.ucReadySem != NULL)
    {
        CB_SEM_DESTROY(&gEthFwTsnObj.ucCfg.ucReadySem);
    }
    if (gEthFwTsnObj.tsnInit)
    {
        for (i = 0; i < ETHFWTSN_MAX_TASK_IDX; i++)
        {
            EthFwTsn_stopModule(i);
        }
    }

    unibase_close();

    gEthFwTsnObj.tsnInit    = BFALSE;
    gEthFwTsnObj.logTaskrun = BFALSE;
}

void EthFwTsn_stopModule(uint32_t moduleIdx)
{
    if ((moduleIdx >= 0) && (moduleIdx < ETHFWTSN_MAX_TASK_IDX))
    {
        EthFwTsn_ModuleCfg *mod;
        mod = &gModCfgTable[moduleIdx];
        if (mod->hTaskHandle != NULL)
        {
            mod->stopFlag = BTRUE;
            CB_THREAD_JOIN(mod->hTaskHandle, NULL);
            mod->hTaskHandle = NULL;
            ETHFWTRACE_INFO("Task: %s is terminated.", mod->taskName);
        }
    }
}

int32_t EthFwTsn_startModule(uint32_t moduleIdx)
{
    int32_t status = ETHFW_SOK;

    if ((moduleIdx >= 0) && (moduleIdx < ETHFWTSN_MAX_TASK_IDX))
    {
        EthFwTsn_ModuleCfg *mod;
        mod = &gModCfgTable[moduleIdx];

        if ((mod->enable == BTRUE) && (mod->stopFlag == BTRUE))
        {
            status = EthFwTsn_startModTask(mod, moduleIdx);
            ETHFWTRACE_INFO_IF((ETHFW_SOK==status),"Task: %s is created.", mod->taskName);
        }
    }
    else
    {
        status = ETHFW_EFAIL;
    }
    return status;
}

int32_t EthFwTsn_initTimeSyncPtp(const uint8_t *hostMacAddr,
                                 uint32_t portMask)
{
    lld_ethdev_t ethdevs[MAX_NUMBER_ENET_DEVS] = {0};
    Enet_MacPort macPort;
    int32_t status = ETHFW_SOK;
    uint32_t i;
    uint32_t j = 0U;

    for (i = 0U; i < ETHFW_TSN_CFG_NUM_MAC_PORTS; i++)
    {
        if (ENET_IS_BIT_SET(portMask, i))
        {
            macPort = ENET_MACPORT_DENORM(i);

            /* Linking each MAC port with an interface name */
            snprintf(&gEthFwTsnObj.netDevs[j][0], IFNAMSIZ, "tilld%d", i + 1);
            gEthFwTsnObj.gPtpNetDevs[j] = &gEthFwTsnObj.netDevs[j][0];
            ethdevs[j].netdev  = gEthFwTsnObj.netDevs[j];
            ethdevs[j].macport = macPort;
            memcpy(&ethdevs[j].srcmac, hostMacAddr, ENET_MAC_ADDR_LEN);

            ETHFWTRACE_INFO("ETHFW: Enable gPTP on MAC port %u (%s)",
                            ENET_MACPORT_ID(macPort), gEthFwTsnObj.gPtpNetDevs[j]);
            j++;
        }
    }

    gEthFwTsnObj.numNetDevs = j;

    /* Filling netdev table where each entry consists of an interface,
     * its MAC port and mac addr (if any) */
    if (status == ETHFW_SOK)
    {
        status  = cb_lld_init_devs_table(ethdevs, gEthFwTsnObj.numNetDevs,
                                         (Enet_Type) gEthFwTsnObj.enetType,
                                         gEthFwTsnObj.instId);
        
        if (ETHFW_SOK != status)
        {
            status = ETHFW_EFAIL;
            ETHFWTRACE_ERR(status, "ETHFW: Failed to int devs table");
        }
    }

    if (ETHFW_SOK == status)
    {
        status = EthFwTsn_startMod();
        ETHFWTRACE_INFO_IF((ETHFW_SOK == status), "ETHFW: TimeSync PTP enabled");
    }

    return status;
}

static int32_t EthFwTsn_uniconfInit(EthFwTsn_dbArgs *dbArgs)
{
    int32_t status = ETHFW_SOK;
    char buffer[MAX_BUFFER_SIZE]={0};
    uint32_t i;

    for (i = 0U; i < gEthFwTsnObj.numNetDevs; i++)
    {
        snprintf(buffer, sizeof(buffer),
                 "/ietf-interfaces/interfaces/interface|name:%s|/enabled",
                 gEthFwTsnObj.netDevs[i]);
        status=yang_db_runtime_put_oneline(dbArgs->ydrd, buffer, (char*)"true",
                                        YANG_DB_ONHW_NOACTION);

        ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, 
                          "ETHFW: yang_db_runtime_put_oneline failed");
    }

    return status;
}

static int32_t EthFwTsn_gptpDbInit(EthFwTsn_dbArgs *dbArgs)
{
    EthFwTsn_UniconfCfg *ucCfg = &gEthFwTsnObj.ucCfg;
    int32_t status = ETHFW_SOK;
    uint32_t i;

    if (gGptpOpt.numConf == 0U)
    {
        /* There is no config file is specified, set config file for gptp */
        for (i = 0; i < gGptpOpt.domainNum; i++)
        {
            status = EthFwTsn_gptpYangConfig(dbArgs->ydrd, gGptpOpt.instNum,
                                             gGptpOpt.domains[i]);

            ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status,
                                  "ETHFW: Failed to set gptp run time config");
        }
    }

    return status;
}

static int32_t EthFwTsn_gptpNonYangConfig(uint8_t instance)
{
    uint32_t i;
    int32_t status;

    for (i = 0U; i < sizeof(gGptpNonYangDs)/sizeof(gGptpNonYangDs[0]); i++)
    {
        status = gptpgcfg_set_item(instance, gGptpNonYangDs[i].item,
                                   YDBI_CONFIG, &gGptpNonYangDs[i].val,
                                   sizeof(gGptpNonYangDs[i].val));

        ETHFWTRACE_DBG_IF(status == ETHFW_SOK,"%s:XL4_EXTMOD_XL4GPTP_%s=%d\n", __func__,
                               gGptpNonYangDs[i].name, gGptpNonYangDs[i].val); 

        if ((status != ETHFW_SOK))
        {
            ETHFWTRACE_ERR(ETHFW_EFAIL,"%s: failed to set nonyang param: %s\n",
                            __func__, gGptpNonYangDs[i].name); 
            break;  
        }
    }
    return status;
}

static int32_t EthFwTsn_gptpYangConfig(yang_db_runtime_dataq_t *ydrd,
                                       uint32_t instance,
                                       uint32_t domain)
{
    char buffer[MAX_BUFFER_SIZE];
    char valueStr[MAX_KEY_SIZE];
    const char *plus;
    uint32_t i;
    int32_t status = ETHFW_SOK;
    EthFwTsn_UniconfCfg *cfg = &gEthFwTsnObj.ucCfg;

    ETHFWTRACE_INFO("%s:domain=%d", __func__, domain);

    do {
        /* Skip setting of 'rw' yang configs when db is already initialized */
        if (!cfg->dbInitFlag)
        {
            plus = ((instance | domain) != 0) ? "+": "";
            snprintf(buffer, sizeof(buffer), "/ieee1588-ptp-tt/ptp/instance-domain-map%s",
                     plus);
            snprintf(valueStr, sizeof(valueStr), "0x%04x", instance<<8|domain);
            yang_db_runtime_put_oneline(ydrd, buffer,
                                        valueStr, YANG_DB_ONHW_NOACTION);
        }

        /* Set for default-ds */
        EthFwTsn_cfgGptpDefaultDs(ydrd, instance, domain, cfg->dbInitFlag);

        /* Portindex starts from 1 */ 
        for (i = 0; i < gEthFwTsnObj.numNetDevs; i++)
        {
            /* Skip setting of 'rw' yang configs when db is already initialized */
            if (!cfg->dbInitFlag)
            {
                snprintf(buffer, sizeof(buffer),
                         "/ieee1588-ptp-tt/ptp/instances/instance|instance-index:%d,%d|"
                         "/ports/port|port-index:%d|/underlying-interface",
                         instance, domain, i+1);
                yang_db_runtime_put_oneline(ydrd, buffer, gEthFwTsnObj.netDevs[i],
                                            YANG_DB_ONHW_NOACTION);
            }

            /* Set for port-ds */
            EthFwTsn_cfgGptpPortDs(ydrd, instance, domain, i+1, cfg->dbInitFlag);
        }

        /* Skip setting of 'rw' yang configs when db is already initialized */
        if (!cfg->dbInitFlag)
        {
            /* Disable performance by default */
            snprintf(buffer, sizeof(buffer),
                     "/ieee1588-ptp-tt/ptp/instances/instance|instance-index:%d,%d|"
                     "/performance-monitoring-ds/enable",
                     instance, domain);
            yang_db_runtime_put_oneline(ydrd, buffer, "false", YANG_DB_ONHW_NOACTION);
        }
    } while (0U);

    return status;
}

static void EthFwTsn_cfgGptpDefaultDs(yang_db_runtime_dataq_t *ydrd,
                                      uint32_t instance,
                                      uint32_t domain,
                                      bool dbInitFlag)
{
    uint32_t i;
    char buffer[MAX_BUFFER_SIZE];

    if (!dbInitFlag)
    {
        for (i = 0U; i < sizeof(gGptpDefaultDsRw)/sizeof(gGptpDefaultDsRw[0]); i++)
        {
            snprintf(buffer, sizeof(buffer),
                     "/ieee1588-ptp-tt/ptp/instances/instance|instance-index:%d,%d|"
                     "/default-ds/%s",
                     instance, domain, gGptpDefaultDsRw[i].name);
            yang_db_runtime_put_oneline(ydrd, buffer, gGptpDefaultDsRw[i].val,
                                        YANG_DB_ONHW_NOACTION);
        }
    }

    for (i = 0U; i < sizeof(gGptpDefaultDsRo)/sizeof(gGptpDefaultDsRo[0]); i++)
    {
        snprintf(buffer, sizeof(buffer),
                 "/ieee1588-ptp-tt/ptp/instances/instance|instance-index:%d,%d|"
                 "/default-ds/%s",
                 instance, domain, gGptpDefaultDsRo[i].name);
        yang_db_runtime_put_oneline(ydrd, buffer, gGptpDefaultDsRo[i].val,
                                    YANG_DB_ONHW_NOACTION);
    }
}

static void EthFwTsn_cfgGptpPortDs(yang_db_runtime_dataq_t *ydrd,
                                   uint32_t instance,
                                   uint32_t domain,
                                   int portIndex,
                                   bool dbInitFlag)
{
    uint32_t i;
    char buffer[MAX_BUFFER_SIZE];

    if (!dbInitFlag)
    {
        for (i = 0U; i < sizeof(gGptpPortDsRw)/sizeof(gGptpPortDsRw[0]); i++)
        {
            snprintf(buffer, sizeof(buffer),
                     "/ieee1588-ptp-tt/ptp/instances/instance|instance-index:%d,%d|"
                     "/ports/port|port-index:%d|/port-ds/%s",
                     instance, domain, portIndex, gGptpPortDsRw[i].name);

            yang_db_runtime_put_oneline(ydrd, buffer, gGptpPortDsRw[i].val,
                                        YANG_DB_ONHW_NOACTION);
        }
    }

    for (i = 0U; i < sizeof(gGptpPortDsRo)/sizeof(gGptpPortDsRo[0]); i++)
    {
        snprintf(buffer, sizeof(buffer),
                 "/ieee1588-ptp-tt/ptp/instances/instance|instance-index:%d,%d|"
                 "/ports/port|port-index:%d|/port-ds/%s",
                 instance, domain, portIndex, gGptpPortDsRo[i].name);

        yang_db_runtime_put_oneline(ydrd, buffer, gGptpPortDsRo[i].val,
                                    YANG_DB_ONHW_NOACTION);
    }
}


static int32_t EthFwTsn_startMod(void)
{
    uint32_t i;
    int32_t status = ETHFW_SOK;
    EthFwTsn_ModuleCfg *mod;

    if (CB_SEM_INIT(&gEthFwTsnObj.ucCfg.ucReadySem, 0U, 0U) != ETHFW_SOK)
    {
        status = ETHFW_EFAIL;
        ETHFWTRACE_ERR(status, "ETHFW: Failed to initialize ucReadySem semaphore!");
    }

    if (gEthFwTsnObj.tsnInit)
    {
        for (i = ETHFWTSN_UNICONF_TASK_IDX; i < ETHFWTSN_MAX_TASK_IDX; i++)
        {
            mod = &gModCfgTable[i];
            if (mod->enable == BFALSE)
            {
                status = EthFwTsn_startModTask(mod, i);
                ETHFWTRACE_ERR_IF((status != ETHFW_SOK), status, 
                                  "ETHFW: Failed to start Task for moduleIdx %u", i);
            }
        }
    }

    return status;
}

static int32_t EthFwTsn_startModTask(EthFwTsn_ModuleCfg *modCfg, uint32_t moduleIdx)
{
    cb_tsn_thread_attr_t attr;
    int32_t status = ETHFW_SOK;

     if (gEthFwTsnObj.ucCfg.ucReadySem != NULL)
    {
        cb_tsn_thread_attr_init(&attr, modCfg->taskPriority,
                                modCfg->stackSize, modCfg->taskName);
        cb_tsn_thread_attr_set_stackaddr(&attr, modCfg->stackBuffer);

        status = CB_THREAD_CREATE(&modCfg->hTaskHandle,
                                  &attr, modCfg->onModuleRunner, modCfg);

        if (ETHFW_SOK != status)
        {
            status = ETHFW_EFAIL;
            ETHFWTRACE_ERR(status, "ETHFW: Failed to create %s task!\n", &modCfg->taskName);
            EnetAppUtils_assert(BFALSE);
        }

        else
        {
            modCfg->stopFlag = BFALSE;
            modCfg->enable = BTRUE;
            if (moduleIdx == ETHFWTSN_UNICONF_TASK_IDX)
            {
                /* initDb must be run right after UNICONF is started and
                   before starting any other tasks. */
                status = EthFwTsn_initDb(&gEthFwTsnObj.ucCfg);
            }
        }
    }

    return status;
}

static int32_t EthFwTsn_initDb(EthFwTsn_UniconfCfg *ucCfg)
{
    EthFwTsn_ModuleCfg *mod;
    EthFwTsn_dbArgs dbArgs;
    uint32_t i;
    int32_t status = ETHFW_SOK;
    uint32_t timeout_ms = 1000U;

    do
    {
        /*waiting for the uniconf to be ready */
        status = CB_SEM_WAIT(&ucCfg->ucReadySem);
        if (ETHFW_SOK != status)
        {
            ETHFWTRACE_ERR(status, "ETHFW: Failed to wait for the uniconf");
            break;
        }

        status = uniconf_ready(ucCfg->dbName, UC_CALLMODE_THREAD, timeout_ms);
        if (status)
        {
            ETHFWTRACE_ERR(status, "ETHFW: The uniconf must be run first!");
            break;
        }

        dbArgs.dbald = uc_dbal_open(ucCfg->dbName, "w", UC_CALLMODE_THREAD);
        if (!dbArgs.dbald)
        {
            status = ETHFW_EFAIL;
            ETHFWTRACE_ERR(status, "ETHFW: Failed to open DB!\n");
            break;
        }
        dbArgs.ydrd = yang_db_runtime_init(dbArgs.dbald, NULL);
        if (!dbArgs.ydrd)
        {
            status = ETHFW_EFAIL;
            ETHFWTRACE_ERR(status, "ETHFW: Failed to init yang db runtime");
            break;
        }

        for (i = 0U; i < ETHFWTSN_MAX_TASK_IDX; i++)
        {
            mod = &gModCfgTable[i];
            if (mod->onModuleDBInit != NULL)
            {
                status = mod->onModuleDBInit(&dbArgs);
                if (ETHFW_SOK != status)
                {
                    ETHFWTRACE_ERR(ETHFW_EFAIL, "ETHFW: Module DB Init failed for ModuleIdx %u", i);
                    break;
                }
            }
        }
    } while (0U);

    if (dbArgs.ydrd)
    {
        yang_db_runtime_close(dbArgs.ydrd);
    }
    if (dbArgs.dbald)
    {
        uc_dbal_close(dbArgs.dbald, UC_CALLMODE_THREAD);
    }

    return status;
}
