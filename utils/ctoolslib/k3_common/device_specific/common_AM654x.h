/**
* @file common_AM654x.h
* @brief AM654x device specific definitions
*
* Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
*
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*  Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
*
*  Redistributions in binary form must reproduce the above copyright
*  notice, this list of conditions and the following disclaimer in the
*  documentation and/or other materials provided with the
*  distribution.
*
*  Neither the name of Texas Instruments Incorporated nor the names of
*  its contributors may be used to endorse or promote products derived
*  from this software without specific prior written permission.
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
#ifndef __COMMON_AM654x_H
#define __COMMON_AM654x_H

#include <stdlib.h>
#include <stdint.h>              // The library uses C99 exact-width integer types 

#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////////////////////////////////////////
// Keystone3 Platform Specific Definitions
////////////////////////////////////////////////////////////////////
#define KEYSTONE3           1
#define SYSETB_PRESENT      0
#define ARMETB_PRESENT      0

// CPTracer2 probe Offset address 
#define CPT2_PROBEn_OFFSET(n)       (0x00020000 + n*0x0001000) // CPT2 probe N offset in Aggregator space

// Debug Cell sub-components offset address
#define TBR_OFFSET                  0x00005000 // TBR offset in Debug Cell space
#define ATBREP_OFFSET               0x00004000 // ATB Replicator offset in Debug Cell space

////////////////////////////////////////////////////////////////////
// Device Specific Definitions
////////////////////////////////////////////////////////////////////

////////// GTC definitions ///////////////////////////
// GTC base address in system memory view
#define GTC_BADDR              (0x00a90000) //Global time counter GTC base address, this address is 32-bit so no need to be re-mapped for R5 or M3

////////// ETB definitions ///////////////////////////
#define NUM_ETB_INSTANCES   3 // Number of ETBs in device

///////// Debug Cell definitions ////////////////////
/*! \par eDebugCell_ID_am654x
    Debug Cell ID for AM654x
*/
typedef enum
{
    eCC_DBGCELL_ID = 0,                  /*!< 0 - Compute Cluster Debug Cell */
    eSOC_DBGCELL_ID = 1,                 /*!< 1 - SoC Debug Cell */
    eMCU_DBGCELL_ID = 2                  /*!< 2 - MCU Debug Cell */
} eDebugCell_ID_am654x_t ;


// Debug Cell Base address in system memory view
#define CC_DBGCELL_BADDR       (0x4C3C000000) // Compute Cluster Debug Cell base address
#define SOC_DBGCELL_BADDR      (0x4C3C010000) // SoC Debug Cell base address
#define MCU_DBGCELL_BADDR      (0x4C3C020000) // MCU Debug Cell base address

// for R5 and M3, we will need to turn on RAT to map 64-bit ETB address in SoC system memory view to 32-bit address in CPU (R5 or M3) view
#if defined(RAT_ON)
// For Cortex-R5 and M3, this address can be changed by user. cTools library will need to be rebuilt if this is changed.
#define DBGCELL_BADDR_MAPPED        0x90000000 // Debug Cell, 64KB
#endif

#if defined(RAT_ON) // for Cortex-R5 and M3
#define _DBGCELL_BaseAddress(dc_id) DBGCELL_BADDR_MAPPED
#else // for Cortex-A53 and A72
#define _DBGCELL_BaseAddress(dc_id) ((dc_id==CC_DBGCELL_ID)? CC_DBGCELL_BADDR:( (dc_id==SOC_DBGCELL_ID)? SOC_DBGCELL_BADDR : MCU_DBGCELL_BADDR))
#endif

// ETB base address
#define _ETB_BaseAddress(dc_id)		(_DBGCELL_BaseAddress(dc_id)+TBR_OFFSET)

// ATB Replicator base address
#define _ATBREP_BaseAddress(dc_id)	(_DBGCELL_BaseAddress(dc_id)+ATBREP_OFFSET)

////////// CPTracer2 definitions /////////////////////

//CPT2 Aggregators IDs
#define MSMC0_CPT2_ID       0
#define MSMC1_CPT2_ID       1
#define SOC_CPT2_ID         2
#define MCU_CPT2_ID         3

// CPTracer2 Aggregator Base address in system memory view
#define MSMC0_CPT2_BADDR           (0x4C30140000)
#define MSMC1_CPT2_BADDR           (0x4C30180000)
#define MCU_CPT2_BADDR			   (0x4C3E000000)
#define SOC_CPT2_BADDR			   (0x4C3E100000)


