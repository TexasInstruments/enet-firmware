/**
* cpt2_helper.c
*
* CPT2Lib Helper Functions
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

#include <stdint.h>
#include "CPT2DeviceSpecific.h"

#if 0//!defined(AM654x) && !defined(J7ES) // Unknown device
    #define NUM_CPT2_PROBES     0
#endif

#if defined(AM654x)
cpt2_access_t_am654x g_cpt2_table[NUM_CPT2_PROBES] =
{

 /* MSMC CPTracer2 instance 0 probes */
 {eCpTracer2_Probe_0, 0, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 160 },
 {eCpTracer2_Probe_1, 1, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 161 },
 {eCpTracer2_Probe_2, 2, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 162 },
 {eCpTracer2_Probe_3, 3, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 163 },
 {eCpTracer2_Probe_4, 4, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 164 },
 {eCpTracer2_Probe_5, 5, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 165 },

 /* MSMC CPTracer2 instance 1 probes */
 {eCpTracer2_Probe_6, 0, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC1_CPT2_ID, _CPT2_BaseAddress(MSMC1_CPT2_ID), 192 },
 {eCpTracer2_Probe_7, 2, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC1_CPT2_ID, _CPT2_BaseAddress(MSMC1_CPT2_ID), 194 },
 {eCpTracer2_Probe_8, 3, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC1_CPT2_ID, _CPT2_BaseAddress(MSMC1_CPT2_ID), 195 },

 /* SoC CPTracer2 probes */
 {eCpTracer2_Probe_9,  0, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 224 },
 {eCpTracer2_Probe_10, 2, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 226 },
 {eCpTracer2_Probe_11, 3, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 227 },
 {eCpTracer2_Probe_12, 4, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 228 },
 {eCpTracer2_Probe_13, 5, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 229 },
 {eCpTracer2_Probe_14, 6, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 230 },

 /* MCU CPTracer2 probes */
 {eCpTracer2_Probe_15, 0, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), MCU_CPT2_ID, _CPT2_BaseAddress(MCU_CPT2_ID), 32 },
 {eCpTracer2_Probe_16, 1, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), MCU_CPT2_ID, _CPT2_BaseAddress(MCU_CPT2_ID), 33 },
 {eCpTracer2_Probe_17, 2, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), MCU_CPT2_ID, _CPT2_BaseAddress(MCU_CPT2_ID), 34 },
 {eCpTracer2_Probe_18, 3, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), MCU_CPT2_ID, _CPT2_BaseAddress(MCU_CPT2_ID), 35 }

};
#elif defined(J7ES)
cpt2_access_t_j7es g_cpt2_table[NUM_CPT2_PROBES] =
{
	//Huimin, need to check with Mark on the Master ID definition
	/* MSMC CPTracer2 instance 0 probes */
	{ eCpTracer2_Probe_0, 0, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 224 },
	{ eCpTracer2_Probe_1, 1, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 225 },
	{ eCpTracer2_Probe_2, 2, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 226 },
	{ eCpTracer2_Probe_3, 3, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 227 },
	{ eCpTracer2_Probe_4, 4, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 228 },
	{ eCpTracer2_Probe_5, 5, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 229 },
	{ eCpTracer2_Probe_6, 6, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 232 },
	{ eCpTracer2_Probe_7, 7, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 233 },
	{ eCpTracer2_Probe_8, 8, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 236 },
	{ eCpTracer2_Probe_9, 9, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC0_CPT2_ID, _CPT2_BaseAddress(MSMC0_CPT2_ID), 237 },

	/* MSMC CPTracer2 instance 1 probes */
	{ eCpTracer2_Probe_10, 0,  eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC1_CPT2_ID, _CPT2_BaseAddress(MSMC1_CPT2_ID), 32 },
	{ eCpTracer2_Probe_11, 10, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC1_CPT2_ID, _CPT2_BaseAddress(MSMC1_CPT2_ID), 42 },
	{ eCpTracer2_Probe_12, 11, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), MSMC1_CPT2_ID, _CPT2_BaseAddress(MSMC1_CPT2_ID), 43 },

	/* SoC CPTracer2 probes */
	{ eCpTracer2_Probe_13, 4, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 68 },
	{ eCpTracer2_Probe_14, 5, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 69 },
	{ eCpTracer2_Probe_15, 6, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 70 },
	{ eCpTracer2_Probe_16, 7, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 71 },
	{ eCpTracer2_Probe_17, 9, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 72 },
	{ eCpTracer2_Probe_18, 10, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 74 },
	{ eCpTracer2_Probe_19, 11, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 75 },
	{ eCpTracer2_Probe_20, 12, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 76 },
	{ eCpTracer2_Probe_21, 13, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 77 },
	{ eCpTracer2_Probe_22, 14, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 78 },
	{ eCpTracer2_Probe_23, 15, eSOC_DBGCELL_ID, _DBGCELL_BaseAddress(eSOC_DBGCELL_ID), SOC_CPT2_ID, _CPT2_BaseAddress(SOC_CPT2_ID), 79 },

	/* SoC AC CPTracer2 probes */
	{ eCpTracer2_Probe_24, 0, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_AC_CPT2_ID, _CPT2_BaseAddress(SOC_AC_CPT2_ID), 100 },
	{ eCpTracer2_Probe_25, 1, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_AC_CPT2_ID, _CPT2_BaseAddress(SOC_AC_CPT2_ID), 101 },
	{ eCpTracer2_Probe_26, 2, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_AC_CPT2_ID, _CPT2_BaseAddress(SOC_AC_CPT2_ID), 102 },
	{ eCpTracer2_Probe_27, 3, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_AC_CPT2_ID, _CPT2_BaseAddress(SOC_AC_CPT2_ID), 103 },
	{ eCpTracer2_Probe_28, 4, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_AC_CPT2_ID, _CPT2_BaseAddress(SOC_AC_CPT2_ID), 105 },
	{ eCpTracer2_Probe_29, 5, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_AC_CPT2_ID, _CPT2_BaseAddress(SOC_AC_CPT2_ID), 106 },

	/* SoC HC CPTracer2 probes */
	{ eCpTracer2_Probe_30, 0, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_HC_CPT2_ID, _CPT2_BaseAddress(SOC_HC_CPT2_ID), 128 },
	{ eCpTracer2_Probe_31, 1, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_HC_CPT2_ID, _CPT2_BaseAddress(SOC_HC_CPT2_ID), 129 },
	{ eCpTracer2_Probe_32, 2, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_HC_CPT2_ID, _CPT2_BaseAddress(SOC_HC_CPT2_ID), 130 },
	{ eCpTracer2_Probe_33, 3, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_HC_CPT2_ID, _CPT2_BaseAddress(SOC_HC_CPT2_ID), 131 },
	{ eCpTracer2_Probe_34, 4, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_HC_CPT2_ID, _CPT2_BaseAddress(SOC_HC_CPT2_ID), 132 },
	{ eCpTracer2_Probe_35, 5, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_HC_CPT2_ID, _CPT2_BaseAddress(SOC_HC_CPT2_ID), 133 },
	{ eCpTracer2_Probe_36, 6, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_HC_CPT2_ID, _CPT2_BaseAddress(SOC_HC_CPT2_ID), 134 },
	{ eCpTracer2_Probe_37, 7, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), SOC_HC_CPT2_ID, _CPT2_BaseAddress(SOC_HC_CPT2_ID), 135 },

	/* MCU CPTracer2 probes */
	{ eCpTracer2_Probe_38, 0, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), MCU_CPT2_ID, _CPT2_BaseAddress(MCU_CPT2_ID), 160 },
	{ eCpTracer2_Probe_39, 1, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), MCU_CPT2_ID, _CPT2_BaseAddress(MCU_CPT2_ID), 161 },
	{ eCpTracer2_Probe_40, 2, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), MCU_CPT2_ID, _CPT2_BaseAddress(MCU_CPT2_ID), 162 },
	{ eCpTracer2_Probe_41, 3, eMCU_DBGCELL_ID, _DBGCELL_BaseAddress(eMCU_DBGCELL_ID), MCU_CPT2_ID, _CPT2_BaseAddress(MCU_CPT2_ID), 163 },

    /* C7x CPTracer2 probes */
    { eCpTracer2_Probe_42, 0, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), C7X_CPT2_ID, _CPT2_BaseAddress(C7X_CPT2_ID), 192 },
    { eCpTracer2_Probe_43, 1, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), C7X_CPT2_ID, _CPT2_BaseAddress(C7X_CPT2_ID), 193 },
    { eCpTracer2_Probe_44, 2, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), C7X_CPT2_ID, _CPT2_BaseAddress(C7X_CPT2_ID), 194 },
    { eCpTracer2_Probe_45, 3, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), C7X_CPT2_ID, _CPT2_BaseAddress(C7X_CPT2_ID), 195 },
    { eCpTracer2_Probe_46, 4, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), C7X_CPT2_ID, _CPT2_BaseAddress(C7X_CPT2_ID), 196 },
    { eCpTracer2_Probe_47, 5, eCC_DBGCELL_ID, _DBGCELL_BaseAddress(eCC_DBGCELL_ID), C7X_CPT2_ID, _CPT2_BaseAddress(C7X_CPT2_ID), 197 } //Huimin, need to double check with Mark's code


};
#endif


