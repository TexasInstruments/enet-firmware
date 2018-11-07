/*
 * CPT_SimpleExample.c
 *
 * CPT Simple Example L2 public functions. 
 *
 * Copyright (C) 2009,2010 Texas Instruments Incorporated - http://www.ti.com/ 
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

#include "CPTLib.h"
#include "CPTHelper.h"
#include "StmLibrary.h"

#if defined(_C66AK2Hxx) || defined(_TCI6630K2L) || defined(_66AK2Exx)
#include "DeviceSpecific_C66AK2Hxx.h"
#endif
#if defined(_C6670) || defined(_C6657)
#include "DeviceSpecific_C6670.h"
#endif
#ifdef _TCI6614
#include "DeviceSpecific_C6614.h"
#endif
#ifdef _66AK2Gxx
#include "DeviceSpecific_66AK2Gxx.h"
#endif
#ifdef _ETB
#include "ETBInterface.h"
#endif
#include <stdio.h>

#if defined(_66AK2Exx) || defined(_66AK2Gxx)
// Since ther is only one DSP in a 66AK2Exx device must use EDAM to generate L2 activity.
extern void DMA_Channel_Config(int chNum);
extern void DMA_Channel_Enable(int chNum);
extern int DMA_Channel_Complete(int chNum);
#endif

//Modify the example
#define SYSTEM_BANDWIDTH_PROFILING 1
#define SYSTEM_LATENCY_PROFILING 0

//L2 memory addresses (The linker command file has reserved 64K at the start of L2 for buffer space)
#define GEM0_L2_START       0x10800000
#define GEM1_L2_START       0x11800000
#define GEM2_L2_START       0x12800000
#define GEM3_L2_START       0x13800000
#define GEM4_L2_START       0x14800000
#define GEM5_L2_START       0x15800000
#define GEM6_L2_START       0x16800000
#define GEM7_L2_START       0x17800000
#define DDR_START           0x80100000
#define MSMC_START          0x0C000000

//STM Chaanle assignements
#define STM_HelperCh 0
#define STM_CPTLogCh 1

//Local support function prototypes
#if defined(_66AK2Exx) || defined(_66AK2Gxx)
void generate_L2SlaveActivity( );
#endif
void generate_SlaveActivity( int writeCnt, int readCnt, uint32_t addr );
void CPT_ErrorExit(CPT_Handle_Pntr pCPT_Handle);

//If the example is built with "C++", the callback function must be exported as a "C" function.
#ifdef __cplusplus
extern "C" {
#endif
void CPT_CallBackLog(const char * funcName, eCPT_Error err)
{
    printf("CPT failure %d in function: %s \n", err, funcName );
}

#ifdef __cplusplus
}
#endif

#ifdef _ETB
//ETB Function prototypes
uint8_t transferETBData(ETBHandle* pETBHandle, ETBStatus etbStatus, uint32_t* pBuffer);
uint8_t transferETBConfig(uint8_t wrapped);
#endif

CPT_CfgOptions CPTConfig;
CPT_CfgOptions CPTConfig_fast;

STMHandle * pSTMhdl;                                // STMLib handle pointer

void main()
{
    //CPT declarations
    eCPT_Error CPTErr;                                  //CPTLib errors

    //STM declarations
    STMBufObj * pSTMBufInfo = NULL;                     // Set pSTMBufInfo to NULL to set STM Lib for blocking IO 
    STMConfigObj STMConfigInfo;                         // STMLib configuration options struct

    // Clear the trigger for other GEM programs
    *(unsigned int *)GEM0_L2_START = 0;

#ifdef _ETB    
    //ETB declarations
    ETBHandle* pETBHandle = NULL;
    uint32_t etbWidth=0;
    eETB_Error  etbRet = eETB_Success;
    ETB_errorCallback pETBErrCallBack = NULL;
    ETBStatus etbStatus;
    uint32_t* pBuffer = NULL;

    //////////////////////////////////////////////////////
    //Initialize ETB                                    //
    //////////////////////////////////////////////////////
    
    if ( etbRet = ETB_open(pETBErrCallBack, eETB_Circular, SYS_ETB, &pETBHandle, &etbWidth) )
    {
        printf("Error %d opening ETB \n", etbRet);
        exit(1);
    }
    
    etbRet = ETB_enable(pETBHandle, 0);
    if(etbRet != eETB_Success)
    {
        printf("Error %d enabling ETB\n", etbRet);
        exit(1);
    }

#endif
    //////////////////////////////////////////////////////
    //Initialize STM Library                            //
    //////////////////////////////////////////////////////

    STMConfigInfo.STM_XportBaseAddr=STM_XPORT_BASE_ADDR;
	STMConfigInfo.STM_ChannelResolution=STM_CHAN_RESOLUTION;
	STMConfigInfo.STM_CntlBaseAddr = STM_CONFIG_BASE_ADDR;
	STMConfigInfo.xmit_printf_mode = eSend_optimized;
	STMConfigInfo.optimize_strings = true;              // Not forming any strings dynamically so optimized strings ok
    STMConfigInfo.pCallBack = NULL;                     // Set STM Callback to NULL 

    // Open STMLib
    pSTMhdl = STMXport_open( pSTMBufInfo, &STMConfigInfo);
    if ( NULL == pSTMhdl ) exit(EXIT_FAILURE);

#ifdef _ETB

    // Configure STM for capturing trace to MIPI STM TBR
    Config_STMData_Routing();
    Config_STM_for_ETB(pSTMhdl);

#endif
        
    /////////////////////////////////////////////////////
    //Initialize CPT Library for L2_1 CP Tracer        //
    /////////////////////////////////////////////////////
    CPTConfig.ForceOwnership = true;                   // If module owned by another instance of CPTLib (on this core or another)
	                                                        //   then force the ownership to this instance.
    CPTConfig.AddrExportMask = 0;                      // Export address bits 10:0 with event messsages
#if SYSTEM_BANDWIDTH_PROFILING
    CPTConfig.CPT_UseCaseId = eCPT_UseCases_SysBandwidthProfile;
#endif
#if SYSTEM_LATENCY_PROFILING
    CPTConfig.CPT_UseCaseId = eCPT_UseCases_SysLatencyProfile;
#endif
    CPTConfig.CPUClockRateMhz = 983;                   // Provide CPU clock rate
    CPTConfig.DataOptions = 0;                         // Set supress zero data but since this example enables
                                                            // and disables CPT messages as needed there is not really
                                                            // any 0 data generated
    CPTConfig.pCPT_CallBack = CPT_CallBackLog;         // Callback on error
#ifdef _66AK2Exx
    CPTConfig.CPUClockRateMhz = 1250;                  // Provide CPU clock rate
    CPTConfig.SampleWindowSize = 83200;                   // 83200 yields 1 sample every 200 micro-seconds
                                                          //  for 1250Mhz CPU clock.
                                                          // Note: L2 CPT is running at divide-by-factor of 3
#elif defined(_66AK2Gxx)
    CPTConfig.CPUClockRateMhz = 1000;                  // Provide CPU clock rate
    CPTConfig.SampleWindowSize = 33330;                   // 66660 yields 1 sample every 100 micro-seconds
                                                          //  for 1000Mhz CPU clock.
                                                          // Note: L2 CPT is running at divide-by-factor of 3
#else
    CPTConfig.CPUClockRateMhz = 983;                   // Provide CPU clock rate
    CPTConfig.SampleWindowSize = 81916;                // 81916 yields 1 sample every 250 micro-seconds
                                                            //  for 983Mhz CPU clock. 
                                                            // Note: 83333 yields 1 sample every 250 micro-seconds
                                                            //   for 1000 Mhz CPU clock)
                                                            // Note: L2 CPT is running at divide-by-factor of 3
#endif

    CPTConfig.pSTMHandle = pSTMhdl;
    CPTConfig.STM_LogMsgEnable = true;                 // Enable STM software message logging from CPTLib
    CPTConfig.STMMessageCh = STM_CPTLogCh;             // Set the STM software message logging channel

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Example 1 - Total Bandwidth Profile using default qualification (no qualification).         //
    // Select GEM0 master using default qualification (all cycle types (CPUInst, CPUData, DMA)     //
    // and access types(read/write) enabled                                                        //
    // Note: Default qualification setup by CPT_OpenModule().                                      //
    /////////////////////////////////////////////////////////////////////////////////////////////////

    {
        typedef struct _CPT_SysProfCfg {
            CPT_Handle_Pntr pCPT_Handle;
            eCPT_ModID CPT_ModId;      
            CPT_CfgOptions * pCPT_CfgOptions;
            CPT_Qualifiers * pCPT_TPCntQual;
            int openFlg;
        }CPT_SysProfCfg;

#if defined(_66AK2Exx) || defined(_66AK2Gxx)

        CPTConfig_fast = CPTConfig;
        CPTConfig_fast.SampleWindowSize = CPTConfig.SampleWindowSize*3;

        /* Note: Using the same sample window value for MSMC and DDR3 CP tracers will result in
         * samples occurring 3x faster than for the L2 CP Tracer.
         */
        CPT_SysProfCfg SPTable[] ={ { NULL, eCPT_L2_0, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_0, &CPTConfig_fast, NULL, 0},
#ifdef _66AK2Exx
                                    { NULL, eCPT_MSMC_1, &CPTConfig_fast, NULL, 0},
                                    { NULL, eCPT_MSMC_2, &CPTConfig_fast, NULL, 0},
                                    { NULL, eCPT_MSMC_3, &CPTConfig_fast, NULL, 0},
#endif
                                    { NULL, eCPT_DDR, &CPTConfig_fast, NULL, 0}
        };