// for R5 and M3, we will need to turn on RAT to map 64-bit ETB address in SoC system memory view to 32-bit address in CPU (R5 or M3) view
#if defined(RAT_ON)
// For Cortex-R5 and M3, this address can be changed by user. cTools library will need to be rebuilt if this is changed.
#define CPT2_BADDR_MAPPED           0x90040000 // CPTracer2 trace aggregator, 256KB
#endif


#if defined(RAT_ON) // for Cortex-R5 and M3
#define _CPT2_BaseAddress(cpt2_id)  CPT2_BADDR_MAPPED
#else // for Cortex-A53 and A72
#define _CPT2_BaseAddress(cpt2_id) ((cpt2_id==MSMC0_CPT2_ID) ? MSMC0_CPT2_BADDR : ((cpt2_id==MSMC1_CPT2_ID) ? MSMC1_CPT2_BADDR : ((cpt2_id==SOC_CPT2_ID) ? SOC_CPT2_BADDR : MCU_CPT2_BADDR)))
#endif

// CPTracer2 probe base address
#define _CPT2Probe_BaseAddress(cpt2_id, pb_index)	(_CPT2_BaseAddress(cpt2_id) + CPT2_PROBEn_OFFSET(pb_index))


#define NUM_CPT2_PROBES     19 // Number of CPTracer2 probes
/*! \par cpt2pb_id_t_am654x
    CPTracer2 probe IDs for AM654x
*/
typedef enum {
    /* MSMC CPTracer2 instance 0 probes */
    eCpTracer2_Probe_0,        /**< 0 - DRU0_TARGET */
    eCpTracer2_Probe_1,        /**< 1 - DRU0_INITIATOR */
    eCpTracer2_Probe_2,        /**< 2 - DRU1_TARGET */
    eCpTracer2_Probe_3,        /**< 3 - DRU1_INITIATOR */
    eCpTracer2_Probe_4,        /**< 4 - CSI0_TARGET */
    eCpTracer2_Probe_5,        /**< 5 - CSI0_INITIATOR */

    /* MSMC CPTracer2 instance 1 probes */
    eCpTracer2_Probe_6,        /**< 6 - EMIF0_INITIATOR */
    eCpTracer2_Probe_7,        /**< 7 - CSI1_TARGET */
    eCpTracer2_Probe_8,        /**< 8 - CSI1_INITIATOR */

    /* SoC CPTracer2 probes */
    eCpTracer2_Probe_9,        /**< 9  - MAIN_CAL0_0 */
    eCpTracer2_Probe_10,       /**< 10 - MAIN_DSS_2 */
    eCpTracer2_Probe_11,       /**< 11 - MAIN_NAVSRAMHI_3 */
    eCpTracer2_Probe_12,       /**< 12 - MAIN_NAVSRAMLO_4 */
    eCpTracer2_Probe_13,       /**< 13 - MAIN_NAVDDRHI_5 */
    eCpTracer2_Probe_14,       /**< 14 - MAIN_NAVDDRLO_6 */

    /* MCU CPTracer2 probes */
    eCpTracer2_Probe_15,       /**< 15 - MCU_EXPORT_SLV_0 */
    eCpTracer2_Probe_16,       /**< 16 - MCU_SRAM_SLV_1 */
    eCpTracer2_Probe_17,       /**< 17 - MCU_FSS_S0_2 */
    eCpTracer2_Probe_18       /**< 18 - MCU_FSS_S1_3 */

} cpt2pb_id_t_am654x;

// CPTracer2 data type definitions
typedef struct {
    cpt2pb_id_t_am654x  probe_id; /* CPT2 probe ID */
    uint8_t             probe_port_index; /* Port number in CPT2 Aggregator */
    uint8_t             dbgcell_id; /* Debug Cell ID */
    uint64_t            dbgcell_baddr; /* Debug Cell base address */
    uint8_t             aggr_id; /* CPT2 Aggregator ID */
    uint64_t            aggr_baddr; /* CPT2 Aggregator base address */
    uint8_t             master_id; /* Master ID for the CPT2 probe; this needs to match with the definition in Keystone3 platform database file in emupack */
} cpt2_access_t_am654x;



#ifdef __cplusplus
}
#endif

#endif /* __COMMON_AM654x_H */
