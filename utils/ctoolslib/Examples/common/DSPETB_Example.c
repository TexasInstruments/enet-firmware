
/****************************************************************************
CToolsLib - Trace Example

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
****************************************************************************/

/*! \file TraceExample.c
    \version 1.0
*/
#define USE_TEND 1
#if !defined(TCI6488) && !defined(TCI6486)
#define USE_TI_ETB 1
#endif

#ifdef _DRA7xx
#include "DeviceSpecific_DRA7xx.h"
#endif
#ifdef _TDA3x
#include "DeviceSpecific_TDA3x.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "DSPTraceExport.h"
#include "ETBInterface.h"

/*******************************************************************************
 *               INSTRUCTIONS FOR RUNNING THIS EXAMPLE CODE
 *******************************************************************************
 *  The system events and configuration is setup to run on DSP core 0.
 
 *  - Trace, ETB based, data is collected and stored in c:\temp\etbdata.bin
 *  - bin2tdf utility is required, located in:
 *    -> cd <CCS install dir>\ccs_base\emulation\analysis\bin
 *  - To convert binary trace data captured by the ETB, use the following
 *     command line:
 *    -> bin2tdf -bin C:/temp/etbdata.bin -app \CToolsLib\Examples\bin\
 *       dsptrace_tietb_d_corepacN.c6670.out -procid 66x -sirev 1 -rcvr ETB
 *       -output C:/temp/myTrace.tdf -dcmfile c:/temp/foo.dcm
 *       -sourcepaths \CToolsLib\Examples\C667x\DSPTrace_TIETB_Ex_CorePacN
 *  - The myTrace.tdf file may be opened from the CCS menu Tools->
 *     Trace Analyzer->Open Trace File In New View...
 *
 *******************************************************************************
 ******************************************************************************/


#define AET_JOB 1

#ifdef AET_JOB	

#include "aet.h" 

/* jobParams Structures for the Start and End trace jobs */
AET_jobParams StartTraceParams;
AET_jobParams EndTraceParams;

#endif

void doneFunction()
{
	volatile int dummy1 =10, dummy2=0;
	
	dummy2 = dummy1 - 1;
	
	while(dummy2 > 1)
	{
		dummy2--; 
	}
	
}

void runFunction()
{
	volatile int dummy1 =0, dummy2=0;
	dummy1++;
	
	dummy2 += dummy1 + 20;
}

#define BYTE_SWAP32(n) \
	( ((((uint32_t) n) << 24) & 0xFF000000) |	\
	  ((((uint32_t) n) <<  8) & 0x00FF0000) |	\
	  ((((uint32_t) n) >>  8) & 0x0000FF00) |	\
	  ((((uint32_t) n) >> 24) & 0x000000FF) )


