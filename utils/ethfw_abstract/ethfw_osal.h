/*
 *  Copyright (c) Texas Instruments Incorporated 2024
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
 * \file  ethfw_osal.h
 *
 * \brief Ethernet Firmware OSAL interface.
 */

#ifndef ETHFW_OSAL_H_
#define ETHFW_OSAL_H_
/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <ethfw_al.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                                 Macros                                     */
/* ========================================================================== */

/** Timer will be started by the user */
#define ETHFWCLOCK_STARTMODE_USER               ((uint8_t) 0U)
/** Timer starts automatically after create or scheduler start */
#define ETHFWCLOCK_STARTMODE_AUTO               ((uint8_t) 1U)
/** Timer runs for a single period values and stops */
#define ETHFWCLOCK_RUNMODE_ONESHOT              ((uint8_t) 0U)
/** Timer is periodic and runs continuously */
#define ETHFWCLOCK_RUNMODE_CONTINUOUS           ((uint8_t) 1U)

/* EventP_wait will return when ANY of the bits set in mask are set in the Event bits */
#define ETHFWEVENT_WAITMODE_ANY                 ((uint8_t) 0U)
/* EventP_wait will return when ALL the bits set in mask are set in the Event bits */
#define ETHFWEVENT_WAITMODE_ALL                 ((uint8_t) 1U)

/* ========================================================================== */
/*                         Structures and Enums                               */
/* ========================================================================== */

/*!
 * \brief Task configuration params
 *
 * common task config params for Jacinto and Sitara
 */
typedef struct EthFwOsal_TaskParams_s
{
    /*! Name of the task instance */
    const char *name;
    /*! The priority of the task */
    int8_t priority;
    /*! pointer to stack memory, shall be non-null value */
    void *stack;
    /*! The stack size of the task */
    uint32_t stacksize;
    /*! argument 0 */
    void *arg0;
    /*! [SafeRTOS, FreeRTOS only] Pointer to user-defined data */
    void *userData;
} EthFwOsal_TaskParams;

/*!
 * \brief Clock configuration params
 *
 * common clock config params for Jacinto and Sitara
 */
typedef struct EthFwOsal_ClockParams_s
{
    /*! Timer Start Mode */
    uint8_t  startMode;
    /*! The clock period in units of clock ticks */
    uint32_t period;
    /*! Timer Run Mode */
    uint8_t  runMode;
    /*! User argument that is available inside the callback */
    void *arg;
} EthFwOsal_ClockParams;

/*!
 * \brief Event configuration params
 *
 * Event config params for Jacinto, don't care for Sitara
 */
typedef struct EthFwOsal_EventParams_s
{
    /*! Name of the task instance.*/
    void *instance;
} EthFwOsal_EventParams;

/*!
 * \brief Mailbox params for Jacinto and Sitara
 */
typedef MailboxP_Params EthFwOsal_MailboxParams;

/* ========================================================================== */
/*                         Global Variables Declarations                      */
/* ========================================================================== */

typedef void *EthFwOsal_TaskHandle;

typedef void *EthFwOsal_SemHandle;

typedef void *EthFwOsal_ClockHandle;

typedef void *EthFwOsal_EventHandle;

typedef void *EthFwOsal_MutexHandle;

typedef void *EthFwOsal_MailboxHandle;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

void EthFwOsal_init(void);

/* -----------------------TaskP APIs---------------------------------------*/

void EthFwOsal_initTaskParams(EthFwOsal_TaskParams *params);

EthFwOsal_TaskHandle EthFwOsal_createTask(void (*func)(void*), EthFwOsal_TaskParams *params);

int32_t EthFwOsal_deleteTask(EthFwOsal_TaskHandle *taskHandle);

void EthFwOsal_exitTask(void);

uint32_t EthFwOsal_isTaskTerminated(EthFwOsal_TaskHandle taskHandle);

EthFwOsal_TaskHandle EthFwOsal_getTaskSelf(void);

void EthFwOsal_setTaskPrio(EthFwOsal_TaskHandle taskHandle, uint32_t priority);

void EthFwOsal_sleepTaskinMsecs(uint32_t timeoutInMsecs);

uint64_t EthFwOsal_getTimeInUsecs(void);

void EthFwOsal_sleepTask(uint32_t timeout);

void EthFwOsal_yieldTask(void);

/* -----------------------CacheP APIs---------------------------------------*/

void EthFwOsal_wbCache(const void *addr, uint32_t size);

void EthFwOsal_invCache(const void *addr, uint32_t size);

void EthFwOsal_wbInvCache(const void *addr, uint32_t size);

/* -----------------------SemaphoreP APIs------------------------------------*/

EthFwOsal_SemHandle EthFwOsal_createSemaphore(uint32_t count);

int32_t EthFwOsal_deleteSemaphore(EthFwOsal_SemHandle handle);

int32_t EthFwOsal_pendSemaphore(EthFwOsal_SemHandle handle, uint32_t timeout);

int32_t EthFwOsal_postSemaphore(EthFwOsal_SemHandle handle);

/* -----------------------MutexP APIs------------------------------------*/

EthFwOsal_MutexHandle EthFwOsal_createMutex(void);

void EthFwOsal_deleteMutex(void *EthFwOsal_MutexHandle);

void EthFwOsal_lockMutex(void *EthFwOsal_MutexHandle);

void EthFwOsal_unlockMutex(void *EthFwOsal_MutexHandle);

/* -----------------------ClockP APIs------------------------------------*/

void EthFwOsal_initClockParams(EthFwOsal_ClockParams *params);

EthFwOsal_ClockHandle EthFwOsal_createClock(void (*func)(void*), EthFwOsal_ClockParams *params);

int32_t EthFwOsal_startClock(EthFwOsal_ClockHandle handle);

int32_t EthFwOsal_stopClock(EthFwOsal_ClockHandle handle);

int32_t EthFwOsal_deleteClock(EthFwOsal_ClockHandle handle);

/* -----------------------EventP APIs------------------------------------*/

void EthFwOsal_initEventParams(EthFwOsal_EventParams *params);

EthFwOsal_EventHandle EthFwOsal_createEvent(EthFwOsal_EventParams *params);

uint32_t EthFwOsal_waitEvent(EthFwOsal_EventHandle handle,
                            uint32_t eventMask,
                            uint8_t waitMode,
                            uint32_t timeout);

int32_t EthFwOsal_postEvent(EthFwOsal_EventHandle handle, uint32_t eventBits);

/* ----------------------MailboxP APIs------------------------------------*/

void EthFwOsal_initMailboxParams(EthFwOsal_MailboxParams *params);

EthFwOsal_MailboxHandle EthFwOsal_createMailbox(EthFwOsal_MailboxParams *params);

int32_t EthFwOsal_deleteMailbox(EthFwOsal_MailboxHandle handle);

int32_t EthFwOsal_postMailbox(MailboxP_Handle handle,
                              void *msg,
                              uint32_t timeout);

int32_t EthFwOsal_pendMailbox(MailboxP_Handle handle,
                              void *msg,
                              uint32_t timeout);


#ifdef __cplusplus
}
#endif

#endif /* ETHFW_OSAL_H_ */
