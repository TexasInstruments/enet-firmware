/**
* @file cpt2.h
* @brief CP Tracer 2 (CPT) API definitions.
*
* Copyright (C) 2017-2018 Texas Instruments Incorporated - http://www.ti.com/ 
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

#ifndef __CPT2_H
#define __CPT2_H

#include <stdint.h>
#include "td_error.h"
#include "target_access.h"

#ifdef _SELF_HOSTED
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t  CPT2_LIBRARY_MAJOR_VERSION = 1;
static const uint32_t  CPT2_LIBRARY_MINOR_VERSION = 0;

/**
* @brief CPT2 Internal Data
*
*/
typedef struct _cpt2_data_t {
    target_access_t ta; /**< Target access data structure definition */
} cpt2_data_t;

/*
typedef struct {
    uint32_t offset;
    const char* name;
} cpt2_mmr_t;
*/

/**
* @brief CPT2 instance handle.
*
* Forward reference serves as opaque pointer type private to the CPT2 module.
* Returned by CPT2_open() and passed into subsequent calls into the module
*/
typedef struct _cpt2_data_t* cpt2_handle_t;

/**
* @brief CPT2 initialization structure.
*
* Structure of device specific information to program a CPT2 unit.
*
*/
typedef struct cpt2_init_t {
    target_access_handle_t ta; /**< target access handle */
} cpt2_init_t;

/**
* @brief CPT2 probe operation mode
*/
typedef enum cpt2_op_mode_t {
    eCPT2_MODE_LATENCY,
    eCPT2_MODE_THROUGHGPUT,
    eCPT2_MODE_TRANSACTION,
    eCPT2_MODE_UNDEFINED
} cpt2_op_mode_t;

/**
* @brief CPT2 probe bus type
*
* Indicates which type of bus is being probed for a given CPT2 probe.
*/
typedef enum cpt2_bus_type_t {
    eCPT2_BUS_VBUSP,
    eCPT2_BUS_VBUSM,
    eCPT2_BUS_VBUSMC,
} cpt2_bus_type_t;

