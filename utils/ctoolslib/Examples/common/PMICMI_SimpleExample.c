#include "PMILib.h"
#include "CMILib.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _DRA7xx
#include "DeviceSpecific_DRA7xx.h"
#endif
#ifdef _OMAP5430
#include "DeviceSpecific_OMAP5430.h"
#endif
#ifdef _OMAP4430
#include "DeviceSpecific_OMAP44xx.h"
#endif

#ifdef _ETB
    #include "ETBInterface.h"

    ETBHandle *pETBHandle = NULL;
    void transferEtbData(ETBStatus etbStatus, uint32_t* pBuffer);
    void writeDcmFile(uint8_t wrapped);
#endif

    STMHandle * pSTMHandle = NULL;
    STMXport_callback pSTMCallBack;

#define EXIT_ERROR_CODE       -1
#define EXIT_SUCCESS_CODE      0

/* Structure used to interface PMI module and one or two CMI   *
 * modules with the PMILib/CMILib APIs used in this example.   */
struct PMICMI_Lib_Interface
{
    PMI_Handle_Pntr pPMIHandle;
    PMI_CfgParams PMI_Config;
    CMI_Handle_Pntr pCMIHandles[eCMI_CM_ModuleCnt];
    CMI_CfgParams CMI_Configs[eCMI_CM_ModuleCnt];
};

int Open_PMICMI_Modules(struct PMICMI_Lib_Interface * pmicmi_lib_interface);
int Check_Library_Versions(struct PMICMI_Lib_Interface * pmicmi_lib_interface);
void Exit_Example(struct PMICMI_Lib_Interface * pmicmi_lib_interface, int exitCode);
void CMI_Log_Message(CMI_Handle_Pntr pCMI_Handles[], const char * logMessage);
int Enable_CMI_Modules(CMI_Handle_Pntr pCMI_Handles[], eCMI_ModEnableType);
int Disable_CMI_Modules(CMI_Handle_Pntr pCMI_Handles[], int32_t retain);

void PMI_Callback(const char * funcName, ePMI_Error errCode)
{
    const char *  errStr = PMI_GetErrorMsg(errCode);
    printf( "PMI Error function %s: %s\n", funcName, errStr);
}

void CM1_Callback(const char * funcName, ePMI_Error errCode)
{
    const char *  errStr = CMI_GetErrorMsg(errCode);
    printf( "CM1 Error function %s: %s\n", funcName, errStr);
}

void CM2_Callback(const char * funcName, ePMI_Error errCode)
{
    const char *  errStr = CMI_GetErrorMsg(errCode);
    printf( "CM2 Error function %s: %s\n", funcName, errStr);
}

#define BYTE_SWAP32(n) \
    ( ((((uint32_t) n) << 24) & 0xFF000000) |   \
      ((((uint32_t) n) <<  8) & 0x00FF0000) |   \
      ((((uint32_t) n) >>  8) & 0x0000FF00) |   \
      ((((uint32_t) n) >> 24) & 0x000000FF) )

