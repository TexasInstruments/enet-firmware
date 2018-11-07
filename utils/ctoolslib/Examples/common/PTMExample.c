/*******************************************************************************
CToolsLib - ETM Trace Example

Copyright (c) 2009-2010 Texas Instruments Inc. (www.ti.com)
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
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "ETBInterface.h"
#include "etm_lib.h"
#ifdef _DRA7xx
#include "DeviceSpecific_DRA7xx.h"
#endif
#ifdef _OMAP5430
#include "DeviceSpecific_OMAP5430.h"
#endif
#ifdef C66AK2Hxx_CSSTM_ETB
#include "DeviceSpecific_C66AK2Hxx.h"
#endif
#ifdef _66AK2Gxx_CSSTM_ETB
#include "DeviceSpecific_66AK2Gxx.h"
#endif

/*******************************************************************************
 *               INSTRUCTIONS FOR RUNNING THIS EXAMPLE CODE
 *******************************************************************************
 * To enable the CCS target only trace configuration/collection, make sure the
 *  _ETB define (2) is uncommented to collect and transfer the trace data to the
 *  host. Details on this configuration:
 *  - Trace, ETB based, data is collected and stored in c:\temp\ETM_etbdata.bin
 *  - bin2tdf utility is required, located in:
 *    -> <CCS install dir>\ccs_base\emulation\analysis\bin
 *  - The following command line applies for this example running on a cortexA8:
 *    -> bin2tdf -bin c:\temp\ETM_etbdata.bin -app CToolsLib\Examples\bin\
 *       etm_csetb_d_A8.c6a816x.out -sirev 3 -rcvr ETB -dcmfile c:\temp\
 *       ETM_etbdata.dcm -procid cortexa8 -sourcepaths CToolsLib\Examples\
 *       C6A816x\ETM_Ex_A8 -output c:\temp\etmTraceEx.tdf
 *  - The .tdf file can not get opened from the CCS menu Tools->Trace Analyzer->
 *     Open Trace File In New View...
 *
 *******************************************************************************
 ******************************************************************************/

/***************************************
 * DEFINITIONS FOR EXAMPLE CODE TYPE
 **************************************/
#define _ETB             /* (2) Collect/transfer ETB buffer to host */

#ifdef _ETB
    ETBHandle *pETBHandle = NULL;
    void transferEtbData(ETBStatus etbStatus);
    void writeDcmFile(struct etm_dcm *p_dcm);
#endif

/*******************************************************************************
 * FUNCTIONS AND PARAMETERS USED FOR EXAMPLES
 */
uint32_t globalCntr[10] = {0, };
uint32_t globalMarker = 0;

void endFunction()
{
	int dummy1 =10, dummy2=0;
	unsigned int condition = 0x2AA;
	
	dummy2 = dummy1 - 1;
    globalMarker = 10;

    if(condition & (1 << dummy2))
    {
    	globalCntr[dummy2]++;
    }
    dummy2--;

    if(condition & (1 << dummy2))
	{
		globalCntr[dummy2]++;
	}
    dummy2--;

    if(condition & (1 << dummy2))
	{
		globalCntr[dummy2]++;
	}
    dummy2--;

    if(condition & (1 << dummy2))
	{
		globalCntr[dummy2]++;
	}
    dummy2--;

    if(condition & (1 << dummy2))
	{
		globalCntr[dummy2]++;
	}
    dummy2--;

    if(condition & (1 << dummy2))
	{
		globalCntr[dummy2]++;
	}
    dummy2--;

    if(condition & (1 << dummy2))
	{
		globalCntr[dummy2]++;
	}
    dummy2--;

    if(condition & (1 << dummy2))
	{
		globalCntr[dummy2]++;
	}
    dummy2--;

    if(condition & (1 << dummy2))
	{
		globalCntr[dummy2]++;
	}
    dummy2--;
    
    if(condition & (1 << dummy2))
	{
		globalCntr[dummy2]++;
	}

    globalMarker = 20;
    return;
}

void startFunction()
{
	int dummy1 =0, dummy2=0;
	dummy1++;
	
	dummy2 += dummy1 + 20;
    
    globalCntr[8]++;
    
    return;
}

/***************************************
 * Functions setup explicitly for using
 *  their addresses for start/end
 *  trace ranges.
 **************************************/
void startTrace()
{
    volatile uint32_t start;  /* Keep from optimizing out function */
    start;
}
void endTrace()
{
    volatile uint32_t end;    /* Keep from optimizing out function */
    end;
}

