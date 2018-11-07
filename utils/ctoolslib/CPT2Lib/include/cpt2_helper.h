/**
* @file cpt2_helper.h
* @brief CPTracer 2 (CPT2) API definitions.
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

#ifndef __CPT2HELPER_H
#define __CPT2HELPER_H

#include <stdint.h>
#include "CPT2DeviceSpecific.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
* @brief CPT2 Helper Function -- Get the base address for debug cell and trace aggregator based on the CPT2 probe ID
*
* Based on the input CPT2 probe ID, get the base address for the corresponding debug cell and trace aggregator to use.
* The base address returned here is the 48-bit base address in system memory view. For CPUs that support 32-bit addressing only 
* (Cortex-R5 etc.), this address will need to be mappd to 32-bit CPU memory space via RAT first.
*
* Return value: 0 -- pass, 1 -- fail
*
* @param[in]  cpt2_probe_id CPTracer2 probe ID (definition can be found in device specific common_<device>.h file)
* @param[out] pdbgcell_baddr_toMap pointer to the base address for the corresponding debug cell in system memory view
* @param[out] pcpt2_aggr_baddr_toMap pointer to the base address for the corresponding CPTracer2 aggregator in system memory view
*/
uint8_t CPT2H_get_addrToMap(cpt2pb_id_t cpt2_probe_id, uint64_t * pdbgcell_baddr_toMap, uint64_t * pcpt2_aggr_baddr_toMap);

/**
* @brief CPT2 Helper Function -- Get the relevant device info based on the CPT2 probe ID
*
* Based on the input CPT2 probe ID, get the relevant device info.
* The base address returned here is the 48-bit base address in system memory view. For CPUs that support 32-bit addressing only
* (Cortex-R5 etc.), this address will need to be mappd to 32-bit CPU memory space via RAT first.
*
* Return value: 0 -- pass, 1 -- fail
*
* @param[in]  cpt2_probe_id CPTracer2 probe ID (definition can be found in device specific common_<device>.h file)
* @param[out] pdbgcell_id pointer to the debug cell ID for the corresponding debug cell.
* @param[out] pdbgcell_baddr pointer to the base address for the corresponding debug cell. If RAT is turned on, then RAT mapped address is returned.
* @param[out] patbrep_baddr pointer to the base address for the corresponding ATB replicator. If RAT is turned on , then RAT mapped address is returned.
* @param[out] pcpt2_aggr_baddr pointer to the base address for the corresponding CPTracer2 aggregator. If RAT is turned on, then RAT mapped address is returned.
* @param[out] pcpt2_probe_baddr pointer to the base address for the corresponding CPTracer2 probe. If RAT is turned on, then RAT mapped address is returned.
* @param[out] pcpt2_mst_id pointer to the master ID value for the corresponding CPTracer2 probe.
*/
uint8_t CPT2H_get_device_info(cpt2pb_id_t cpt2_probe_id, uint8_t *pdbgcell_id, uint64_t * pdbgcell_baddr, uint64_t * patbrep_baddr, uint64_t * pcpt2_aggr_baddr, uint64_t * pcpt2_probe_baddr, uint8_t *pcpt2_mst_id);

#ifdef __cplusplus
}
#endif
#endif //__CPT2HELPER_H

