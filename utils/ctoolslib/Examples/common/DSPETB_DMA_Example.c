/****************************************************************************
CToolsLib - Trace Example

Copyright (c) 2009-2012 Texas Instruments Inc. (www.ti.com)
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
#include <stdio.h>
#include <stdlib.h>
#include <c6x.h>

#include "DSPTraceExport.h"
#include "ETBInterface.h"
#include "ETBAddr.h"
#include "edma_dev-c66xx.h"
#include "aet.h"

#ifdef C6670
#include "DeviceSpecific_C6670.h"
#endif

#ifdef C6678
#include "DeviceSpecific_C6678.h"
#endif

#ifdef C6657
#include "DeviceSpecific_C6657.h"
#endif

#ifdef C66AK2Hxx
#include "DeviceSpecific_C66AK2Hxx.h"
#endif

#ifdef _66AK2Gxx
#include "DeviceSpecific_66AK2Gxx.h"
#endif

/*******************************************************************************
 *               INSTRUCTIONS FOR RUNNING THIS EXAMPLE CODE
 *******************************************************************************
 *  The system events and configuration is setup to run on DSP core 0.
 
 *  - Trace, ETB based, data is collected and stored in c:\temp\DSP_etbdata.bin
 *  - bin2tdf utility is required, located in:
 *    -> cd <CCS install dir>\ccs_base\emulation\analysis\bin
 *  - To convert binary trace data captured by the ETB, use the following
 *     command line:
 *    -> bin2tdf -bin C:/temp/DSP_etbdata.bin -app \CToolsLib\Examples\bin\
 *       dsptrace_tietb-edma_d_corepacN.c6670.out -procid 66x -sirev 1 -rcvr ETB
 *       -output C:/temp/myTrace.tdf -dcmfile c:/temp/foo.dcm
 *       -sourcepaths \CToolsLib\Examples\C667x\DSPTrace_TIETB_EDMA_Ex_CorePacN
 *  - The myTrace.tdf file may be opened from the CCS menu Tools->
 *     Trace Analyzer->Open Trace File In New View...
 *
 *******************************************************************************
 ******************************************************************************/


/*! \file EDMA3 based TraceExample.c
    \version 1.0
*/
#define USE_TEND 1
#define EXTERNAL_BUFFER_WRAPPING 1

/* This definition sets the number of loops through the application section of code and
 *  will allow testing the 16kB buffer for a wrap vs. non-wrap condition. With the
 *  value set to 20, the drain buffer should contain around 3683 words.
 */
#if EXTERNAL_BUFFER_WRAPPING
  #define N_APP_FUNCTION_RUNS 100
#else
  #define N_APP_FUNCTION_RUNS 20
#endif

/* jobParams Structures for the Start and End trace jobs */
AET_jobParams StartTraceParams;
AET_jobParams EndTraceParams;

/*******************************************************************************
 * FUNCTIONS AND PARAMETERS USED FOR EXAMPLE APPLICATION
 */
uint32_t globalCntr[10] = {0, };
uint32_t globalMarker = 0;

void endFunction()
{
   volatile int dummy1 =10, dummy2=0;

   dummy2 = dummy1 - 1;
    globalMarker = 10;

    globalCntr[dummy2]++;
    dummy2--;
    globalCntr[dummy2]++;
    dummy2--;
    globalCntr[dummy2]++;
    dummy2--;
    globalCntr[dummy2]++;
    dummy2--;
    globalCntr[dummy2]++;
    dummy2--;
    globalCntr[dummy2]++;
    dummy2--;
    globalCntr[dummy2]++;
    dummy2--;
    globalCntr[dummy2]++;
    dummy2--;
    globalCntr[dummy2]++;
    dummy2--;
    globalCntr[dummy2]++;

    globalMarker = 20;
    return;
}

void startFunction()
{
   volatile int dummy1 =0, dummy2=0;
   dummy1++;

   dummy2 += dummy1 + 20;

    globalCntr[8]++;

    return;
}


void appFunction()
{
   volatile int dummy1 =100, dummy2=0;

   dummy2 = dummy1 - 1;

   while(dummy2 > 1)
   {
      dummy2--;
        if(dummy2 & 1)
        {
            endFunction();
        }
        if(dummy2 & 0x4)
        {
            startFunction();
        }
   }

}

void runFunction()
{
   volatile int dummy1 =0, dummy2=0;
   dummy1++;

   dummy2 += dummy1 + 20;
}

void doneFunction(void)
{
    volatile uint32_t temp;
}

#define BYTE_SWAP32(n) \
   ( ((((uint32_t) n) << 24) & 0xFF000000) | \
     ((((uint32_t) n) <<  8) & 0x00FF0000) | \
     ((((uint32_t) n) >>  8) & 0x0000FF00) | \
     ((((uint32_t) n) >> 24) & 0x000000FF) )

/* The _USE_MANUAL_READ_ definition is used to select whether the drain buffer
 *  is read using ETB_read function or manually read from the DMA status
 *  information returned from the ETB_flush_dma function.
 */
