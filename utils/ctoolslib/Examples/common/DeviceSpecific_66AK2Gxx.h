/*
 * DeviceSpecific_C66AK2Hxx.h
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
#ifndef __DEVICE_SPECIFIC_C66AK2Gxx_H
#define __DEVICE_SPECIFIC_C66AK2Gxx_H

#ifdef _MIPI_STM
#include "StmLibrary.h"
#endif

#include <stdint.h>

#ifndef _66AK2Gxx_CSSTM_ETB
#if defined (_ETB_EDMA) || defined(UCLIB_ETB_EDMA)
#include "c6x.h"
#define GET_GLOBAL_ADDR(addr) \
    (uint32_t)(((uint32_t)(addr)) < (uint32_t)0x00900000 ? \
    ((uint32_t)(addr) | (uint32_t)((DNUM + 16) << 24)) : (uint32_t)(addr))
#endif
#endif

#ifdef _ETB_EDMA
#include "ETBInterface.h"
#endif

/*******************************************************************************
 * Base Address Definitions
 ******************************************************************************/
#ifdef _66AK2Gxx
#define STM_XPORT_BASE_ADDR       0x20000000
#define STM_CHAN_RESOLUTION       0x1000
#define STM_CONFIG_BASE_ADDR      0x03018000
#endif
#ifdef _66AK2Gxx_M3
#define STM_XPORT_BASE_ADDR       0x70000000
#define STM_CHAN_RESOLUTION       0x1000
#define STM_CONFIG_BASE_ADDR      0x63018000
#endif


#define MIPI_STM_TBR_SIZE         0x8000 //32 KB

#define ETB_USING_TWP 1

#define BYTE_SWAP32(n) \
	( ((((uint32_t) n) << 24) & 0xFF000000) |	\
	  ((((uint32_t) n) <<  8) & 0x00FF0000) |	\
	  ((((uint32_t) n) >>  8) & 0x0000FF00) |	\
	  ((((uint32_t) n) >> 24) & 0x000000FF) )

/*******************************************************************************
 * Device Specific Function Prototypes
 ******************************************************************************/
#ifdef _MIPI_STM
eSTM_STATUS Config_STM_for_ETB(STMHandle* pSTMHandle);
void Config_STMData_Routing(void);
#endif

#ifdef _ETB_EDMA
void Config_EDMA_for_SYSETB(ETBHandle* pETBHandle, uint32_t edma_buffer_start, uint32_t buffer_size_in_words);
void Config_EDMA_for_DSPETB(ETBHandle* pETBHandle, uint32_t edma_buffer_start, uint32_t buffer_size_in_words, uint8_t coreID);
#endif

#ifdef _66AK2Gxx_CSSTM_ETB

/*******************************************************************************
 * Base Address Definitions for C66AK2Hxx Target board
 ******************************************************************************/
#define PTM0_BASE_ADDRESS                    0x0306C000 //A15_0 PTM base address
#define PTM1_BASE_ADDRESS                    0x0306D000 //A15_1 PTM base address
#define PTM2_BASE_ADDRESS                    0x0306E000 //A15_2 PTM base address
#define PTM3_BASE_ADDRESS                    0x0306F000 //A15_3 PTM base address

//CS FUNNEL Register definitions
#define CSFUNNEL_CTL                        0x03031000
#define CSFUNNEL_LOCKACC                    0x03031FB0

void etm_config_for_etb(void);

#ifdef _ETB_EDMA
void Config_EDMA_for_PTMETB(ETBHandle* pETBHandle, uint32_t edma_buffer_start, uint32_t buffer_size_in_words);
#endif

#endif

#ifdef UCLIB_ETB_EDMA
ctools_Result UCLib_Config_EDMA_for_DSPETB(uint32_t edma_buffer_start, uint32_t buffer_size_in_words, uint8_t coreID);
ctools_Result UCLib_Config_EDMA_for_SYSETB(uint32_t edma_buffer_start, uint32_t buffer_size_in_words);
#endif

#endif /* __DEVICE_SPECIFIC_C66AK2Hxx_H */

