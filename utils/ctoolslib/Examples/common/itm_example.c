/*
 * itm_example.c
 *
 * ITM Example public functions.
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

#include "itm.h"
#include <stdio.h>

#define BUF_SIZE 100

/* Application entry point */
void main(void)
{
	unsigned trial_num;
	ITM_port_t port_num;
	size_t buf_size = BUF_SIZE;
	char tempBuf[BUF_SIZE];
	eITM_Error error;

	ITM_put_string("This is the beginning of ITM example.\nUsing stimulus port 0 as ASCII character port to display strings.");
	ITM_put_string("Stimulus port 1 to 31 are used to transmit binary data");
	ITM_put_string("Testing all 4 null termination boundaries ...");
	ITM_put_string("a");
	ITM_put_string("ab");
	ITM_put_string("abc");
	ITM_put_string("abcd");

	/* All ITM_put functions can return an error, but if you don't
	 * want to check on every call, you can still check for
	 * an error when it's convenient with ITM_getFirstError().
	 * ITMLib preserves the first error until ITM_getFirstError()
	 * is called.
	 */
	error = ITM_getFirstError();
	if (error !=  eITM_Success) {
	    printf("Example terminated with ITMLib error %d\n", error);
	    return;
	}

	for (trial_num = 0; trial_num < 2; trial_num++)
	{
		snprintf(&tempBuf[0], buf_size, "Iteration = %d", trial_num);
		ITM_put_string(&tempBuf[0]);

		/* For binary values ports, 1 to 31 are valid (port 0 used for strings only) */
		for (port_num = 1; port_num < ITM_NUM_PORTS; ++port_num) {

			/* Send text header for this test */
			snprintf(&tempBuf[0], buf_size, "Transmit binary data through port %d", port_num);
			ITM_put_string(&tempBuf[0]);

			/* Send numeric data to the current port */
			ITM_put_32(port_num, trial_num);
			ITM_put_32(port_num, 0x12345678);
			ITM_put_16(port_num, trial_num);
			ITM_put_16(port_num, 0x9abc);
			ITM_put_8(port_num, trial_num);
			ITM_put_8(port_num, 0xde);
		}
	}

	ITM_put_string("End of ITM Test.");

    error = ITM_getFirstError();
    if (error !=  eITM_Success) {
        printf("Example terminated with ITMLib error %d\n", error);
    } else {
        printf("Example terminated with no errors\n");
    }

}
