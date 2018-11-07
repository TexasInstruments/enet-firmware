/*
 * DeviceSpecific_TDA3x.h
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
#ifndef __DEVICE_SPECIFIC_TDA3x_H
#define __DEVICE_SPECIFIC_TDA3x_H

#include <stdint.h>
/*******************************************************************************
 * Base Address Definitions
 ******************************************************************************/

#ifdef _MIPI_STM
#define STM_XPORT_BASE_ADDR  0x54000000
#define STM_CHAN_RESOLUTION  0x1000
#define STM_CONFIG_BASE_ADDR 0x54161000
#endif

/* Trace funnel base address values */
#define CS_TF_DEBUGSS_BASE  0x54164000

#define CS_TPIU_BASE        0x54163000

#define PMI_BASE_ADDRESS     0x4AE07F00
#define PMI_CLKCTRL_REG      0x4AE06040

#define CM1_BASE_ADDRESS     0x4A005F00
#define CM1_CLKCTRL_REG      0x4A005040

/* PTM base address values */
#define PTM0_BASE_ADDRESS	0x5414C000
#define PTM1_BASE_ADDRESS	0x5414D000

/*******************************************************************************
 * CS Unlock Registers
 ******************************************************************************/
#define CS_CONFIG_UNLOCK_OFFSET  0xFB0

#define CSTF_MPU_UNLOCK_REG \
            *(volatile uint32_t *)(CS_TF_MPU_BASE+CS_CONFIG_UNLOCK_OFFSET)
#define CSTF_DSS_UNLOCK_REG \
            *(volatile uint32_t *)(CS_TF_DEBUGSS_BASE+CS_CONFIG_UNLOCK_OFFSET)
#define CS_TPIU_UNLOCK_REG \
            *(volatile uint32_t *)(CS_TPIU_BASE+CS_CONFIG_UNLOCK_OFFSET)

#define CS_UNLOCK_VALUE                    0xC5ACCE55
/*******************************************************************************
 * CS Trace Funnel Control Registers
 ******************************************************************************/
#define CSTF_CTRL_DSS_REG *(volatile uint32_t *)(CS_TF_DEBUGSS_BASE)

#define CSTF_CTRL_DSS_DSP1 (1 << 1)
#define CSTF_CTRL_DSS_DSP2 (1 << 2)
#define CSTF_CTRL_DSS_CTSTM   (1 << 7)

/*******************************************************************************
 * CS TPIU Formatter and Flush Status Register - CSTPIU_FMT_STAT
 ******************************************************************************/
#define CSTPIU_FMT_STAT_OFFSET  0x300

#define CSTPIU_FMT_STAT_REG \
            *(volatile uint32_t *)(CS_TPIU_BASE+CSTPIU_FMT_STAT_OFFSET)

#define CS_FMT_STAT_STOPPED  (1 << 1)
#define CS_FMT_STAT_PROGRESS (1 << 0)

/*******************************************************************************
 * CS TPIU Formatter and Flush Control Register (Offset - 0x304) - CSTPIU_FMT_CTRL
 ******************************************************************************/
#define CSTPIU_FMT_CTRL_OFFSET  0x304

#define CSTPIU_FMT_CTRL_REG \
            *(volatile uint32_t *)(CS_TPIU_BASE+CSTPIU_FMT_CTRL_OFFSET)

#define CS_FMT_CTRL_STOP     (1 << 12)
#define CS_FMT_CTRL_MANFLUSH (1 << 6)

/*******************************************************************************
 * Other Definitions
 ******************************************************************************/
/* System ETB Size */
#define ETB_SIZE 32768
#define ETB_USING_TWP 1
/*******************************************************************************
 * Device Specific Function Prototypes
 ******************************************************************************/
void Config_Device();

#ifdef _MIPI_STM
eSTM_STATUS Config_STM_for_ETB(STMHandle* pSTMHandle);
void Config_STMData_Routing(void);
#endif
//void etm_config_for_etb(void);
void Config_DSPData_Routing(uint8_t coreID);
/*******************************************************************************
 * ETB Trace data format conversion
 ******************************************************************************/
#define BYTE_SWAP32(n) \
    ( ((((uint32_t) n) << 24) & 0xFF000000) |   \
      ((((uint32_t) n) <<  8) & 0x00FF0000) |   \
      ((((uint32_t) n) >>  8) & 0x0000FF00) |   \
      ((((uint32_t) n) >> 24) & 0x000000FF) )

#endif /* __DEVICE_SPECIFIC_DRA7xx_H */

/*******************************************************************************
 * TDA3x Specific Configuration parameters
 ******************************************************************************/

#define DSP1_CORE_ID 1
#define DSP2_CORE_ID 2


