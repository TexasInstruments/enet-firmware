/*
 *  Copyright (c) Texas Instruments Incorporated 2020
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*!
 * \file     app_profile.c
 *
 * \brief    This file has application profiling helper functions.
 */

 /* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Types.h>
#include <xdc/runtime/Timestamp.h>


/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/utils/Load.h>

#include <ti/sysbios/hal/Seconds.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/hal/Cache.h>


/* OSAL Header files */
#include <ti/osal/osal.h>

#include <ti/csl/arch/r5/csl_arm_r5.h>
#include <ti/csl/arch/r5/csl_arm_r5_pmu.h>

/* This module's Header files */
#include <utils/profile/include/app_profile.h>


#define PMU_EVENT_COUNTER_1 (CSL_ARM_R5_PMU_EVENT_TYPE_CYCLE_CNT)
#define PMU_EVENT_COUNTER_2 (CSL_ARM_R5_PMU_EVENT_TYPE_NONCACHEABLE_ACCESS)
#define PMU_EVENT_COUNTER_3 (CSL_ARM_R5_PMU_EVENT_TYPE_ICACHE_STALL)

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define UTILS_ARRAYSIZE(x) sizeof(x)/sizeof (x[0U])
#define UTILS_ALIGN(x,align)  ((((x) + ((align) - 1))/(align)) * (align))



#define THRLOAD_LOAD_TABLE_LENGTH         100

#define APP_PROFILE_START_EVENT_ID       (Event_Id_00)
#define APP_PROFILE_STOP_EVENT_ID        (Event_Id_01)
#define APP_PROFILE_DEINIT_EVENT_ID      (Event_Id_02)

#define APP_PROFILE_TSK_STACK_SIZE       (4 * 1024)
#define APP_PROFILE_PRINT_DURATION       (30000)
#define APP_PROFILE_WINDOW_MS            (500)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */
typedef struct loadEntry {
  char      *tskName;
  Load_Stat load_stat;
}loadEntry;

typedef struct fxnProfile_s
{
    uint32_t startTime[3];
    uint32_t deltaTotalTime[3];
    uint32_t count;
    uint32_t key;
}fxnProfile;

typedef struct loadEntryRecord_s {
      loadEntry            tsk[THRLOAD_MAX_NUM_TASKS];
      Load_Stat            isr;
      Load_Stat            swi;
      uint32_t             cpuLoad;
      uint32_t             activeTaskCount;
      uint32_t             totalTaskCount;
      Types_Timestamp64    windowTs64;
      uint32_t             pmuCounters[3];
      uint32_t             packetCount;
      fxnProfile           fxn[APP_MAX_PROFILE_FXNS];
} loadEntryRecord;


typedef struct loadEntryTbl {
  appProfileConfig     profileConfig;
  Event_Struct         profileControlEvt;
  Event_Handle         hProfileControlEvt;
  Task_Handle          taskProfile;
  uint32_t             loadEntryIdx;
  loadEntryRecord      record[THRLOAD_LOAD_TABLE_LENGTH];
} loadEntryTbl;


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
#pragma DATA_SECTION(apploadEntryTbl,".benchmark_buffer")
loadEntryTbl apploadEntryTbl = 
{.hProfileControlEvt = NULL};
volatile uint32_t appLoadDebugFlag = 1;


#pragma DATA_SECTION(FXNPROFILE,".bss:appStack")
fxnProfile FXNPROFILE[APP_MAX_PROFILE_FXNS];



/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */
static void app_loadInit(void)
{

    apploadEntryTbl.loadEntryIdx = 0;
    //memset (&apploadEntryTbl.record[0], 0, sizeof (apploadEntryTbl.record));
    memset((void *)&FXNPROFILE[0],0,sizeof(FXNPROFILE));
}