#else
        CPT_SysProfCfg SPTable[] ={ { NULL, eCPT_L2_0, &CPTConfig, NULL, 0}, //eCPT_L2_0 configuration parameter
#ifndef _66AK2Exx
                                    { NULL, eCPT_L2_1, &CPTConfig, NULL, 0}, //eCPT_L2_1 configuration parameters
#endif
#if defined(_C6670) || defined(_TCI6614) || defined(_C66AK2Hxx) || defined(_TCI6630K2L)
                                    { NULL, eCPT_L2_2, &CPTConfig, NULL, 0}, //eCPT_L2_2 configuration parameters
                                    { NULL, eCPT_L2_3, &CPTConfig, NULL, 0}, //eCPT_L2_3 configuration parameters
#endif                                    
                                    { NULL, eCPT_QM_MST, &CPTConfig, NULL, 0},
#if defined(_C66AK2Hxx)
                                    { NULL, eCPT_L2_4, &CPTConfig, NULL, 0}, //eCPT_L2_4 configuration parameters
                                    { NULL, eCPT_L2_5, &CPTConfig, NULL, 0}, //eCPT_L2_5 configuration parameters
                                    { NULL, eCPT_L2_6, &CPTConfig, NULL, 0}, //eCPT_L2_6 configuration parameters
                                    { NULL, eCPT_L2_7, &CPTConfig, NULL, 0}, //eCPT_L2_7 configuration parameters
                                    { NULL, eCPT_DDR3A, &CPTConfig, NULL, 0},
#else
                                    { NULL, eCPT_DDR, &CPTConfig, NULL, 0},
#endif
                                    { NULL, eCPT_SM, &CPTConfig, NULL, 0},
#if defined(_C66AK2Hxx) || defined(_TCI6630K2L) || defined(_66AK2Exx)
                                    { NULL, eCPT_QM_CFG1, &CPTConfig, NULL, 0},
#else
                                    { NULL, eCPT_QM_CFG, &CPTConfig, NULL, 0},
#endif
                                    { NULL, eCPT_SCR3_CFG, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_0, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_1, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_2, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_3, &CPTConfig, NULL, 0},
#if !defined(_C6657) && !defined(_66AK2Exx)
                                    { NULL, eCPT_RAC, &CPTConfig, NULL, 0},
#if defined(_C66AK2Hxx) || defined(_TCI6630K2L)
                                    { NULL, eCPT_RAC_CFG1, &CPTConfig, NULL, 0},
#else
                                    { NULL, eCPT_RAC_CFG, &CPTConfig, NULL, 0},
#endif
                                    { NULL, eCPT_TAC, &CPTConfig, NULL, 0},
#endif
#if defined(_TCI6614) || defined(_TCI6612) || defined(_C6657)
                                    { NULL, eCPT_SCR_6P_A, &CPTConfig, NULL, 0},
#endif
#if defined(_TCI6614) || defined(_TCI6612)
                                    { NULL, eCPT_DDR_2, &CPTConfig, NULL, 0}
#endif
#if defined(_C66AK2Hxx)
                                    { NULL, eCPT_QM_CFG2, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_RAC_CFG2, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_DDR3B, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_BCR_CFG, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_TPCC_0_4, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_TPCC_1_2_3, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_INTC_CFG, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_4, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_5, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_6, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_7, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_SPI_ROM_EMIF16, &CPTConfig, NULL, 0}
#endif
#if defined(_TCI6630K2L)
                                    { NULL, eCPT_QM_CFG2, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_OSR_PCIE1_CFG, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_TPCC_0, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_TPCC_1_2, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_INTC_CFG, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_4, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_5, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_6, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_7, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_SPI_ROM_EMIF16, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_CFG_3P_U, &CPTConfig, NULL, 0}
#endif
#ifdef _66AK2Exx
                                    { NULL, eCPT_QM_CFG2, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_TPCC_0_4, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_TPCC_1_2_3, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_INTC_CFG, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_4, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_5, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_6, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_MSMC_7, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_SPI_ROM_EMIF16, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_NETCP_USB_CFG, &CPTConfig, NULL, 0},
                                    { NULL, eCPT_PCIE1_CFG, &CPTConfig, NULL, 0}
#endif
        };