/**
* @brief CPT2 configuration structure.
*
* Configuration options for CPT2 probe.
*
* @param sampling_period Min/Max sampling window range: 0x00001000-0x00FFFFFF clock cycles
* @param mode operation mode
* @param bus_type specify bus type
* @param filt_addr_low_lsb 32 LSBs of the low address bound used for range comparison
* @param filt_addr_low_msb 16 MSBs of the low address bound used for range comparison
* @param filt_addr_low_range_exclude Range Filter Include/Exclude Control 
*            false – The range matches if an access falls inside the defined range  
*            true – The range if an access falls outside the defined range 
* @param filt_addr_high_lsb 32 LSBs of the high address bound used for range comparison
* @param filt_addr_high_msb 16 MSBs of the high address bound used for range comparison
* @param filt_channel_id Filter: the value of the Channel ID Bus (0x0 - 0xFFF)
* @param filt_channel_id_mask Filter mask: the value of the Channel ID Bus (0x0 - 0xFFF)
* @param filt_pable Filter: the memory request can be pre-fetched
* @param filt_pable_mask Filter mask: the memory request can be pre-fetched
* @param filt_cmsideband Filter: the CMSIDEBAND bus mirrored sideband (0x0 - 0x3F)
* @param filt_cmsideband_mask Filter mask: the CMSIDEBAND bus mirrored sideband (0x0 - 0x3F)
* @param filt_csideband Filter: the CSIDEBAND bus (0x0 - 0xFF)
* @param filt_csideband_mask Filter mask: the CSIDEBAND bus (0x0 - 0xFF)
* @param filt_order_id Filter: the extended initiator identifier used for ordering for a specific route ID (0x0 - 0xF)
* @param filt_order_id_mask Filter mask: the extended initiator identifier used for ordering for a specific route ID (0x0 - 0xF)
* @param filt_excl Filter: the transaction is exclusive
* @param filt_excl_mask Filter mask: the transaction is exclusive
* @param filt_dir Filter: the direction of the transaction
* @param filt_dir_mask Filter mask: the direction of the transaction
* @param filt_amode Filter: the value of the transaction address mode (0x0 - 0x3)
* @param filt_amode_mask Filter mask: the value of the transaction address mode (0x0 - 0x3)
* @param filt_route_id Filter: route ID (0x0 - 0xFFF)
* @param filt_route_id_mask Filter mask: route ID (0x0 - 0xFFF)
* @param filt_rsel Filter: the region of the transaction (0x0 - 0xF)
* @param filt_rsel_mask Filter mask: the region of the transaction (0x0 - 0xF)
* @param filt_dtype Filter: the data type of the transaction (0x0 - 0x3)
* @param filt_dtype_mask Filter mask: the data type of the transaction (0x0 - 0x3)
* @param filt_priv Filter: the privilege level of the transaction (0x0 - 0x3)
* @param filt_priv_mask Filter mask: the privilege level of the transaction (0x0 - 0x3)
* @param filt_clsize Filter: the line size wrap qualifier for the transaction (0x0 - 0x7)
* @param filt_clsize_mask Filter mask: the line size wrap qualifier for the transaction (0x0 - 0x7)
* @param filt_epriority Filter: the extended priority level of the transaction (0x0 - 0x7)
* @param filt_epriority_mask Filter mask: the extended priority level of the transaction (0x0 - 0x7)
* @param filt_priority Filter: the priority level of the transaction (0x0 - 0x7)
* @param filt_priority_mask Filter mask: the priority level of the transaction (0x0 - 0x7)
* @param filt_bytecount Filter: the byte count for the transaction (0x0 - 0x3FF)
* @param filt_bytecount_mask Filter mask: the byte count for the transaction (0x0 - 0x3FF)
* @param filt_opcode Filter: the opcode of the transaction (0x0 - 0x3F)
* @param filt_opcode_mask Filter mask: the opcode of the transaction (0x0 - 0x3F)
* @param filt_memtype Filter: the memory type of the transaction (0x0 - 0x3)
* @param filt_memtype_mask Filter mask: the memory type of the transaction (0x0 - 0x3)
* @param filt_secure Filter: the transaction is secure
* @param filt_secure_mask Filter mask: the transaction is secure
* @param filt_qos Filter: the QOS of the transaction (0x0 - 0x7)
* @param filt_qos_mask Filter mask: the QOS of the transaction (0x0 - 0x7)
* @param filt_sdomain Filter: the shareability domain of the transaction (0x0 - 0x3)
* @param filt_sdomain_mask Filter mask: the shareability domain of the transaction (0x0 - 0x3)
* @param filt_outer Filter: transaction for outer cache domain (0x0 - 0x3)
* @param filt_outer_mask Filter mask: transaction for outer cache domain (0x0 - 0x3)
* @param filt_inner Filter: transaction for inner cache domain (0x0 - 0x3)
* @param filt_inner_mask Filter mask: transaction for inner cache domain (0x0 - 0x3)
* @param filt_emudbg Filter: transaction initiated by debug
* @param filt_emudbg_mask Filter mask: transaction initiated by debug
* @param filt_interest Filter: interest bit qualifier of the transaction
* @param filt_interest_mask Filter mask: interest bit qualifier of the transaction
* @param filt_flush Filter: signal to slave ignore invalid transaction
* @param filt_flush_mask Filter mask: signal to slave ignore invalid transaction
* @param filt_fwpass Filter: firewall pass status (0x0 - 0x3)
* @param filt_fwpass_mask Filter mask: firewall pass status (0x0 - 0x3)
* @param filt_asel Filter: external address selection index (0x0 - 0xF)
* @param filt_asel_mask Filter mask: external address selection index (0x0 - 0xF)
* @param filt_atype Filter: the address type designation for the transaction (0x0 - 0x3)
* @param filt_atype_mask Filter mask: the address type designation for the transaction (0x0 - 0x3)
* @param filt_privid Filter: the (secure) privilege ID of the transaction (0x0 - 0x0FF)
* @param filt_privid_mask Filter mask: the (secure) privilege ID of the transaction (0x0 - 0x0FF)
* @param filt_virtid Filter: the virtual master ID for the transaction (0x0 - 0xFFF)
* @param filt_virtid_mask Filter mask: the virtual master ID for the transaction (0x0 - 0xFFF)
* @param mast_id_3_msbs Specify the 3 most significant bits of the master ID (0x0 - 0x7)
*/
typedef struct cpt2_options_t {
    uint32_t sampling_period;
    cpt2_op_mode_t mode;
    uint32_t filt_addr_low_lsb;
    uint16_t filt_addr_low_msb;
    bool filt_addr_low_range_exclude;
    uint32_t filt_addr_high_lsb;
    uint16_t filt_addr_high_msb;
    uint16_t filt_channel_id;
    uint16_t filt_channel_id_mask;
    bool filt_pable;
    bool filt_pable_mask;
    uint8_t filt_cmsideband;
    uint8_t filt_cmsideband_mask;
    uint8_t filt_csideband;
    uint8_t filt_csideband_mask;
    uint8_t filt_order_id;
    uint8_t filt_order_id_mask;
    bool filt_excl;
    bool filt_excl_mask;
    bool filt_dir;
    bool filt_dir_mask;
    uint8_t filt_amode;
    uint8_t filt_amode_mask;
    uint16_t filt_route_id;
    uint16_t filt_route_id_mask;
    uint8_t filt_rsel;
    uint8_t filt_rsel_mask;
    uint8_t filt_dtype;
    uint8_t filt_dtype_mask;
    uint8_t filt_priv;
    uint8_t filt_priv_mask;
    uint8_t filt_clsize;
    uint8_t filt_clsize_mask;
    uint8_t filt_epriority;
    uint8_t filt_epriority_mask;
    uint8_t filt_priority;
    uint8_t filt_priority_mask;
    uint16_t filt_bytecount;
    uint16_t filt_bytecount_mask;
    uint8_t filt_opcode;
    uint8_t filt_opcode_mask;
    uint8_t filt_memtype;
    uint8_t filt_memtype_mask;
    bool filt_secure;
    bool filt_secure_mask;
    uint8_t filt_qos;
    uint8_t filt_qos_mask;
    uint8_t filt_sdomain;
    uint8_t filt_sdomain_mask;
    uint8_t filt_outer;
    uint8_t filt_outer_mask;
    uint8_t filt_inner;
    uint8_t filt_inner_mask;
    bool filt_emudbg;
    bool filt_emudbg_mask;
    bool filt_interest;
    bool filt_interest_mask;
    bool filt_flush;
    bool filt_flush_mask;
    uint8_t filt_fwpass;
    uint8_t filt_fwpass_mask;
    uint8_t filt_asel;
    uint8_t filt_asel_mask;
    uint8_t filt_atype;
    uint8_t filt_atype_mask;
    uint8_t filt_privid;
    uint8_t filt_privid_mask;
    uint16_t filt_virtid;
    uint16_t filt_virtid_mask;
    uint32_t mast_id_3_msbs;
} cpt2_options_t;

