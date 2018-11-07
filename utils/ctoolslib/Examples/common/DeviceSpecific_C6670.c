/*
 * DeviceSpecific_C6670.c
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

#ifdef _MIPI_STM
#include "StmLibrary.h"
#endif

#ifdef _ETB_EDMA
#include "ETBInterface.h"
#include "edma_dev-c66xx.h"
#endif

#ifdef UCLIB_ETB_EDMA
#include "ctools_uclib.h"
#endif

#include "DeviceSpecific_C6670.h"

#ifdef _MIPI_STM

void Config_STMData_Routing()
{
	//For C6670 no routing required
	return;
}
eSTM_STATUS Config_STM_for_ETB(STMHandle* pSTMHandle)
{
	eSTM_STATUS retval;
	STM_MIPI_ConfigObj MIPI_Config;

	//Enable all SW Masters
	MIPI_Config.SW_MasterMapping =  0x0C080400;
	MIPI_Config.SW_MasterMask = 0x03030303;
	//Enable the CP Tracer HW Masters
	MIPI_Config.HW_MasterMapping = 0x80808080;

	retval = STMXport_config_MIPI(pSTMHandle, &MIPI_Config);

	return retval;

}

#endif

#ifdef _ETB_EDMA
void Config_EDMA_for_SYSETB(ETBHandle* pETBHandle, uint32_t edma_buffer_start, uint32_t buffer_size_in_words)
{
	DMAConfig dmaConfig;

	 /********************************************************************************
	 * **  DMA CONFIGURATION REQUIRED BY APPLICATION CODE  **
	 *
	 */
	/* Map Channel Controller 1 DMA channel 45 to PaRAM 93 and Channel Controller 1
	 *  DMA channel 0 to PaRAM 94.
	 */
	EDMA3_DCHMAP_REG(1,45) = (93 << 5);
	EDMA3_DCHMAP_REG(1, 0) = (94 << 5);  /* Using SPIINT1 Event */

	/* Configure DMA parameters */
	dmaConfig.cc           = 1;  /* Using channel controller 1 */
	dmaConfig.clrChannel   = 45; /* DMA channel 45 used for clearing system event */
	dmaConfig.etbChannel   = 0;  /* DMA channel 0 used for ETB data transfers */
	dmaConfig.linkparam[0] = 95; /* 3 additional PaRAM sets for links */
	dmaConfig.linkparam[1] = 96;
	dmaConfig.linkparam[2] = 97;
	dmaConfig.dbufAddress  = GET_GLOBAL_ADDR(edma_buffer_start);
	dmaConfig.dbufWords    = buffer_size_in_words;
	/* For the System (STM) ETB DMA configuration, this option must be set to Circular for the STM
	 * data to decode properly.
	 */
	dmaConfig.mode         = eDMA_Circular;/* Options are eDMA_Stop_Buffer or
											   *  eDMA_Circular.
											   */

	dmaConfig.cic = eCIC_1;     /* Using CIC1, event specifics shown below */

	ETB_config_dma(pETBHandle, &dmaConfig);

	/* The following 2 registers enable the CIC1 inputs and map them to a
	 *  specific output. The System ETB half-full and full output interrupts
	 *  are connected as event inputs to CIC1 event 8 and 9. Both input events
	 *  are mapped to output channel 2 using the channel map byte locations for
	 *  events 8 and 9 (reg0 3-0, reg1 7-4, reg2 11-8, ...). Output channel 2 is
	 *  connected to the EDMA3 input channel 45, assigned above.
	 */
	CIC_CHMAP_REG2(eCIC_1)  = 0x00000202;
	CIC_ENABLE_REG(eCIC_1,0) = (0x3 << 8); /* Set bits 8 & 9 */

	/* Enable host(output) event 2 in relation to channel mapping */
	CIC_ENABLE_HINT_REG(eCIC_1,0) = 0x4;
	CIC_ENABLE_GHINT_REG(eCIC_1) = 0x1;    /* Global host interrupt enable */

	/*
	 * **  END OF DMA CONFIGURATION REQUIRED BY APPLICATION CODE  **
	 ********************************************************************************/
}

