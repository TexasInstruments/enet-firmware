/*
 *
 * Copyright (c) 2017 Texas Instruments Incorporated
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
#include <stdio.h>
#include <stdint.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <ti/osal/SemaphoreP.h>

#include <ti/drv/ipc/ipc.h>

#include <ethremotecfg/protocol/rpmsg-kdrv-transport-ethswitch.h>
#include <client-rtos/remote-device.h>
#include <ethremotecfg/client/include/ethremotecfg_client.h>

#include <apps/ipc_cfg/app_ipc_rsctable.h>

#define IPC_RPMESSAGE_OBJ_SIZE  256
#define VQ_BUF_SIZE             2048
#define REMOTE_DEVICE_ENDPT     26
#define RPMSG_DATA_SIZE         (256*512 + IPC_RPMESSAGE_OBJ_SIZE)
#define VRING_BASE_ADDRESS      0xBA000000
#define VRING_BUFFER_SIZE       0x02000000

static uint8_t g_monitorStackBuf[IPC_TASK_STACKSIZE]
    __attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
    ;

    static uint8_t g_rdevStackBuf[IPC_TASK_STACKSIZE]
    __attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
    ;

    static uint8_t g_ipcStackBuf[IPC_TASK_STACKSIZE]
    __attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
    ;

    static uint8_t g_vdevMonStackBuf[IPC_TASK_STACKSIZE]
    __attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
    ;

    static uint8_t g_mainStackBuf[IPC_TASK_STACKSIZE]
    __attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
    ;

    static uint8_t ctrlTaskBuf[IPC_TASK_STACKSIZE]
    __attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
    ;

    static uint8_t g_messageTaskStack[IPC_TASK_STACKSIZE]
    __attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
    ;

    static uint8_t g_requestTaskStack[IPC_TASK_STACKSIZE]
    __attribute__ ((section(".bss:taskStackSection")))
__attribute__ ((aligned(8192)))
    ;

    static uint8_t  sysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section ("ipc_data_buffer"), aligned (8)));
    static uint8_t  gCntrlBuf[RPMSG_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned (8)));

    static SemaphoreP_Handle g_ipc_init_wait_sem;
    static SemaphoreP_Handle g_rdev_start_sem;

    static uint32_t selfProcId = IPC_MCU2_1;
    static uint32_t gRemoteProc[] =
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
};
static uint32_t gNumRemoteProc = sizeof(gRemoteProc)/sizeof(uint32_t);

void appLogPrintf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    System_vprintf(format, args);
    va_end(args);
}

static void rpmsg_vdevMonitorFxn(UArg arg0, UArg arg1)
{
    int32_t status;

    /* Wait for Linux VDev ready... */
    while(!Ipc_isRemoteReady(IPC_MPU1_0))
    {
        Task_sleep(10);
    }

    /* Create the VRing now ... */
    status = Ipc_lateVirtioCreate(IPC_MPU1_0);
    if(status != IPC_SOK)
    {
        System_printf("%s: Ipc_lateVirtioCreate failed\n", __func__);
        return;
    }

    status = RPMessage_lateInit(IPC_MPU1_0);
    if(status != IPC_SOK)
    {
        System_printf("%s: RPMessage_lateInit failed\n", __func__);
        return;
    }
}

static void messageLoopFn(UArg a0, UArg a1)
{
    uint32_t cnt = 0;
    uint32_t device_id = (uint32_t)a0;
    char     msgText[512];

    while(TRUE) {
        snprintf(msgText, sizeof(msgText), "ping-message %d", cnt);
        System_printf("%s: sending message\n", __func__);
        rdevEthSwitchClient_sendText(device_id, msgText);
        Task_sleep(10);
        cnt++;
    }
}

static void requestLoopFn(UArg a0, UArg a1)
{
    uint32_t cnt = 0;
    char     msgText[512];
    uint8_t resp[512];
    uint32_t resp_len;
    uint32_t device_id = (uint32_t)a0;

    while(TRUE) {
        snprintf(msgText, sizeof(msgText), "ping-request %d", cnt);
        System_printf("%s: sending request\n", __func__);
        rdevEthSwitchClient_sendPing(device_id, msgText, resp, sizeof(resp), &resp_len);
        System_printf("%s: respose %s\n", __func__, resp);
        cnt++;
    }
}

static void startMessageAndRequestLoop(uint32_t device_id)
{
    Task_Params params;

    Task_Params_init(&params);
    params.priority = 3;
    params.stackSize = IPC_TASK_STACKSIZE;
    params.stack = &g_messageTaskStack[0];
    params.stackSize = IPC_TASK_STACKSIZE;
    params.arg0 = device_id;
    Task_create(messageLoopFn, &params, NULL);

    Task_Params_init(&params);
    params.priority = 3;
    params.stackSize = IPC_TASK_STACKSIZE;
    params.stack = &g_requestTaskStack[0];
    params.stackSize = IPC_TASK_STACKSIZE;
    params.arg0 = device_id;
    Task_create(requestLoopFn, &params, NULL);
}