static void app_GetTaskLoad(Task_Handle curTask,loadEntry *pLocalLoadEntry,uint32_t *pActiveTaskCnt)
{
    Bool retVal;

    if (NULL != curTask)
    {
        retVal = Load_getTaskLoad(curTask, &pLocalLoadEntry->load_stat);
        //Assert_isTrue((TRUE == retVal), Assert_E_assertFailed);
        if ((retVal == TRUE) && (pLocalLoadEntry->load_stat.threadTime != 0) /* && (strcmp(Task_Handle_name(curTask),"Forwarding_Task") == 0) */)
        {
            if (*pActiveTaskCnt < THRLOAD_MAX_NUM_TASKS)
            {
                //Assert_isTrue((TRUE == retVal), Assert_E_assertFailed);
                pLocalLoadEntry->tskName =  Task_Handle_name(curTask),
                *pActiveTaskCnt =  *pActiveTaskCnt + 1;
            }
        }
    }
}


static void app_loadSetPmuCounters(struct loadEntryRecord_s * curRecord)
{

    curRecord->pmuCounters[0] = CSL_armR5PmuReadCntr(0);
    curRecord->pmuCounters[1] = CSL_armR5PmuReadCntr(1);
    curRecord->pmuCounters[2] = CSL_armR5PmuReadCntr(2);
    if (apploadEntryTbl.profileConfig.configPmu)
    {
        CSL_armR5PmuResetCntrs();
    }
}


#pragma CODE_SECTION(app_fxnEntry,".text_boot")
void app_fxnEntry(uint32_t profileIndex)
{
    FXNPROFILE[profileIndex].key = Hwi_disable();
    FXNPROFILE[profileIndex].startTime[0]  = CSL_armR5PmuReadCntr(0);
    FXNPROFILE[profileIndex].startTime[1]  = CSL_armR5PmuReadCntr(1);
    FXNPROFILE[profileIndex].startTime[2]  = CSL_armR5PmuReadCntr(2);
}

#pragma CODE_SECTION(app_fxnExit,".text_boot")
void app_fxnExit(uint32_t profileIndex)
{
    FXNPROFILE[profileIndex].deltaTotalTime[0]  += CSL_armR5PmuReadCntr(0)  - FXNPROFILE[profileIndex].startTime[0] ;
    FXNPROFILE[profileIndex].deltaTotalTime[1]  += CSL_armR5PmuReadCntr(1)  - FXNPROFILE[profileIndex].startTime[1] ;
    FXNPROFILE[profileIndex].deltaTotalTime[2]  += CSL_armR5PmuReadCntr(2)  - FXNPROFILE[profileIndex].startTime[2] ;
    FXNPROFILE[profileIndex].count++;
    Hwi_restore(FXNPROFILE[profileIndex].key);
}

void app_loadSetFxnCounters(struct loadEntryRecord_s * curRecord)
{
    memcpy((void *)&curRecord->fxn[0],(void *)&FXNPROFILE[0],sizeof(curRecord->fxn));
    memset((void *)&FXNPROFILE[0],0,sizeof(FXNPROFILE));
}




