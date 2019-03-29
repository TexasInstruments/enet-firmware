/*
 *  Copyright (c) Texas Instruments Incorporated 2018
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
 * \file     main_tirtos.c
 *
 * \brief    Main file for TI-RTOS build.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <ti/drv/sciclient/sciclient.h>
#include <ti/drv/cpsw/cpsw.h>
#include <ti/drv/cpsw/examples/cpsw_apputils/inc/cpsw_apputils.h>

#include "app_switch.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* Test application stack size */
#define APP_TSK_STACK_MAIN                    (10U * 1024U)
#define NUM_LOOPBACK_ITERATION                (10)
/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* Task function */
static Void taskFxn(UArg a0, UArg a1);

/* CPSW loopback test */
extern Cpsw_Handle CpswApp_getCpswHandle(void);

static CpswApp_ClkHandle CpswApp_createClock(void);
static void CpswApp_delete();

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* Test application stack */
static uint8_t gAppTskStackMain[APP_TSK_STACK_MAIN]
                                __attribute__((aligned(32)));
static uint8_t gAppTskStackTick[APP_TSK_STACK_MAIN]
                                __attribute__((aligned(32)));
volatile uint32_t exitFlag = 0U;

static Clock_Handle hTimer;

static     Task_Handle task_periodicTick;
static     Semaphore_Handle timerSem;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int main(void)
{
    Task_Handle task;
    Task_Params params;
    Error_Block eb;

    Error_init(&eb);

    /* Initialize the task params. Set the task priority higher than the
     * default priority (1) */
    Task_Params_init(&params);
    params.priority  = 2U;
    params.stack     = gAppTskStackMain;
    params.stackSize = sizeof(gAppTskStackMain);
    task = Task_create(taskFxn, &params, &eb);
    if (task == NULL)
    {
        BIOS_exit(0);
    }

    /* Does not return */
    BIOS_start();

    return 0;
}

static Void taskFxn(UArg a0, UArg a1)
{
    CpswApp_ClkHandle clkhandle = CpswApp_createClock();
    uint8_t i;

    if(clkhandle != NULL)
    {
        for (i = 0; i < NUM_LOOPBACK_ITERATION; i++)
        {
            /* Run the loopback test */
            CpswApp_loopbackTest(clkhandle, i);
        }
    }
    CpswApp_delete();
}

Void CpswApp_timerCallback(UArg arg)
{
    Semaphore_Handle timerSem = (Semaphore_Handle)arg;

    /* Tick! */
    Semaphore_post(timerSem);
}

void CpswApp_periodicTick(UArg a0, UArg a1)
{
    Semaphore_Handle timerSem = (Semaphore_Handle)a0;

    while (!exitFlag)
    {
        Semaphore_pend(timerSem, BIOS_WAIT_FOREVER);
        Cpsw_periodicTick(CpswApp_getCpswHandle());
    }
}

static CpswApp_ClkHandle CpswApp_createClock(void)
{
    Task_Params params;
    Semaphore_Params semParams;
    Clock_Params clkParams;
    UInt period = 100U; /* msecs */
    Error_Block eb;

    Error_init(&eb);

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_COUNTING;
    timerSem = Semaphore_create(0, &semParams, NULL);

    /* Initialize the taskperiodicTick params. Set the task priority higher than the
     * default priority (1) */
    Task_Params_init(&params);
    params.priority  = 7U;
    params.stack     = gAppTskStackTick;
    params.stackSize = sizeof(gAppTskStackTick);
    params.arg0      = (UArg) timerSem;

    task_periodicTick = Task_create(CpswApp_periodicTick, &params, &eb);
    if (task_periodicTick == NULL)
    {
        BIOS_exit(0);
    }

    Clock_Params_init(&clkParams);
    clkParams.startFlag = FALSE;
    clkParams.period    = period;
    clkParams.arg       = (UArg) timerSem;

    /* Creating timer and setting timer callback function*/
    hTimer = Clock_create((Clock_FuncPtr) &CpswApp_timerCallback,
                          period,
                          &clkParams,
                          NULL);
    if (hTimer != NULL)
    {
        /* Set timer expiry time in OS ticks */
        Clock_setTimeout(hTimer, period);
        Clock_setPeriod(hTimer, period);

    }
    else
    {
        CpswAppUtils_print("CpswApp_createClock() failed to create clock: %d\n");
    }

    return (CpswApp_ClkHandle)hTimer;
}

void CpswApp_startClock(CpswApp_ClkHandle handle)
{
    Clock_Handle clkHandle = (Clock_Handle) handle;

    /* Start timer */
    Clock_start(clkHandle);

}

void CpswApp_stopClock(CpswApp_ClkHandle handle)
{
    Clock_Handle clkHandle = (Clock_Handle) handle;

    /* Stop and delete the tick timer */
    Clock_stop(clkHandle);
}

void CpswAppUtils_wait(uint32_t waitTime)
{
    Task_sleep(waitTime);
}
static void CpswApp_delete(void)
{
    if (hTimer!= NULL)
    {
        Clock_delete(&hTimer);
    }
    if (timerSem != NULL)
    {
        Semaphore_delete(&timerSem);
    }
    if (task_periodicTick != NULL)
    {
        Task_delete(&task_periodicTick);
    }
}