void main(void) 
{
    //////////////////////////////////////////////////////////////////////////////
    // Declare variables                                                        //
    //////////////////////////////////////////////////////////////////////////////
#ifdef _ETB
    /* ETB parameters */
    eETB_Error        etbRet;
    ETB_errorCallback pETBErrCallBack = NULL;
    ETBStatus         etbStatus;
    uint32_t          etbWidth;
    uint32_t          *pEtbBuffer = NULL;
#endif

    int appError = EXIT_SUCCESS_CODE;
    ePCMI_Error retErr = ePCMI_Success;
    struct PMICMI_Lib_Interface pmicmi_lib_interface;

    pmicmi_lib_interface.pPMIHandle = 0;
    pmicmi_lib_interface.pCMIHandles[0] = 0;
#ifndef _DRA7xx
    pmicmi_lib_interface.pCMIHandles[1] = 0;
#endif

    pmicmi_lib_interface.PMI_Config.PMI_Module_BaseAddr = PMI_BASE_ADDRESS;
    pmicmi_lib_interface.PMI_Config.PMI_ClockCtl_BaseAddr = PMI_CLKCTRL_REG;
    pmicmi_lib_interface.PMI_Config.pPMI_CallBack = PMI_Callback;
    pmicmi_lib_interface.PMI_Config.pSTMHandle = NULL;
    pmicmi_lib_interface.PMI_Config.STMMessageCh = 0;
    pmicmi_lib_interface.PMI_Config.FuncClockDivider = 16;
    pmicmi_lib_interface.PMI_Config.SampleWindowSize = 256;

    pmicmi_lib_interface.CMI_Configs[0].CMI_Module_BaseAddr = CM1_BASE_ADDRESS;
    pmicmi_lib_interface.CMI_Configs[0].CMI_ClockCtl_BaseAddr = CM1_CLKCTRL_REG;
    pmicmi_lib_interface.CMI_Configs[0].pCMI_CallBack = CM1_Callback;
    pmicmi_lib_interface.CMI_Configs[0].pSTMHandle = NULL;
    pmicmi_lib_interface.CMI_Configs[0].STMMessageCh = 0;
    pmicmi_lib_interface.CMI_Configs[0].FuncClockDivider = 16;
    pmicmi_lib_interface.CMI_Configs[0].SampleWindowSize = 256;

    /* OMAP4430 and OMAP5430 have two clock modules */
#ifndef _DRA7xx
    pmicmi_lib_interface.CMI_Configs[1].CMI_Module_BaseAddr = CM2_BASE_ADDRESS;
    pmicmi_lib_interface.CMI_Configs[1].CMI_ClockCtl_BaseAddr = CM2_CLKCTRL_REG;
    pmicmi_lib_interface.CMI_Configs[1].pCMI_CallBack = CM2_Callback;
    pmicmi_lib_interface.CMI_Configs[1].pSTMHandle = NULL;
    pmicmi_lib_interface.CMI_Configs[1].STMMessageCh = 0;
    pmicmi_lib_interface.CMI_Configs[1].FuncClockDivider = 16;
    pmicmi_lib_interface.CMI_Configs[1].SampleWindowSize = 256;
#endif

    STMBufObj * pSTMBufInfo = 0;
    STMConfigObj STMConfig;

    STMConfig.STM_CntlBaseAddr = STM_CONFIG_BASE_ADDR;
    STMConfig.xmit_printf_mode = eSend_optimized;
    STMConfig.optimize_strings =  false;
    STMConfig.STM_XportBaseAddr = STM_XPORT_BASE_ADDR;
    STMConfig.STM_ChannelResolution = STM_CHAN_RESOLUTION;

    STMConfig.pCallBack             = NULL;
  
    //////////////////////////////////////////////////////////////////////////////
    // Open STM Lib in blocking mode, then open an instance of the PMI Library  //
    // and two instances of the CMI Library (CM1 and CM2).                      //
    //////////////////////////////////////////////////////////////////////////////
    pSTMHandle = STMXport_open(pSTMBufInfo, &STMConfig);
    if ( NULL == pSTMHandle )
    {
        printf("STM Library open error - exiting\n");
        exit(EXIT_ERROR_CODE);
    }
    
    pmicmi_lib_interface.PMI_Config.pSTMHandle = pSTMHandle;
    pmicmi_lib_interface.CMI_Configs[0].pSTMHandle = pSTMHandle;
#ifndef _DRA7xx
    pmicmi_lib_interface.CMI_Configs[1].pSTMHandle = pSTMHandle;
#endif

    /* Message for logging with PMI_LogMsg and CMI_Log_Message functions */
    char * loggingMsg;

#ifdef _ETB

    //Setup routing to get STM data to the ETB
    Config_STMData_Routing();
    // In some cases the STM module can transport a SYNC message
    // thus the reason the ETB must be setup first.
    Config_STM_for_ETB(pSTMHandle);

	/* Open ETB module */
	etbRet = ETB_open(pETBErrCallBack, eETB_Circular, 0, &pETBHandle, &etbWidth);
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

	// Open all PMI and CMI modules.
	appError = Open_PMICMI_Modules(&pmicmi_lib_interface);
	if (appError != EXIT_SUCCESS_CODE)
		Exit_Example(&pmicmi_lib_interface, appError);

	// Check the versions and function IDs for each module.
	appError = Check_Library_Versions(&pmicmi_lib_interface);
	if (appError != EXIT_SUCCESS_CODE)
		Exit_Example(&pmicmi_lib_interface, appError);
 
	// CMI_LogMsg can be used to log clock modifications
	// Example: "Switching A8_1 clock to low freq in 500Usec"
	// Note : PMI_LogMsg can be used to log device power modifications that you expect
	// to see in the HW monitoring.

	loggingMsg = "Enabling PM for event monitoring";
    PMI_LogMsg(pmicmi_lib_interface.pPMIHandle, loggingMsg, NULL);
    loggingMsg = "Enabling CM%d for event monitoring";
    CMI_Log_Message(pmicmi_lib_interface.pCMIHandles, loggingMsg);

    // The CMI_Mark and PMI_Mark functions provide a standard set of messages
    // for logging clock and power state changes that should be reflected in the
    // PMI and CMI HW monitored data. Since these messages are standard you can use
    // these with higher level tools (CCS) to measure the time between when the SW
    // changes the state and the state change actually occurs.
   
    //////////////////////////////////////////////////////////////////////////////
    // Enable PMI for Event monitoring                                          //
    //////////////////////////////////////////////////////////////////////////////
   
    //If you wish to modify the sample window size or the events that are enabled
    //use:
    // PMI_ConfigModule() to modify start/stop triggers or the event enables
    // PMI_SetSampleWindow() to modify the sample window.
    // If you want to modify these parameters after PMI_ModuleActivityEnable() has been called
    // you must call PMI_ModuleActivityDisable() first.
    
    // Enable Module activity using the default parameters 
    // All events enables and the sample window size is set to 4096
    retErr = PMI_ModuleActivityEnable(pmicmi_lib_interface.pPMIHandle);
    if ( ePCMI_Success != retErr )
    {
    	Exit_Example(&pmicmi_lib_interface, EXIT_ERROR_CODE);
    }

    //////////////////////////////////////////////////////////////////////////////
    // Enable CM1 and CM2 for Event monitoring                                  //
    //////////////////////////////////////////////////////////////////////////////
    
    //If you wish to modify the sample window size or the events that are enabled
    //use:
    // CMI_ConfigModule() to modify start/stop triggers or the event enables
    // CMI_SetSampleWindow() to modify the sample window.
    // If you want to modify these parameters after CMI_ModuleEnable() has been called
    // you must call CMI_ModuleDisable() first.

    appError = Enable_CMI_Modules(pmicmi_lib_interface.pCMIHandles, eCMI_Event);
	if (appError != EXIT_SUCCESS_CODE)
		Exit_Example(&pmicmi_lib_interface, appError);

    //////////////////////////////////////////////////////////////////////////////
    // Loop here until waitCnt decrements to 0                                  //
    //////////////////////////////////////////////////////////////////////////////
	const int waitCntStart = 2500;
	int waitCnt = waitCntStart;
	bool oneTime = false;
        
	//The first half of the following stall loop is used to allow time for the
	//PM, CM1 and CM2 event monitoring, while the second half of the stall loop
	//is for CM1 and CM2 activity monitoring.
	printf("Entering stall loop\n");
        
	while ( waitCnt--)
	{
		if (( waitCnt < ( waitCntStart/2 ) ) && ( false == oneTime ))
		{
			// CMI_LogMsg can be used to log clock modifications
			// Example: "Switching A8_1 clock to low freq in 500Usec"
			// Note : PMI_LogMsg can be used to log device power modifications that you expect
			// to see in the HW monitoring.
			loggingMsg = "Switching CM%d from event to activity monitoring";
			CMI_Log_Message(pmicmi_lib_interface.pCMIHandles, loggingMsg);

			//The CMI_Mark and PMI_Mark functions provide a standard set of messages
			//for logging clock and power state changes that should be reflected in the
			//PMI and CMI HW monitored data. Since these messages are standard you can use
			//these with higher level tools (CCS) to measure the time between when the SW
			//changes the state and the state change actually occurs.
   
			//////////////////////////////////////////////////////////////////////////////
			// Disable CM1 for Event monitoring                                         //
			//////////////////////////////////////////////////////////////////////////////
			appError = Disable_CMI_Modules(pmicmi_lib_interface.pCMIHandles, false);
			if (appError != EXIT_SUCCESS_CODE)
				Exit_Example(&pmicmi_lib_interface, appError);
    
            //////////////////////////////////////////////////////////////////////////////
            // Enable CM1 and CM2 for Activity monitoring                               //
            //////////////////////////////////////////////////////////////////////////////
            //If you wish to modify the sample window size or the events that are enabled
            //use:
            // CMI_ConfigModule() to modify start/stop triggers or the event enables
            // CMI_SetSampleWindow() to modify the sample window.
            // If you want to modify these parameters after CMI_ModuleEnable() has been called
            // you must call CMI_ModuleDisable() first.
		    appError = Enable_CMI_Modules(pmicmi_lib_interface.pCMIHandles, eCMI_Activity);
			if (appError != EXIT_SUCCESS_CODE)
				Exit_Example(&pmicmi_lib_interface, appError);
    
			oneTime = true;
        }
    }
    //////////////////////////////////////////////////////////////////////////////
    // Disable PMI for Event monitoring and close PMI Library                   //
    //////////////////////////////////////////////////////////////////////////////
	int32_t retain = false;
	retErr =  PMI_ModuleActivityDisable(pmicmi_lib_interface.pPMIHandle, retain);
	if ( ePCMI_Success != retErr )
		Exit_Example(&pmicmi_lib_interface, EXIT_ERROR_CODE);
 
    //////////////////////////////////////////////////////////////////////////////
    // Disable CM1 and CM2 for Activity monitoring and close CMI Library        //
    //////////////////////////////////////////////////////////////////////////////
	appError = Disable_CMI_Modules(pmicmi_lib_interface.pCMIHandles, false);
	if (appError != EXIT_SUCCESS_CODE)
		Exit_Example(&pmicmi_lib_interface, appError);

    STMXport_flush(pSTMHandle);

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
            transferEtbData(etbStatus, pEtbBuffer);
            writeDcmFile(etbStatus.isWrapped);
	}
    else {
		printf("Error getting ETB status (%d)\n", etbRet);
    }
	
	/*** Now we are done and close all the handles **/
	etbRet  = ETB_close(pETBHandle);
	if(etbRet != eETB_Success)
	{
		printf("Error closing ETB (%d)\n", etbRet);
		return;
	}
    if(pEtbBuffer != NULL) {
        free(pEtbBuffer);
    }
