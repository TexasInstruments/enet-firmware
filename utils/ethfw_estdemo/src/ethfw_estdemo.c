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
 *  \file ethfw_estdemo.c
 *
 *  \brief Ethernet Firmware EST demo App.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ethremotecfg/server/include/ethfw_tsn.h>
#include <utils/ethfw_estdemo/include/ethfw_estdemo.h>
#include <tsn_unibase/unibase.h>
#include <tsn_unibase/unibase_binding.h>
#include <tsn_uniconf/yangs/yang_modules.h>
#include <ti/drv/enet/include/core/enet_mod_tas.h>
#include <utils/console_io/include/app_log.h>
#include <utils/ethfw_common/include/ethfw_utils.h>
#include <utils/ethfw_common/include/ethfw_trace.h>
#include <tsn_gptp/gptpmasterclock.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define DISPLAY_BITRATE_INTERVAL_SEC    (20U)

#define ESTAPP_TASK_PRIORITY            (1U)
#define ESTAPP_TASK_NAME                "EthFw EST demo Task"
#define ESTAPP_TALKER_NAME              "EthFw EST demo Talker"
#define ESTAPP_LISTENER_NAME            "EthFw EST demo Listener"

#define AVTP_PKT_PAYLOAD_LEN            (1200U)
#define MIN_INTERVAL_NS                 (62000U)
#define ADMIN_DELAY_OFFSET_FACTOR       (100000)

#define EST_DEMO_TSK_STACK_SIZE         (16U * 1024U)
#define EST_DEMO_TSK_STACK_ALIGN        (32U)

/*! Base path of admin list parameters in yang file of Qbv */
#define GATE_PARAM_TABLE_NODE "/ietf-interfaces/interfaces/interface|name:%s|" \
    "/bridge-port/gate-parameter-table"
#define GATE_CONTROL_ENTRY_NODE GATE_PARAM_TABLE_NODE   \
    "/admin-control-list/gate-control-entry"

/*! Base path of clock-state node in  yang file for checking PTP synchronized */
#define IEEE1588_PTP_TT_CLOCKSTATE_NODE  "/ieee1588-ptp-tt/ptp/instances" \
    "/instance|instance-index:0,0|/clock-state"

/*! Base path of port-state node in  yang file for checking PTP synchronized */
#define IEE1588_PTP_PORT_STATE_NODE  "/ieee1588-ptp-tt/ptp/instances" \
    "/instance|instance-index:0,0|/ports/port|port-index:%d|/port-ds"

/* 18: length of layer 2 header */
#define CALC_BITRATE_KBPS(pl_bytes, interval_us)  \
        (uint32_t)( (((uint64_t)(pl_bytes)+18)*8*UB_SEC_US)/ ((interval_us)*1000ULL) )

