/**
* @file trace_aggr.h
* @brief Trace aggregator definitions.
*
* Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/
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

#ifndef __TRACE_AGGR_H
#define __TRACE_AGGR_H

#include <stdint.h>
#include "td_error.h"
#include "target_access.h"

#ifdef __cplusplus
extern "C" {
#endif

    static const uint32_t  TRACE_AGGR_LIBRARY_MAJOR_VERSION = 1;
    static const uint32_t  TRACE_AGGR_LIBRARY_MINOR_VERSION = 0;

    /**
    * @brief CPT Aggregator Internal Data
    *
    */
    typedef struct _trace_aggr_data_t {
        target_access_t ta; /**< target access data structure */
    } trace_aggr_data_t;


    /**
    * @brief CPT Aggregator instance handle.
    *
    * Forward reference serves as opaque pointer type private to the CPT aggregator module.
    * Returned by TRACE_AGGR_open() and passed into subsequent calls into the module
    */
    typedef struct _trace_aggr_data_t* trace_aggr_handle_t;


    /**
    * @brief CPT aggregator initialization structure.
    *
    * Structure of device specific information to program a TRACE_AGGR unit.
    *
    * @param ta target access handle for register/memory accesses (_REMOTE_HOST)
    */
    typedef struct trace_aggr_init_t {
        target_access_handle_t ta;
    } trace_aggr_init_t;


    /**
    * @brief Trace aggregator configuration structure.
    *
    * Configuration options for a trace aggregator.
    *
    * @param atb_id
    */
    typedef struct trace_aggr_options_t {
        uint32_t atb_id;
    } trace_aggr_options_t;


    /**
    * @brief Open GTC programming interface.
    *
    * Populate handle with configuration structure.
    * Return valid GTC handle.
    *
    * @param[in] p_init initialization object (pass null for self-hosted
    *               implementation)
    * @param[out] p_handle pointer to GTC handle
    */
    td_error_t TRACE_AGGR_open(const trace_aggr_init_t *p_init,
        trace_aggr_handle_t* p_handle);


    /**
    * @brief Configure trace aggregator programming interface.
    *
    * @param[in] handle trace_aggr handle
    * @param[in] p_options pointer to options structure
    */
    td_error_t TRACE_AGGR_configure(
        trace_aggr_handle_t handle,
        trace_aggr_options_t* p_options);


    /**
    * @brief Enable CPT aggregator
    *
    * Enable CPT aggregator
    * @param[in] handle GTC session handle
    */
    td_error_t TRACE_AGGR_enable(trace_aggr_handle_t handle);


    /**
    * @brief Disabale CPT aggregator
    *
    * Disable CPT aggregator
    * @param[in] handle GTC session handle
    */
    td_error_t TRACE_AGGR_disable(trace_aggr_handle_t handle);


    /**
    * @brief Close CPT aggregator programming interface.
    *
    * @param[in] handle TRACE_AGGR session handle
    */
    td_error_t TRACE_AGGR_close(trace_aggr_handle_t handle);


#ifdef __cplusplus
}
#endif

#endif //__TRACE_AGGR_H

/*! \mainpage
\par Introduction
The trace aggregator library provides the interface to configure trace aggregators.

\par Devices supported by the library are:

- K3M4

\n\n
\par CPT Aggregator Driver Revision History
<TABLE>
<TR><TD> Revision </TD> <TD>Date</TD> <TD>Notes</TD></TR>

<TR><TD> 1.0 </TD> <TD> 11/15/2017 </TD>
<TD> Trace aggregator initial implementation.  </TD>

</TABLE>

*/
