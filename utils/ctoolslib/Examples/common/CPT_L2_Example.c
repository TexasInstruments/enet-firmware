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
#define TOTAL_BANDWIDTH_PROFILING 1
#define MASTER_BANDWIDTH_PROFILING 0
#define EVENT_USE_CASE 0
// Note - if you enable this the CP Tracer is polled on every L2 access,
//        slowing BW dramatically (as expected). We recommend that you 
//        either implement a CP Tracer ISR or use a timer running at 
//        least 2x the CP tracer sample rate to poll on.
#define CPTRACER_DATA_POLLING 0

//L2 memory addresses (The linker command file has reserved 64K at the start of L2 for buffer space)
#define GEM0_L2_START       0x10800000
#define GEM1_L2_START       0x11800000
#define GEM2_L2_START       0x12800000
#define GEM3_L2_START       0x13800000

//STM Chaanle assignements
#define STM_HelperCh 0
#define STM_CPTLogCh 1

//Local support function prototypes
void generate_L2SlaveActivity( int writeCnt, int readCnt, uint32_t addr );
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

#if CPTRACER_DATA_POLLING
int getCPTracerDataIfAvaiable(CPT_Handle_Pntr const pCPT_Handle);
static CPT_Handle_Pntr pCPT_Handle4polling;
#endif

STMHandle * pSTMhdl;                                // STMLib handle pointer

