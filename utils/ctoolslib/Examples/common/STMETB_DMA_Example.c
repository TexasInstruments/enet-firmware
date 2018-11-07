
/****************************************************************************
CToolsLib - STM Trace Example

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

#include <stdio.h>
#include <stdlib.h>

#include "ETBInterface.h"
#include "edma_dev-c66xx.h"
#include "StmLibrary.h"

#if defined(C6670) && !defined(TCI6614)
#include "DeviceSpecific_C6670.h"
#endif

#ifdef C6678
#include "DeviceSpecific_C6678.h"
#endif

#ifdef C6657
#include "DeviceSpecific_C6657.h"
#endif

#ifdef TCI6614
#include "DeviceSpecific_C6614.h"
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
 *  This example is setup to run on DSP core 0.
 *
 *  - Trace, ETB based, data is collected and stored in file:
 *     c:\temp\STM_etbdata.bin
 *  - bin2tdf utility is required, located in:
 *    -> cd <CCS install dir>\ccs_base\emulation\analysis\bin
 *  - To convert binary trace data captured by the ETB, use the following
 *    command line:
 *    -> bin2tdf -bin C:/temp/STM_etbdata.bin -app C66X_0=\CToolsLib\Examples\
 *       bin\stm_tietb-edma_d_corepacN.c6670.out -procid stm -sirev 1 -rcvr ETB
 *       -cpuname CSSTM_0 -devicekey 0x0009D02F -dcmfile
 *       C:/temp/STM_etbdata.dcm -output C:/temp/stmEdmaC66x.tdf -sourcepaths 
 *       \CToolsLib\Examples\C667x\STM_TIETB_EDMA_Ex_CorePacN
 *  - The stmEdmaC66x.tdf file may be opened from the CCS menu at
 *    Tools->Trace Analyzer->Open Trace File In New View...
 *
 *******************************************************************************
 ******************************************************************************/

ETBHandle* pETBHandle=0;
uint32_t etbSize=0;
STMHandle *pSTMHandle =0;
STMConfigObj STMConfigInfo; 
int iChannel = 0;

void transferETBData(void);
void transferSTMConfig(STMHandle* pSTMHandle);
void runApp(uint8_t core);

#define EDMA_DEST_ADDRESS_1  0x0c000000
#define EDMA_BFR_WORDS       0x8000   /* 0x20000 (128k) bytes */
#define TEMP_BFR_READ_SIZE 0x400

volatile uint32_t *pDmaMemory = (volatile uint32_t *)EDMA_DEST_ADDRESS_1;

/* DMA status structure that relates to specific DMA buffer.
 *  Status is gathered during the flush_dma function and set using the
 *  function ETB_setDmaStatus.
 */
DMAStatus mDmaStatus;

/****************************************************************************************
 *
 * Main Function
 *
 ****************************************************************************************/
void main()
{
    int idx;
    eETB_Error  etbRet;
	ETB_errorCallback pETBErrCallBack =0;
    DMAStatus *pDmaStatus = &mDmaStatus;

    /* Initialize status parameters */
    mDmaStatus.availableWords = 0;

    /* Fill EDMA destination memory with pattern to test for writes */
    for(idx = 0; idx < (EDMA_BFR_WORDS + 100); idx++)
    {
        pDmaMemory[idx] = 0xcccccccc;
    }
    
    /***************************************************************************     
     * Open STM library for app usage
     * STM library must be opened by the CPU beore using STM instrumentation
     **************************************************************************/
	STMConfigInfo.STM_XportBaseAddr=STM_XPORT_BASE_ADDR;
	STMConfigInfo.STM_ChannelResolution=STM_CHAN_RESOLUTION;
	STMConfigInfo.STM_CntlBaseAddr = STM_CONFIG_BASE_ADDR;
    STMConfigInfo.xmit_printf_mode = eSend_optimized;
    STMConfigInfo.optimize_strings =  true;

    pSTMHandle = STMXport_open(NULL, &STMConfigInfo);
    if(pSTMHandle ==0)
    {
        printf("Error enabling STM library\n");
        exit(1);
    }
    
    /********************************************************************************
     * Open ETB to receive STM data. Once ETB is opened successfully, ETB can
     * can be enabled and disabled for "start capture" and "stop capture"
     * respectively.
     * Please note that SYS ETB ID needs to be passed in for STM collection. 0 for
     *  core (ETM)
     ********************************************************************************/
    etbRet = ETB_open(pETBErrCallBack, eETB_TI_Mode, SYS_ETB, &pETBHandle, &etbSize);
    if(etbRet != eETB_Success)
    {
        printf("Error opening ETB\n");
        exit(1);
    }

    /****************************************************************************
     * This setup function MUST be run after ETB_open, the registers are not
     *  available until after the power domain is configured.
     ****************************************************************************/     
    Config_STM_for_ETB(pSTMHandle);

    //Setup EDMA
    Config_EDMA_for_SYSETB(pETBHandle, EDMA_DEST_ADDRESS_1, EDMA_BFR_WORDS);

    /****************************************************************************
     * Enable ETB to capture STM data.
     ****************************************************************************/
    /* Enable ETB receiver */
    etbRet = ETB_enable(pETBHandle, 0);
    printf("%d %d\n",etbRet,eETB_Success);
    if(etbRet != eETB_Success)
    {
        printf("Error enabling ETB\n");
        exit(1);
    }

    /* **************************IMPORTANT **********************************************
     * **********************************************************************************
     * In real app this is the logical point where all the STM initialization has been
     *  done and STM messages can be generated by the app and received by the ETB
     * **********************************************************************************
     * **********************************************************************************/
    for(idx = 0; idx < 3000; idx++)
    {
        runApp(0);
    }

    printf("Preparing to get ETB data, may take close to 30 seconds...\n");

    /* Flush the STM and ETB, then flush the DMA to transfer
     *  any remaining data from the ETB that hasn't reached the half or
     *  full thresholds.
	 *  Also stop the formatter
     */
    etbRet = ETB_flush(pETBHandle);
    if(etbRet != eETB_Success)
    {
        printf("Error Flushing ETB\n");
        exit(1);
    }

    etbRet = ETB_flush_dma(pETBHandle, pDmaStatus);
    if(etbRet != eETB_Success)
    {
        printf("ETB_flush_dma bfr2 error %d\n", etbRet);
        exit(1);
    }

    /* Read and transfer the ETB contents to to PC host for further decode
     *  and analysis
     */
    transferETBData();

    /* Transer the STM configuration to help decode */
    transferSTMConfig(pSTMHandle);

    /****************************************************************************
     * Close ETB - no ETB usage after this
     ****************************************************************************/
    /* Now disable trace capture - ETB receiver */
    etbRet = ETB_disable(pETBHandle);
    if(etbRet != eETB_Success)
    {
        printf("Error disabling ETB\n");
        return;
    }
     
    etbRet  = ETB_close(pETBHandle);
    if(etbRet != eETB_Success)
    {
        printf("Error closing ETB\n");
        exit(1);
    }

 	/****************************************************************************/
    /* STM library must be closed before app terminates or */
    /* no STM instrumentation desired after a point*/
    /****************************************************************************/
    if(pSTMHandle)
    {
        STMXport_close (pSTMHandle);
    }

    return;
}

