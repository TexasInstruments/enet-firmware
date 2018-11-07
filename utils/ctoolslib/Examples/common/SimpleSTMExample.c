/*
 * STM Simple Example
 *
 * Copyright (C) 2009-2012 Texas Instruments Incorporated - http://www.ti.com/
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

/*
 *  ======== SimpleSTMExample.c ========
 *  This example is a test example demonstrating
 *  simple STM APIs calls via STMLib.
 */

//  Include Header Files
#include "StmLibrary.h"
#ifdef _AM437x
#include "DeviceSpecific_AM437x.h"
#endif
#ifdef _DRA7xx
#include "DeviceSpecific_DRA7xx.h"
#endif
#ifdef _OMAP5430
#include "DeviceSpecific_OMAP5430.h"
#endif
#ifdef _OMAP44xx
#include "DeviceSpecific_OMAP44xx.h"
#endif
#ifdef _C6670
#include "DeviceSpecific_C6670.h"
#endif
#ifdef _C6614
#include "DeviceSpecific_C6614.h"
#endif
#ifdef TI816x
#include "DeviceSpecific_C6A816x.h"
#endif
#ifdef _C66AK2Hxx
#include "DeviceSpecific_C66AK2Hxx.h"
#endif
#if defined(_66AK2Gxx) || defined(_66AK2Gxx_M3)
#include "DeviceSpecific_66AK2Gxx.h"
#endif
#ifdef _TDA3x
#include "DeviceSpecific_TDA3x.h"
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef _ETB
#include "ETBInterface.h"

ETBHandle *pETBHandle = NULL;
void transferEtbData(ETBStatus etbStatus, uint32_t* pBuffer);
void writeDcmFile(STMHandle* pSTMHandle, uint8_t wrapped);
#endif

#define NUM_ELEMENTS 4
unsigned short rawData[NUM_ELEMENTS] = {1,2,3,4};

#ifdef __cplusplus
extern "C" {
#endif
void STMLog_CallBack(const char * funcName, eSTM_STATUS stmStatus)
{
    printf("STMLog_ErrHandler:: Error in function %s(), %d\n", funcName, stmStatus);
}
#ifdef __cplusplus
}
#endif