#define WR_MEM_32(addr, data)   *(unsigned int*)(addr) =(unsigned int)(data)
#define RD_MEM_32(addr)         *(unsigned int*)(addr)

/*******************************************************************************
        TDA3xx Registers Map Defines: IPU view
*******************************************************************************/
#define DSP1_L2_SRAM_TARG            0x40800000
#define DSP1_SYSTEM_TARG             0x40D00000
#define DSP1_MMU0CFG_TARG            0x40D01000
#define DSP1_MMU1CFG_TARG            0x40D02000
#define DSP1_EDMA_CC_TARG            0x40D10000
#define DSP2_L2_SRAM_TARG            0x41000000
#define EVE_TARG                     0x62000000
#define L4_CFG_TARG                  0x4A000000
#define L4_WKUP_TARG                 0x4AE00000

/*******************************************************************************
        L4_CFG Registers Map Defines: IPU view
*******************************************************************************/
#define CTRL_MODULE_CORE             (L4_CFG_TARG + 0x2000)
#define CM_CORE_AON                  (L4_CFG_TARG + 0x5000)
#define CM_CORE                      (L4_CFG_TARG + 0x8000)

/*******************************************************************************
        L4_WKUP Registers Map Defines: IPU view
*******************************************************************************/
#define PRM                          (L4_WKUP_TARG + 0x6000)

/*******************************************************************************
        PRCM Registers Map Defines: IPU view
*******************************************************************************/
#define IPU_CM_CORE_AON              (CM_CORE_AON + 0x500)
#define DSP1_CM_CORE_AON             (CM_CORE_AON + 0x400)
#define DSP2_CM_CORE_AON             (CM_CORE_AON + 0x600)
#define EVE_CM_CORE_AON              (CM_CORE_AON + 0x640)
#define DSP1_PRM                     (PRM + 0x400)
#define DSP2_PRM                     (PRM + 0x1B00)
#define EVE_PRM                      (PRM + 0x1B40)

#define IPU1_BASE_ADDR               (IPU1_TARGET_TARG + 0x20000)
#define IPU1_MMU_CFG                 (IPU1_TARGET_TARG + 0x80000)

/*******************************************************************************
        DSPSS1 Address Map defines: IPU view
*******************************************************************************/
#define CM_DSP1_CLKSTCTRL            (DSP1_CM_CORE_AON + 0x0)
#define CM_DSP1_DSP1_CLKCTRL         (DSP1_CM_CORE_AON + 0x20)
#define RM_DSP1_RSTCTRL              (DSP1_PRM + 0x10)
#define RM_DSP1_RSTST                (DSP1_PRM + 0x14)

#define DSPSS1BOOTADDR               (CTRL_MODULE_CORE + 0x55C)
#define DSPSS1BOOTADDRVALUE          (DSP1_L2_SRAM_TARG - 0x40000000)

/*******************************************************************************
        DSPSS2 Address Map defines: IPU view
*******************************************************************************/
#define CM_DSP2_CLKSTCTRL            (DSP2_CM_CORE_AON + 0x0)
#define CM_DSP2_DSP2_CLKCTRL         (DSP2_CM_CORE_AON + 0x20)
#define RM_DSP2_RSTCTRL              (DSP2_PRM + 0x10)
#define RM_DSP2_RSTST                (DSP2_PRM + 0x14)

#define DSPSS2BOOTADDR               (CTRL_MODULE_CORE + 0x560)
#define DSPSS2BOOTADDRVALUE          (DSP2_L2_SRAM_TARG - 0x40800000)

/*******************************************************************************
            DSPSS CPU Independent defines
*******************************************************************************/
#define CM_DSP_CLKSTCTRL             ((cpu_num == 1) ? (CM_DSP1_CLKSTCTRL) : (CM_DSP2_CLKSTCTRL))
#define CM_DSP_DSP_CLKCTRL           ((cpu_num == 1) ? (CM_DSP1_DSP1_CLKCTRL) : (CM_DSP2_DSP2_CLKCTRL))
#define RM_DSP_RSTCTRL               ((cpu_num == 1) ? (RM_DSP1_RSTCTRL) : (RM_DSP2_RSTCTRL))
#define RM_DSP_RSTST                 ((cpu_num == 1) ? (RM_DSP1_RSTST) : (RM_DSP2_RSTST))

#define DSPSSBOOTADDR                ((cpu_num == 1) ? (DSPSS1BOOTADDR) : (DSPSS2BOOTADDR))
#define DSPSSBOOTADDRVALUE           ((cpu_num == 1) ? (DSPSS1BOOTADDRVALUE) : (DSPSS2BOOTADDRVALUE))

#define DSP_L2_SRAM_TARG             ((cpu_num == 1) ? (DSP1_L2_SRAM_TARG) : (DSP2_L2_SRAM_TARG))