#endif

    Exit_Example(&pmicmi_lib_interface, EXIT_SUCCESS_CODE);

}

int Open_PMICMI_Modules(struct PMICMI_Lib_Interface * pmicmi_lib_interface)
{
	ePCMI_Error retErr;
	eCMI_ModID i;
	int returnCode = EXIT_SUCCESS_CODE;

	retErr = PMI_OpenModule(&pmicmi_lib_interface->pPMIHandle, ePMI_PM, &pmicmi_lib_interface->PMI_Config);

	if ( ePCMI_Success != retErr )
	{
		printf("PMI Library open error - exiting (%d)\n", retErr);
		returnCode = EXIT_ERROR_CODE;
	}

	for(i=eCMI_CM1; i<eCMI_CM_ModuleCnt; i++)
	{
		retErr = CMI_OpenModule(&pmicmi_lib_interface->pCMIHandles[i], i, &pmicmi_lib_interface->CMI_Configs[i]);

		if ( ePCMI_Success != retErr )
		{
	    	printf("CM%d Library open error - exiting (%d)\n", i+1, retErr);
	    	returnCode = EXIT_ERROR_CODE;
		}
	}

	return returnCode;
}

int Check_Library_Versions(struct PMICMI_Lib_Interface * pmicmi_lib_interface)
{
    uint32_t libMajorVersion, libMinorVersion, libFuncID, modFuncID;
    ePMI_Error pmiErr;
    eCMI_Error cmiErr;
    eCMI_ModID i;
    int returnCode = EXIT_SUCCESS_CODE;

    pmiErr = PMI_GetVersion(pmicmi_lib_interface->pPMIHandle, &libMajorVersion, &libMinorVersion, &libFuncID, &modFuncID);
    if (pmiErr != ePCMI_Success)
    	returnCode = EXIT_ERROR_CODE;
    printf("PMILib version %d.%d\n", libMajorVersion, libMinorVersion);

	for(i=eCMI_CM1; i<eCMI_CM_ModuleCnt; i++)
	{
		cmiErr = CMI_GetVersion(pmicmi_lib_interface->pCMIHandles[i], &libMajorVersion, &libMinorVersion, &libFuncID, &modFuncID);
		if (cmiErr != ePCMI_Success)
			returnCode = EXIT_ERROR_CODE;
		printf("CM%dLib version %d.%d\n", i+1, libMajorVersion, libMinorVersion);
	}

    return returnCode;
}