void app_loadUpdateCb(void)
{
    Task_Handle curTask = NULL, nextTask = NULL;
    uint32_t nTaskCnt,activeTaskCnt;
    uint32_t staticTaskIndex;
    loadEntry *pLocalLoadEntry;
    struct loadEntryRecord_s *curRecord;

    nTaskCnt = 0;
    activeTaskCnt = 0;

    if (apploadEntryTbl.loadEntryIdx >= THRLOAD_LOAD_TABLE_LENGTH) {
        apploadEntryTbl.loadEntryIdx = 0;
    }

    curRecord = &apploadEntryTbl.record[apploadEntryTbl.loadEntryIdx];
    Timestamp_get64(&curRecord->windowTs64);
    for (staticTaskIndex = 0;staticTaskIndex < Task_Object_count();staticTaskIndex++)
    {
        pLocalLoadEntry = &(curRecord->tsk[activeTaskCnt]);
        curTask = Task_Object_get(NULL,staticTaskIndex);
        if (NULL != curTask)
        {
            app_GetTaskLoad(curTask,pLocalLoadEntry,&activeTaskCnt);
            nTaskCnt++;
        }
    }

    nextTask = Task_Object_first();
    do {
        pLocalLoadEntry = &(curRecord->tsk[activeTaskCnt]);
        curTask = nextTask;
        if (NULL != curTask) {
            app_GetTaskLoad(curTask,pLocalLoadEntry,&activeTaskCnt);
            
            nTaskCnt++;
            nextTask = Task_Object_next(curTask);
        }
    } while (nextTask != NULL);
    Load_getGlobalHwiLoad (&curRecord->isr);
    Load_getGlobalSwiLoad (&curRecord->swi);
    curRecord->cpuLoad = Load_getCPULoad();
    curRecord->activeTaskCount = activeTaskCnt;
    curRecord->totalTaskCount = nTaskCnt;
    curRecord->packetCount = apploadEntryTbl.profileConfig.getPacketCountFxn();
    app_loadSetPmuCounters(curRecord);
    app_loadSetFxnCounters(curRecord);

    apploadEntryTbl.loadEntryIdx++;
}

static void loadPrintLoadStats(Load_Stat *stats)
{
    
    apploadEntryTbl.profileConfig.printFxn("%u,",
                Load_calculateLoad(stats));

}

static void loadPrintFxnProfile(fxnProfile *fxn)
{
    apploadEntryTbl.profileConfig.printFxn("::,%u,%u,%u,%u,",
                fxn->count,fxn->deltaTotalTime[0],fxn->deltaTotalTime[1],fxn->deltaTotalTime[2]);
}


/* Function to print Load info */
static void loadPrint (void)
{
  uint32_t windowIdx,taskIdx;
  struct loadEntryRecord_s *curRecord;

  /* Profile information */
  apploadEntryTbl.profileConfig.printFxn ("Num profile Windows, %d \n",
                        apploadEntryTbl.loadEntryIdx);
  apploadEntryTbl.profileConfig.printFxn ("NUMTSK,ACTIVETSK,"
               "CPU,"
               "HWI Load,"
               "SWI Load,"
               "TSK name,TSK Load,");
  apploadEntryTbl.profileConfig.printFxn ("PMU1,PMU2,PMU3,");
  apploadEntryTbl.profileConfig.printFxn ("PKT,");
  apploadEntryTbl.profileConfig.printFxn ("FXN1_COUNT,FXN1_TIM0,FXN1_TIM1,FXN1_TIM2,");
  apploadEntryTbl.profileConfig.printFxn("\n");

  for (windowIdx = 0; windowIdx < apploadEntryTbl.loadEntryIdx; windowIdx++)
  {
    curRecord = &apploadEntryTbl.record[windowIdx];
    apploadEntryTbl.profileConfig.printFxn("%u,",
                windowIdx);
    apploadEntryTbl.profileConfig.printFxn("%u,",
                curRecord->totalTaskCount);
    apploadEntryTbl.profileConfig.printFxn("%u,",
                curRecord->activeTaskCount);
    apploadEntryTbl.profileConfig.printFxn("%u,",
                curRecord->cpuLoad);
    loadPrintLoadStats(&curRecord->isr);
    loadPrintLoadStats(&curRecord->swi);
    for (taskIdx = 0;taskIdx < curRecord->activeTaskCount; taskIdx++)
    {
      apploadEntryTbl.profileConfig.printFxn("%s,",curRecord->tsk[taskIdx].tskName);
      loadPrintLoadStats(&curRecord->tsk[taskIdx].load_stat);
    }
    apploadEntryTbl.profileConfig.printFxn("%u,%u,%u,",curRecord->pmuCounters[0],curRecord->pmuCounters[1],curRecord->pmuCounters[2]);
    apploadEntryTbl.profileConfig.printFxn("%u,",curRecord->packetCount);
    loadPrintFxnProfile(&curRecord->fxn[0]);
    loadPrintFxnProfile(&curRecord->fxn[1]);
    apploadEntryTbl.profileConfig.printFxn ("\n");
  }
  return;
}