// Mostly derived from reset values.
static const cpt2_options_t g_defaultCpt2Options = {
	0xFFFF, //sampling_period = 0xFFFF;
	eCPT2_MODE_UNDEFINED,//mode = eCPT2_MODE_UNDEFINED; // Reset value is actually eCPT2_MODE_LATENCY
	false,//filt_addr_low_range_exclude = false;
	0x0,//filt_addr_low_lsb = 0x0;
	0x0,//filt_addr_low_msb = 0x0;
	0xFFFFFFFF,//filt_addr_high_lsb = 0xFFFFFFFF;
	0xFFFF,//filt_addr_high_msb = 0xFFFF;
	0x0,//filt_channel_id = 0x0;
	0x0,//filt_channel_id_mask = 0x0;
	false,//filt_pable = false;
	false,//filt_pable_mask = false;
	0x0,//filt_cmsideband = 0x0;
	0x0,//filt_cmsideband_mask = 0x0;
	0x0,//filt_csideband = 0x0;
	0x0,//filt_csideband_mask = 0x0;
	0x0,//filt_order_id = 0x0;
	0x0,//filt_order_id_mask = 0x0;
	false,//filt_excl = false;
	false,//filt_excl_mask = false;
	false,//filt_dir = false;
	false,//filt_dir_mask = false;
	0x0,//filt_amode = 0x0;
	0x0,//filt_amode_mask = 0x0;
	0x0,//filt_route_id = 0x0;
	0x0,//filt_route_id_mask = 0x0;
	0x0,//filt_rsel = 0x0;
	0x0,//filt_rsel_mask = 0x0;
	0x0,//filt_dtype = 0x0;
	0x0,//filt_dtype_mask = 0x0;
	0x0,//filt_priv = 0x0;
	0x0,//filt_priv_mask = 0x0;
	0x0,//filt_clsize = 0x0;
	0x0,//filt_clsize_mask = 0x0;
	0x0,//filt_epriority = 0x0;
	0x0,//filt_epriority_mask = 0x0;
	0x0,//filt_priority = 0x0;
	0x0,//filt_priority_mask = 0x0;
	0x0,//filt_bytecount = 0x0;
	0x0,//filt_bytecount_mask = 0x0;
	0x0,//filt_opcode = 0x0;
	0x0,//filt_opcode_mask = 0x0;
	0x0,//filt_memtype = 0x0;
	0x0,//filt_memtype_mask = 0x0;
	false,//filt_secure = false;
	false,//filt_secure_mask = false;
	0x0,//filt_qos = 0x0;
	0x0,//filt_qos_mask = 0x0;
	0x0,//filt_sdomain = 0x0;
	0x0,//filt_sdomain_mask = 0x0;
	0x0,//filt_outer = 0x0;
	0x0,//filt_outer_mask = 0x0;
	0x0,//filt_inner = 0x0;
	0x0,//filt_inner_mask = 0x0;
	false,//filt_emudbg = false;
	false,//filt_emudbg_mask = false;
	false,//filt_interest = false;
	false,//filt_interest_mask = false;
	false,//filt_flush = false;
	false,//filt_flush_mask = false;
	0x0,//filt_fwpass = 0x0;
	0x0,//filt_fwpass_mask = 0x0;
	0x0,//filt_asel = 0x0;
	0x0,//filt_asel_mask = 0x0;
	0x0,//filt_atype = 0x0;
	0x0,//filt_atype_mask = 0x0;
	0x0,//filt_privid = 0x0;
	0x0,//filt_privid_mask = 0x0;
	0x0,//filt_virtid = 0x0;
	0x0,//filt_virtid_mask = 0x0;
	0x0 //mast_id_3_msbs = 0x0;
};


