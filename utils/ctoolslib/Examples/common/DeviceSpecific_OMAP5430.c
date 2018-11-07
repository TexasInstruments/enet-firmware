/*
 * DeviceSpecific_C6670.c
 *
 * Configuration support functions provided for the specific device.
 *
 * Copyright (C) 2010 - 2012 Texas Instruments Incorporated - http://www.ti.com/
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
#include "DeviceSpecific_OMAP5430.h"

void Config_STMData_Routing()
{
#ifdef _MIPI_STM
#if 0
    *((volatile uint32_t*)DTF_LOCK(0)) = ETB_UNLOCK_VAL;
    *((volatile uint32_t*)DTF_CNTL(0)) = 0x380; /* Slave port 7 and HT=3 */
#else
    CSTF_DSS_UNLOCK_REG = CS_UNLOCK_VALUE;
    CSTF_CTRL_DSS_REG |= CSTF_CTRL_DSS_CTSTM;
#endif
#endif

#ifdef _CS_STM
    CSTF_MPU_UNLOCK_REG = CS_UNLOCK_VALUE;
    CSTF_DSS_UNLOCK_REG = CS_UNLOCK_VALUE;
    CS_TPIU_UNLOCK_REG  = CS_UNLOCK_VALUE;

    /* Enable Trace Funnel slave ports for CS_STM and MPU ATB interface */
	CSTF_CTRL_MPU_REG |= CSTF_CTRL_MPU_CSSTM;
    CSTF_CTRL_DSS_REG |= CSTF_CTRL_DSS_MPU_ATB;

    /* Stop and flush the TPIU formatter, wait for completion */
    CSTPIU_FMT_CTRL_REG |= (CS_FMT_CTRL_STOP | CS_FMT_CTRL_MANFLUSH);
    while((CSTPIU_FMT_STAT_REG & CS_FMT_STAT_STOPPED) == 0)
    {
        /* Add timeout... */
    }
#endif

}

eSTM_STATUS Config_STM_for_ETB(STMHandle* pSTMHandle)
{
	eSTM_STATUS retval = eSTM_SUCCESS;

#ifdef _CS_STM
    STM_CS_ConfigObj STM_CS_Config;
    STM_CS_Config.TraceBufSize = ETB_SIZE;

    retval = STMXport_config_CS(pSTMHandle, &STM_CS_Config);
#endif
#ifdef _MIPI_STM
    STM_MIPI_ConfigObj MIPI_Config;
    MIPI_Config.TraceBufSize = ETB_SIZE;

    /* Default enable STM SW masters - MPUSS (0x0), IVAHD(0x08), Tesla(0x28)
     *  and Ducati(0x38)
     */
	MIPI_Config.SW_MasterMapping =   0x10204400;
	MIPI_Config.SW_MasterMask = 0x03030303;
    /* Default enable HW masters for OCP probe, OCP profile, Statistics
     *  Collector0 and IVA_HD_SMSET0
     */
	MIPI_Config.HW_MasterMapping = 0x747C7860;

	retval = STMXport_config_MIPI(pSTMHandle, &MIPI_Config);
#endif

	return retval;

}

void etm_config_for_etb(void)
{
	/* Configure CS Trace Funnel */
	CSTF_MPU_UNLOCK_REG = CS_UNLOCK_VALUE;
	/* Enable port 0,1 to pass PTM to ETB
	   Bits 8:11 - Minimum hold time (3 cycles) */
	CSTF_CTRL_MPU_REG |= 0x300 + CSTF_CTRL_MPU_C0 + CSTF_CTRL_MPU_C1;
	/* Configure Trace Funnel */
	CSTF_DSS_UNLOCK_REG = CS_UNLOCK_VALUE;
	/* Enable port 0 to pass MPU ATB data to ETB
	   Bits 8:11 - Minimum hold time (3 cycles) */
	CSTF_CTRL_DSS_REG |= 0x300 + CSTF_CTRL_DSS_MPU_ATB;

}