void app_profileStart(void)
{
    if (apploadEntryTbl.hProfileControlEvt)
    {
        Types_Timestamp64    windowTs64;
        
        Timestamp_get64(&windowTs64);
        apploadEntryTbl.profileConfig.printFxn ("\n\nStartTime: Hi:Lo::%u:%u\n\n",windowTs64.hi,windowTs64.lo);
        Event_post(apploadEntryTbl.hProfileControlEvt,APP_PROFILE_START_EVENT_ID);
    }
    else
    {
        while (appLoadDebugFlag);
    }
}

void app_profileStop()
{
    if (apploadEntryTbl.hProfileControlEvt)
    {
        Types_Timestamp64    windowTs64;
        
        Timestamp_get64(&windowTs64);
        apploadEntryTbl.profileConfig.printFxn ("\n\nStopTime: Hi:Lo::%u:%u\n\n",windowTs64.hi,windowTs64.lo);
        
        Event_post(apploadEntryTbl.hProfileControlEvt,APP_PROFILE_STOP_EVENT_ID);
    }
    else
    {
        while (appLoadDebugFlag);
    }    
}


int config_pmu(void)
{

    CSL_armR5PmuCfg(0, 0 ,1);
    CSL_armR5PmuEnableAllCntrs(1);
    int num_cnt = CSL_armR5PmuGetNumCntrs();

    CSL_armR5PmuCfgCntr(0, PMU_EVENT_COUNTER_1);
    CSL_armR5PmuCfgCntr(1, PMU_EVENT_COUNTER_2);
    CSL_armR5PmuCfgCntr(2, PMU_EVENT_COUNTER_3);

    CSL_armR5PmuEnableCntrOverflowIntr(0, 0);
    CSL_armR5PmuEnableCntrOverflowIntr(1, 0);
    CSL_armR5PmuEnableCntrOverflowIntr(2, 0);
    CSL_armR5PmuResetCntrs();
    CSL_armR5PmuEnableCntr(0, 1);
    CSL_armR5PmuEnableCntr(1, 1);
    CSL_armR5PmuEnableCntr(2, 1);
    return 0;
}


void app_profile(UArg a0, UArg a1)
{
    uint32_t profilePrintInterval = apploadEntryTbl.profileConfig.profilePrintPeriodMs;
    uint32_t profileWindowMs = (uint32_t) a0;
    Event_Params evtParams;
    UInt postedEvent;
    bool enableProfiling;
    uint32_t profileElapsedTime;
    bool exitTask;
    
    Event_Params_init(&evtParams);  // Initialize this config-params structure with supplier-specified defaults before instance creation
    Event_construct(&apploadEntryTbl.profileControlEvt,&evtParams);
    apploadEntryTbl.hProfileControlEvt = Event_handle(&apploadEntryTbl.profileControlEvt);

    enableProfiling = FALSE;
    profileElapsedTime = 0;
    if (apploadEntryTbl.profileConfig.configPmu == true)
    {
        config_pmu();
    }
    exitTask = false;
    while (exitTask != true)
    {
        postedEvent = 
                   Event_pend(apploadEntryTbl.hProfileControlEvt,
                   0,
                   (APP_PROFILE_START_EVENT_ID | APP_PROFILE_STOP_EVENT_ID | APP_PROFILE_DEINIT_EVENT_ID),
                   profileWindowMs);
        if (postedEvent)
        {
            switch(postedEvent)
            {
                case APP_PROFILE_START_EVENT_ID:
                    Load_reset();
                    app_loadInit();
                    enableProfiling = TRUE;
                    profileElapsedTime = 0;
                    break;
                case APP_PROFILE_STOP_EVENT_ID:
                    Load_reset();
                    loadPrint();
                    enableProfiling = FALSE;
                    profileElapsedTime = 0;
                    break;
                case APP_PROFILE_DEINIT_EVENT_ID:
                    Load_reset();
                    enableProfiling = FALSE;
                    profileElapsedTime = 0;
                    exitTask = true;
                    break;
                default:
                    /* No other event combo should be posted */
                    while (appLoadDebugFlag);
            }
        }
        if (enableProfiling)
        {
            Load_update();
        }
        if (enableProfiling)
        {
            profileElapsedTime += profileWindowMs;
            if ((apploadEntryTbl.profileConfig.enablePeriodicProfilePrint) && (profileElapsedTime >= profilePrintInterval))
            {
                Load_reset();
                loadPrint();
                app_loadInit();
                profileElapsedTime = 0;
            }            
        }
    }
}

