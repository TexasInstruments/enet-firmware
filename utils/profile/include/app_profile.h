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
 * \file     app_profile.h
 *
 * \brief    This file contains the generic definitions and functions
 *           used for application profiling.
 */

#ifndef APP_PROFILE_H_
#define APP_PROFILE_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define THRLOAD_MAX_NUM_TASKS                (8)

#define APP_PROFILE_TSK_PRI                  (15)

#define APP_MAX_PROFILE_FXNS                 (5)

#define APP_PROFILE_FXN_ID_0                 (0)
#define APP_PROFILE_FXN_ID_1                 (1)
#define APP_PROFILE_FXN_ID_2                 (2)
#define APP_PROFILE_FXN_ID_3                 (3)
#define APP_PROFILE_FXN_ID_4                 (4)

#define APP_PROFILE_TSK_NAME_SIZE            (20U)


typedef void (* AppProfilePrintFxn)(const char *format, ...);
typedef uint32_t (* AppProfileGetPacketCountFxn)(void);

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct appProfileConfig_s
{
    bool enablePeriodicProfilePrint;
    uint32_t profilePrintPeriodMs;
    bool configPmu;
    AppProfilePrintFxn printFxn;
    AppProfileGetPacketCountFxn getPacketCountFxn;
} appProfileConfig;

typedef struct appProfileFxnLoad_s {
    uint32_t deltaTime[3];
    uint32_t count;
} appProfileFxnLoad;

typedef struct appProfileFxnLoadInfo_s {
    uint32_t numFunctions;
    appProfileFxnLoad fxn[APP_MAX_PROFILE_FXNS];
} appProfileFxnLoadInfo;


typedef struct appProfileAvgLoadInfo_s
{
    struct appProfileTskLoad_s {
        char      tskName[APP_PROFILE_TSK_NAME_SIZE];
        uint32_t  load;
    } tskLoad[THRLOAD_MAX_NUM_TASKS];
    uint32_t             isr;
    uint32_t             swi;
    uint32_t             cpuLoad;
    uint32_t             activeTaskCount;
    uint32_t             totalTaskCount;
    uint32_t             packetCount;
} appProfileAvgLoadInfo;


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

void app_profileInit(appProfileConfig *profileConfig);

void app_profileStart(void);

void app_profileGetAvgLoad(uint32_t avgTimePeriod, appProfileAvgLoadInfo *avgProfileInfo);

void app_profileGetFxnLoad(uint32_t avgTimePeriod, appProfileFxnLoadInfo *fxnLoadInfo);

/* ========================================================================== */
/*                       Static Function Definitions                          */
/* ========================================================================== */

/* None */

#ifdef __cplusplus
}
#endif

#endif /* APP_PROFILE_H__ */