/****************************************************************************/     
/* Main Function                                                          */     
/****************************************************************************/     
void main()
{
	eETB_Error  etbRet;
	ETB_errorCallback pETBErrCallBack =0;
	uint8_t coreID =0;
	ETBHandle* pETBHandle=0;
	ETBStatus etbStatus;
	uint32_t etbWidth, retSize=0;
	uint32_t *pBuffer=0;
		
	eDSPTrace_Error dspRet;
	DSPTrace_errorCallback pDSPErrorCallBack=0;
	DSPTraceHandle* pDSPHandle=0;

#ifdef ETB_USING_TWP
	char *dcmfile = "C:\\temp\\DSP_etbdata.dcm";
	FILE *pDcmFile;
#endif

#ifdef AET_JOB
    /* NOTE: THESE VARIABLES MUST BE DEFINED IN THE MAIN, NOT GLOBAL, FOR THE TRACE TO
     *        WORK CORRECTLY
     */
    /* jobIndex holders for the Start and End trace jobs */
    AET_jobIndex startJob;

#if !USE_TEND
    AET_jobIndex endJob;
#endif

#endif

	/*** Setup ETB receiver ***/
	/* Open ETB module */
#if !USE_TI_ETB
	etbRet = ETB_open(pETBErrCallBack, eETB_Circular, coreID, &pETBHandle, &etbWidth);
#else
    etbRet = ETB_open(pETBErrCallBack, eETB_TI_Mode, coreID, &pETBHandle, &etbWidth);
#endif	
    if(etbRet != eETB_Success)
	{
		printf("Error %d opening ETB\n", etbRet);
		return;
	}
		
	/*** Setup Trace Export ***/
	/* Open DSP Trace export module */
	dspRet = DSPTrace_open( pDSPErrorCallBack, &pDSPHandle);
	if(dspRet != eDSPTrace_Success)
	{
		printf("Error %d opening DSP Trace Export block\n", dspRet);
		return;
	}
    /* Setup trace export clock to FCLK/3 */
#if  !defined(_DRA7xx) && !defined(_TDA3x)
	dspRet= DSPTrace_setClock(pDSPHandle, 3);
#endif
	if(dspRet != eDSPTrace_Success)
	{
		printf("Error %d setting up DSP trace export clock\n", dspRet);
		return;
	}
	
	/* Enable trace funnel if applicable for device */
#ifdef CONFIG_TRACE_FUNNEL
    Config_DSPData_Routing(DNUM);
#endif

	dspRet= DSPTrace_enable(pDSPHandle, 0, 0);
    if(dspRet != eDSPTrace_Success)
	{
		printf("Error %d enabling DSP trace export\n", dspRet);
		return;
	}
	
	/* Enable ETB receiver */
	etbRet = ETB_enable(pETBHandle, 0);
	if(etbRet != eETB_Success)
	{
		printf("Error %d enabling ETB\n", etbRet);
		return;
	}
    
	/* ******************************* IMP. NOTE ****************************/
	/* AETLIB should be used to setup Trace triggers before enabling DSP Trace.*/
	/* ******************************* IMP. NOTE ****************************/
    /* Set up trace source to generate trace */
		
#ifdef AET_JOB	
	
	StartTraceParams = AET_JOBPARAMS; /* Initialize Job Parameter Structure */
	StartTraceParams.programAddress = (uint32_t) &runFunction;
	StartTraceParams.traceTriggers = AET_TRACE_TIMING | AET_TRACE_PA;
	StartTraceParams.traceActive = AET_TRACE_ACTIVE;
 
	AET_init();
	
	/* Claim the AET resource */
	if (AET_claim())
	{
		printf("Error claiming AET resources\n");
		return;
	}

#if USE_TEND
    {
    /* ******************************* IMP. NOTE ****************************/
    /* DSPTrace_setState will not change the TEND state until after         */
    /* AET_claim is executed successfully. Must clear TEND if currently set.*/
    /* ******************************* IMP. NOTE ****************************/
        uint32_t traceState = 0;
        uint32_t new_traceState = DSPTRACE_CLR_TEND;
        DSPTrace_getState(pDSPHandle, &traceState);
        if ( (traceState & DSPTRACE_STATE_TEND) == DSPTRACE_STATE_TEND ){

            DSPTrace_setState(pDSPHandle, new_traceState);
        }    

        DSPTrace_getState(pDSPHandle, &traceState);
        if ( (traceState & DSPTRACE_STATE_TEND) == DSPTRACE_STATE_TEND )
            printf("TEND detected - BAD\n");
        else
            printf("TEND not detected - GOOD\n"); 
        
    }
#endif
	
	/* Set up the desired job */
	if (AET_setupJob(AET_JOB_START_STOP_TRACE_ON_PC, &StartTraceParams))
	{
		printf("Error setting up AET resources\n");
		return;
	}

	startJob = StartTraceParams.jobIndex;

	printf("AET start job index is %d\n", startJob);

#if !USE_TEND   
    EndTraceParams = AET_JOBPARAMS; 
    EndTraceParams.programAddress = (uint32_t) &doneFunction;
	EndTraceParams.traceTriggers = AET_TRACE_STOP_PC;
	EndTraceParams.traceActive = AET_TRACE_INACTIVE;

  	if (AET_setupJob(AET_JOB_START_STOP_TRACE_ON_PC, &EndTraceParams))
		return;
    
    endJob = EndTraceParams.jobIndex;

    printf("The AET stop job index is %d\n", endJob );
#endif
#endif

    
#ifdef AET_JOB
	/* Enable AET */
	if (AET_enable())
	{
		printf("Error enabling AET \n");
		return;
	}
#endif

	/***** Now the app runs and generates trace data ****/
	runFunction();
    doneFunction();

#if USE_TEND
    {
    /* ******************************* IMP. NOTE ****************************/
    /* Setting TEND will terminate Trace gracefully if you don't have an    */
    /* End Trace Job or an End All Trace job.                               */
    /* ******************************* IMP. NOTE ****************************/
        uint32_t traceState = 0;
        uint32_t new_traceState = DSPTRACE_SET_TEND;
        DSPTrace_getState(pDSPHandle, &traceState);
        if ( (traceState & DSPTRACE_STATE_TEND) != DSPTRACE_STATE_TEND ){

            DSPTrace_setState(pDSPHandle, new_traceState);
        }    
        
    }
#endif

#if USE_TI_ETB
    /* Flush the DTF (Trace Formatter) output and the ETB input to guarantee all
     *  of the data will get read out of the FIFO.
     */
    etbRet= ETB_flush(pETBHandle);
    if(etbRet != eETB_Success)
    {
        printf("Error flushing ETB %d\n", etbRet);
        return;
    }
#endif

    /* Now disable trace capture - ETB receiver */
	etbRet = ETB_disable(pETBHandle);
	if(etbRet != eETB_Success)
	{
		printf("Error disabling ETB\n");
		return;
	}

    /* Disable DSP Trace Export */
	dspRet= DSPTrace_disable(pDSPHandle);
	if(dspRet != eDSPTrace_Success)
	{
		printf("Error disabling DSP trace export\n");
		return;
	}

    /*** Get ETB data ***/
	/* Check the ETB status */
	etbRet= ETB_status(pETBHandle, &etbStatus);
	if(etbRet != eETB_Success)
	{
		printf("Error getting ETB status\n");
		return;
	}
	
	if(etbStatus.canRead == 1)
	{
		if(etbStatus.isWrapped == 1)
			printf ("ETB is wrapped; ETB words = %d\n", etbStatus.availableWords);
		else
			printf ("ETB is not wrapped; ETB words = %d\n", etbStatus.availableWords);
			
		pBuffer = (uint32_t*) malloc(etbStatus.availableWords * 4); // ETB words are 32 bit long

		if(pBuffer)
		{
			etbRet = ETB_read(pETBHandle, pBuffer, etbStatus.availableWords, 0, etbStatus.availableWords, &retSize);
			if(etbRet != eETB_Success)
			{
				printf("Error reading ETB data\n");
				return;
			}
		}
	}
	
	/* Transport ETB data */	
	if(pBuffer)
	{
		/* this example uses JTAG debugger via CIO to transport data to host PC */
		/* An app can deploy any other transport mechanism to move the ETB buffer to the PC */
		FILE* fp = fopen("C:\\temp\\etbdata.bin", "wb");
		if(fp)
		{
			uint32_t sz = 0;
			uint32_t i = 1;
			size_t fret;
   			char *le = (char *) &i;
			
			while(sz < retSize)
			{
				uint32_t etbword = *(pBuffer+sz);
				if(le[0] != 1) //Big endian
				 	etbword = BYTE_SWAP32(etbword);
				 	
				fret = fwrite((void*) &etbword, 4, 1, fp);

				if ( fret < 1 ) {
					printf("Error writing data to  - %s \n", "C:\\temp\\etbdata.bin");
					return;
				}

				sz++;
			}
			
			printf("Successfully transported ETB data\n");
		}	
	}


#ifdef ETB_USING_TWP
	/* Create a .dcm file with the TWP flag set for decoding. */
	pDcmFile = fopen(dcmfile, "w");
    if(pDcmFile) {
        fprintf(pDcmFile, "TWP_Protocol=%d\n", ETB_USING_TWP);
        fclose(pDcmFile);
        printf("Writing to %s complete.\n", dcmfile);
    }
    else {
        printf("Error opening %s for writing\n", dcmfile);
    }
#endif

	/*** Now we are done and close all the handles **/
	etbRet  = ETB_close(pETBHandle);
	if(etbRet != eETB_Success)
	{
		printf("Error closing ETB\n");
		return;
	}

	
	dspRet= DSPTrace_close(pDSPHandle);
	if(dspRet != eDSPTrace_Success)
	{
		printf("Error closing DSP trace export module\n");
		return;
	}
	
#ifdef AET_JOB
	AET_releaseJob(startJob);
#if !USE_TEND
    AET_releaseJob(endJob);
#endif
	AET_release();	
#endif
	
	
}