void main()
{
    //CPT declarations
    CPT_Handle_Pntr pCPT_Handle_L2 = NULL;            //Initialize CPT handle pointer to NULL required by CPTLib
    eCPT_Error CPTErr;                                  //CPTLib errors
    CPT_CfgOptions CPTConfig_L2;                      //CPTLib configuration options struct

    //STM declarations
    STMBufObj * pSTMBufInfo = NULL;                     // Set pSTMBufInfo to NULL to set STM Lib for blocking IO
    STMConfigObj STMConfigInfo;                         // STMLib configuration options struct

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
    CPTConfig_L2.ForceOwnership = true;                   // If module owned by another instance of CPTLib (on this core or another)
    														//   then force the ownership to this instance.
    CPTConfig_L2.AddrExportMask = 0;                      // Export address bits 10:0 with event messages
    CPTConfig_L2.CPT_UseCaseId = eCPT_UseCase_TotalProfile;   // Select Total Bandwidth Profile Use Case

    CPTConfig_L2.DataOptions = 0;                         // Set suppress zero data but since this example enables
                                                            // and disables CPT messages as needed there is not really
                                                            // any 0 data generated
    CPTConfig_L2.pCPT_CallBack = CPT_CallBackLog;         // Callback on error
    CPTConfig_L2.pSTMHandle = pSTMhdl;

    CPTConfig_L2.STM_LogMsgEnable = true;                 // Enable STM software message logging from CPTLib
    CPTConfig_L2.STMMessageCh = STM_CPTLogCh;             // Set the STM software message logging channel
#ifdef _66AK2Exx
    CPTConfig_L2.CPUClockRateMhz = 1250;                  // Provide CPU clock rate
    CPTConfig_L2.SampleWindowSize = 8320;                 // 8320 yields 1 sample every 20 micro-seconds
                                                          //  for 1250Mhz CPU clock.
                                                          // Note: L2 CPT is running at divide-by-factor of 3
#elif defined(_66AK2Gxx)
    CPTConfig_L2.CPUClockRateMhz = 1000;                  // Provide CPU clock rate
    CPTConfig_L2.SampleWindowSize = 3333;                 // 3333 yields 1 sample every 10 micro-seconds
                                                          //  for 1000Mhz CPU clock.
                                                          // Note: L2 CPT is running at divide-by-factor of 3
#else
    CPTConfig_L2.CPUClockRateMhz = 983;                   // Provide CPU clock rate
    CPTConfig_L2.SampleWindowSize = 81916;                // 81916 yields 1 sample every 250 micro-seconds
                                                            //  for 983Mhz CPU clock. 
                                                            // Note: 83333 yields 1 sample every 250 micro-seconds
                                                            //   for 1000 Mhz CPU clock)
                                                            // Note: L2 CPT is running at divide-by-factor of 3
#endif
#if TOTAL_BANDWIDTH_PROFILING
    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Example 1 - Total Bandwidth Profile using default qualification (no qualification).         //
    // Select GEM0 master using default qualification (all cycle types (CPUInst, CPUData, DMA)     //
    // and access types(read/write) enabled                                                        //
    // Note: Default qualification setup by CPT_OpenModule().                                      //
    /////////////////////////////////////////////////////////////////////////////////////////////////
    
//    STMXport_printf(pSTMhdl, STM_HelperCh, "CPT Example - Total Bandwidth Profiling Use Case" );
    
    {    
        CPT_Qualifiers * pCPT_TPCntQual = NULL;
        
#if defined(_66AK2Exx) || defined(_66AK2Gxx)
        eCPT_ModID CPT_ModID = eCPT_L2_0;
        eCPT_MasterID CPT_MasterID = eCPT_MID_EDMA0_TC0_RD;
#else
        eCPT_ModID CPT_ModID = eCPT_L2_1;
        eCPT_MasterID CPT_MasterID = eCPT_MID_GEM0;
#endif

        //Open CorePac1 L2 CPT Module. 
        if (  CPTErr = CPT_OpenModule(&pCPT_Handle_L2, CPT_ModID,  &CPTConfig_L2 ) )
        {
            //CPT_OpenModule() does not call Callback on an error so must do it here
            CPT_CallBackLog(__FUNCTION__, CPTErr);
            exit(EXIT_FAILURE);
        }

#if CPTRACER_DATA_POLLING
        CPT_CfgInt(pCPT_Handle_L2, eCPT_IntMask_Enable);
        pCPT_Handle4polling = pCPT_Handle_L2;
#endif
    
        //Log STM message and setup CP Tracer for Total Bandwidth Profile use case
        if (   ( eCPT_Success != CPT_LogMsg(pCPT_Handle_L2, "L2 CP Tracer - Example 1 Total Bandwidth Profile, Default Qualifiers", NULL) )
             || ( eCPT_Success != CPTCfg_TotalProfile( pCPT_Handle_L2, CPT_MasterID, pCPT_TPCntQual) ) )
        {
            //Errors handled by callback so only thing left to do is exit
            CPT_ErrorExit(pCPT_Handle_L2);
        }
                
        //Start activity from this GEM (GEM0) to GEM1 L2. Note that for 66Ak2Exx and 66AK2Gxx devices, generate_L2SlaveActivity() uses
        //EDMA to transfer data from/to L2_0 rather than GEM1_L2_START+4.
        { 
            int writeCnt = 0x10000-4;
            int readCnt  = 0x10000-4;
            generate_L2SlaveActivity(readCnt, writeCnt, GEM1_L2_START+4 );
        }
        
        //Disable and close the CP Tracer module.
        if (    ( eCPT_Success != CPT_ModuleDisable (pCPT_Handle_L2, eCPT_WaitEnable ) )
             || ( eCPT_Success != CPT_CloseModule(&pCPT_Handle_L2 ) ) )
        {
            //Errors handled by callback so only thing left to do is exit 
            CPT_ErrorExit(pCPT_Handle_L2);
        }
        
        
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Example 2 - Total Bandwidth Profile using Address and Cycle qualification.                  //
    // Select GEM0 master using default qualification (all cycle types (CPUInst, CPUData, DMA)     //
    // and access types read only enabled.                                                         //
    // Note: Default qualification setup by CPT_OpenModule().                                      //
    /////////////////////////////////////////////////////////////////////////////////////////////////
    {    
        CPT_Qualifiers CPT_TPCntQual;

#if defined(_66AK2Exx) || defined(_66AK2Gxx)
        eCPT_ModID CPT_ModID = eCPT_L2_0;
        eCPT_MasterID CPT_MasterID = eCPT_MID_EDMA0_TC0_RD;
#else
        eCPT_ModID CPT_ModID = eCPT_L2_1;
        eCPT_MasterID CPT_MasterID = eCPT_MID_GEM0;
#endif

        //Open CorePac1 L2 CPT Module.
        if (  CPTErr = CPT_OpenModule(&pCPT_Handle_L2, CPT_ModID,  &CPTConfig_L2 ) )
        {
            //CPT_OpenModule() does not call Callback on an error so must do it here
            CPT_CallBackLog(__FUNCTION__, CPTErr);
            exit(EXIT_FAILURE);
        }
    
        
        //Setup Address Qualification
        {
            eCPT_FilterMode CPT_FilterMode = eCPT_FilterMode_Inclusive;
            uint32_t AddrFilterMSBs = 0;
#if defined(_66AK2Exx) || defined(_66AK2Gxx)
            // Filter all addresses outside the EDMA read address range
            uint32_t StartAddrFilterLSBs = 0x10800000;
            uint32_t EndAddrFilterLSBs = 0x10807fff;
#else
            uint32_t StartAddrFilterLSBs = 0x11800000;
            uint32_t EndAddrFilterLSBs = 0x11807fff;
#endif
            if (  eCPT_Success != CPT_CfgAddrFilter( pCPT_Handle_L2, AddrFilterMSBs, StartAddrFilterLSBs, EndAddrFilterLSBs, CPT_FilterMode ) )
            {
                CPT_ErrorExit(pCPT_Handle_L2);
            } 
        }
         
        //Setup data qualifier - Should yeild half as much data as Example 1
        CPT_TPCntQual.NewReqSrcQual = eCPT_SrcQual_IncludeALL; 
        CPT_TPCntQual.RWQual = eCPT_RWQual_ReadOnly;                      
        
        //Log STM message and setup CP Tracer Total Bandwidth Profile use case  
        if (    ( eCPT_Success != CPT_LogMsg(pCPT_Handle_L2, "L2 CP Tracer - Example 2 Total Bandwidth Profile, Address Filtering", NULL) )
             || ( eCPT_Success != CPTCfg_TotalProfile( pCPT_Handle_L2, CPT_MasterID, &CPT_TPCntQual) ) )
        {
            CPT_ErrorExit( pCPT_Handle_L2);
        }    
        
        //Start activity from this GEM (GEM0) to GEM1 L2                     
        { 
            int writeCnt = 0x10000-4;
            int readCnt  = 0x10000-4;            
            generate_L2SlaveActivity( writeCnt, readCnt, GEM1_L2_START+4 );
        }

        //Disable and close the CP Tracer module.
        if (    ( eCPT_Success != CPT_ModuleDisable (pCPT_Handle_L2, eCPT_WaitEnable ) )
             || ( eCPT_Success != CPT_CloseModule(&pCPT_Handle_L2 ) ) )
        {
            //Errors handled by callback so only thing left to do is exit 
            CPT_ErrorExit(pCPT_Handle_L2);
        }
    
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Example 3 - Total Bandwidth Profile selecting multiple masters                              //
    // Select GEM0 master using default qualification (all cycle types (CPUInst, CPUData, DMA)     //
    // and access types read only enabled.                                                         //
    // Note: Default qualification setup by CPT_OpenModule().                                      //
    /////////////////////////////////////////////////////////////////////////////////////////////////
    {    
        CPT_Qualifiers CPT_TPCntQual;

#if defined(_66AK2Exx) || defined(_66AK2Gxx)
        eCPT_ModID CPT_ModID = eCPT_L2_0;
        eCPT_MasterID CPT_MasterID[] = { eCPT_MID_GEM0, eCPT_MID_EDMA0_TC0_RD, eCPT_MID_EDMA0_TC0_WR };
        int CPT_MasterIDCnt = 3;
#else
        eCPT_ModID CPT_ModID = eCPT_L2_1;
        eCPT_MasterID CPT_MasterID[] = { eCPT_MID_GEM0, eCPT_MID_GEM1 };
        int CPT_MasterIDCnt = 2;
#endif

        if ( CPTErr = CPT_OpenModule(&pCPT_Handle_L2, CPT_ModID,  &CPTConfig_L2 ) )
        {
            CPT_CallBackLog(__FUNCTION__, CPTErr);
            exit(EXIT_FAILURE);
        }
                            
        //Setup masters and data qualifier 
        { 
            //Select two masters to monitor
            CPT_TPCntQual.NewReqSrcQual = eCPT_SrcQual_IncludeALL; 
            CPT_TPCntQual.RWQual = eCPT_RWQual_WriteOnly;
            
            //Log STM message and setup CP Tracer for Total Bandwidth Profile use case 
            // with multiple masters enabled. 
            // Warning: The GEM1 master is idle so it has no impact on the CPT data                    
            if (    ( eCPT_Success != CPT_LogMsg(pCPT_Handle_L2, "L2 GEM1 CP Tracer - Example 3, Total Bandwidth Profile, Multiple Master Selection", NULL ) )
                 || ( eCPT_Success != CPTCfg_TotalProfileMult( pCPT_Handle_L2, CPT_MasterID, CPT_MasterIDCnt, &CPT_TPCntQual ) ) )
            {
                //Errors handled by callback so just exit
                CPT_ErrorExit(pCPT_Handle_L2);
            }
        }
        
        //Start activity from this GEM (GEM0) to GEM1 L2  (writes 2x reads)                  
        { 
            int writeCnt = 0x10000-4;
            int readCnt  = 0x8000-4;            
            generate_L2SlaveActivity( writeCnt, readCnt, GEM1_L2_START+4 );
        }
                
        //Disable and close the CP Tracer module.
        if (    ( eCPT_Success != CPT_ModuleDisable (pCPT_Handle_L2, eCPT_WaitEnable ) )
             || ( eCPT_Success != CPT_CloseModule(&pCPT_Handle_L2 ) ) )
        {
            //Errors handled by callback so just exit 
            CPT_ErrorExit(pCPT_Handle_L2);
        }        
               
    }
#endif
#if MASTER_BANDWIDTH_PROFILING
    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Example 4  - Master Bandwidth Profile                                                        //
    // Throughput Counter 0: Select GEM0 master qualified for read cycles                          //
    // Throughput Counter 1: Select GEM0 master qualified for write cycles                         //
    // Typically you would use this capabilty to compare two different masters, but this is a      //
    //  simple example.                                                                            //
    // Note: This example is commented out on purpose. It causes the ETB to wrap and thus data     //
    //       from the previous examples is lost.                                                   //
    // Note: This example can not be ran at the same time as the Total Bandwidth Profile example.  //
    //       The reason for this is the host processing supports only one use case per data set.   // 
    /////////////////////////////////////////////////////////////////////////////////////////////////
    
    STMXport_printf(pSTMhdl, STM_HelperCh, "CPT Example - Master Bandwidth Profiling Use Case" );
    
    {    
        CPT_Qualifiers CPT_TPCnt0Qual;
        CPT_Qualifiers CPT_TPCnt1Qual;
        
#if defined(_66AK2Exx) || defined(_66AK2Gxx)
        eCPT_ModID CPT_ModID = eCPT_L2_0;
        eCPT_MasterID CPT_MasterID_1 = eCPT_MID_EDMA0_TC0_RD;
        eCPT_MasterID CPT_MasterID_2 = eCPT_MID_EDMA0_TC0_WR;
#else
        eCPT_ModID CPT_ModID = eCPT_L2_1;
        eCPT_MasterID CPT_MasterID_1 = eCPT_MID_GEM0;
        eCPT_MasterID CPT_MasterID_2 = eCPT_MID_GEM1;
#endif

        //Modify the use case
        CPTConfig_L2.CPT_UseCaseId = eCPT_UseCase_MasterProfile;

        CPTErr = CPT_OpenModule(&pCPT_Handle_L2, CPT_ModID,  &CPTConfig_L2 );
        if (  eCPT_Success != CPTErr )
        {
            CPT_CallBackLog(__FUNCTION__, CPTErr);
            exit(EXIT_FAILURE);
        }
    
        CPTErr = CPT_LogMsg(pCPT_Handle_L2, "L2 GEM1 CP Tracer - Example 4 ", NULL);

        //Setup data qualifier - Should yeild half as much data as Example 1
        CPT_TPCnt0Qual.NewReqSrcQual = eCPT_SrcQual_IncludeALL; 
        CPT_TPCnt0Qual.RWQual = eCPT_RWQual_ReadOnly;
        CPT_TPCnt1Qual.NewReqSrcQual = eCPT_SrcQual_IncludeALL; 
        CPT_TPCnt1Qual.RWQual = eCPT_RWQual_WriteOnly;        
        
        CPTErr = CPTCfg_MasterProfile( pCPT_Handle_L2, CPT_MasterID_1, &CPT_TPCnt0Qual,
        											   CPT_MasterID_2, &CPT_TPCnt1Qual);

                
        //Start activity from this GEM (GEM0) to GEM1 L2, GEM2 should already be runing             
         { 
            int writeCnt = 0x10000-4;
            int readCnt  = 0x10000-4;            
            generate_L2SlaveActivity(writeCnt, readCnt, GEM1_L2_START+4 );
        }
        
        CPTErr = CPT_ModuleDisable (pCPT_Handle_L2, eCPT_WaitEnable );
        if (  eCPT_Success != CPTErr )
        {
            CPT_ErrorExit(pCPT_Handle_L2);
        }
    
        CPTErr = CPT_CloseModule(&pCPT_Handle_L2 );
        if (  eCPT_Success != CPTErr )
        {
            CPT_ErrorExit(pCPT_Handle_L2);
        }
        
    }
#endif        
#if EVENT_USE_CASE
    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Example 5  - Event Use Case                                                                 //
    //                                                                                             //
    // Note: This example is commented out on purpose. It causes the ETB to wrap and thus data     //
    //       from the previous examples is lost.                                                   //
    /////////////////////////////////////////////////////////////////////////////////////////////////
    
    STMXport_printf(pSTMhdl, STM_HelperCh, "CPT Example - Event Use Case" );
    
    {
#if defined(_66AK2Exx) || defined(_66AK2Gxx)
        eCPT_ModID CPT_ModID = eCPT_L2_0;
        eCPT_MasterID CPT_MasterID[] = {eCPT_MID_EDMA0_TC0_RD, eCPT_MID_EDMA0_TC0_WR};
        int CPT_MasterIDCnt = 2;
#else
        eCPT_ModID CPT_ModID = eCPT_L2_1;
        eCPT_MasterID CPT_MasterID[] = { eCPT_MID_GEM0, eCPT_MID_GEM1 }; //Master enabled for Event B (New Request) and Event E (Last Read)
        int CPT_MasterIDCnt = 2;
#endif
        
        //Modify the use case
        CPTConfig_L2.CPT_UseCaseId = eCPT_UseCase_TotalProfile;

        if (  CPTErr = CPT_OpenModule(&pCPT_Handle_L2, CPT_ModID,  &CPTConfig_L2 ) )
        {
            CPT_CallBackLog(__FUNCTION__, CPTErr);
            exit(EXIT_FAILURE);
        }
            
        {

            // Warning: selecting more than one Event type could cause overflow if the access rate is high
            eCPT_MsgSelects CPT_MsgSelects = (eCPT_MsgSelects)eCPT_MsgSelect_Event_NewReq;
            CPT_Qualifiers CPT_TPEventQual;
            CPT_TPEventQual.NewReqSrcQual = eCPT_SrcQual_IncludeALL; 
            CPT_TPEventQual.RWQual = eCPT_RWQual_ReadWrite;        
            
            //Log out STM message and setup CP Tracer for event generation            
            if (    ( eCPT_Success != CPT_LogMsg(pCPT_Handle_L2, "L2 GEM1 CP Tracer - Example 5 Events ", NULL) )
                 || ( eCPT_Success != CPTCfg_Events( pCPT_Handle_L2, CPT_MasterID, CPT_MasterIDCnt, CPT_MsgSelects, &CPT_TPEventQual) ) )
            {
                //Errors handled by callback so just need to exit
                CPT_ErrorExit(pCPT_Handle_L2);
            }       
         
        }
               
        //Start activity from this GEM (GEM0) to GEM1 L2             
        { 
            int writeCnt = 0x80;
            int readCnt  = 0x40;            
            generate_L2SlaveActivity( writeCnt, readCnt, GEM1_L2_START+4 );
        }
        
        //Disable and close the CP Tracer module.
        if (    ( eCPT_Success != CPT_ModuleDisable (pCPT_Handle_L2, eCPT_WaitEnable ) )
             || ( eCPT_Success != CPT_CloseModule(&pCPT_Handle_L2 ) ) )
        {
            //Errors handled by callback so just need to exit 
            CPT_ErrorExit(pCPT_Handle_L2);
        }   
        
    }
#endif    
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

//Generate read/writes to the designated L2 space
void generate_L2SlaveActivity( int writeCnt, int readCnt, uint32_t addr )
{
#if defined(_66AK2Exx) || defined(_66AK2Gxx)
//Note: writeCnt, readCnt, and addr not used for 66AK2Exx L2 Activity
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
#else
    unsigned int i, n;
    int maxCnt = ( writeCnt >= readCnt ) ? writeCnt : readCnt;
    unsigned int* pTrig = ( unsigned int* )( GEM1_L2_START );
 
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
                
#if CPTRACER_DATA_POLLING
                // Normally you would want to either poll based on 
                // a CP Tracer interrupt. You could conceivably use
                // a timer interrupt whose period is ~2x the CP 
                // tracer period. 
                getCPTracerDataIfAvaiable(pCPT_Handle4polling);
#endif                
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
#endif
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

#if CPTRACER_DATA_POLLING

#define SAMPLE_MAX_CNT 100

struct _cpt_data {
    uint32_t cnt0;
    uint32_t cnt1;
    uint32_t waitCnt;
    uint32_t grantCnt;
    uint32_t access;
} cpt_data[SAMPLE_MAX_CNT];

static int sc = 0;

int getCPTracerDataIfAvaiable(CPT_Handle_Pntr const pCPT_Handle)
{
    eCPT_IntStatus CPT_IntStatus;
    
    if (sc == SAMPLE_MAX_CNT)
        return -1;
    
    CPT_GetIntStatus ( pCPT_Handle, &CPT_IntStatus);
    if ( eCPT_IntStatus_Active == CPT_IntStatus )
    {                                                                 
            CPT_GetCurrentState ( pCPT_Handle, &cpt_data[sc].cnt0, &cpt_data[sc].cnt1,  
                                               &cpt_data[sc].waitCnt, &cpt_data[sc].grantCnt,
                                               &cpt_data[sc].access);
                                               
           CPT_ClrIntStatus ( pCPT_Handle );
           
           sc++;                                    
    }
    
    return 0;    
}
#endif