uint8_t CPT2H_get_addrToMap(cpt2pb_id_t cpt2_probe_id, uint64_t * pdbgcell_baddr_toMap, uint64_t * pcpt2_aggr_baddr_toMap)
{
    if (NUM_CPT2_PROBES != 0) {
        uint8_t     dbgcell_id = g_cpt2_table[cpt2_probe_id].dbgcell_id;
        uint8_t     aggr_id    = g_cpt2_table[cpt2_probe_id].aggr_id;

        switch (dbgcell_id)
        {
            case eCC_DBGCELL_ID:
                *pdbgcell_baddr_toMap     = CC_DBGCELL_BADDR;
                break;
            case eSOC_DBGCELL_ID:
                *pdbgcell_baddr_toMap     = SOC_DBGCELL_BADDR;
                break;
            case eMCU_DBGCELL_ID:
                *pdbgcell_baddr_toMap     = MCU_DBGCELL_BADDR;
                break;
            default:
                break;
        }

        switch (aggr_id)
        {
            case MSMC0_CPT2_ID:
                *pcpt2_aggr_baddr_toMap     = MSMC0_CPT2_BADDR;
                break;
            case MSMC1_CPT2_ID:
                *pcpt2_aggr_baddr_toMap     = MSMC1_CPT2_BADDR;
                break;
            case SOC_CPT2_ID:
                *pcpt2_aggr_baddr_toMap     = SOC_CPT2_BADDR;
                break;
            case MCU_CPT2_ID:
                *pcpt2_aggr_baddr_toMap     = MCU_CPT2_BADDR;
                break;
#if defined(J7ES)
            case C7X_CPT2_ID:
                *pcpt2_aggr_baddr_toMap     = C7X_CPT2_BADDR;
                break;
#endif
            default:
                break;
        }

        return 0;
    } else {
        return 1;
    }

}

