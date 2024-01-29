/*
 *
 * Copyright (c) 2023 Texas Instruments Incorporated
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
#define ETHFWTRACE_MOD_ID 0x106

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* TSN header files */
#include <tsn_tilld_include.h>
#include <tsn_combase/combase.h>
#include <tsn_unibase/unibase_binding.h>
#include <tsn_gptp/gptp_config.h>
#include <tsn_gptp/gptpman.h>
#include <tsn_combase/tilld/lldenet.h>

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

/*! gPTP stack priority - should be higher than other non-critical tasks which could interfere*/
#define ETHFW_TSN_GPTP_TASK_PRIORITY                                (7U)

/*! TSN stack size and alignment */
#define ETHFW_TSN_TASK_STACK_SIZE                                   (16U * 1024U)
#define ETHFW_TSN_TASK_STACK_ALIGN                                  ETHFW_TSN_TASK_STACK_SIZE

/*! Log task's buffer stack size and alignment */
#define ETHFW_TSN_LOGGER_TASK_STACK_SIZE                            (2U * 1024U)
#define ETHFW_TSN_LOGGER_TASK_STACK_ALIGN                           (32U)

/*! Size of the buffer used for storing logs before printing */
#define ETHFW_TSN_BUFFER_SIZE                                       (5120U)

/*! Max size of the TSN log print buffer length */
#define ETHFW_TSN_TRACE_MAX_BUFFER_SIZE                             (250U)

/*! Number of MAC ports supported by the gPTP stack */
#if defined(SOC_J721E) || defined(SOC_J784S4)
#define ETHFW_TSN_CFG_NUM_MAC_PORTS                                 (8U)
#else
#define ETHFW_TSN_CFG_NUM_MAC_PORTS                                 (4U)
#endif

/*!
 * \brief Structure holding gPTP and TSN configs
 */
typedef struct EthFwTsn_Obj_s
{
    /* Whether TSN stack has been initialized or not */
    bool tsnInit;

    /* Whether gPTP has been started or not */
    bool ptpStarted;

    /* To run log Task in the loop*/
    bool logTaskrun;

    /* TSN stack netdevs */
    char netDevs[ETHFW_TSN_CFG_NUM_MAC_PORTS][IFNAMSIZ];

    /* gPTP stack netdevs */
    char *gPtpNetDevs[ETHFW_TSN_CFG_NUM_MAC_PORTS + 1];

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

    /* Trace max buffer */
    char traceBuf[ETHFW_TSN_TRACE_MAX_BUFFER_SIZE];

    /* gPTP config callback from app */
    EthFwTsn_configPtpCb configPtpCb;

    /* gPTP config callback argument */
    void *configPtpCbArg;
} EthFwTsn_Obj;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

void EthFwTsn_init(void);

void EthFwTsn_deInit(void);

static void EthFwTsn_gptpStart(char *netdevs[],
                               uint32_t numNetDevs);

static int32_t EthFwTsn_logBuffer(bool flush, const char *str);

static void EthFwTsn_startLogTask(void);

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

EthFwTsn_Obj gEthFwTsnObj;

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
        if (len > 0)
        {
            memcpy(gEthFwTsnObj.printBuf, gEthFwTsnObj.logBuf, len);
            gEthFwTsnObj.logBuf[0] = 0;
            gEthFwTsnObj.printBuf[len] = 0;
        }
        MutexP_unlock(gEthFwTsnObj.hLogMutex);

        if (len > 0)
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

        TaskP_sleep(1000);
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

