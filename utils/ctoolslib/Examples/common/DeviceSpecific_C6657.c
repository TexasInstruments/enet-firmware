/*
 * DeviceSpecific_C6657.c
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
#endif

#ifdef UCLIB_ETB_EDMA
#include "ctools_uclib.h"
#endif

#include "DeviceSpecific_C6657.h"

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

	/* Configure DMA parameters */
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

	ETB_config_dma(pETBHandle, &dmaConfig);

	/*
	 * **  END OF DMA CONFIGURATION REQUIRED BY APPLICATION CODE  **
	 ********************************************************************************/
}

void Config_EDMA_for_DSPETB(ETBHandle* pETBHandle, uint32_t edma_buffer_start, uint32_t buffer_size_in_words, uint8_t coreID)
{
	DMAConfig dmaConfig;

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

	/* Configure the DMA parameters */
	dmaConfig.linkparam[0] = 100+coreID;  /* 3 additional PaRAM sets for links */
	dmaConfig.linkparam[1] = 110+coreID;
	dmaConfig.linkparam[2] = 120+coreID;
	dmaConfig.dbufAddress  = GET_GLOBAL_ADDR(edma_buffer_start);
	dmaConfig.dbufWords    = buffer_size_in_words;
	dmaConfig.mode         = eDMA_Circular; /* Using circular buffer, could also set to
											 *  eDMA_Stop_Buffer for non-circular.
											 */

	ETB_config_dma(pETBHandle, &dmaConfig);
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

	/* param array */
	uint32_t  edma3_params[10] = {0};

	//For EDMA based ETB extension: Need only 3 params
	//The EDMA channels directly allocated for ETB events are used by default internally inside the ETBLib implementation
	//Please refer to the data manual for the exact EDMA channels being used
	edma3_params[0] = 100+coreID;
	edma3_params[1] = 110+coreID;
	edma3_params[2] = 120+coreID;

	config.edmaConfig.channels_ptr = NULL;
	config.edmaConfig.param_ptr    = &edma3_params[0];
	config.edmaConfig.dbufAddress  = GET_GLOBAL_ADDR(edma_buffer_start);
	config.edmaConfig.dbufBytes    = buffer_size_in_words*4;
	config.edmaConfig.mode         = CTOOLS_USECASE_EDMA_CIRCULAR_MODE;

	ctools_ret = ctools_etb_init(CTOOLS_DRAIN_ETB_EDMA, &config, CTOOLS_DSP_ETB);

	if (ctools_ret != CTOOLS_SOK)
	{
	    return(ctools_ret);
	}

	//return success
	return(ctools_ret);
}

ctools_Result UCLib_Config_EDMA_for_SYSETB(uint32_t edma_buffer_start, uint32_t buffer_size_in_words)
{
	ctools_etb_config_t config = {0};
	ctools_Result ctools_ret = CTOOLS_SOK;

	/* param array */
	uint32_t  edma3_params[10] = {0};


	//For EDMA based ETB extension: Need only 3 params
	//The EDMA channels directly allocated for ETB events are used by default internally inside the ETBLib implementation
	//Please refer to the data manual for the exact EDMA channels being used
	edma3_params[0] = 95;
	edma3_params[1] = 96;
	edma3_params[2] = 97;

	config.edmaConfig.channels_ptr = NULL;
	config.edmaConfig.param_ptr    = &edma3_params[0];
	config.edmaConfig.dbufAddress  = GET_GLOBAL_ADDR(edma_buffer_start);
	config.edmaConfig.dbufBytes    = buffer_size_in_words*4;
	config.edmaConfig.mode         = CTOOLS_USECASE_EDMA_CIRCULAR_MODE;

	ctools_ret = ctools_etb_init(CTOOLS_DRAIN_ETB_EDMA, &config, CTOOLS_SYS_ETB);

	if (ctools_ret != CTOOLS_SOK)
	{
	    return(ctools_ret);
	}

	//return success
	return(ctools_ret);
}

#endif




