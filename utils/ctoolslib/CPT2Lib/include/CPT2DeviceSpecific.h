/****************************************************************************
CToolsLib - CPT2 Library 

Copyright (c) 2018 Texas Instruments Inc. (www.ti.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
****************************************************************************/
#ifndef __CPT2_DEVICE_SPECIFIC_H
#define __CPT2_DEVICE_SPECIFIC_H

#ifdef __cplusplus
extern "C" {
#endif

/*! \file CPT2DeviceSpecific.h

    This file contains device specific definitions for the CPT2 library
 */

/************************************************************************** 
   Device directory
**************************************************************************/
/*! \par cpt2pb_id_t
    CPTracer2 probe ID type definition.

    For AM654x:
    typedef cpt2pb_id_t_am654x cpt2pb_id_t;

    For J7ES:
    typedef cpt2pb_id_t_j7es cpt2pb_id_t;

*/
#if defined(AM654x)
	#include "common_AM654x.h"
    typedef cpt2pb_id_t_am654x cpt2pb_id_t;
#elif defined(J7ES)
	#include "common_J7ES.h"
    typedef cpt2pb_id_t_j7es cpt2pb_id_t;
#endif


#ifdef __cplusplus
}
#endif

#endif //__CPT2_DEVICE_SPECIFIC_H