static Void monitorAndUnlockRdev(UArg a0, UArg a1)
{
    int32_t ret = 0;
    rdevEthSwitchClientInitPrms_t prm;

    SemaphoreP_pend(g_ipc_init_wait_sem, SemaphoreP_WAIT_FOREVER);
    SemaphoreP_post(g_rdev_start_sem);


    sprintf(prm.device_name, ETHREMOTEDEVICE_DEVICE_NAME_MCU_2_1);
    prm.cbHandler = rdevEthSwitchClient_printText;
    while(TRUE) {
        ret = rdevEthSwitchClient_connect(&prm);
        if(ret != 0)
            System_printf("error in device query\n");

        if(ret != 0 || (ret == 0 && prm.device_id != APP_REMOTE_DEVICE_DEVICE_ID_EAGAIN))
            break;
        if (ret == 0 && (prm.device_id == APP_REMOTE_DEVICE_DEVICE_ID_EAGAIN))
        {
            Task_sleep(10);
        }
    }

    if(ret == 0) {
        System_printf("Registered a device name = %s, data = %s, id = %u, type = %u\n",
                "mcu2_0-ethswitch-0", prm.data, prm.device_id, prm.device_type);
    }

    startMessageAndRequestLoop(prm.device_id);

}

static Void ipc_init(UArg a0, UArg a1)
{
    Task_Params       params;
    uint32_t          numProc = gNumRemoteProc;
    Ipc_VirtIoParams  vqParam;

    /* Step1 : Initialize the multiproc */
    Ipc_mpSetConfig(selfProcId, numProc, &gRemoteProc[0]);

    System_printf("IPC_echo_test (core : %s) .....\r\n",
            Ipc_mpGetSelfName());


    Ipc_loadResourceTable(appGetIpcResourceTable());

    /* Step2 : Initialize Virtio */
    vqParam.vqObjBaseAddr = (void*)&sysVqBuf[0];
    vqParam.vqBufSize     = numProc * Ipc_getVqObjMemoryRequiredPerCore();
    vqParam.vringBaseAddr = (void*)VRING_BASE_ADDRESS;
    vqParam.vringBufSize  = VRING_BUFFER_SIZE;
    vqParam.timeoutCnt    = 100;  /* Wait for counts */
    Ipc_initVirtIO(&vqParam);

    /* Step 3: Initialize RPMessage */
    RPMessage_Params cntrlParam;

    /* Initialize the param */
    RPMessageParams_init(&cntrlParam);

    /* Set memory for HeapMemory for control task */
    cntrlParam.buf         = &gCntrlBuf[0];
    cntrlParam.bufSize     = RPMSG_DATA_SIZE;
    cntrlParam.stackBuffer = &ctrlTaskBuf[0];
    cntrlParam.stackSize   = IPC_TASK_STACKSIZE;
    RPMessage_init(&cntrlParam);

    SemaphoreP_post(g_ipc_init_wait_sem);

    Task_Params_init(&params);
    params.priority = 3;
    params.stackSize = IPC_TASK_STACKSIZE;
    params.stack = &g_vdevMonStackBuf[0];
    params.stackSize = IPC_TASK_STACKSIZE;
    Task_create(rpmsg_vdevMonitorFxn, &params, NULL);
}

static Void remotedev_init(UArg a0, UArg a1)
{
    app_remote_device_init_prm_t remote_dev_init_prm;

    appRemoteDeviceInitParamsInit(&remote_dev_init_prm);

    remote_dev_init_prm.rpmsg_buf_size = 256;
    remote_dev_init_prm.remote_endpt = REMOTE_DEVICE_ENDPT;
    remote_dev_init_prm.wait_sem = g_rdev_start_sem;
    remote_dev_init_prm.num_cores = 1;
    remote_dev_init_prm.cores[0] = IPC_MCU2_0;

    appRemoteDeviceInit(&remote_dev_init_prm);
    System_printf("Remote device (core : mcu2_1) .....\r\n");

}

static Void taskFxn(UArg a0, UArg a1)
{

    Task_Params ipc_taskParams;
    Task_Params rdev_taskParams;
    Task_Params monitor_taskParams;
    SemaphoreP_Params sem_params;

    SemaphoreP_Params_init(&sem_params);
    sem_params.mode = SemaphoreP_Mode_BINARY;
    g_ipc_init_wait_sem = SemaphoreP_create(0, &sem_params);

    SemaphoreP_Params_init(&sem_params);
    sem_params.mode = SemaphoreP_Mode_BINARY;
    g_rdev_start_sem = SemaphoreP_create(0, &sem_params);

    Task_Params_init(&ipc_taskParams);
    ipc_taskParams.priority = 2;
    ipc_taskParams.stack = &g_ipcStackBuf[0];
    ipc_taskParams.stackSize = IPC_TASK_STACKSIZE;
    Task_create(ipc_init, &ipc_taskParams, NULL);

    Task_Params_init(&rdev_taskParams);
    rdev_taskParams.priority = 2;
    rdev_taskParams.stack = &g_rdevStackBuf[0];
    rdev_taskParams.stackSize = IPC_TASK_STACKSIZE;
    Task_create(remotedev_init, &rdev_taskParams, NULL);

    Task_Params_init(&monitor_taskParams);
    monitor_taskParams.priority = 2;
    monitor_taskParams.stack = &g_monitorStackBuf[0];
    monitor_taskParams.stackSize = IPC_TASK_STACKSIZE;
    Task_create(monitorAndUnlockRdev, &monitor_taskParams, NULL);
}

int main(void)
{
    Task_Handle task;
    Task_Params taskParams;


    /* Initialize the task params */
    Task_Params_init(&taskParams);
    /* Set the task priority higher than the default priority (1) */
    taskParams.priority = 2;
    taskParams.stack = &g_mainStackBuf[0];
    taskParams.stackSize = IPC_TASK_STACKSIZE;

    task = Task_create(taskFxn, &taskParams, NULL);
    if(NULL == task)
    {
        BIOS_exit(0);
    }
    BIOS_start();    /* does not return */

    return(0);
}