#endif //_66AK2Exx || _66AK2Gxx
        int i;
        int SPTable_elementCnt = sizeof(SPTable)/ sizeof(CPT_SysProfCfg);
        int errFlg = 0;
        
        STMXport_printf(pSTMhdl, STM_HelperCh, "CPT Example - System Profiling Use Case" );
        STMXport_printf(pSTMhdl, STM_HelperCh, "CPT_Example - Profiling across %d CPT modules", SPTable_elementCnt );
    
        for ( i=0; i < SPTable_elementCnt; i++ ) {
        
            if (  CPTErr = CPT_OpenModule(&SPTable[i].pCPT_Handle, SPTable[i].CPT_ModId, SPTable[i].pCPT_CfgOptions ) )
            {
                //CPT_OpenModule() does not call Callback on an error so must do it here
                CPT_CallBackLog(__FUNCTION__, CPTErr);
                errFlg = 1;
                break;
            }
            SPTable[i].openFlg = 1;
            
            if ( eCPT_Success != CPTCfg_SystemProfile( SPTable[i].pCPT_Handle, SPTable[i].pCPT_TPCntQual) )
            {
                errFlg = 1;
            }
        }
        
        //If no errors then ok to start activity
        if ( 0 == errFlg ) {
            
            //Enable Each CPT module
            for ( i=0; i < SPTable_elementCnt; i++ ) {
			
#if SYSTEM_BANDWIDTH_PROFILING

                if ( eCPT_Success != CPT_ModuleEnable (SPTable[i].pCPT_Handle, eCPT_MsgSelect_Statistics, eCPT_CntSelect_None ) )
                {
                    errFlg = 1;
                }
				
#endif

#if SYSTEM_LATENCY_PROFILING

                if ( eCPT_Success != CPT_ModuleEnable (SPTable[i].pCPT_Handle, eCPT_MsgSelect_Statistics, eCPT_CntSelect_WaitAndGrant ) )
                {
                    errFlg = 1;
                }
				
#endif
            }
            
            //Start activity from this GEM (GEM0) to GEM1 L2
             
            //Note: No L2 CP Tracer activity will be captured for the
            //DSP this code is running on because these are internal
            //accesses not visible to the CP Tracer.
#if defined(_66AK2Exx) || defined(_66AK2Gxx)
            generate_L2SlaveActivity();                                       /* Use EDMA to xfer into 66x L2 */

            int writeCnt = 8192;
            int readCnt  = 8192;
            generate_SlaveActivity(readCnt, writeCnt, DDR_START );            /* Use DSP to xfer into DDR */

            writeCnt = 12288;
            readCnt  = 12288;
            generate_SlaveActivity(readCnt, writeCnt, MSMC_START );           /* Use DSP to xfer into MSMC */
#else
            int writeCnt = 0x2000-1;
            int readCnt  = 0x2000-1;
            generate_SlaveActivity(readCnt, writeCnt, GEM0_L2_START+4 );
            generate_SlaveActivity(readCnt, writeCnt, GEM1_L2_START+4 );
#ifndef _C6657
            generate_SlaveActivity(readCnt, writeCnt, GEM2_L2_START+4 );
            generate_SlaveActivity(readCnt, writeCnt, GEM3_L2_START+4 );
#endif
            generate_SlaveActivity(readCnt, writeCnt, DDR_START );
#endif
            
        }        
        //Activity done - Disable and close any open CPT modules 
        for ( i=0; i < SPTable_elementCnt; i++ ) {
            if ( 1 == SPTable[i].openFlg) {
                CPT_ModuleDisable (SPTable[i].pCPT_Handle, eCPT_WaitEnable );
                CPT_CloseModule(&SPTable[i].pCPT_Handle );
            }
        }
        
        if ( 1 == errFlg ) { 
            exit(EXIT_FAILURE);
        }
             
    }
    
