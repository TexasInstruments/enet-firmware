/*
 * DeviceSpecific_TDA3x.c
 *
 * Configuration support functions provided for the specific device.
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 * 
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
 *
*/
/*! \file DeviceSpecific.c
    \brief Support code for this specific device

*/

#ifdef _MIPI_STM
#include "StmLibrary.h"
#endif
#include "DeviceSpecific_TDA3x.h"
#include <stdio.h>


DSPSS_ClkEnable(uint32_t cpu_num)
{
    printf("Initialize the DSP_%d Sub System for EDMA\n", cpu_num);

    /* DSPSS Boot Address */
    WR_MEM_32(DSPSSBOOTADDR, (DSPSSBOOTADDRVALUE >> 10));

    /* Ware reset asserted for DSP_LRST, DSP Cache and Slave */
    WR_MEM_32(RM_DSP_RSTCTRL, 0x3);

    /* Start a SW force wakeup for DSPSS */
    WR_MEM_32(CM_DSP_CLKSTCTRL, 0x2);
    /* Enable DSPSS clock */
    WR_MEM_32(CM_DSP_DSP_CLKCTRL, 0x1);

    /* Check whether GFCLK is gated or not */
    while ((RD_MEM_32(CM_DSP_CLKSTCTRL) & 0x100) != 0x100);

    /* Reset de-assertion for DSPSS */
    WR_MEM_32(RM_DSP_RSTCTRL, 0x1);
    /* Check the reset state: DSPSS */
    while ((RD_MEM_32(RM_DSP_RSTST) & 0x2) != 0x2);

    WR_MEM_32(DSP_L2_SRAM_TARG, 0x0000A120); //Self branch loop for DSP

    /* Reset de-assertion for DSP CPUs */
    WR_MEM_32(RM_DSP_RSTCTRL, 0x0);
    /* Check the reset state: DSPSS Core, Cache and Slave interface */
    while ((RD_MEM_32(RM_DSP_RSTST) & 0x3) != 0x3);
    /* Check module mode */
    while ((RD_MEM_32(CM_DSP_DSP_CLKCTRL) & 0x30000) != 0x0);
}

void Config_Device()
{
    DSPSS_ClkEnable(DSP1_CORE_ID);
}

#ifdef _MIPI_STM
void Config_STMData_Routing()
{
    CSTF_DSS_UNLOCK_REG = CS_UNLOCK_VALUE;
    CSTF_CTRL_DSS_REG |= CSTF_CTRL_DSS_CTSTM;
}

eSTM_STATUS Config_STM_for_ETB(STMHandle* pSTMHandle)
{
	eSTM_STATUS retval = eSTM_SUCCESS;

    STM_MIPI_ConfigObj MIPI_Config;
    MIPI_Config.TraceBufSize = ETB_SIZE;

    /* Enable STM Masters DSP1_MDMA, DSP2_MDMA, EVE & IPU(M4s) */
    MIPI_Config.SW_MasterMapping = 0x60403420;
    MIPI_Config.SW_MasterMask = 0x03030303;
    /* Default enable HW masters for EVE, Statistic Probes, OCP Watchpoint, DMA Profiling */
    MIPI_Config.HW_MasterMapping = 0x68646050;

	retval = STMXport_config_MIPI(pSTMHandle, &MIPI_Config);

//	*(uint32_t *)0x54160280 = 0x4 << 4;

	return retval;

}
#endif


void Config_DSPData_Routing(uint8_t coreID)
{
    /* Configure Trace Funnel */
    CSTF_DSS_UNLOCK_REG = CS_UNLOCK_VALUE;

    /* Enable port 1 to pass DSP_1 trace data to ETB */
    /* Enable port 2 to pass DSP_2 trace data to ETB */
    /* Bits 8:11 - Minimum hold time (3 cycles) */
    /* Set the trace funnel up for the DSP the application is running on. */
    /* Note: If you are collecting trace data in the ETB for both DSP's
     * simultaneously, then modify this code to OR in the port enable bits.
     * With both DSP ports enabled, an ETB flush will not complete unless
     * both DSP DTF modules respond to the flush request.
     */
    if(coreID == 0)
        CSTF_CTRL_DSS_REG = 0x300 + CSTF_CTRL_DSS_DSP1;
    if(coreID == 1)
        CSTF_CTRL_DSS_REG = 0x300 + CSTF_CTRL_DSS_DSP2;
}