void app_profileAccumulateFxnLoad(struct appProfileFxnLoad_s *avgFxnLoad, fxnProfile *fxnProfile, uint32_t *numActiveFxns)
{
    uint32_t i, j;

    *numActiveFxns = 0;
    for (i = 0; i < APP_MAX_PROFILE_FXNS; i++)
    {
        for (j = 0; j < UTILS_ARRAYSIZE(avgFxnLoad->deltaTime); j++)
        {
            if (fxnProfile[i].count)
            {
                avgFxnLoad[i].deltaTime[j] += fxnProfile[i].deltaTotalTime[j] / fxnProfile[i].count;
                *numActiveFxns = *numActiveFxns + 1;
            }
        }
    }

}

void app_profileGetAvgFxnLoad(struct appProfileFxnLoad_s *avgFxnLoad, uint32_t numWindows)
{
    uint32_t i, j;

    for (i = 0; i < APP_MAX_PROFILE_FXNS; i++)
    {
        for (j = 0; j < UTILS_ARRAYSIZE(avgFxnLoad->deltaTime); j++)
        {
            avgFxnLoad[i].deltaTime[j] /= numWindows;
        }
    }
}

void app_profileRegisterTaskLoad(struct appProfileTskLoad_s * avgTaskLoad, loadEntry *curTaskRecord, uint32_t  *activeTaskCount, uint32_t maxTaskCount)
{
    uint32_t i;

    for (i = 0; i < *activeTaskCount; i++)
    {
        if (avgTaskLoad[i].tskName == curTaskRecord->tskName)
        {
            avgTaskLoad[i].load += Load_calculateLoad(&curTaskRecord->load_stat);
            avgTaskLoad[i].load /= 2;
            break;
        }
    }
    if (i == *activeTaskCount)
    {
        if ((Load_calculateLoad(&curTaskRecord->load_stat) > 0) && (*activeTaskCount < maxTaskCount))
        {
            avgTaskLoad[*activeTaskCount].tskName = curTaskRecord->tskName;
            avgTaskLoad[*activeTaskCount].load    = Load_calculateLoad(&curTaskRecord->load_stat);
            *activeTaskCount = *activeTaskCount + 1;
        }
    }
}


void app_profileGetAvgTaskLoad(appProfileAvgLoadInfo * avgProfileInfo, struct loadEntryRecord_s *curRecord)
{
    uint32_t i;
    
    for (i = 0; i < curRecord->activeTaskCount; i++)
    {
        app_profileRegisterTaskLoad(avgProfileInfo->tskLoad, &curRecord->tsk[i], &avgProfileInfo->activeTaskCount, UTILS_ARRAYSIZE(avgProfileInfo->tskLoad));
    }
}