/*******************************************************************************
 *  ERROR HANDLER
 ******************************************************************************/     
void etm_errhandler(etmhandle_t handle, const char *func, enum etm_error err)
{
    enum etm_error retVal = ETM_SUCCESS;
    
    printf("ETMLib failure %d in function: %s \n", err, func );
    
#ifdef _ETB
    if(NULL != pETBHandle) {
        ETB_disable(pETBHandle);
        ETB_close(pETBHandle);
    }
#endif
    if(NULL != handle) {
        retVal = etm_close(&handle);
    }
          
    if (retVal != ETM_SUCCESS) {
        exit(-2);
    }
    else {
        exit (-3);
    }
}


#define BYTE_SWAP32(n) \
    ( ((((uint32_t) n) << 24) & 0xFF000000) |   \
      ((((uint32_t) n) <<  8) & 0x00FF0000) |   \
      ((((uint32_t) n) >>  8) & 0x0000FF00) |   \
      ((((uint32_t) n) >> 24) & 0x000000FF) )

/*********************************************************************
 * These defines are used to enable or disable running the examples in
 *  the code below. This allows showing the results for specific
 *  examples in the trace output.
 *
 * NOTE: There are dependencies on the configuration of some examples
 *        for others to operate correctly. Read the comments in the
 *        code to check for dependencies.
 ********************************************************************/
#define USE_EXAMPLE_PROGRAM_TRACE
#define USE_EXAMPLE_START_END_TRACE
#define USE_EXAMPLE_INSTRUCTION_RANGE

/*******************************************************************************
 * Main Function
 ******************************************************************************/