/*
#define _USE_MANUAL_READ_
*/
#define EDMA_DEST_ADDRESS  0x0c000000
#define EDMA_BFR_WORDS     0x1000   /* 0x4000 (16384) bytes */

#define TEMP_BFR_READ_SIZE 0x400

uint32_t *pDmaMemory = (uint32_t *)EDMA_DEST_ADDRESS;

/****************************************************************************/
/* Main Function                                                            */
/****************************************************************************/
void main()
{
    int idx;
    eETB_Error  etbRet;
    ETB_errorCallback pETBErrCallBack =0;
    uint8_t coreID =0;
    ETBHandle* pETBHandle=0;
    uint32_t etbWidth, retSize=0;
    uint32_t *pBuffer=0;

    eDSPTrace_Error dspRet;
    DSPTrace_errorCallback pDSPErrorCallBack=0;
    DSPTraceHandle* pDSPHandle=0;
    DMAStatus dmaStatus;

    /* NOTE: THESE VARIABLES MUST BE DEFINED IN THE MAIN, NOT GLOBAL, FOR THE TRACE TO
     *        WORK CORRECTLY
     */
    /* jobIndex holders for the Start and End trace jobs */
    AET_jobIndex startJob;
#if !USE_TEND
    AET_jobIndex endJob;
#endif

   /* Determine the specific CorePac this application is running on */
    coreID = DNUM;
    printf("\n CORE ID: %d\n\n", coreID);

   /*** Setup ETB receiver ***/
   /* Open ETB module */
    etbRet = ETB_open(pETBErrCallBack, eETB_TI_Mode, coreID, &pETBHandle, &etbWidth);
    if(etbRet != eETB_Success)
   {
      printf("Error opening ETB\n");
      return;
   }

    /* Fill EDMA destination memory with pattern to test for writes */
    for(idx = 0; idx < (EDMA_BFR_WORDS+100); idx++)
    {
        pDmaMemory[idx] = 0xcccccccc;
    }

    //Setup EDMA
    Config_EDMA_for_DSPETB(pETBHandle, EDMA_DEST_ADDRESS, EDMA_BFR_WORDS, coreID);

    /* ** Setup Trace Export ** */
    /* Open DSP Trace export module */
    dspRet = DSPTrace_open( pDSPErrorCallBack, &pDSPHandle);
    if(dspRet != eDSPTrace_Success)
    {
        printf("Error opening DSP Trace Export block\n");
        return;
    }

    /* Setup trace export clock to FCLK/3 */
    dspRet= DSPTrace_setClock(pDSPHandle, 3);
    if(dspRet != eDSPTrace_Success)
    {
        printf("Error setting up DSP trace export clock\n");
        return;
    }

    dspRet= DSPTrace_enable(pDSPHandle, 0, 0);
    if(dspRet != eDSPTrace_Success)
    {
        printf("Error enabling DSP trace export\n");
        return;
    }
	
    /* Enable ETB receiver */
    etbRet = ETB_enable(pETBHandle, 0);
    if(etbRet != eETB_Success)
    {
        printf("Error enabling ETB\n");
        return;
    }

    /* ******************************* IMP. NOTE ****************************/
    /* AETLIB should be used to setup Trace triggers before enabling DSP Trace.*/
    /* ******************************* IMP. NOTE ****************************/
    /* Set up trace source to generate trace */
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
    EndTraceParams.programAddress = (uint32_t)&doneFunction;
    EndTraceParams.traceTriggers = AET_TRACE_STOP_PC | AET_TRACE_STOP_TIMING;
    EndTraceParams.traceActive = AET_TRACE_INACTIVE;

    if (AET_setupJob(AET_JOB_START_STOP_TRACE_ON_PC, &EndTraceParams))
        return;

    endJob = EndTraceParams.jobIndex;

    printf("The AET stop job index is %d\n", endJob );
#endif

    /* Enable AET */
    if (AET_enable())
    {
        printf("Error enabling AET \n");
        return;
    }

    /* ******************************************************************************** */
    /* ******************************************************************************** */
    /* ****************************************************************** APP CODE **** */
    /***** Now the app runs and generates trace data ****/
    runFunction();
    for(idx = 0; idx < N_APP_FUNCTION_RUNS; idx++)
    {
        appFunction();
    }
    doneFunction();
    /* ******************************************************************************** */
    /* ******************************************************************************** */
    /* ******************************************************************************** */

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

    /* Flush the DTF (Trace Formatter) output and the ETB input to guarantee all
     *  of the data will get read out of the FIFO.
	 * Also stop the DTF formatter
     */

    etbRet= ETB_flush(pETBHandle);

    if(etbRet != eETB_Success)
    {
        printf("Error flushing ETB %d\n", etbRet);
        return;
    }

    /* NOTE:
     *  ** THIS FLUSH API CALL IS REQUIRED TO GET THE DMA STATUS **
     *
     * Before reading from the ETB drain buffer, make sure all data has been
     *  read out of the ETB.
     */
    etbRet = ETB_flush_dma(pETBHandle, &dmaStatus);
    if(etbRet == eETB_Success)
    {
        /* NOTE:
         * There are 2 methods for reading the data from the ETB drain buffer.
         * -The 1st method is manually reading from the buffer's known location
         *  using the information returned in the dmaStatus structure when
         *  ETB_flush_dma is called. In that case the pBuffer pointer is set to
         *  the start address of the buffer and the wrap case is handled below
         *  while writing to an output file.
         * -The 2nd method uses the ETB_read API function to read parts of the
         *  drain buffer into a smaller temporary buffer that is then used to
         *  copy to the output file in seperate packets. A 1kB buffer is
         *  allocated and assigned to pBuffer.
         */
#ifdef _USE_MANUAL_READ_
        pBuffer = (uint32_t *)dmaStatus.startAddr;
#else
        pBuffer = (uint32_t*)malloc(TEMP_BFR_READ_SIZE);
#endif
        retSize = dmaStatus.availableWords;
        if(dmaStatus.isWrapped)
        {
            printf("ETB DrainBfr wrapped, Start: 0x%x, Size: %d\n",
                    dmaStatus.startAddr, retSize);
        }
        else
        {
            printf("ETB DrainBfr not wrapped, Start: 0x%x, Size: %d\n",
                    dmaStatus.startAddr, retSize);
        }
    }
    else
    {
        pBuffer = 0;
        printf("ETB_flush_dma ERROR: %d\n", etbRet);
    }

    /* Transport ETB data */
    if(pBuffer)
    {
        /* This example uses JTAG debugger via CIO to transport data to host PC
         *  An app can deploy any other transport mechanism to move the ETB
         *  buffer to the PC
         *
         * This process is slow and anything over a 16Kbyte drain buffer will
         *  take a considerable amount of time to transfer. A 256Kbyte buffer
         *  will take close to 40 seconds.
         */
        char * pFileName = "C:\\temp\\DSP_etbdata.bin";
        FILE* fp = fopen(pFileName, "wb");
        if(fp)
        {
            int32_t totalSize = 0;
            int32_t  cnt;
            uint32_t sz = 0;
            uint32_t i = 1;
            char *le = (char *) &i;

#ifdef _USE_MANUAL_READ_
            /* For manual ETB drain buffer read mode, set the buffer ending
             *  address to know when the buffer has wrapped back to the
             *  beginning.
             */
            uint32_t endAddress = dmaConfig.dbufAddress + (dmaConfig.dbufWords*4) - 1;
            sz = 0;
            for(cnt = 0; cnt < retSize; cnt++)
            {
                uint32_t etbword = *(pBuffer+sz);
                if(le[0] != 1) //Big endian
                   etbword = BYTE_SWAP32(etbword);

                size_t fret = fwrite((void*) &etbword, 4, 1, fp);
                if ( fret < 1 ) {
                    printf("Error writing data to  - %s \n", pFileName);
                    return;
                }
                sz++;

                /* Check for wrap condition with circular buffer */
                if((uint32_t)(pBuffer+sz) > endAddress)
                {
                    pBuffer = (uint32_t *)dmaConfig.dbufAddress;
                    sz = 0;
                }
            }
            totalSize = retSize;
#else
            /* Loop through the necessary number of packets to read the data
             *  from the ETB drain buffer.
             * - 16 1kB packets.
             * After the packet has been successfully read, it is written to
             *  the specified output file.
             */
            int32_t pktCnt;
            uint32_t maxPackets = ((EDMA_BFR_WORDS * 4) / TEMP_BFR_READ_SIZE);
            for(pktCnt = 0; pktCnt < maxPackets; pktCnt++)
            {
                etbRet = ETB_read(pETBHandle, pBuffer,
                                  (EDMA_BFR_WORDS/maxPackets),
                                  (pktCnt*(EDMA_BFR_WORDS/maxPackets)),
                                  (EDMA_BFR_WORDS/maxPackets), &retSize);
                totalSize += retSize;
                if(etbRet != eETB_Success)
                {
                    printf("Error reading ETB data\n");
                    break;
                }

                sz = 0;
                for(cnt = 0; cnt < retSize; cnt++)
                {
                    uint32_t etbword = *(pBuffer+sz);
                    if(le[0] != 1) //Big endian
                       etbword = BYTE_SWAP32(etbword);

                    size_t fret = fwrite((void*) &etbword, 4, 1, fp);
                    if ( fret < 1 ) {
                        printf("Error writing data to  - %s \n", pFileName);
                        return;
                    }
                    sz++;
                }
            }
#endif
            printf("Successfully transported ETB data - %s, %d words\n",
                    pFileName, totalSize);

            fclose(fp);
        }
        else {
            printf("Error opening file - %s\n", pFileName);
        }
#ifndef _USE_MANUAL_READ_
        free(pBuffer);
#endif
    }

    /*** Now we are done and close all the handles **/
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

    AET_releaseJob(startJob);
#if !USE_TEND
    AET_releaseJob(endJob);
#endif
    AET_release();

    return;
}