void Config_EDMA_for_DSPETB(ETBHandle* pETBHandle, uint32_t edma_buffer_start, uint32_t buffer_size_in_words, uint8_t coreID)
{
	DMAConfig dmaConfig;
	uint32_t edmaEvtNum;

	/************************************************************************************
	 * **  DMA CONFIGURATION REQUIRED BY APPLICATION CODE  **
	 *
	 *  A detailed explanation of the configuration requirements for the application
	 *   versus the ETB library is in the main page of the ETB library doxygen based
	 *   documentation. This main page is at the bottom of the ETBInterface.h file.
	 */

	/* Map specific parameter RAM sets to DMA channels
	 *  Using channels starting at 45 for core0 ETB data transfers and 0 for DMA channels
	 *  to clear the specific CIC interrupts.
	 */

	edmaEvtNum = (46 + coreID);

	EDMA3_DCHMAP_REG(1,edmaEvtNum) = (edmaEvtNum << 5);
	EDMA3_DCHMAP_REG(1,coreID)     = ((coreID+14) << 5);

	/* Configure the DMA parameters */
	dmaConfig.cc           = 1;           /* Using channel controller 1 */
	dmaConfig.clrChannel   = edmaEvtNum;  /* DMA channel 45+core used for clearing system event */
	dmaConfig.etbChannel   = coreID;      /* DMA channel 0+core used for ETB data transfers */
	dmaConfig.linkparam[0] = 100+coreID;  /* 3 additional PaRAM sets for links */
	dmaConfig.linkparam[1] = 110+coreID;
	dmaConfig.linkparam[2] = 120+coreID;
	dmaConfig.dbufAddress  = GET_GLOBAL_ADDR(edma_buffer_start);
	dmaConfig.dbufWords    = buffer_size_in_words;
	dmaConfig.mode         = eDMA_Circular; /* Using circular buffer, could also set to
											 *  eDMA_Stop_Buffer for non-circular.
											 */
	dmaConfig.cic = eCIC_1;

	ETB_config_dma(pETBHandle, &dmaConfig);

	/* TI-ETB half-full & full interrupt mapping from input CIC1 event from EDMA3 to CIC1 output.
	 *  Mapping events 11/12 from ETB0 to output channel 3 to EDMA CC1 input 46,
	 *        - events 14/15 from ETB1 to output channel 4 to EDMA CC1 input 47,
	 *        - events 17/18 from ETB2 to output channel 5 to EDMA CC1 input 48,
	 *        - events 20/21 from ETB3 to output channel 6 to EDMA CC1 input 49,
	 */

	CIC_CHMAP_REG(eCIC_1,2)   = 0x03000000;
	CIC_CHMAP_REG(eCIC_1,3)   = 0x04040003;
	CIC_CHMAP_REG(eCIC_1,4)   = 0x00050500;
	CIC_CHMAP_REG(eCIC_1,5)   = 0x00000606;

	/* Set the corresponding enable bits for specific C6670 core TETB HFULL/FULL:
	 *  Core 0-3, events 11/12, 14/15, 17/18, 20/21  ==> Register offset 0 */

	CIC_ENABLE_REG(eCIC_1,0) = (0x3 << (11 + (3*coreID)));

	CIC_ENABLE_HINT_REG(eCIC_1,0) = 0x78;  /* Enable host interrupts 3-10 */
	CIC_ENABLE_GHINT_REG(eCIC_1)  = 0x1;    /* Global host interrupt enable */

	/*
	 * **  END OF DMA CONFIGURATION REQUIRED BY APPLICATION CODE  **
	 ************************************************************************************/
}

#endif

#ifdef UCLIB_ETB_EDMA