/**
* @brief Open CPT2 programming interface.
*
* Verify hardware PID.
* Return valid CPT2 handle.
*
* @param[in] p_init initialization object
* @param[out] p_handle pointer to CPT2 handle
*/
td_error_t CPT2_open(
    const cpt2_init_t *p_init,
    cpt2_handle_t* p_handle);

/**
* @brief Configure and enable CPT2 trace generation.
*
* @param[in] handle CPT2 handle
* @param[in] p_options configuration options structure
*/
td_error_t CPT2_enable(
    cpt2_handle_t handle,
    const cpt2_options_t *p_options);

/**
* @brief Disable CPT2 trace generation.
*
* @param[in] handle CPT2 handle
*/
td_error_t CPT2_disable(
    cpt2_handle_t handle);

/**
* @brief Close CPT2 programming interface.
*
* Release ownership of CPT2 module.
* @param[in] handle CPT2 session handle
*/
td_error_t CPT2_close(cpt2_handle_t handle);

/**
* @brief Get options string.
*
* Used for creating a DCM file entry for standalone decode.
* @param[in] p_options CPT2 options to be converted to string.
* @param[out] buf output character buffer that will be interpreted by the decoder.
* @param[out] length length of the buffer returned.
*/
td_error_t CPT2_get_options_string(cpt2_options_t* p_options, char* buf, size_t length);

#ifdef __cplusplus
}
#endif
#endif //__CPT2_H

/*! \mainpage
\par Introduction
The CPT2 IP driver provides the interface to configure and control individual CP Tracer 
2 modules.

\par Devices supported by the library are:

   - AM654x

\n\n
\par CPT2 Library Revision History
<TABLE>
<TR><TD> Revision </TD> <TD>Date</TD> <TD>Notes</TD></TR>

<TR><TD> 1.0 </TD> <TD> 3/22/2018 </TD>
<TD> Initial implementation </TD>
<TR><TD> 1.1 </TD> <TD> 8/10/2018 </TD>
<TD> Added CPT2_get_options_string() for standalone decode. </TD>

</TABLE>  

*/