#ifdef _ETB

    printf("Preparing to get ETB data\n");

    /****************************************************************************/     
    /* Disable ETB to stop capturing STM trace. ETB must be disabled before reading contents */
    /****************************************************************************/     

    etbRet = ETB_disable(pETBHandle);
    if(etbRet != eETB_Success)
    {
        printf("Error %d disabling ETB\n", etbRet);
        exit(1);
    }

    
    /****************************************************************************/     
    /* Check the ETB status */
    /****************************************************************************/     
    etbRet= ETB_status(pETBHandle, &etbStatus);
    if(etbRet != eETB_Success)
    {
        printf("Error %d getting ETB status\n", etbRet);
        exit(1);
    }

    /* Allocate buffer */
    pBuffer = (uint32_t*) malloc(etbStatus.availableWords * 4); // ETB words are 32 bit long   
    if(pBuffer ==0)
    {
        printf("Cannot allocate %d byte buffer to read ETB\n", etbStatus.availableWords * 4);
        exit(1);
    }
    /* Now read and transfer the ETB contents to to PC host for further decode and analysis*/
    if( 0 != transferETBData(pETBHandle, etbStatus, pBuffer) )
    {
        if(pBuffer)
            free(pBuffer);
        exit(1);
    }
    
    /* Now transfer the ETB configuration to help decode */
    transferETBConfig(etbStatus.isWrapped);

    /* Free buffer */
    if(pBuffer)
        free(pBuffer);

    
    /****************************************************************************/     
    /* Close ETB - no ETB usage after this*/
    /****************************************************************************/     

    /*** Now we are done and close all the handles **/
    etbRet  = ETB_close(pETBHandle);
    if(etbRet != eETB_Success)
    {
        printf("Error closing ETB\n");
        exit(1);
    }
    