int main()
{
#ifdef _ETB
    // ETB parameters
    eETB_Error        etbRet;
    ETB_errorCallback pETBErrCallBack = NULL;
    ETBStatus         etbStatus;
    uint32_t          etbWidth;
    uint32_t          *pEtbBuffer = NULL;
#endif
    // The following parameters are required to initialize STM
    STMHandle * pSTMHandle;
    STMBufObj * pSTMBufInfo = NULL;
    STMConfigObj STMConfigInfo;

    int STMChn;    //STM channel used by example
    int i=0;
    char * pFileStr = "SimpleSTMExample";

#ifdef _ETB

	// Open ETB module
	etbRet = ETB_open(pETBErrCallBack, eETB_Circular, SYS_ETB, &pETBHandle, &etbWidth);
	if(etbRet != eETB_Success) {
		printf("Error opening ETB\n");
		return 0;
	}

	// Enable ETB receiver
	etbRet = ETB_enable(pETBHandle, 0);
	if(etbRet != eETB_Success) {
		printf("Error enabling ETB\n");
		return 0;
	}
#endif

    // Open STMLib in normal mode i.e. pSTMBufInfo = NULL (verses CIO mode)
	//
	// STMConfigInfo.xmit_printf_mode - STMLib can transport STMXport_printf and STMXport_logMsg messages
	//                                  as complete strings (eSend_char_stream) or compressed (eSend_optimized).
	//                                  Sending messages as complete strings takes ~twice the STM bandwidth as
	//                                  compressed messages. Compressed messages send just the pointer to the
	//                                  format string, but the pointer must be a const char pointer, and cannot be
	//                                  modified at run time.
    // STMConfigInfo.optimize_strings - Only used if STMConfigInfo.xmit_printf_mode is eSend_optimized
	//                                  if false %s printf format character strings are transported by
	//                                  value (the complete string). If true, only the pointer to the string is
	//                                  transported, but the pointer must be a const char pointer, and can not
	//                                  be modified at run time.. Setting to false is useful for cases where strings
    //                                  are generated dynamically at run time.

    STMConfigInfo.STM_CntlBaseAddr = STM_CONFIG_BASE_ADDR;
#ifdef _ETB
    STMConfigInfo.xmit_printf_mode = eSend_optimized;
    STMConfigInfo.optimize_strings =  true;
#else
    STMConfigInfo.xmit_printf_mode = eSend_char_stream;     // or eSend_optimized
#endif
    STMConfigInfo.STM_XportBaseAddr = STM_XPORT_BASE_ADDR;
    STMConfigInfo.STM_ChannelResolution = STM_CHAN_RESOLUTION;
    STMConfigInfo.pCallBack = STMLog_CallBack;

    pSTMHandle = STMXport_open(pSTMBufInfo, &STMConfigInfo );
	//If not opened, exit
    if ( pSTMHandle == 0 )
		exit(0);

#ifdef _ETB
    //Setup routing to get STM data to the ETB
    Config_STMData_Routing();

    // In some cases the STM module can transport a SYNC message
    // thus the reason the ETB must be setup first.
    Config_STM_for_ETB(pSTMHandle);
#endif
	// STM channel - user can define own channels as their application requires.
    // Note: Threads, tasks and ISRs must use unique channels for STM's
    // interleaved message support to work properly.
    STMChn = 10;

    //Loop through and demonstrate different STM API examples
	while (i < 10) {

		STMXport_printf(pSTMHandle, STMChn, "printf: loop count is %d", i);

		//logMsg and logMsgN functions can only pass ints but will be much more efficient than a printf

		STMXport_logMsg0(pSTMHandle, STMChn, "logMsg0: Log message with no arguments");

        STMXport_logMsg1(pSTMHandle, STMChn, "logMsg1: Log message with one int arg arg1=%d", 1);

        STMXport_logMsg2(pSTMHandle, STMChn, "logMsg2: Log message with two int args arg1=%d arg2=%d", 1, 2 );

        STMXport_logMsg(pSTMHandle, STMChn, "logMsg: Log Message with more than two int arguments arg1=%d, arg2=%d arg3=%d", 1*i, 2*i, 3*i);

        STMXport_printf(pSTMHandle, STMChn, "%s::%s->Line Number %d, loop %d",pFileStr, __FUNCTION__,__LINE__, i );

        STMXport_logMsg0(pSTMHandle, STMChn, "logMsg0: Dumping 1 byte with putByte");
        STMXport_putShort (pSTMHandle, STMChn, 0x12);

        STMXport_logMsg0(pSTMHandle, STMChn, "logMsg0: Dumping 1 short with putShort");
        STMXport_putShort (pSTMHandle, STMChn, 0x3456);

        STMXport_logMsg0(pSTMHandle, STMChn, "logMsg0: Dumping 1 word with putWord");
        STMXport_putWord (pSTMHandle, STMChn, 0x789abcde);

        STMXport_logMsg0(pSTMHandle, STMChn, "logMsg0: Dumping 4 shorts with putBuf");
        STMXport_putBuf (pSTMHandle, STMChn, (void *)rawData, eShort, NUM_ELEMENTS);


        // STMXport_putMsg example, this will transport the whole message, while
        // STMXport_printf, STMXport_logMsg, and STMXport_logMsgN only transport
        // pointers to const stings if STMConfigInfo.xmit_printf_mode set to eSend_optimized.

        {

            int cnt = 0;
            char * myBuf;
            char myString[] = "End of loop %d";
            cnt = sizeof(myString)+10;
            myBuf = (char *)malloc(sizeof(myString) + 10);
            cnt = sprintf( myBuf, myString, i);
            //Must add one for string termination character ('\0').
            cnt++;
            STMXport_putMsg(pSTMHandle, STMChn, (char *)myBuf, cnt);
            free(myBuf);
        }

		i++;
	}


	STMXport_logMsg0(pSTMHandle, STMChn, "logMsg0: End of logging");

	STMXport_flush(pSTMHandle);

#ifdef _ETB

    // Disable ETB trace capture
	etbRet = ETB_disable(pETBHandle);
	if(etbRet != eETB_Success) {
		printf("Error disabling ETB (%d)\n", etbRet);
	}

    // Get ETB data
	// Check the ETB status
	etbRet= ETB_status(pETBHandle, &etbStatus);
	if(etbRet == eETB_Success) {
            transferEtbData(etbStatus, pEtbBuffer);
            writeDcmFile(pSTMHandle, etbStatus.isWrapped);
	}
    else {
		printf("Error getting ETB status (%d)\n", etbRet);
    }

	// All done
	etbRet  = ETB_close(pETBHandle);
	if(etbRet != eETB_Success)
	{
		printf("Error closing ETB (%d)\n", etbRet);
		return 0;
	}
    if(pEtbBuffer != NULL) {
        free(pEtbBuffer);
    }
#endif

	STMXport_close (pSTMHandle);

}