void app_profileGetAvgLoad(uint32_t avgTimePeriod, appProfileAvgLoadInfo *avgProfileInfo)
{
    uint32_t numRecords = avgTimePeriod / APP_PROFILE_WINDOW_MS;
    uint32_t iterCount;
    struct loadEntryRecord_s *curRecord;
    int32_t curRecordIndex;

    iterCount = 0;
    if (numRecords >= UTILS_ARRAYSIZE(apploadEntryTbl.record))
    {
        numRecords = UTILS_ARRAYSIZE(apploadEntryTbl.record);
    }
    curRecordIndex = apploadEntryTbl.loadEntryIdx;
    curRecordIndex--;
    memset(avgProfileInfo, 0, sizeof(*avgProfileInfo));
    while (iterCount < numRecords)
    {
        if (curRecordIndex < 0)
        {
            curRecordIndex = UTILS_ARRAYSIZE(apploadEntryTbl.record) - 1;
        }
        curRecord = &apploadEntryTbl.record[curRecordIndex];
        avgProfileInfo->cpuLoad += curRecord->cpuLoad;
        avgProfileInfo->isr += Load_calculateLoad(&curRecord->isr);
        avgProfileInfo->swi += Load_calculateLoad(&curRecord->swi);
        avgProfileInfo->packetCount += curRecord->packetCount;
        if (avgProfileInfo->totalTaskCount < curRecord->totalTaskCount)
        {
            avgProfileInfo->totalTaskCount = curRecord->totalTaskCount;
        }
        app_profileGetAvgTaskLoad(avgProfileInfo, curRecord);
        iterCount++;
        curRecordIndex--;
    }
    avgProfileInfo->cpuLoad  /= numRecords;
    avgProfileInfo->isr /= numRecords;
    avgProfileInfo->swi /= numRecords;
    avgProfileInfo->packetCount /= numRecords;
}

void app_profileGetFxnLoad(uint32_t avgTimePeriod, appProfileFxnLoadInfo *fxnLoadInfo)
{
    uint32_t numRecords = avgTimePeriod / APP_PROFILE_WINDOW_MS;
    uint32_t iterCount;
    struct loadEntryRecord_s *curRecord;
    int32_t curRecordIndex;

    iterCount = 0;
    if (numRecords >= UTILS_ARRAYSIZE(apploadEntryTbl.record))
    {
        numRecords = UTILS_ARRAYSIZE(apploadEntryTbl.record);
    }
    curRecordIndex = apploadEntryTbl.loadEntryIdx;
    curRecordIndex--;
    memset(fxnLoadInfo, 0, sizeof(*fxnLoadInfo));
    while (iterCount < numRecords)
    {
        if (curRecordIndex < 0)
        {
            curRecordIndex = UTILS_ARRAYSIZE(apploadEntryTbl.record) - 1;
        }
        curRecord = &apploadEntryTbl.record[curRecordIndex];
        app_profileAccumulateFxnLoad(fxnLoadInfo->fxn, curRecord->fxn, &fxnLoadInfo->numFunctions);
        iterCount++;
        curRecordIndex--;
    }
    app_profileGetAvgFxnLoad(fxnLoadInfo->fxn, numRecords);
}


void app_profileInit(appProfileConfig *profileConfig)
{
    Task_Params tskParams;
    Error_Block ebOj;

    apploadEntryTbl.profileConfig = *profileConfig;
    Error_init(&ebOj);
    Task_Params_init(&tskParams);
    tskParams.arg0 = APP_PROFILE_WINDOW_MS;
    tskParams.priority = APP_PROFILE_TSK_PRI;
    tskParams.stackSize = APP_PROFILE_TSK_STACK_SIZE;
    tskParams.instance->name = "app_profile_task";

    apploadEntryTbl.taskProfile = Task_create(&app_profile, &tskParams, &ebOj);
    Assert_isTrue((Error_check(&ebOj) == FALSE), Assert_E_assertFailed);
}

void app_profileDeInit()
{
    Task_Handle hPktTask;

    hPktTask = apploadEntryTbl.taskProfile;
    Event_post(apploadEntryTbl.hProfileControlEvt,APP_PROFILE_DEINIT_EVENT_ID);
    while (Task_getMode(hPktTask) != Task_Mode_TERMINATED)
    {
        Task_sleep(1);
    }
    Task_delete(&apploadEntryTbl.taskProfile);


}


/* ========================================================================== */
/*                          Static Function Definitions                       */
/* ========================================================================== */