#endif

    /****************************************************************************/
	/* STM library must be closed before app terminates or */
	/* no STM instrumentation desired after a point*/
	/****************************************************************************/
	if(pSTMhdl)
	{
		STMXport_close (pSTMhdl);
	}

    
} //End of main

#if defined(_66AK2Exx) || defined(_66AK2Gxx)
// Generate read/writes to L2 space using DMA
// Note: The reason DMA is used is that DSP L2 accesses to the local L2 uses an internal path which will not trigger L2 CP Tracer activity.
void generate_L2SlaveActivity( )
{
    int i;
    const int dma_loop = 8;
    for (i = 0; i < dma_loop; i++) {
        int chNum = 0;
        int retry = 100;
        int done = 0;

        STMXport_logMsg2(pSTMhdl, STM_HelperCh, "DMA Loop iteration %d of %d", i+1, dma_loop);

        DMA_Channel_Config(chNum);
        DMA_Channel_Enable(chNum);
        STMXport_logMsg1(pSTMhdl, STM_HelperCh, "DMA Channel %d Active", chNum);

        do {
            int wait = 1000;
            while (wait-- != 0);
            if (done = DMA_Channel_Complete(chNum)) {
                STMXport_logMsg1(pSTMhdl, STM_HelperCh, "DMA Channel %d Active", chNum);
            } else {
                STMXport_logMsg1(pSTMhdl, STM_HelperCh, "DMA Channel %d Complete", chNum);
            }
            retry--;

        } while ((done) && (retry > 0));
    }
}
#endif
void generate_SlaveActivity( int writeCnt, int readCnt, uint32_t addr )
{
    unsigned int i, n;
    int maxCnt = ( writeCnt >= readCnt ) ? writeCnt : readCnt;
    unsigned int* pTrig = ( unsigned int* )( GEM0_L2_START );
 
    //safe to trigger other processors waiting
    *pTrig = 0xbabe;
 
    // Transfer words
    {
        volatile unsigned int locData;
        
        //fixed outer loop to extend data
        for ( n = 0; n < 10; n++ ) 
        {   
            for( i=0; i < maxCnt; i++ )
            {
                unsigned int* pData = ( unsigned int* )( addr+i*4 );
                if ( i < readCnt ) locData = *pData;
                if ( i < writeCnt ) *pData = i;
            }
        }
    }
    
    // Transfer half words
    {
        volatile unsigned short locData;
            
        for( i=0; i < maxCnt; i++ )
        {
            unsigned short* pData = ( unsigned short* )( addr+i*4 );
            if ( i < readCnt )
            { 
                locData = *pData;
                locData = *(pData+1);
            }

            // Wait a bit every 1K accesses
            if ( !( i % 512 ) ) 
            {
                int cnt = 0;
                while( cnt++ < 10000 );
            }

            if ( i < writeCnt )
            {
                *pData = i;
                *(pData+1) = (i & 0xffff0000) >> 16;
            }
            
        }
    
    }   

    // Transfer bytes
    {
        volatile unsigned char locData;
            
        for( i=0; i < maxCnt; i++ )
        {
            unsigned char * pData = ( unsigned char* )( addr+i*4 );
            if ( i < readCnt ) 
            {
                locData = *pData;
                locData = *(pData+1);
                locData = *(pData+2);
                locData = *(pData+3);
                
            }
            if ( i < writeCnt ) 
            {
                *(pData+3) = ( i & 0xff000000 ) >> 24;
                *(pData+2) = ( i & 0x00ff0000 ) >> 16;
                *(pData+1) = ( i & 0x0000ff00 ) >> 8;
                *pData = ( i & 0x000000ff );
                
            }
        }
    
    }       
    
    //Clear trigger
    *pTrig = 0;
}

