/*
 * DeviceSpecific_C6614.c
 *
 * Configuration support functions provided for the specific device.
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
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
#if _MIPI_STM
#include "StmLibrary.h"
#endif
#include "DeviceSpecific_am437x.h"

#ifdef _MIPI_STM
void Config_STMData_Routing()
{
    /* Unlock Trace Funnel */
    *((volatile uint32_t*)AM437x_TF_LOCK_REG) = TF_UNLOCK_VALUE;

    /* Set Trace Funnel for STM input*/
    *((volatile uint32_t*)AM437x_TF_CNTL_REG) = TF_STM_ENABLE_VALUE;

	return;
}

eSTM_STATUS Config_STM_for_ETB(STMHandle* pSTMHandle)
{
	eSTM_STATUS retval;
	STM_MIPI_ConfigObj MIPI_Config;

	 // Default enable STM SW masters - MPUSS (0x4), PRU0(0x30), PRU1(0x34) and Wake-up M3(0x50)
	MIPI_Config.SW_MasterMapping =   0x50343004;
	MIPI_Config.SW_MasterMask = 0x03030303;
    // Default enable HW masters for Statistics Collectors
	MIPI_Config.HW_MasterMapping = 0xFCF8F4F0;

	retval = STMXport_config_MIPI(pSTMHandle, &MIPI_Config);

	return retval;

}
#endif

void etm_config_for_etb(void)
{
    /* Unlock and set Trace Funnel for ETM input */
    *((volatile uint32_t*)AM437x_TF_LOCK_REG) = TF_UNLOCK_VALUE;
    *((volatile uint32_t*)AM437x_TF_CNTL_REG) = TF_ETM_ENABLE_VALUE;
}