int Enable_CMI_Modules(CMI_Handle_Pntr pCMI_Handles[], eCMI_ModEnableType cmiEnableType)
{
	int returnCode = EXIT_SUCCESS_CODE;
	eCMI_Error cmiErr;
	eCMI_ModID i;

	for(i=eCMI_CM1; i<eCMI_CM_ModuleCnt; i++)
	{
		cmiErr = CMI_ModuleEnable(pCMI_Handles[i], cmiEnableType);
		if (cmiErr != ePCMI_Success) {
			printf("Failure enabling CM%d\n",i+1);
			returnCode = EXIT_ERROR_CODE;
		}
	}

	return returnCode;
}

int Disable_CMI_Modules(CMI_Handle_Pntr pCMI_Handles[], int32_t retain)
{
	int returnCode = EXIT_SUCCESS_CODE;
	eCMI_Error cmiErr;
	eCMI_ModID i;

	for(i=eCMI_CM1; i<eCMI_CM_ModuleCnt; i++)
	{
		cmiErr = CMI_ModuleDisable(pCMI_Handles[i], retain);
		if (cmiErr != ePCMI_Success) {
			printf("Failure disabling CM%d\n",i+1);
			returnCode = EXIT_ERROR_CODE;
		}
	}

	return returnCode;
}

