/**
* @file ti_operation_error.h
* @brief Error handling definitions.
*
* Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/ 
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

#ifndef __SOCTUNE_ERROR_H
#define __SOCTUNE_ERROR_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        eReserved = 0,
        eAtbRepDriver,
        eCmiDriver,
        eCptDriver,
        eCsStmDriver,
        eCs64StmDriver,
        eCstfDriver,
        eDrmDriver,
        eEtmV4Driver,
        eItmDriver,
        eMipiStmDriver,
        ePmiDriver,
        ePmuDriver,
        eStmXprtDriver,
        eCsEtbDriver,
        eTbrDriver,
        eTetbDriver,
        eTpiuDriver,
        eC66TrcDriver,
        eC66AetDriver,
        eC71TrcDriver,
        eC71AetDriver,
        eARMDebugDriver,
		eCtset2Driver,
        eUndefinedComponent
    } eComponentType_t;

// Number of errors allocated for each component
#define COMPONENT_ERROR_COUNT 1000

    // utility functions
    inline uint32_t GetErrorBase(eComponentType_t type) {
        return (((uint32_t)type) * COMPONENT_ERROR_COUNT);
    }

    inline eComponentType_t GetComponentType(uint32_t error_code) {
        uint32_t type = (error_code / COMPONENT_ERROR_COUNT);
        if (type < eUndefinedComponent) {
            return (eComponentType_t)type;
        }
        return eUndefinedComponent;
    };

	// Call back function pointer
    typedef void (error_add_fn_t)(void *handle, uint32_t errorCode, bool resource, const char *errorMsg);

    struct TiErrorHandler_t {
        error_add_fn_t *on_error;
        void *          handle;
    };


#ifdef __cplusplus
}
#endif

#endif //__SOCTUNE_ERROR_H