#ifdef _ETB
/*******************************************************************************
 * ETB Support Functions
 ******************************************************************************/

/*******************************************************************************
 * Write ETB Data to a bin file
 ******************************************************************************/
void transferEtbData(ETBStatus etbStatus, uint32_t* pBuffer)
{
    eETB_Error  etbRet;
	uint32_t retSize=0;

    if(etbStatus.canRead == 1) {
		if(etbStatus.isWrapped == 1)
			printf ("ETB is wrapped; ETB words = %d\n",
                                                    etbStatus.availableWords);
		else
			printf ("ETB is not wrapped; ETB words = %d\n",
                                                    etbStatus.availableWords);
        /* ETB words are 32 bit long */
		pBuffer = (uint32_t*) malloc(etbStatus.availableWords * 4);
		if(pBuffer) {
			etbRet = ETB_read(pETBHandle, pBuffer, etbStatus.availableWords, 0,
                              etbStatus.availableWords, &retSize);
			if(etbRet != eETB_Success) {
				printf("Error reading ETB data\n");
				return;
			}
		}
		else {
			printf("Error allocating memory for ETB buffer (%d bytes)\n",
                                            (etbStatus.availableWords * 4) );
		}
	}

	/* Transport ETB data */
	if(pBuffer)
	{
		/* This example uses JTAG debugger via CIO to transport data to host PC
		 *  An app can deploy any other transport mechanism to move the ETB
         *  buffer to the PC
         */
        char * pFileName = "C:\\temp\\STM_etbdata.bin";
        FILE* fp = fopen(pFileName, "wb");
		if(fp) {
			size_t fret;
            uint32_t sz = 0;
			uint32_t i = 1;
   			char *le = (char *) &i;

			while(sz < retSize) {
				uint32_t etbword = *(pBuffer+sz);
				if(le[0] != 1) //Big endian
				 	etbword = BYTE_SWAP32(etbword);

				fret = fwrite((void*) &etbword, 4, 1, fp);
                if ( fret < 1 ) {
                    printf("Error writing data to  - %s \n", pFileName);
                    return;
                }
				sz++;
			}

			printf("Successfully transported ETB data - %s\n", pFileName);

            fclose(fp);
		}
        else {
            printf("Error opening file - %s\n", pFileName);
        }
	}
    return;
}

/*******************************************************************************
 * Write DCM data required to decode the ETB captured data
 ******************************************************************************/
void writeDcmFile(STMHandle* pSTMHandle, uint8_t wrapped)
{
    char *file_name = "C:\\temp\\STM_etbdata.dcm";
    FILE *fp = fopen(file_name, "w");
    if(fp) {

        STM_DCM_InfoObj  DCM_Info;
        STMXport_getDCMInfo(pSTMHandle, &DCM_Info);

        fprintf(fp, "STM_data_flip=%d\n", DCM_Info.stm_data_flip);
        fprintf(fp, "STM_Buffer_Wrapped=%d\n", wrapped);
        fprintf(fp, "HEAD_Present_0=%d\n", DCM_Info.atb_head_present_0);
        fprintf(fp, "HEAD_Pointer_0=%d\n", DCM_Info.atb_head_pointer_0);
        fprintf(fp, "HEAD_Present_1=%d\n", DCM_Info.atb_head_present_1);
        fprintf(fp, "HEAD_Pointer_1=%d\n", DCM_Info.atb_head_pointer_1);
        fprintf(fp, "STM_STP_Version=%d\n",DCM_Info.stpVersion);
        fprintf(fp, "TWP_Protocol=%d\n", ETB_USING_TWP);

        fclose(fp);
        printf("Writing to %s complete.\n", file_name);
    }
    else {
        printf("Error opening %s for writing\n", file_name);
    }

    return;
}

#endif