void CMI_Log_Message(CMI_Handle_Pntr pCMI_Handles[], const char * logMessage)
{
	eCMI_ModID i;
	int32_t modID;

	for(i=eCMI_CM1; i<eCMI_CM_ModuleCnt; i++)
	{
		modID = i+1;
		CMI_LogMsg(pCMI_Handles[i], logMessage, &modID);
	}
}

void Exit_Example(struct PMICMI_Lib_Interface * pmicmi_lib_interface, int exitCode)
{
	eCMI_ModID i;
    printf ("terminating with exit code %d\n", exitCode);

    //Close all modules before exiting
    PMI_CloseModule(pmicmi_lib_interface->pPMIHandle);

	for(i=eCMI_CM1; i<eCMI_CM_ModuleCnt; i++)
	{
		CMI_CloseModule(pmicmi_lib_interface->pCMIHandles[i]);
	}

    if(pSTMHandle != NULL)
    {
        STMXport_close(pSTMHandle);
    }

    exit(exitCode);
}

#ifdef _ETB
/*******************************************************************************
 * 
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
        char * pFileName = "C:\\temp\\STM-PMCM_etbdata.bin";
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
void writeDcmFile(uint8_t wrapped)
{
    char *file_name = "C:\\temp\\STM-PMCM_etbdata.dcm";
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