static const uint8_t STREAMID_PREFIX[7]= {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
static uint8_t MCAST_MAC_ADDR[ENET_MAC_ADDR_LEN] = {0x91, 0xE0, 0xF0, 0x00, 0xFE, 0x00};

typedef struct EstDemoTimeSlot_s
{
    uint64_t start; /*! Expected start time for receiving packet */
    uint64_t end;   /*! Expected end time for receiving packet */
} EstDemoTimeSlot;

typedef struct EstDemoPerPriorityTimeSlot_s
{
    int32_t nLength; /*! Num of timeslots for each priority */
    EstDemoTimeSlot timeSlots[ENET_TAS_MAX_CMD_LISTS]; /*! Timeslot for each priority */
} EstDemoPerPriorityTimeSlot;

typedef struct EstDemoStatsInfo_s
{
    uint64_t nGoodPkt;         /*! Num of packets received inside timeslot */
    uint64_t nBadPkt;          /*! Num of packets received outside timeslot */
} EstDemoStatsInfo;

typedef struct EthFwEstDemoCtx_s
{
    EthFwTsn_ModuleCfg appCtx;/*! Common context param is general for all QoS applications. */
    uint32_t schedIdx;              /*! Index of EST schedule applied for talker and listener. */
    EstDemoStatsInfo estStatsInfo[ESTDEMO_PRIORITY_MAX];
    /*! Expected timeslot for all priority traffic */
    EstDemoPerPriorityTimeSlot exptTimeSlots[ESTDEMO_PRIORITY_MAX];
} EthFwEstDemoCtx;

typedef struct EthFwEstDemoTestParam_s
{
    EnetTas_ControlList list;          /*! List of Admin param for EST */
    EstDemoStreamConfig stParam; /*! Streams parameters */
} EthFwEstDemoTestParam;

UB_SD_GETMEM_DEF_EXTERN(YANGINIT_GEN_SMEM);

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

#if defined(ETHFW_EST_DEMO_SUPPORT)
static void *EthFw_estDemoTask(void *arg1);
#endif

/* ========================================================================== */
/*                          Extern variables                                  */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

static uint8_t gEthFwEstDemoStackBuf[EST_DEMO_TSK_STACK_SIZE]
__attribute__ ((aligned(EST_DEMO_TSK_STACK_ALIGN)));

static uint8_t gEthFwEstDemoTalkerStackBuf[EST_DEMO_TSK_STACK_SIZE]
__attribute__ ((aligned(EST_DEMO_TSK_STACK_ALIGN)));

static uint8_t gEthFwEstDemoListenerStackBuf[EST_DEMO_TSK_STACK_SIZE]
__attribute__ ((aligned(EST_DEMO_TSK_STACK_ALIGN)));

static EthFwEstDemoCtx gEthFwEstDemoCtx;

static EthFwEstDemoTestParam gEthFwEstDemoTestLists[] =
{
    {
        .list =
        {
            .baseTime    = 0ULL,
            .cycleTime   = 4*MIN_INTERVAL_NS,
            .gateCmdList =
            {
                { .gateStateMask = ENET_TAS_GATE_MASK(1, 0, 0, 0, 0, 0, 0, 1),
                  .timeInterval =  MIN_INTERVAL_NS
                },
                { .gateStateMask = ENET_TAS_GATE_MASK(1, 0, 0, 0, 0, 1, 0, 0),
                  .timeInterval =  MIN_INTERVAL_NS
                },
                { .gateStateMask = ENET_TAS_GATE_MASK(1, 0, 0, 0, 0, 0, 0, 1),
                  .timeInterval =  MIN_INTERVAL_NS
                },
                { .gateStateMask = ENET_TAS_GATE_MASK(1, 0, 0, 0, 0, 1, 0, 0),
                  .timeInterval =  MIN_INTERVAL_NS
                },
            },
            .listLength = 4U,
        },
        .stParam =
        {
            .streamParams =
            {
                /* test appliction sends packet with interval 200us */
                {.bitRateKbps = CALC_BITRATE_KBPS(AVTP_PKT_PAYLOAD_LEN, 200),
                 .payloadLen = AVTP_PKT_PAYLOAD_LEN,
                 .tc = 0U,
                 .priority = 0U,
                },

                /* test appliction sends packet with interval 200us */
                {.bitRateKbps = CALC_BITRATE_KBPS(AVTP_PKT_PAYLOAD_LEN, 200),
                 .payloadLen = AVTP_PKT_PAYLOAD_LEN,
                 .tc = 2U,
                 .priority = 2U,
                },
            },
            .nStreams = 2U,
        }
    },
};

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

void EthFwEstDemo_addEstAppModCtx(EthFwTsn_ModuleCfg *modCtxTbl)
{
#if defined(ETHFW_EST_DEMO_SUPPORT)
    EthFwTsn_ModuleCfg estAppModCtx =
    {
        .enable = BFALSE,
        .stopFlag = BTRUE,
        .taskPriority = ESTAPP_TASK_PRIORITY,
        .taskName = ESTAPP_TASK_NAME,
        .stackBuffer = gEthFwEstDemoStackBuf,
        .stackSize = sizeof(gEthFwEstDemoStackBuf),
        .onModuleDBInit = NULL,
        .onModuleRunner = EthFw_estDemoTask,
    };

    memcpy(&modCtxTbl[ETHFWTSN_EST_TASK_IDX], &estAppModCtx,
           sizeof(EthFwTsn_ModuleCfg));
#endif
}

static void EthFwEstDemo_printAdminControlList(EnetTas_ControlList *list)
{
    uint8_t gateMask = 0U;
    uint32_t start = 0U;
    uint32_t end;
    uint32_t dur;
    uint32_t i;

    appLogPrintf("The following AdminList param will be configured for EST:\n");
    for (i = 0U; i < list->listLength; i++)
    {
        gateMask = list->gateCmdList[i].gateStateMask;
        dur = list->gateCmdList[i].timeInterval;
        end = start + dur - 1U;
        /* o = Gate open, C = Gate closed */
        appLogPrintf("GateMask[7..0]=%s%s%s%s%s%s%s%s (0x%02x), start=%u ns, end=%u ns, dur=%u ns\n",
               ENET_IS_BIT_SET(gateMask, 7U) ? "o" : "C",
               ENET_IS_BIT_SET(gateMask, 6U) ? "o" : "C",
               ENET_IS_BIT_SET(gateMask, 5U) ? "o" : "C",
               ENET_IS_BIT_SET(gateMask, 4U) ? "o" : "C",
               ENET_IS_BIT_SET(gateMask, 3U) ? "o" : "C",
               ENET_IS_BIT_SET(gateMask, 2U) ? "o" : "C",
               ENET_IS_BIT_SET(gateMask, 1U) ? "o" : "C",
               ENET_IS_BIT_SET(gateMask, 0U) ? "o" : "C",
               gateMask, start, end, dur);
        start += dur;
    }
    char buffer[MAX_LOG_LEN];
    snprintf(buffer, sizeof(buffer), "Base time=%lluns,Cycle time=%lluns",
             list->baseTime,list->cycleTime);
    appLogPrintf("%s\n", buffer);
}

static int32_t EthFwEstDemo_setAdminControlList(EnetTas_ControlList *list, char *ifname,
                                            yang_db_runtime_dataq_t *ydrd,
                                            uc_notice_data_t *ucntd)
{
    int32_t i;
    int32_t status = ETHFW_SOK;
    char buffer[MAX_KEY_SIZE];
    char val[MAX_VAL_SIZE];

    EthFwEstDemo_printAdminControlList(list);

    if (list->cycleTime > 0U)
    {
        /* Expected unit is mircosecond. */
        uint32_t cycletime_numerator = list->cycleTime/1000U;
        uint32_t cycletime_denominator = 1000000UL;
        snprintf(buffer, sizeof(buffer),
                 GATE_PARAM_TABLE_NODE"/admin-cycle-time/numerator",
                 ifname);
        snprintf(val, sizeof(val), "%d", cycletime_numerator);
        YANGDB_RUNTIME_WRITE(buffer, val);

        snprintf(buffer, sizeof(buffer),
                 GATE_PARAM_TABLE_NODE"/admin-cycle-time/denominator",
                 ifname);
        snprintf(val, sizeof(val), "%d", cycletime_denominator);
        YANGDB_RUNTIME_WRITE(buffer, val);
    }

    uint32_t second = list->baseTime/1000000000ULL;
    uint32_t nanosecond = list->baseTime%1000000000ULL;
    snprintf(buffer, sizeof(buffer),
             GATE_PARAM_TABLE_NODE"/admin-base-time/seconds",
             ifname);
    snprintf(val, sizeof(val), "%d", second);
    YANGDB_RUNTIME_WRITE(buffer, val);

    snprintf(buffer, sizeof(buffer),
             GATE_PARAM_TABLE_NODE"/admin-base-time/nanoseconds",
             ifname);
    snprintf(val, sizeof(val), "%d", nanosecond);
    YANGDB_RUNTIME_WRITE(buffer, val);

    for (i = 0U; i < list->listLength; i++)
    {
        snprintf(buffer, sizeof(buffer),
                 GATE_CONTROL_ENTRY_NODE"|index:%d|/operation-name",
                 ifname, i);
        strcpy(val, "set-gate-states");
        YANGDB_RUNTIME_WRITE(buffer, val);

        snprintf(buffer, sizeof(buffer),
                 GATE_CONTROL_ENTRY_NODE"|index:%d|/time-interval-value",
                 ifname, i);
        snprintf(val, sizeof(val), "%d",
                 list->gateCmdList[i].timeInterval);
        YANGDB_RUNTIME_WRITE(buffer, val);

        snprintf(buffer, sizeof(buffer),
                 GATE_CONTROL_ENTRY_NODE"|index:%d|/gate-states-value",
                 ifname, i);
        snprintf(val, sizeof(val), "%d",
                 list->gateCmdList[i].gateStateMask);
        YANGDB_RUNTIME_WRITE(buffer, val);
    }

    snprintf(buffer, sizeof(buffer), GATE_PARAM_TABLE_NODE"/gate-enabled", ifname);
    strcpy(val, "true");
    YANGDB_RUNTIME_WRITE(buffer, val);

    /* Trigger the uniconf to write parameters from DB to HW */
    status = yang_db_runtime_askaction(ydrd, ucntd);

    return status;
}

static bool EthFwEstDemo_isPTPClockStateSync(EstDemoAppCtx *ctx,
                                           char *netdev)
{
    int32_t status = ETHFW_EFAIL;
    bool syncFlag = BFALSE;
    EthFwTsn_dbArgs dbarg;
    char *dbName = ETHFW_TSN_UC_DBFILE_PATH;

    status = EthFwEstDemo_openDB(&dbarg, dbName, "w");
    if (status)
    {
        appLogPrintf("Failed to open DB!\n");
    }
    do
    {
        char buffer[MAX_KEY_SIZE];
        void *val = NULL;
        uint32_t vsize;
        uint8_t portState = 0U;
        /* Checks for gPTP sync and enables EST on this port */
        int8_t portIdx = DEFAULT_INTERFACE_INDEX;
        DebugP_assert(portIdx >= 0U && portIdx < ETHFW_TSN_CFG_NUM_MAC_PORTS + 1);

        snprintf(buffer, sizeof(buffer),
                 IEEE1588_PTP_TT_CLOCKSTATE_NODE"/gmstate");
        status = yang_db_runtime_get_oneline(dbarg.ydrd, buffer, &val, &vsize);
        if (status == ETHFW_EFAIL)
        {
            appLogPrintf("Failed to read %s from the DB!\n", buffer);
            break;
        }

        syncFlag = *(uint8_t *)val == 2? BTRUE: BFALSE;
        UB_SD_RELMEM(YANGINIT_GEN_SMEM, val);
        val  = NULL;

        if (!syncFlag)
        {
            break;
        }
        syncFlag = BFALSE;
        /* gPTP port index in the DB started from 1 */
        snprintf(buffer, sizeof(buffer),
                 IEE1588_PTP_PORT_STATE_NODE"/port-state", portIdx+1);
        status = yang_db_runtime_get_oneline(dbarg.ydrd, buffer, &val, &vsize);
        if (status == ETHFW_EFAIL)
        {
            appLogPrintf("Failed to read %s \n", buffer);
            break;
        }
        portState =  *(uint8_t *)val;
        UB_SD_RELMEM(YANGINIT_GEN_SMEM, val);
        val  = NULL;

        /* check ieee1588-ptp-tt.yang for description of portState */
        if (portState != 6 && portState != 9)
        {
            appLogPrintf("Current port-state: %d \n", portState);
            break;
        }

        snprintf(buffer, sizeof(buffer), IEE1588_PTP_PORT_STATE_NODE"/as-capable", portIdx+1);
        status = yang_db_runtime_get_oneline(dbarg.ydrd, buffer, &val, &vsize);
        if (status == ETHFW_EFAIL)
        {
            appLogPrintf("Failed to read %s \n", buffer);
            break;
        }
        bool asCapable = *(uint8_t*)val? BTRUE: BFALSE;
        UB_SD_RELMEM(YANGINIT_GEN_SMEM, val);
        if ((portState == 6 || portState == 9) && asCapable)
        {
            syncFlag = BTRUE;
        }
        else if (portState == 9 && !asCapable)
        {
            syncFlag = BTRUE;
        }
    } while (0U);

    EthFwEstDemo_closeDB(&dbarg);

    return syncFlag;
}

static void EthFwEstDemo_showMenu(void)
{
    appLogPrintf("\r\nCPSW EST Test Menu\n:");

    appLogPrintf(" 't'  -  Start Talker\r\n");
    appLogPrintf(" 'l'  -  Start Listener\r\n");
    appLogPrintf(" 'b'  -  Start EST Bridge mode\r\n");
    appLogPrintf(" 'x'  -  Stop Talker\r\n");
    appLogPrintf(" 'n'  -  Stop Listener\r\n");


    appLogPrintf(" Others:\r\n");
    appLogPrintf(" 's'  -  Print statistics\r\n");
    appLogPrintf(" 'S'  -  Reset statistics\r\n");
    appLogPrintf(" 'd'  -  Show EST stats good/bad packets\r\n");
    appLogPrintf(" 'h'  -  Show this menu\r\n");
}

static int32_t EthFwEstDemo_getAdminBaseTime(uint64_t *time)
{
    int32_t status = ETHFW_SOK;
    int64_t ts = gptpmasterclock_getts64();
    if (ts < 0U)
    {
        status = ETHFW_EFAIL;
    }
    else
    {
        *time = ts;
    }
    return status;
}

static int32_t EthFwEstDemo_runSchedule(EstDemoAppCtx *ctx,
                                    EnetTas_ControlList *adminList,
                                    char *netdev)
{
    bool openDBSuccess = BFALSE;
    EthFwTsn_dbArgs dbarg;
    char *dbName = ETHFW_TSN_UC_DBFILE_PATH;
    int32_t status;
    uint32_t i;

    do
    {
        status = EthFwEstDemo_openDB(&dbarg, dbName, "w");
        if (status)
        {
            appLogPrintf("Failed to open DB!\n");
            break;
        }
        openDBSuccess = BTRUE;

        EstDemoCommonParam prm =
        {
            .netdev = netdev,
            /* initialize with invalid TC value */
            {-1, -1, -1, -1, -1, -1, -1, -1},
            .nTCs = ESTDEMO_PRIORITY_MAX,
            .nQueues = ESTDEMO_PRIORITY_MAX,
        };
        for (i = 0U; i < prm.nTCs; i++)
        {
            /* Use one-to-one mapping between TC and priority */
            prm.priority2TcMapping[i] = i;
        }

        status = EthFwEstDemo_setCommonParam(&prm, &dbarg);
        if (status)
        {
            appLogPrintf("Failed to set EST common param!\n");
            break;
        }

        if (EthFwEstDemo_getAdminBaseTime(&adminList->baseTime) == 0U)
        {
            /* Add a delay time to allow the admin list scheduled in the future
            * the offset should be large enough to have  both EST schedules from
            * talker and listener started at the same time
            */
            int64_t offset = ADMIN_DELAY_OFFSET_FACTOR*adminList->cycleTime;
            adminList->baseTime = ((adminList->baseTime+offset)/offset)*offset;
            ctx->adminDelayOffset = offset/1000; /* Convert to microsecond */
        }
        status = EthFwEstDemo_setAdminControlList(adminList,
                                             netdev,
                                             dbarg.ydrd, dbarg.ucntd);
        if (status)
        {
            appLogPrintf("Failed to set admin control list for %s\n",
                   netdev);
            break;
        }
        else
        {
            appLogPrintf("Set admin control list succesfully\n");
        }
    } while (0U);

    if (openDBSuccess)
    {
        EthFwEstDemo_closeDB(&dbarg);
    }

    return status;
}

static void EthFwEstDemo_startTalker(EstDemoAppCtx *ctx,
                                     EstDemoStreamConfig *stParam)
{
    uint32_t i;
    EstDemoTaskCfg cfg =
    {
        .name = ESTAPP_TALKER_NAME,
        .priority  = ESTDEMO_TASK_PRIORITY,
        .stackBuffer = gEthFwEstDemoTalkerStackBuf,
        .stackBufferSize = sizeof(gEthFwEstDemoTalkerStackBuf),
    };

    for (i = 0U; i < stParam->nStreams; i++)
    {
        appLogPrintf("Talker: starting stream %d with, payloadLen: %d, bitrate: %d kbps, priority: %d\n",
               i, stParam->streamParams[i].payloadLen,
               stParam->streamParams[i].bitRateKbps,
               stParam->streamParams[i].priority);
    }
    return EthFwEstDemo_startCfgTalker(ctx, &cfg, stParam);
}

static void EthFwEstDemo_startListener(EstDemoAppCtx *ctx)
{
    EstDemoTaskCfg cfg =
    {
        .name = ESTAPP_LISTENER_NAME,
        .priority  = ESTDEMO_TASK_PRIORITY,
        .stackBuffer = gEthFwEstDemoListenerStackBuf,
        .stackBufferSize = sizeof(gEthFwEstDemoListenerStackBuf),
    };
    return EthFwEstDemo_startCfgListener(ctx, &cfg);
}

static void EthFwEstDemo_rxPacketHandler(void *arg,
                                         EstDemoPacket *pkt)
{
    bool goodPktFlag = BFALSE;
    uint32_t i, priority;
    EthFwEstDemoCtx *estAppCtx = (EthFwEstDemoCtx *)arg;
    EnetTas_ControlList *adminList = &gEthFwEstDemoTestLists[estAppCtx->schedIdx].list;
    int64_t timeSlot = pkt->recvAddr.rxts - adminList->baseTime;

    priority = pkt->recvAddr.tcid;
    if (priority < ESTDEMO_PRIORITY_MAX)
    {
        timeSlot = timeSlot%adminList->cycleTime;

        for (i = 0U; i < estAppCtx->exptTimeSlots[priority].nLength; i++)
        {
            if (timeSlot >= estAppCtx->exptTimeSlots[priority].timeSlots[i].start &&
                timeSlot <= estAppCtx->exptTimeSlots[priority].timeSlots[i].end)
            {
                estAppCtx->estStatsInfo[priority].nGoodPkt++;
                goodPktFlag = BTRUE;
                break;
            }
        }
    }
    if (!goodPktFlag)
    {
        estAppCtx->estStatsInfo[priority].nBadPkt++;
    }
}

static void EthFwEstDemo_showEstStats(EstDemoAppCtx *ctx)
{
    uint32_t i;
    EthFwEstDemoCtx *estAppCtx = (EthFwEstDemoCtx *)ctx;
    char buffer[MAX_LOG_LEN];
    uint64_t goodPkts, badPkts;

    for (i = 0U; i < ESTDEMO_PRIORITY_MAX; i++)
    {
        goodPkts = estAppCtx->estStatsInfo[i].nGoodPkt;
        badPkts = estAppCtx->estStatsInfo[i].nBadPkt;
        if (goodPkts > 0U ||  badPkts > 0U)
        {
            snprintf(buffer, sizeof(buffer),
                     "PacketPriority: %d, nGoodPackets: %llu, nBadPackets: %llu, percentage of bad packets: %llu%%",
                     i, goodPkts, badPkts, (badPkts*100)/(badPkts+goodPkts));
            appLogPrintf("%s\n", buffer);
        }
        else
        {
            appLogPrintf("No good or bad packets received of priority: %d\n", i);
        }
    }
}

static void EthFwEstDemo_runTalker(EstDemoAppCtx *ctx)
{
    int32_t status;

    while (!EthFwEstDemo_isPTPClockStateSync(ctx, ctx->netdev[ctx->ifidx]))
    {
        appLogPrintf("Waiting for PTP clock to be synchronized!\n");
        TaskP_sleepInMsecs(2000ULL);
    }
    uint32_t schedIdx = 0U;
    status = EthFwEstDemo_runSchedule(ctx,
                                   &gEthFwEstDemoTestLists[schedIdx].list,
                                   ctx->netdev[ctx->ifidx]);
    if (status == 0U)
    {
        TaskP_sleepInMsecs((ctx->adminDelayOffset)/1000);

        EthFwEstDemo_startTalker(ctx, &gEthFwEstDemoTestLists[schedIdx].stParam);
    }
}

static void EthFwEstDemo_defragmentTimeSlots(EstDemoPerPriorityTimeSlot *prm)
{
    uint32_t i;
    EstDemoTimeSlot timeSlots[ENET_TAS_MAX_CMD_LISTS];
    uint32_t count = 0U;

    for (i = 0U; i < ENET_TAS_MAX_CMD_LISTS; i++)
    {
        if (prm->timeSlots[i].end > 0U)
        {
            memcpy(&timeSlots[count],
                   &prm->timeSlots[i], sizeof(EstDemoTimeSlot));
            count++;
        }
    }
    memcpy(prm->timeSlots, timeSlots, count*sizeof(EstDemoTimeSlot));
    prm->nLength = count;
}

static void EthFwEstDemo_calcExpectedTimeSlot(EstDemoAppCtx *ctx,
                                            EnetTas_ControlList *list,
                                            EstDemoStreamConfig *stParam)
{
    uint32_t i, j, priority, n = 0U;
    EthFwEstDemoCtx *estAppCtx = (EthFwEstDemoCtx *)ctx;

    for (i = 0U; i < stParam->nStreams; i++)
    {
        priority = stParam->streamParams[i].priority;
        for (j = 0U; j < list->listLength; j++)
        {
            if (list->gateCmdList[j].gateStateMask&(1<<priority))
            {
                estAppCtx->exptTimeSlots[priority].timeSlots[j].start =
                    j*list->gateCmdList[j].timeInterval;
                estAppCtx->exptTimeSlots[priority].timeSlots[j].end =
                    (j+1)*list->gateCmdList[j].timeInterval;
                estAppCtx->exptTimeSlots[priority].nLength++;
            }
        }
        EthFwEstDemo_defragmentTimeSlots(&estAppCtx->exptTimeSlots[priority]);
    }

    // Show expected time slots of each stream for debug purpose.
    char buffer[MAX_LOG_LEN];
    for (i = 0U; i < stParam->nStreams; i++)
    {
        priority = stParam->streamParams[i].priority;
        n = snprintf(buffer, sizeof(buffer), "TimeSlots of PacketPriority: %d: ", priority);
        for (j = 0U; j < estAppCtx->exptTimeSlots[priority].nLength; j++)
        {
            n += snprintf(&buffer[n], sizeof(buffer)-n, "  [%llu, %llu]ns,",
                          estAppCtx->exptTimeSlots[priority].timeSlots[j].start,
                          estAppCtx->exptTimeSlots[priority].timeSlots[j].end);
            DebugP_assert(n < sizeof(buffer));
        }
        appLogPrintf("%s\n", buffer);
    }
}

static void EthFwEstDemo_runListener(EstDemoAppCtx *ctx)
{
    EthFwEstDemoCtx *estAppCtx = (EthFwEstDemoCtx *)ctx;
    int32_t status = ETHFW_SOK;
    uint32_t schedIdx = 0U;

    while (!EthFwEstDemo_isPTPClockStateSync(ctx, ctx->netdev[ctx->ifidx]))
    {
        appLogPrintf("Waiting for PTP clock to be synchronized!\n");
        TaskP_sleepInMsecs(2000ULL);
    }

    EthFwEstDemo_calcExpectedTimeSlot(ctx, &gEthFwEstDemoTestLists[schedIdx].list,
                                    &gEthFwEstDemoTestLists[schedIdx].stParam);

    status = EthFwEstDemo_runSchedule(ctx, &gEthFwEstDemoTestLists[schedIdx].list,
                                 ctx->netdev[ctx->ifidx]);
    if (status == ETHFW_SOK)
    {
        estAppCtx->schedIdx = schedIdx;
        memset(estAppCtx->estStatsInfo, 0U, sizeof(estAppCtx->estStatsInfo));

        EthFwEstDemo_startListener(ctx);
    }
}

static void EthFwEstDemo_runBridgeMode(EstDemoAppCtx *ctx)
{
    uint32_t i, schedIdx = 0U;
    int32_t status = ETHFW_SOK;

    for (i = 0U; i < ctx->netdevSize; i++)
    {
        while (!EthFwEstDemo_isPTPClockStateSync(ctx, ctx->netdev[i]))
        {
            appLogPrintf("Waiting for PTP clock to be synchronized!\n");
            TaskP_sleepInMsecs(2000ULL);
        }
        status = EthFwEstDemo_runSchedule(ctx, &gEthFwEstDemoTestLists[schedIdx].list,
                                     ctx->netdev[i]);
        DebugP_assert(status == ETHFW_SOK);
    }
}

static void *EthFw_estDemoTask(void *arg1)
{
#if defined(ETHFW_EST_DEMO_SUPPORT)
    int32_t status;
    char option;
    EthFwTsn_ModuleCfg *modCtx = (EthFwTsn_ModuleCfg *)arg1;
    EstDemoAppCtx *ctx =  (EstDemoAppCtx *)&gEthFwEstDemoCtx;

    status = EthFwEstDemo_initialize(ctx, modCtx,
                                     EthFwEstDemo_rxPacketHandler);
    DebugP_assert(status == ETHFW_SOK);

    EthFwEstDemo_showMenu(); //TODO

    while (modCtx->enable)
    {
        option = EnetAppUtils_getChar2();

        switch(option)
        {
        case 't':
            appLogPrintf("ETHFW: Starting Talker!\n");
            EthFwEstDemo_runTalker(ctx);
            break;
        case 'l':
            appLogPrintf("ETHFW: Starting Listener!\n");
            EthFwEstDemo_runListener(ctx);
            break;
        case 'b':
            EthFwEstDemo_runBridgeMode(ctx);
            break;
        case 's':
            EthFwEstDemo_printStats(ctx);
            break;
        case 'r':
            EthFwEstDemo_resetStats(ctx);
            break;
        case 'd':
            EthFwEstDemo_showEstStats(ctx);
            break;
        case 'x':
            EthFwEstDemo_stopTalker(ctx);
            break;
        case 'n':
            EthFwEstDemo_stopListener(ctx);
            break;
        default:
            EthFwEstDemo_showMenu();
            break;
        }
        TaskP_yield();
    }
    EthFwEstDemo_stopListener(ctx);
#else
    appLogPrintf("ETHFW: EST Demo build flag is disabled!\n");
#endif
    return NULL;
}

uint64_t EthFwEstDemo_getCurrentTimeUs(void)
{
    return (cb_lld_gettime64(UB_CLOCK_REALTIME)/1000U);
}

void EthFwEstDemo_initEthFwEstDemo_BitrateCtrl(EstDemoBitrateCtrl *bc,
                                  uint32_t maxCapacity,
                                  uint64_t bitRate)
{
    bc->maxCapacity = maxCapacity;
    bc->tokens = bc->maxCapacity;
    bc->bitRate = bitRate;
    bc->lastTs = 0U;
}

void EthFwEstDemo_deinitEthFwEstDemo_BitrateCtrl(EstDemoBitrateCtrl *bc)
{
    memset(bc, 0, sizeof(EstDemoBitrateCtrl));
}

static void EthFwEstDemo_updateTokensForEthFwEstDemo_BitrateCtrl(EstDemoBitrateCtrl *bc)
{
    if (bc->lastTs == 0U)
    {
        bc->lastTs = EthFwEstDemo_getCurrentTimeUs();
    }
    else
    {
        uint64_t curTs = EthFwEstDemo_getCurrentTimeUs();
        uint64_t delta = curTs > bc->lastTs? curTs - bc->lastTs: 0U;
        if (delta > 0U)
        {
            uint64_t tokens = ((bc->bitRate)*delta)/1000000U;
            bc->tokens += (uint32_t)(tokens/8);
            if (bc->tokens > bc->maxCapacity)
            {
                bc->tokens = bc->maxCapacity;
            }
        }
        bc->lastTs = curTs;
    }
}

bool EthFwEstDemo_checkTransmitReady(EstDemoBitrateCtrl *bc,
                                     uint32_t bytes,
                                     uint64_t *sleepTimeUs)
{
    bool res = BFALSE;
    EthFwEstDemo_updateTokensForEthFwEstDemo_BitrateCtrl(bc);
    if (bytes <= bc->tokens)
    {
        bc->tokens -= bytes;
        res = BTRUE;
        *sleepTimeUs = 0U;
    }
    else
    {
        uint64_t tokens = bytes-bc->tokens;
        *sleepTimeUs = (tokens*8*1000000U)/bc->bitRate;
        if (*sleepTimeUs == 0U)
        {
            res = BTRUE;
            bc->tokens = 0U;
        }
    }
    return res;
}

/* mode could be "w" for writing  or "r" for reading */
int32_t EthFwEstDemo_openDB(EthFwTsn_dbArgs *dbarg, char *dbName, const char *mode)
{
    int32_t status = ETHFW_SOK;
    uint32_t timeout_ms = 500;
    do {
        status = uniconf_ready(dbName, UC_CALLMODE_THREAD, timeout_ms);
        if (status != 0U)
        {
            appLogPrintf("The uniconf must be run first!\n");
            break;
        }
        status = ETHFW_EFAIL;
        dbarg->dbald = uc_dbal_open(dbName, mode, UC_CALLMODE_THREAD);
        if (!dbarg->dbald)
        {
            appLogPrintf("Failed to open DB for EstApp!\n");
            break;
        }
        dbarg->ucntd = uc_notice_init(UC_CALLMODE_THREAD, dbName);
        if (!dbarg->ucntd)
        {
            appLogPrintf("Failed to open uc notice!\n");
            break;
        }
        dbarg->ydrd = yang_db_runtime_init(dbarg->dbald, NULL);
        if (!dbarg->ydrd)
        {
            appLogPrintf("Failed to init DB runtime!\n");
            break;
        }
        status = ETHFW_SOK;
    } while (0U);
    return status;
}

void EthFwEstDemo_closeDB(EthFwTsn_dbArgs *dbarg)
{
    uc_notice_close(dbarg->ucntd, 0U);
    yang_db_runtime_close(dbarg->ydrd);
    uc_dbal_close(dbarg->dbald, UC_CALLMODE_THREAD);
}

int32_t EthFwEstDemo_setCommonParam(EstDemoCommonParam *prm,
                                EthFwTsn_dbArgs *dbarg)
{
    int32_t status = ETHFW_SOK;
    uint32_t i = 0U;
    char buffer[MAX_KEY_SIZE];
    char val[MAX_VAL_SIZE];
    yang_db_runtime_dataq_t *ydrd = dbarg->ydrd;

    /* Write the num of traffic classes and value of each TC to DB */
    snprintf(buffer, sizeof(buffer),
             TRAFFIC_CLASS_TABLE_NODE"/number-of-traffic-classes",
             prm->netdev);
    snprintf(val, sizeof(val), "%d", prm->nTCs);
    YANGDB_RUNTIME_WRITE(buffer, val);

    /* Use one-to-one mapping of priority to logical queue */
    for (i = 0U; i < prm->nTCs; i++)
    {
        snprintf(buffer, sizeof(buffer),
                 TRAFFIC_CLASS_TABLE_NODE"/priority%d",
                 prm->netdev, i);
        snprintf(val, sizeof(val), "%d",
                 prm->priority2TcMapping[i]);
        YANGDB_RUNTIME_WRITE(buffer, val);

        /* Map same number of priority to logical queue */
        snprintf(buffer, sizeof(buffer),
                 TRAFFIC_CLASS_DATA_NODE"|tc:%d|/lqueue",
                 prm->netdev, prm->priority2TcMapping[i]);
        snprintf(val, sizeof(val), "%d", i);
        YANGDB_RUNTIME_WRITE(buffer, val);
    }

    snprintf(buffer, sizeof(buffer),
             TRAFFIC_CLASS_NODE"/number-of-pqueues", prm->netdev);
    snprintf(val, sizeof(val), "%d", prm->nQueues);
    YANGDB_RUNTIME_WRITE(buffer, val);

    /* Use one-to-one mapping of logical queue to HW queue */
    for (i = 0U; i < prm->nQueues; i++)
    {
        snprintf(buffer, sizeof(buffer),
                 PHYSICAL_QUEUE_MAP_NODE"|pqueue:%d|/lqueue",
                 prm->netdev, i);
        snprintf(val, sizeof(val), "%d", i);
        YANGDB_RUNTIME_WRITE(buffer, val);
    }

    return status;
}

static void EthFwEstDemo_rxNotifyCb(void* arg)
{
    EstDemoAppCtx *ctx = (EstDemoAppCtx *)arg;
    if (ctx->listener.rxPacketSem)
    {
        CB_SEM_POST(&ctx->listener.rxPacketSem);
    }
}

int32_t EthFwEstDemo_initialize(EstDemoAppCtx *ctx,
                            EthFwTsn_ModuleCfg *modCtx,
                            PacketHandlerCb cb)
{
    int32_t status;
    uint32_t i;
    cb_rawsock_paras_t param;
    EthFwTsn_netDevInfo *devInfo;
    EthFwEstDemoCtx *estStatCtx = (EthFwEstDemoCtx *)ctx;
    CpswAle_VlanEntryInfo vlanInArgs;
    Enet_IoctlPrms prms;
    uint32_t aleEntry;
    Enet_Type enetType;
    uint32_t instId;
    uint32_t coreId;
    Enet_Handle hEnet;

#if defined(SOC_J721E) || defined(SOC_J784S4)
    enetType = ENET_CPSW_9G,
    instId   = 0U,
#elif defined(SOC_J7200)
    enetType = ENET_CPSW_5G,
    instId   = 0U,
#endif

    hEnet = Enet_getHandle(enetType, instId);
    coreId = EnetSoc_getCoreId();

    /* Add ALE VLAN entry */
    memset(&vlanInArgs, 0U, sizeof (CpswAle_VlanEntryInfo));
    vlanInArgs.vlanIdInfo.vlanId       = AVTP_VLAN_ID;
    vlanInArgs.vlanIdInfo.tagType      = ENET_VLAN_TAG_TYPE_INNER;
    vlanInArgs.vlanMemberList          = ESTDEMO_VLAN_VID_MASK;
    vlanInArgs.regMcastFloodMask       = ESTDEMO_VLAN_VID_MASK;
    vlanInArgs.unregMcastFloodMask     = ESTDEMO_VLAN_VID_MASK;

    ENET_IOCTL_SET_INOUT_ARGS(&prms, &vlanInArgs, &aleEntry);

    status = Enet_ioctl(hEnet, coreId, CPSW_ALE_IOCTL_ADD_VLAN, &prms);

    if(ETHFW_SOK == status)
    {
        status = EthFwTsn_getDevInfo(devInfo);
    }

    if(ETHFW_SOK == status)
    {
        ctx->netdevSize = devInfo->numNetDevs;
        for (i = 0U; i < ctx->netdevSize; i++)
        {
            ctx->netdev[i] = devInfo->netdevs[i];
        }
    }
    else
    {
        appLogPrintf("Failed to get dev info for EST demo App!, status: %d\n", status);
    }

    ctx->ifidx = DEFAULT_INTERFACE_INDEX;
    ctx->packetHandlerCb = cb;
    ctx->talker.vid = AVTP_VLAN_ID;
    ctx->talker.nStreams = ESTDEMO_NUM_OF_STREAMS;
    ctx->talker.nTCs = ESTDEMO_NUM_OF_STREAMS;

    memset(&param, 0, sizeof(param));
    param.dev = ctx->netdev[ctx->ifidx];
    param.proto = ETH_P_TSN;
    param.vlan_proto = param.proto;
    param.rw_type = CB_RAWSOCK_RDWR;
    param.sock_mode = CB_SOCK_MODE_NORMAL;
    param.vlanid = AVTP_VLAN_ID;
    uint32_t mtuSize = 0U;
    status = cb_rawsock_open(&param, &ctx->est_sock,
                          &ctx->sockAddress, &mtuSize,
                          ctx->source_mac);
    if (status)
    {
        appLogPrintf("Failed to open socket for EST app!, status: %d\n", status);
    }
    else
    {
        DebugP_assert(mtuSize >= sizeof(ctx->talker.buffer));
        status = cb_lld_set_rxnotify_cb(ctx->est_sock,
                                     EthFwEstDemo_rxNotifyCb,
                                     ctx);
    }

    if (status)
    {
        appLogPrintf("Failed to set rx notify callback!, status: %d\n", status);
    }
    else
    {
        /* Initialize gptpmaster clock  to get PTP time */
        while (gptpmasterclock_init(NULL))
        {
            /* wait gptpmaster clock ready */
            TaskP_sleepInMsecs(100000);
        }
    }

    return status;
}

void EthFwEstDemo_deinitialize(EstDemoAppCtx *ctx)
{
    cb_rawsock_close(ctx->est_sock);
    gptpmasterclock_close();
}

static void EthFwEstDemo_createAVTPHeader(EstDemoStreamParams *sprm,
                                          EstDemoCommonStreamHdr *avtphdr,
                                          uint16_t dataLen)
{
    int64_t pts = 0U;
    avtphdr->hh.subtype = AVTP_AAF_SUBTYPE;
    avtphdr->pdLength = Enet_htons(dataLen);
    memcpy(avtphdr->streamId, sprm->sid, sizeof(ub_streamid_t));
    avtphdr->hh.seqn = sprm->seqn;
    sprm->seqn++;
    pts = gptpmasterclock_getts64();
    /* Get lower 32 bits of the timestamp for avtp timestamp */
    avtphdr->headerTimestamp = Enet_htonl(pts);
    avtphdr->bf=cmsh_sv_set_bit_field(avtphdr->bf, 1);
    avtphdr->hh.bf0 |= 0x1;
}

static void EthFwEstDemo_initStreamParams(EstDemoAppCtx *ctx)
{
    uint32_t i;
    EstDemoTaskCtx *talker = &ctx->talker;

    uint8_t streamId[8];

    EthVlanFrame *txFrame;
    txFrame = (EthVlanFrame *)talker->buffer;
    memcpy(streamId, STREAMID_PREFIX, sizeof(STREAMID_PREFIX));

    /* Initialize constant data for ethernet header which are used for all streams */
    memcpy(txFrame->hdr.dstMac, MCAST_MAC_ADDR, ENET_MAC_ADDR_LEN);
    memcpy(txFrame->hdr.srcMac, ctx->source_mac, ENET_MAC_ADDR_LEN);
    txFrame->hdr.tpid = Enet_htons(ESTDEMO_VLAN_TPID);
    txFrame->hdr.etherType = Enet_htons(ETH_P_TSN);

    for (i = 0U; i < talker->nStreams; i++)
    {
        talker->streams[i].priority = i;
        talker->streams[i].tc = i; /* Default setting for TCs */
        streamId[7] = talker->streams[i].tc;
        memcpy(talker->streams[i].sid, streamId, sizeof(streamId));
        talker->streams[i].seqn = 0U;
    }
}

static void EthFwEstDemo_initTalker(EstDemoTaskCtx *talker)
{
    uint32_t i;
    uint32_t payloadLen;
    uint64_t bitRate;

    for (i = 0U; i < talker->nStreams; i++)
    {
        bitRate = talker->streams[i].bitRate;
        EthFwEstDemo_initEthFwEstDemo_BitrateCtrl(&talker->bitrateCtrl[i],
                                   2*sizeof(talker->buffer), bitRate);
        payloadLen = talker->streams[i].payloadLen;
        char buffer[MAX_KEY_SIZE];
        snprintf(buffer, sizeof(buffer),
                 "Talker[%d], payloadLen: %d bytes, bitRate: %lld kbps",
                i, payloadLen, bitRate/1000);
        appLogPrintf("%s\n", buffer);
    }
}

static void *EthFwEstDemo_talkerHandler(void *arg)
{
    uint32_t i;
    int32_t status;
    EstDemoAppCtx *ctx = (EstDemoAppCtx *)arg;
    EstDemoTaskCtx *talker = &ctx->talker;
    uint64_t minSleepTime = UINT64_MAX;
    EthVlanFrame *txFrame;
    EstDemoCommonStreamHdr *avtphdr;
    uint32_t payloadLen, frameLen;
    uint32_t nShortSleep = 0U;
#define HIGH_CPU_LOADHRESHOLD (15)

    while (talker->enable)
    {
        for (i = 0U; i < talker->nStreams; i++)
        {
            payloadLen = talker->streams[i].payloadLen;
            frameLen = sizeof(EthVlanFrameHeader) + payloadLen;
            uint64_t sleepTimeUs = 0U;
            if (EthFwEstDemo_checkTransmitReady(&talker->bitrateCtrl[i],
                                                frameLen,
                                                &sleepTimeUs))
            {
                txFrame = (EthVlanFrame *)talker->buffer;
                uint32_t tci = ESTDEMO_VLAN_TCI(talker->streams[i].priority, 0U, talker->vid);
                txFrame->hdr.tci  = Enet_htons(tci);
                avtphdr = (EstDemoCommonStreamHdr *)txFrame->payload;
                uint16_t dataLen = payloadLen - sizeof(EstDemoCommonStreamHdr);
                EthFwEstDemo_createAVTPHeader(&talker->streams[i], avtphdr, dataLen);
                memset(&txFrame->payload[sizeof(EstDemoCommonStreamHdr)],
                       talker->streams[i].tc, payloadLen - sizeof(EstDemoCommonStreamHdr));
                status = CB_SOCK_SENDTO(ctx->est_sock, talker->buffer,
                                         frameLen, 0U, &ctx->sockAddress,
                                         sizeof(ctx->sockAddress));
                if (status == ETHFW_EFAIL)
                {
                    appLogPrintf("Failed to send %d bytes on stream %d\n", frameLen, i);
                }
                minSleepTime = 0U;
            }
            else
            {
                if (minSleepTime > sleepTimeUs)
                {
                    minSleepTime = sleepTimeUs;
                }
            }
        }

        /* sleep less than 1ms doesn't help to reduce cpuload. Therefore, to reduce
         * the cpu load in case of high bitrate, it need to take a sufficient sleep (1ms)
         */
        nShortSleep = minSleepTime < UB_MSEC_US? (nShortSleep+1): 0U;

        if (nShortSleep >= HIGH_CPU_LOADHRESHOLD && minSleepTime > 0U)
        {
            minSleepTime = UB_MSEC_US;
            TaskP_yield();
            nShortSleep = 0U;
        }
        if (minSleepTime != UINT64_MAX && minSleepTime > 0U)
        {
            TaskP_sleepInMsecs(minSleepTime);
        }
        minSleepTime = UINT64_MAX;
    }

    CB_SEM_POST(&talker->terminatedSem);
    appLogPrintf("EST App talker is terminating ...\n");

    return NULL;
}

static int32_t EthFwEstDemo_setInputParam(EstDemoAppCtx *ctx,
                                      EstDemoStreamConfig *stParams)
{
    uint32_t i = 0U;
    int32_t status = ETHFW_SOK;
    EstDemoTaskCtx *talker = &ctx->talker;
    if (stParams->nStreams == 0U)
    {
        appLogPrintf("Invalid number of stream: %d\n", stParams->nStreams);
        status = ETHFW_EFAIL;
    }
    else
    {
        talker->nStreams = stParams->nStreams;
        for (i = 0U; i < stParams->nStreams; i++)
        {
            talker->streams[i].bitRate = stParams->streamParams[i].bitRateKbps*1000U;
            talker->streams[i].payloadLen = stParams->streamParams[i].payloadLen;
            talker->streams[i].priority = stParams->streamParams[i].priority;
            talker->streams[i].tc = stParams->streamParams[i].tc;
            /* Set the last byte of stream if to priority */
            talker->streams[i].sid[7u] = talker->streams[i].priority;
        }
    }

    return status;
}

static void EthFwEstDemo_initTaskCtx(cb_tsn_thread_attr_t *attr,
                                     EstDemoTaskCfg *cfg)
{
    cb_tsn_thread_attr_init(attr, cfg->priority,
                            cfg->stackBufferSize,
                            cfg->name);
    cb_tsn_thread_attr_set_stackaddr(attr, cfg->stackBuffer);
}

void EthFwEstDemo_startCfgTalker(EstDemoAppCtx *ctx,
                                 EstDemoTaskCfg *cfg,
                                 EstDemoStreamConfig *stParams)
{
    int32_t status = ETHFW_SOK;
    EstDemoTaskCtx *talker = &ctx->talker;

    DebugP_assert(stParams != NULL);
    if (!talker->enable)
    {
        if (CB_SEM_INIT(&talker->terminatedSem, 0U, 0U) < 0U)
        {
            appLogPrintf("Failed to create terminatedSem\n");
            status = ETHFW_EFAIL;
        }
        talker->enable = BTRUE;
        if (!talker->hTaskHandle && status == ETHFW_SOK)
        {
            cb_tsn_thread_attr_t attr;

            /* Init default parameters for all streams */
            EthFwEstDemo_initStreamParams(ctx);

            EthFwEstDemo_initTaskCtx(&attr, cfg);

            status = EthFwEstDemo_setInputParam(ctx, stParams);

            if (status == ETHFW_SOK)
            {
                EthFwEstDemo_initTalker(&ctx->talker);
                status = CB_THREAD_CREATE(&talker->hTaskHandle,
                                       &attr, EthFwEstDemo_talkerHandler,
                                       ctx);
            }
            else
            {
                talker->enable = BFALSE;
                appLogPrintf("User request to terminate the talker\n");
            }
        }
        if (status != ETHFW_SOK)
        {
            appLogPrintf("Failed to start the talker task!\n");
            CB_SEM_DESTROY(&talker->terminatedSem);
            talker->enable = BFALSE;
        }
        else
        {
            appLogPrintf("Start the talker successfully!\n");
        }
    }
    else
    {
        appLogPrintf("The talker has been started, no need to start again!\n");
    }
}

static void EthFwEstDemo_stopTalker(EstDemoAppCtx *ctx)
{
    if (ctx->talker.enable)
    {
        ctx->talker.enable = BFALSE;
        CB_SEM_WAIT(&ctx->talker.terminatedSem);
        CB_THREAD_JOIN(ctx->talker.hTaskHandle, NULL);

        ctx->talker.hTaskHandle = NULL;
        CB_SEM_DESTROY(&ctx->talker.terminatedSem);
        appLogPrintf("Terminate the talker succesfully!\n");
    }
}

static void EthFwEstDemo_stopListener(EstDemoAppCtx *ctx)
{
    if (ctx->listener.enable)
    {
        ctx->listener.enable = BFALSE;
        CB_SEM_WAIT(&ctx->listener.terminatedSem);
        ctx->listener.hTaskHandle = NULL;
        CB_SEM_DESTROY(&ctx->listener.terminatedSem);
        CB_SEM_DESTROY(&ctx->listener.rxPacketSem);
    }
}

static inline  void EthFwEstDemo_showRecvBitrate(EstDemoStreamParams *stream,
                                                 EstDemoPacket *pktinfo)
{
    int64_t now = EthFwEstDemo_getCurrentTimeUs();
    uint64_t duration  = now > stream->prevTs? now - stream->prevTs: 0ULL;
    uint32_t n;

    if (duration >= DISPLAY_BITRATE_INTERVAL_SEC*UB_SEC_US)
    {
        char buffer[MAX_LOG_LEN];
        n = snprintf(buffer, sizeof(buffer),
                     "PacketPriority[%d], frameLength: %dB, receivedBitrate: %lluKbps, nBrokenPackets: %d",
                     pktinfo->recvAddr.tcid, pktinfo->bufferSize, (stream->rxBytes*8*1000)/duration,
                     stream->nBrokenPkt);
        DebugP_assert(n < sizeof(buffer));
        appLogPrintf("%s\n", buffer);

        stream->prevTs = now;
        stream->rxBytes = 0ULL;
        stream->nBrokenPkt = 0U;
    }
}

static void EthFwEstDemo_handleRxPacket(EstDemoAppCtx *ctx,
                                      uint8_t *buffer, uint32_t size,
                                      CB_SOCKADDR_LL_T *addr)
{
    EstDemoTaskCtx *listener = &ctx->listener;
    EthVlanFrame *txFrame = (EthVlanFrame *)buffer;
    uint32_t tci = Enet_ntohs(txFrame->hdr.tci);
    uint8_t pcp = (tci >> ESTDEMO_VLAN_PCP_OFFSET)&ESTDEMO_VLAN_PCP_MASK;
    if (pcp >= ESTDEMO_MAX_STREAMS)
    {
        appLogPrintf("Received packets with unexpected PCP: %d\n", pcp);
    }
    else
    {
        EstDemoCommonStreamHdr *avtphdr;
        uint64_t timestamp = 0U;
        avtphdr = (EstDemoCommonStreamHdr *)txFrame->payload;
        uint8_t expectedSeqn = listener->streams[pcp].seqn+1;
        if (expectedSeqn != avtphdr->hh.seqn)
        {
            listener->streams[pcp].nBrokenPkt++;
        }
        listener->streams[pcp].seqn = avtphdr->hh.seqn;
        listener->streams[pcp].rxBytes += size;
        timestamp = gptpmasterclock_expand_timestamp(Enet_ntohl(avtphdr->headerTimestamp));
        listener->streams[pcp].txts = timestamp;
        listener->streams[pcp].tc = pcp;

        EstDemoPacket pktinfo;
        pktinfo.buffer = buffer;
        pktinfo.bufferSize = size;
        memcpy(&pktinfo.recvAddr, addr, sizeof(CB_SOCKADDR_LL_T));
        pktinfo.recvAddr.tcid = pcp;

        EthFwEstDemo_showRecvBitrate(&listener->streams[pcp], &pktinfo);
        listener->streams[pcp].rxts = addr->rxts;

        ctx->packetHandlerCb(ctx, &pktinfo);
    }
}

static void *EthFwEstDemo_listenerHandler(void *arg)
{
    int32_t status;
    EstDemoAppCtx *ctx = (EstDemoAppCtx *)arg;
    EstDemoTaskCtx *listener = &ctx->listener;
    CB_SOCKADDR_LL_T addr;

    status = cb_reg_multicast_address(ctx->est_sock, ctx->netdev[ctx->ifidx],
                                   MCAST_MAC_ADDR, 0U);
    DebugP_assert(status == ETHFW_SOK);
    while (listener->enable)
    {
        status = cb_lld_recv(ctx->est_sock, listener->buffer,
                          sizeof(listener->buffer),
                          &addr, sizeof(CB_SOCKADDR_LL_T));
        if (status <= ETHFW_SOK)
        {
            TaskP_yield();
            status = CB_SEM_WAIT(&listener->rxPacketSem);
            if (status != ETHFW_SOK)
            {
                appLogPrintf("%s, Failed to wait rxPacketSem\n", __func__);
                break;
            }
        }
        else if (status == 0xFFFF)
        {
            /* When it return 0xFFFF, this packet belongs to another app,
             * this application just ignore the packet
             */
            appLogPrintf("Received an unexpected packet!\n");
        }
        else
        {
            EthFwEstDemo_handleRxPacket(ctx, listener->buffer,
                                        status, &addr);
        }
    }

    appLogPrintf("The listener thread is terminating\n");

    return NULL;
}

void EthFwEstDemo_startCfgListener(EstDemoAppCtx *ctx,
                                   EstDemoTaskCfg *cfg)
{
    int32_t status = ETHFW_SOK;
    EstDemoTaskCtx *listener = &ctx->listener;

    listener->enable = BTRUE;
    status = CB_SEM_INIT(&listener->rxPacketSem, 0U, 0U);
    DebugP_assert(status == ETHFW_SOK);
    status = CB_SEM_INIT(&listener->terminatedSem, 0U, 0U);
    DebugP_assert(status == ETHFW_SOK);
    cb_tsn_thread_attr_t attr;
    EthFwEstDemo_initTaskCtx(&attr, cfg);
    status = CB_THREAD_CREATE(&listener->hTaskHandle,
                           &attr, EthFwEstDemo_listenerHandler,
                           ctx);
    if (status)
    {
        ctx->listener.enable = BFALSE;
        ctx->listener.hTaskHandle = NULL;
        CB_SEM_DESTROY(&ctx->listener.terminatedSem);
        CB_SEM_DESTROY(&ctx->listener.rxPacketSem);
        appLogPrintf("Failed to create listener task!\n");
    }
    else
    {
        appLogPrintf("Create listener task succesfully!\n");
    }
}

void EthFwEstDemo_printStats(EstDemoAppCtx *ctx)
{
    cb_tilld_port_stats_t stats;
    int32_t status;
    uint32_t port, i;

    status = cb_lld_get_port_stats(ctx->est_sock, 0xFFU, &stats);
    if (status)
    {
        appLogPrintf("Failed to get host port stats: %d\n", status);
    }
    else
    {
        appLogPrintf("----------------Host port stats info --------------------\n");
        EnetAppUtils_printHostPortStats9G((CpswStats_HostPort_Ng *)&stats);

        for (i = 0U; i < ctx->netdevSize; i++)
        {
            port = cb_lld_netdev_to_macport(ctx->netdev[i]);
            if (port < 0U)
            {
                appLogPrintf("Invalid mac port translated from netdev %s\n",
                       ctx->netdev[i]);
            }
            else
            {
                memset(&stats, 0, sizeof(cb_tilld_port_stats_t));

                status = cb_lld_get_port_stats(ctx->est_sock, port, &stats);
                if (status == ETHFW_SOK)
                {
                    appLogPrintf("---------MAC port[%d] stats info -----------\n", i);
                    EnetAppUtils_printMacPortStats9G((CpswStats_MacPort_Ng *)&stats);
                }
                else
                {
                    appLogPrintf("Failed to get MAC port stats info\n");
                }
            }
        }
    }
}

void EthFwEstDemo_resetStats(EstDemoAppCtx *ctx)
{
    int32_t i, port;
    cb_lld_reset_port_stats(ctx->est_sock, 0xFFU);
    for (i = 0U; i < ctx->netdevSize; i++)
    {
        port = cb_lld_netdev_to_macport(ctx->netdev[i]);
        cb_lld_reset_port_stats(ctx->est_sock, port);
    }
}