ctools_Result UCLib_Config_EDMA_for_DSPETB(uint32_t edma_buffer_start, uint32_t buffer_size_in_words, uint8_t coreID)
{
	ctools_etb_config_t config = {0};
	ctools_Result ctools_ret = CTOOLS_SOK;
	uint32_t edmaEvtNum;

	/* channel arrary */
	uint32_t  edma3_channels[10] = {0};
	/* param array */
	uint32_t  edma3_params[10] = {0};

	edmaEvtNum = (46 + coreID);

	//For EDMA based ETB extension: Need 2 channels and 5 params
	edma3_channels[0] = edmaEvtNum;
	edma3_channels[1] = coreID;
	edma3_params[0] = edmaEvtNum;
	edma3_params[1] = coreID+14;
	edma3_params[2] = 100+coreID;
	edma3_params[3] = 110+coreID;
	edma3_params[4] = 120+coreID;

	config.edmaConfig.channels_ptr = &edma3_channels[0];
	config.edmaConfig.param_ptr    = &edma3_params[0];
	config.edmaConfig.cic_sel      = eCIC_1;
	config.edmaConfig.cc_sel       = CTOOLS_USECASE_EDMA_CC1;
	config.edmaConfig.dbufAddress  = GET_GLOBAL_ADDR(edma_buffer_start);
	config.edmaConfig.dbufBytes    = buffer_size_in_words*4;
	config.edmaConfig.mode         = CTOOLS_USECASE_EDMA_CIRCULAR_MODE;

	ctools_ret = ctools_etb_init(CTOOLS_DRAIN_ETB_EDMA, &config, CTOOLS_DSP_ETB);

	if (ctools_ret != CTOOLS_SOK)
	{
	    return(ctools_ret);
	}

	/* TI-ETB half-full & full interrupt mapping from input CIC1 event from EDMA3 to CIC1 output.
	 *  Mapping events 11/12 from ETB0 to output channel 3 to EDMA CC1 input 46,
	 *        - events 14/15 from ETB1 to output channel 4 to EDMA CC1 input 47,
	 *        - events 17/18 from ETB2 to output channel 5 to EDMA CC1 input 48,
	 *        - events 20/21 from ETB3 to output channel 6 to EDMA CC1 input 49,
	 */

	CIC_CHMAP_REG(eCIC_1,2)   = 0x03000000;
	CIC_CHMAP_REG(eCIC_1,3)   = 0x04040003;
	CIC_CHMAP_REG(eCIC_1,4)   = 0x00050500;
	CIC_CHMAP_REG(eCIC_1,5)   = 0x00000606;

	/* Set the corresponding enable bits for specific C6670 core TETB HFULL/FULL:
	 *  Core 0-3, events 11/12, 14/15, 17/18, 20/21  ==> Register offset 0 */

	CIC_ENABLE_REG(eCIC_1,0) = (0x3 << (11 + (3*coreID)));

	CIC_ENABLE_HINT_REG(eCIC_1,0) = 0x78;  /* Enable host interrupts 3-10 */
	CIC_ENABLE_GHINT_REG(eCIC_1)  = 0x1;    /* Global host interrupt enable */

	//return success
	return(ctools_ret);
}

ctools_Result UCLib_Config_EDMA_for_SYSETB(uint32_t edma_buffer_start, uint32_t buffer_size_in_words)
{
	ctools_etb_config_t config = {0};
	ctools_Result ctools_ret = CTOOLS_SOK;

	/* channel arrary */
	uint32_t  edma3_channels[10] = {0};
	/* param array */
	uint32_t  edma3_params[10] = {0};


	//For EDMA based ETB extension: Need 2 channels and 5 params
	edma3_channels[0] = 45;
	edma3_channels[1] = 0;
	edma3_params[0] = 93;
	edma3_params[1] = 94;
	edma3_params[2] = 95;
	edma3_params[3] = 96;
	edma3_params[4] = 97;

	config.edmaConfig.channels_ptr = &edma3_channels[0];
	config.edmaConfig.param_ptr    = &edma3_params[0];
    config.edmaConfig.cic_sel      = eCIC_1;
    config.edmaConfig.cc_sel       = CTOOLS_USECASE_EDMA_CC1;
    config.edmaConfig.dbufAddress  = GET_GLOBAL_ADDR(edma_buffer_start);
    config.edmaConfig.dbufBytes    = buffer_size_in_words*4;
    config.edmaConfig.mode         = CTOOLS_USECASE_EDMA_CIRCULAR_MODE;

	ctools_ret = ctools_etb_init(CTOOLS_DRAIN_ETB_EDMA, &config, CTOOLS_SYS_ETB);

	if (ctools_ret != CTOOLS_SOK)
	{
	    return(ctools_ret);
	}

	/* The following 2 registers enable the CIC1 inputs and map them to a
	 *  specific output. The System ETB half-full and full output interrupts
	 *  are connected as event inputs to CIC1 event 8 and 9. Both input events
	 *  are mapped to output channel 2 using the channel map byte locations for
	 *  events 8 and 9 (reg0 3-0, reg1 7-4, reg2 11-8, ...). Output channel 2 is
	 *  connected to the EDMA3 input channel 45, assigned above.
	 */
	CIC_CHMAP_REG2(eCIC_1)  = 0x00000202;
	CIC_ENABLE_REG(eCIC_1,0) = (0x3 << 8); /* Set bits 8 & 9 */

	/* Enable host(output) event 2 in relation to channel mapping */
	CIC_ENABLE_HINT_REG(eCIC_1,0) = 0x4;
	CIC_ENABLE_GHINT_REG(eCIC_1) = 0x1;    /* Global host interrupt enable */

	//return success
	return(ctools_ret);
}

#endif