static void EthFwTsn_gptpTask(void *a0,
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

void EthFwTsn_init(void)
{
    unibase_init_para_t params;

    if (!gEthFwTsnObj.tsnInit)
    {
        /*refer to 'ub_logging.h for logging levels*/
        ubb_default_initpara(&params);
        params.ub_log_initstr    = "5,ubase:5,cbase:5,gptp:5";
        params.cbset.gettime64   = cb_lld_gettime64;
        params.cbset.console_out = EthFwTsn_logBuffer;
        gEthFwTsnObj.logTaskrun = BTRUE;

        EthFwTsn_startLogTask();

        unibase_init(&params);
        ubb_memory_out_init(NULL, 0);
        gEthFwTsnObj.tsnInit = BTRUE;
    }
}

void EthFwTsn_deInit(void)
{
    unibase_close();

    gEthFwTsnObj.logTaskrun = BFALSE;

    if (gEthFwTsnObj.hLogTask != NULL)
    {
        TaskP_delete(gEthFwTsnObj.hLogTask);
        gEthFwTsnObj.hLogTask = NULL;
    }
    if (gEthFwTsnObj.hPtpTask != NULL)
    {
        TaskP_delete(gEthFwTsnObj.hPtpTask);
        gEthFwTsnObj.hPtpTask = NULL;
    }
    if (gEthFwTsnObj.hLogMutex != NULL)
    {
        MutexP_delete(&gEthFwTsnObj.hLogMutex);
        gEthFwTsnObj.hLogMutex = NULL;
    }

    unibase_close();

    gEthFwTsnObj.tsnInit    = BFALSE;
    gEthFwTsnObj.ptpStarted = BFALSE;
    gEthFwTsnObj.logTaskrun = BFALSE;
}

static void EthFwTsn_gptpStart(char *netdevs[],
                               uint32_t numNetDevs)
{
    TaskP_Params params;

    if (!gEthFwTsnObj.ptpStarted)
    {
        /* gPTP Task Init */
        TaskP_Params_init(&params);
        params.priority  = ETHFW_TSN_GPTP_TASK_PRIORITY;
        params.stack     = &gEthFwTsnObj.gPtpStackBuf[0];
        params.stacksize = sizeof(gEthFwTsnObj.gPtpStackBuf);
        params.name      = "ETHFW gPTP Task";
        params.arg0      = netdevs;
        params.arg1      = (void *)numNetDevs;

        gEthFwTsnObj.hPtpTask = TaskP_create(&EthFwTsn_gptpTask, &params);
        if (NULL == gEthFwTsnObj.hPtpTask)
        {
            ETHFWTRACE_ERR(ETHFW_EFAIL, "Failed to create gptp task");
            EnetAppUtils_assert(BFALSE);
        }
        else
        {
            gEthFwTsnObj.ptpStarted = BTRUE;
        }
    }
}

int32_t EthFwTsn_initTimeSyncPtp(EthFwTsn_Config *config,
                                 const uint8_t *hostMacAddr,
                                 uint32_t portMask)
{
    lld_ethdev_t ethdevs[MAX_NUMBER_ENET_DEVS] = {0};
    Enet_MacPort macPort;
    int32_t status = ENET_SOK;
    int32_t singleClk = 1;
    int32_t i;
    int32_t j = 0;

    for (i = 0; i < ETHFW_TSN_CFG_NUM_MAC_PORTS; i++)
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

            ETHFWTRACE_INFO("Enable gPTP on MAC port %u (%s)",
                            ENET_MACPORT_ID(macPort), gEthFwTsnObj.gPtpNetDevs[j]);
            j++;
        }
    }

    /* Save gPTP stack config callback */
    gEthFwTsnObj.configPtpCb = config->configPtpCb;
    gEthFwTsnObj.configPtpCbArg = config->configPtpCbArg;

    gEthFwTsnObj.numNetDevs = j;

    /* Filling netdev table where each entry consists of an interface,
     * its MAC port and mac addr (if any) */
    if (status == ENET_SOK)
    {
        status  = cb_lld_init_devs_table(ethdevs, gEthFwTsnObj.numNetDevs,
                                         (Enet_Type) config->enetType,
                                         config->instId);
        ETHFWTRACE_ERR_IF((status < 0), status, "Failed to int devs table");
    }

    /* CPSW has a single clock for all the ports */
    gptpconf_set_item(CONF_SINGLE_CLOCK_MODE, &singleClk);

    /* Let app overwrite any gPTP configuration parameters */
    if (gEthFwTsnObj.configPtpCb != NULL)
    {
        gEthFwTsnObj.configPtpCb(gEthFwTsnObj.configPtpCbArg);
    }

    EthFwTsn_gptpStart(gEthFwTsnObj.gPtpNetDevs, gEthFwTsnObj.numNetDevs);

    ETHFWTRACE_INFO("TimeSync PTP enabled");

    return status;
}