void CPT_ErrorExit(CPT_Handle_Pntr pCPT_Handle)
{
    eCPT_Error CPTErr;
    
    if ( NULL != pCPT_Handle)
    {
        CPTErr = CPT_CloseModule(&pCPT_Handle );
        if (  eCPT_Success != CPTErr )
        { 
            printf( "CPT_CloseModule failed with error %d, may need to clear ownership manually\n", CPTErr);
        }
    }
    
    printf(" Error Exit");
    exit(EXIT_FAILURE);
}

#ifdef _ETB

/****************************************************************************/     
/* Read and Transfer ETB data*/
/* This examples uses CCS CIO functionality to create a*/ 
/* ETB bin file on the PC. Apps would replace this with a specific data*/
/* transport mechanism to move the binary data to PC host for further */
/* decoding and analysis*/
/****************************************************************************/     
uint8_t transferETBData(ETBHandle* pETBHandle, ETBStatus etbStatus, uint32_t* pBuffer)
{
    eETB_Error  etbRet;
    uint32_t retSize=0;
    
    /*** Read ETB data ***/
    if(etbStatus.canRead == 1)
    {
        if(etbStatus.isWrapped == 1)
            printf ("ETB is wrapped; ETB words = %d\n", etbStatus.availableWords);
        else
            printf ("ETB is not wrapped; ETB words = %d\n", etbStatus.availableWords);
            
        if(pBuffer)
        {
            etbRet = ETB_read(pETBHandle, pBuffer, etbStatus.availableWords, 0, etbStatus.availableWords, &retSize);
            if(etbRet != eETB_Success)
            {
                printf("Error reading ETB data\n");
                return 1;
            }
        }
    }
    
    /* Transport ETB data */    
    if(pBuffer)
    {
        char * pFileName = "C:\\temp\\CPT_etbdata.bin";
        FILE* fp = fopen(pFileName, "wb");
        if(fp)
        {
            uint32_t sz = 0;
            uint32_t i = 1;
            char *le = (char *) &i;
            size_t fret = 0;

            while(sz < (retSize))
            {
                
                uint32_t etbword = *(pBuffer+sz);
                if(le[0] != 1) //Big endian
                {
                    etbword = BYTE_SWAP32(etbword);
                }   
                fret = fwrite((void*) &etbword, 4, 1, fp); 
//              printf("ETB[%d]= 0x%x\n", sz, etbword);

                if ( fret < 1 )
                {
                    printf("Error writing data to  - %s \n", pFileName);
                    return 1;    
                }
                sz++;
            
            }
            printf("Successfully transported ETB data - %s \n", pFileName);
            
            fclose(fp);
        }
        else
        {
            printf("Error opening file - %s \n", pFileName);
            return 1;
        }   
    }
    return 0;
}

