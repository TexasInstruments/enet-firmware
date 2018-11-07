/*
 * DeviceSpecific_C6A816x.h
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
#ifndef __DEVICE_SPECIFIC_C6A816x_H
#define __DEVICE_SPECIFIC_C6A816x_H

#include <stdint.h>

/*******************************************************************************
 * Base Address Definitions
 ******************************************************************************/
/* STM Module */
#define STM_XPORT_BASE_ADDR       0x4B000000
#define STM_CHAN_RESOLUTION       0x1000
#define STM_CONFIG_BASE_ADDR      0x4b161000

/* Trace Funnel */
#define C6A816x_TF_CNTL_REG       0x4B164000
#define C6A816x_TF_LOCK_REG       0x4B164fb0

/* Slave port 7 and HT=3 */
#define TF_STM_ENABLE_VALUE        0x380
#define TF_UNLOCK_VALUE            0xC5ACCE55

/*******************************************************************************
 * Required Definitions
 ******************************************************************************/
#define BYTE_SWAP32(n) \
	( ((((uint32_t) n) << 24) & 0xFF000000) |	\
	  ((((uint32_t) n) <<  8) & 0x00FF0000) |	\
	  ((((uint32_t) n) >>  8) & 0x0000FF00) |	\
	  ((((uint32_t) n) >> 24) & 0x000000FF) )

#define ETB_USING_TWP 0

/*******************************************************************************
 * Device Specific Function Prototypes
 ******************************************************************************/
eSTM_STATUS Config_STM_for_ETB(STMHandle* pSTMHandle);
void Config_STMData_Routing(void);

#endif /* __DEVICE_SPECIFIC_C6A816x_H */