void main()
{
#ifdef _ETB
    /* ETB parameters */
    eETB_Error        etbRet;
    ETB_errorCallback pETBErrCallBack = NULL;
    ETBStatus         etbStatus;
    uint32_t          etbWidth;
    struct etm_dcm    dcm;
#endif
    /* ETM parameters */
    etmhandle_t              etm_handle = NULL;
    struct etm_config        ex_etm_config; 
	enum etm_error           status;
    struct etm_trace_control tcontrol;
    struct etm_inst_range    range;
 //   struct etm_data_address  data_addr;
 //   struct etm_data_range    data_range;
    uint32_t  libMajor;
    uint32_t  libMinor;
    uint32_t  etmMajor;
    uint32_t  etmMinor;

#ifdef _ETB
	/**************************************************************************
	 *** Setup ETB receiver ***
     */
    
    /******************************************
     * INITIALIZATION FOR SPECIFIC TARGET BOARD
     ******************************************/
    /* Specifics located in the DeviceSpecific.h file.
     * TCI6616:
     *  - The initialization for the PSC (Power Sleep Control) registers occur
     *  from a custom Entry that is run before the standard _c_int00 located in
     *  the init.asm file.
     *  - The config function below is empty, nothing to configure.
     * C6A816x:
     *  - The config function below sets up the trace funnel for ETM to ETB.
     */
    etm_config_for_etb();

	/* Open ETB module */
#if defined(C66AK2Hxx_CSSTM_ETB) || defined(_66AK2Gxx_CSSTM_ETB) //Keystone2
    etbRet = ETB_open(pETBErrCallBack, eETB_Circular, ARM_ETB, &pETBHandle, &etbWidth);
#else //All other devices
    etbRet = ETB_open(pETBErrCallBack, eETB_Circular, SYS_ETB, &pETBHandle, &etbWidth);
#endif
	if(etbRet != eETB_Success) {
		printf("Error opening ETB\n");
		return;
	}

	/* Enable ETB receiver */
	etbRet = ETB_enable(pETBHandle, 0);
	if(etbRet != eETB_Success) {
		printf("Error enabling ETB\n");
		return;
	}
#endif
	/**************************************************************************
     * Open ETM library for app usage
     * ETM library must be opened by the CPU before using ETM instrumentation
     **************************************************************************/
	ex_etm_config.core_id = 0;    //A15 core id with the MPU cluster
    ex_etm_config.base_address = PTM0_BASE_ADDRESS + (0x1000 * ex_etm_config.core_id);
    ex_etm_config.p_callback   = etm_errhandler;
    status = etm_open(&etm_handle, &ex_etm_config);
    if((etm_handle == 0) || (status != ETM_SUCCESS)) {
        printf("Error opening ETM library\n");
        exit(1);
    }
    
    /* Provide Version Information */
    status = etm_get_version(etm_handle,
                             &libMajor, &libMinor, &etmMajor, &etmMinor);
    if(status == ETM_SUCCESS)
    {
        printf("\n *** ETM API Library v%d.%d, ETM v%d.%d\n\n",
                    libMajor, libMinor, etmMajor, etmMinor);
    }
    else
    {
        printf("Error etm_get_version (%d)\n", status);
    }
    
    /***************************************
     * INITIAL SETTINGS FOR TRACE CONTROL
     *  STRUCTURE, PASSED AS PARAMETER WHEN
     *  TRACING IS ENABLED
     * - ETM_TRACE_ALL - Trace both PC and
     *                   Data instructions.
     **************************************/
    tcontrol.type          = ETM_TRACE_PROGRAM_ONLY;
    tcontrol.cycle_accurate_tracing = false;
    
    /* Currently, when cycle accurate tracing option is selected, the trace
     *  output is not completing. This problem will get investigated further
     *  when work on the decoder occurs for the A9 implementation.
     */
     
#ifdef USE_EXAMPLE_PROGRAM_TRACE
	/**************************************************************************
     * EXAMPLE 1: TRACE ALL
     *  API call to trace all instructions and data, this will also trace the
     *   enable API function as soon as tracing is enabled.
     *  The output from this example can be used to validate the remaining
     *   examples.
     **************************************************************************/
    status = etm_config_trace_on(etm_handle);
    /* The error handler is taking care of reporting the error, closing, and
     *  exiting. Checking the return status code is left in place to show
     *  examples of when an error handler is not used.
     */
    if(status != ETM_SUCCESS) {
        printf("EX 1: Error etm_config_trace_on (%d)\n", status);
    }
    
    status = etm_enable_trace(etm_handle, &tcontrol);
    if(status != ETM_SUCCESS) {
        printf("EX1: Error etm_enable_trace (%d)\n", status);
    }
    
		/***********************************
		 * APPLICATION CODE
		 **********************************/
		startFunction();
		globalCntr[6]++;
		endFunction();
		/**********************************/

    status = etm_stop_trace(etm_handle);
    if(status != ETM_SUCCESS) {
        printf("EX1: Error etm_stop_trace (%d)\n", status);
    }
#endif
#ifdef USE_EXAMPLE_START_END_TRACE
	/**************************************************************************
     * EXAMPLE 2: START/END TRACE
     *  API call to trace all instructions and data from the beginning of the
     *   startFunction call to the beginning of the endFunction call.
     *
     * NOTE: When the instruction start and instruction end APIs are used, they
     *       enable the Trace start/stop resource to control the ability to
     *       enable and disable tracing. If the start instruction is not
     *       configured, or configured to an address that is not executed, then
     *       tracing will not get enabled.
     *
     **************************************************************************/
    status = etm_config_clear(etm_handle);
    if(status != ETM_SUCCESS) {
        printf("EX 2: Error etm_config_clear (%d)\n", status);
    }

    status = etm_config_inst_start(etm_handle, (uint32_t)&startFunction);
    if(status != ETM_SUCCESS) {
        printf("EX 2: Error etm_config_inst_start (%d)\n", status);
    }
    status = etm_config_inst_end(etm_handle, (uint32_t)&endFunction);
    if(status != ETM_SUCCESS) {
        printf("EX 2: Error etm_config_inst_end (%d)\n", status);
    }
    
    status = etm_enable_trace(etm_handle, &tcontrol);
    if(status != ETM_SUCCESS) {
        printf("EX 2: Error etm_enable_trace (%d)\n", status);
    }

	/***********************************
     * APPLICATION CODE
     **********************************/
    startFunction();
    globalCntr[6]++;
    endFunction();
    /**********************************/

    status = etm_stop_trace(etm_handle);
    if(status != ETM_SUCCESS) {
        printf("EX 2: Error etm_stop_trace (%d)\n", status);
    }
#endif
#ifdef USE_EXAMPLE_INSTRUCTION_RANGE
	/**************************************************************************
     * EXAMPLE 3: INSTRUCTION RANGE
     *  Tracing a section of a specific function.
     *  The Data Address examples below require this range to get set for them
     *   to work correctly, always include the etm_config_inst_range call.
     *
     **************************************************************************/
    status = etm_config_clear(etm_handle);
    if(status != ETM_SUCCESS) {
        printf("EX 3: Error etm_config_clear (%d)\n", status);
    }

    range.low_address  = (uint32_t)&endFunction+0x48;
    range.high_address = ((uint32_t)&endFunction)+0xF0;
    range.type         = ETM_INCLUDE;
    status = etm_config_inst_range(etm_handle, &range);
    if(status != ETM_SUCCESS) {
        printf("EX 3: Error etm_config_inst_range (%d)\n", status);
    }

    status = etm_enable_trace(etm_handle, &tcontrol);
    if(status != ETM_SUCCESS) {
        printf("EX 3: Error etm_enable_trace (%d)\n", status);
    }

	/***********************************
     * APPLICATION CODE
     **********************************/
    startFunction();
    globalCntr[6]++;
    endFunction();
    /**********************************/

    status = etm_stop_trace(etm_handle);
    if(status != ETM_SUCCESS) {
        printf("EX 3: Error etm_stop_trace (%d)\n", status);
    }
#endif
    
#ifdef _ETB
    /**************************************************************************
     * Disable trace capture - ETB receiver
     */
	etbRet = ETB_disable(pETBHandle);
	if(etbRet != eETB_Success) {
		printf("Error disabling ETB (%d)\n", etbRet);
	}
   	
    /*** Get ETB data ***/
	/* Check the ETB status */
	etbRet= ETB_status(pETBHandle, &etbStatus);
	if(etbRet == eETB_Success) {
        dcm.buffer_wrapped = etbStatus.isWrapped;
        dcm.sample_size    = etbStatus.availableWords * 4;
        status = etm_get_dcm(etm_handle, &dcm);
        if(status == ETM_SUCCESS) {
            transferEtbData(etbStatus);
            writeDcmFile(&dcm);
        }
        else {
            printf("EX: Error collecting DCM parameters (%d)\n", status);
        }
	}
    else {
		printf("Error getting ETB status (%d)\n", etbRet);
    }
	
	/*** Now we are done and close all the handles **/
	etbRet  = ETB_close(pETBHandle);
	if(etbRet != eETB_Success)
	{
		printf("Error closing ETB (%d)\n", etbRet);
	}
#endif
     /*********************************************************************
      * ETM library must be closed before app terminates
      *********************************************************************/
    status = etm_close(&etm_handle);
    if(status != ETM_SUCCESS)
    {
        printf("Error closing ETM (%d)\n", status);
    }

    return;
}