uint8_t CPT2H_get_device_info(cpt2pb_id_t cpt2_probe_id, uint8_t *pdbgcell_id, uint64_t * pdbgcell_baddr, uint64_t * patbrep_baddr, uint64_t * pcpt2_aggr_baddr, uint64_t * pcpt2_probe_baddr, uint8_t *pcpt2_mst_id)
{
    if (NUM_CPT2_PROBES != 0) {
        uint8_t     dbgcell_id = g_cpt2_table[cpt2_probe_id].dbgcell_id;
        uint8_t     port_index = g_cpt2_table[cpt2_probe_id].probe_port_index;
        uint8_t     aggr_id    = g_cpt2_table[cpt2_probe_id].aggr_id;

        *pdbgcell_id        = dbgcell_id;
        *pdbgcell_baddr     = g_cpt2_table[cpt2_probe_id].dbgcell_baddr;
        *patbrep_baddr      = _ATBREP_BaseAddress(dbgcell_id);
        *pcpt2_aggr_baddr   = g_cpt2_table[cpt2_probe_id].aggr_baddr;
        *pcpt2_probe_baddr  = _CPT2Probe_BaseAddress(aggr_id, port_index);
        *pcpt2_mst_id       = g_cpt2_table[cpt2_probe_id].master_id;

        return 0;
    } else {
        return 1;
    }

}