/*******************************************************************************/     
/* Status of the ETB STM data - this is required to decode the compressed data */
/*******************************************************************************/     
uint8_t transferETBConfig(uint8_t wrapped)
{
    char * pFileName = "C:\\temp\\CPT_etbdata.dcm";
    FILE* fp = fopen(pFileName, "w");
    if(fp)
    {
        STM_DCM_InfoObj  DCM_Info;
        STMXport_getDCMInfo(pSTMhdl, &DCM_Info);

//        fprintf(fp, "STM_data_flip=1\n");
        fprintf(fp, "STM_data_flip=%d\n", DCM_Info.stm_data_flip);
        fprintf(fp, "STM_Buffer_Wrapped=%d\n", wrapped);
        fprintf(fp, "HEAD_Present_0=%d\n", DCM_Info.atb_head_present_0);
        fprintf(fp, "HEAD_Pointer_0=%d\n", DCM_Info.atb_head_pointer_0);
        fprintf(fp, "HEAD_Present_1=%d\n", DCM_Info.atb_head_present_1);
        fprintf(fp, "HEAD_Pointer_1=%d\n", DCM_Info.atb_head_pointer_1);
        fprintf(fp, "STM_STP_Version=%d\n",DCM_Info.stpVersion);
        fprintf(fp, "TWP_Protocol=%d\n", ETB_USING_TWP);
        
        fclose(fp);
        printf("Successfully transported STM config - %s \n", pFileName);

    }
    else 
    {
        printf("Error opening file - %s \n", pFileName);
        return 1;
    }
    
    return 0;   
}

#endif