#ifdef _ETB
/*******************************************************************************
 * 
 ******************************************************************************/     
void transferEtbData(ETBStatus etbStatus)
{
    eETB_Error  etbRet;
	uint32_t retSize=0;
    uint32_t* pBuffer = NULL;
	
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
            printf("Memory allocation for ETB collection buffer failed\n");
        }
	}
	
    /* Transport ETB data */	
    if(pBuffer)
    {
        /* This example uses JTAG debugger via CIO to transport data to host PC
         *  An app can deploy any other transport mechanism to move the ETB
         *  buffer to the PC
         */
        char * pFileName = "C:\\temp\\ETM_etbdata.bin";
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
 * 
 ******************************************************************************/     
void writeDcmFile(struct etm_dcm *p_dcm)
{
    char *file_name = "C:\\temp\\ETM_etbdata.dcm";
    FILE *fp = fopen(file_name, "w");
    if(fp) {
        fprintf(fp, "TargetMemoryAccessSize=2\n");
        fprintf(fp, "TargetEndianity=%d\n", p_dcm->endian);
        if(p_dcm->target_arm_mode == true) {
            fprintf(fp, "TargetIsArmMode=1\n");
            fprintf(fp, "TargetIsThumbMode=0\n");
        }
        else {
            fprintf(fp, "TargetIsArmMode=0\n");
            fprintf(fp, "TargetIsThumbMode=1\n");
        }
        fprintf(fp, "ETMPacketSize=%d\n", p_dcm->packet_size);
        fprintf(fp, "ETMID=%d\n", p_dcm->id_register);
        fprintf(fp, "ETMIsBufferWrapped=%d\n", p_dcm->buffer_wrapped);
        fprintf(fp, "ETMControl=%d\n", p_dcm->control_reg);
        fprintf(fp, "ETMConfigurationCode=%d\n", p_dcm->config_reg);
        fprintf(fp, "ETMSyncFreq=%d\n", p_dcm->sync_freq_reg);
        fprintf(fp, "ETMSignalSize=%d\n", p_dcm->signal_size);
        fprintf(fp, "BufferSampleSize=%d\n", p_dcm->sample_size);
        fprintf(fp, "CycleModeDelta=0\n");
        fprintf(fp, "TWP_Protocol=1\n");
        
        fclose(fp);
        printf("Writing to %s complete.\n", file_name);
    }
    else {
        printf("Error opening %s for writing\n", file_name);
    }
    
    return;
}

#endif