/****************************************************************************************
 * Read and Transfer ETB data
 * This examples uses CCS CIO functionality to create a
 * ETB bin file on the PC. Apps would replace this with a specific data
 * transport mechanism to move the binary data to PC host for further
 * decoding and analysis
 ****************************************************************************************/
void transferETBData(void)
{
    uint32_t *pBuffer;
    eETB_Error  etbRet;
	uint32_t retSize=0;

    pBuffer = (uint32_t *)malloc(TEMP_BFR_READ_SIZE);
    if(pBuffer)
    {
        char *pFileName;
        pFileName = "C:\\temp\\STM_etbdata.bin";
        FILE* fp;
        fp = fopen(pFileName, "wb");
        if(fp)
        {
            int32_t totalSize = 0;
            int32_t  cnt;
            uint32_t sz = 0;
            uint32_t i = 1;
            char *le = (char *) &i;

            /* Loop through the necessary number of packets to read the data
             *  from the ETB drain buffer.
             * - 16 1kB packets.
             * After the packet has been successfully read, it is written to
             *  the specified output file.
             */
            int32_t pktCnt;
            uint32_t maxPackets = ((EDMA_BFR_WORDS * 4) / TEMP_BFR_READ_SIZE);
            
            {
                /* Set DMA status to specific buffer's status parameters */
                ETB_setDmaStatus(pETBHandle, &mDmaStatus);

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
                            fclose(fp);
                            return;
                        }
                        sz++;
                    }
                }
            }
            printf("Successfully transported ETB data - %s, %d words\n",
                    pFileName, totalSize);

            fclose(fp);
        }
        else {
            printf("Error opening file - %s or %s\n", pFileName);
        }

        free(pBuffer);
    }
    else
    {
        printf("Error allocating memory for ETB read buffer %d bytes\n",
                TEMP_BFR_READ_SIZE);
    }
    return;
}

/****************************************************************************************
 * Status of the STM - this is required to decode the compressed data
 ****************************************************************************************/
void transferSTMConfig(STMHandle* pSTMHandle)
{
    char * file_name = "C:\\temp\\STM_etbdata.dcm";
	FILE* fp = fopen(file_name, "w");
	if(fp)
	{

        STM_DCM_InfoObj  DCM_Info;
        STMXport_getDCMInfo(pSTMHandle, &DCM_Info);

        fprintf(fp, "STM_data_flip=%d\n", DCM_Info.stm_data_flip);
        fprintf(fp, "STM_Buffer_Wrapped=%d\n",  mDmaStatus.isWrapped);
        fprintf(fp, "HEAD_Present_0=%d\n", DCM_Info.atb_head_present_0);
        fprintf(fp, "HEAD_Pointer_0=%d\n", DCM_Info.atb_head_pointer_0);
        fprintf(fp, "HEAD_Present_1=%d\n", DCM_Info.atb_head_present_1);
        fprintf(fp, "HEAD_Pointer_1=%d\n", DCM_Info.atb_head_pointer_1);
        fprintf(fp, "STM_STP_Version=%d\n",DCM_Info.stpVersion);
        fprintf(fp, "TWP_Protocol=%d\n", ETB_USING_TWP);

        fclose(fp);
		printf("Successfully transported STM config - %s\n", file_name);
	}
    else {
        printf("Error opening %s or %s for writing\n", file_name);
    }

    return;
}

/****************************************************************************************
 * Sample code demostrating STM instrumentation
 ****************************************************************************************/
void runApp(uint8_t core)
{
    static uint32_t appCounter = 0;
    uint16_t shortValue;

    iChannel = 100+core; /* A logical channel number. Defined and used by app for grouping */
    STMXport_logMsg1(pSTMHandle, iChannel, "%d\0", appCounter);

    shortValue = (100+appCounter);
    STMXport_putShort (pSTMHandle, iChannel, shortValue);

    STMXport_printf(pSTMHandle, iChannel, "printf: Count is %d\0", appCounter++);

    return;
}

