/*
 * selfhosted_device.h
 *
 * Configuration support functions provided for the specific device.
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
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
#ifndef __SELF_HOSTED_DEVICE_H
#define __SELF_HOSTED_DEVICE_H

#ifdef _66AK2xxx
#include "keystone2\keystone2_addr.h"
#ifdef _CS_STM
#define CS_STM_BASE(N)                 COREPAC_A15_STM(N)
//#define STM_XPORT_BASE              ARM_STIM_PORTS=0x1100000
#else
#define MIPI_STM_BASE               DEBUGSS_STM
//#define STM_XPORT_BASE              CSL_DBG_STM_REGS=0x20000000
#endif
//#define TWP_PROTOCOL                1
#define STM_ATB_ID_MIPI             0x40
#define STM_ATB_ID_ARM2_0           0x41
#define ARM_TBR_SIZE              0x4000 //16 KB
#endif

#ifdef _DRA7xx
//#include "keystone2\keystone2_addr.h"
#ifdef _CS_STM
#define CS_STM_BASE(N)              0x5415A000
#define STM_XPORT_BASE              0x47000000
#else
#define MIPI_STM_BASE               DEBUGSS_STM
#define STM_XPORT_BASE              ARM_STIM_PORTS
#endif
#define STM_ATB_ID_MIPI             0x40
#define STM_ATB_ID_ARM2_0           0x41
#endif

#endif //__SELF_HOSTED_DEVICE_H
