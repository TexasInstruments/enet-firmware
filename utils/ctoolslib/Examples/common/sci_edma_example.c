#include "sci.h"
#include "STMLibrary.h"
#ifdef _AM437x
#include "DeviceSpecific_AM437x.h"
#endif
#ifdef _TDA3x
#include "DeviceSpecific_TDA3x.h"
#endif
#include <stdio.h>

#ifdef _ETB
#include "ETBInterface.h"

ETBHandle *pETBHandle = NULL;
void transferEtbData(ETBStatus etbStatus, uint32_t* pBuffer);
void writeDcmFile(STMHandle* pSTMHandle, uint8_t wrapped);
#endif

void DMA_Channel_Config( int chNum );
void DMA_Channel_Enable( int chNum );
int DMA_Channel_Complete ( int chNum );

#define NumDMAChUsed 1
#define MAX_USECASES 6

//STM addresses
extern uint32_t STM_BaseAddress;
extern uint32_t STM_ChannelResolution; 

//STM channel assignments
#define STM_HelperCh 0
#define STM_SCILogCh 1


//Example Type - also note that examples can change with processor type
#define SAMPLE_MODE_EXAMPLE 0 // Set to 1 to enable application sampling
                              // Note: only valid for OMAP5430, and OMAP4470

#if SAMPLE_MODE_EXAMPLE
// Counter samples for each usecase placed in buf in the following order:
// sample index, usecase 0 counter, usecase 1 counter, ...
// sample index incremented for each sample.
#define MAX_NUM_SAMPLES 1024
uint32_t buf[MAX_NUM_SAMPLES];
uint32_t * sample_buf = &buf[0];
#endif

//DDR usecase configuration
#ifdef _AM437x
/* Note: the master filters in sci_config_mstr are only valid/present for AM437x
 * because this device combines both master and slave probes in the LAT (mstr)
 * Statistic Collectors.
 */
struct sci_config_mstr my_config_emif = {SCI_MSTR_THROUGHPUT,
										SCI_EMIF,
                                         1,
                                         //1st filter parameters
                                         SCI_MSTID_MPU_128,
                                         0xff, //0xff,
                                         SCI_SLVID_EMIF,
                                         0xff,
                                         SCI_WR_ONLY,
                                         SCI_ERR_DONTCARE
										};

struct sci_config_mstr my_config_tc0_rd = {SCI_MSTR_THROUGHPUT,
		                                SCI_EDMA3TC0_RD,//SCI_EMIF,
                                         1,
                                         //1st filter parameters
                                         SCI_MSTID_EDMA3TC0_RD,
                                         0xff,
                                         SCI_SLVID_EMIF,
                                         0xff,
                                         SCI_RD_ONLY,
                                         SCI_ERR_DONTCARE
										};

struct sci_config_mstr my_config_tc0_wr = {SCI_MSTR_THROUGHPUT,
		                                SCI_EDMA3TC0_WR,
                                         1,
                                         //1st filter parameters
                                         SCI_MSTID_EDMA3TC0_WR,
                                         0xff,
                                         SCI_SLVID_EMIF,
                                         0xff,
                                         SCI_WR_ONLY,
                                         SCI_ERR_DONTCARE
										};


char * msg[] = { "DDR EMIF Throughput Profile - (DMA and MPU)" };

struct sci_config_mstr * pmy_cfg[] =  { &my_config_emif, &my_config_tc0_rd, &my_config_tc0_wr };

#endif

#ifdef _TDA3x
/* Note: the master filters in sci_config_mstr are only valid/present for AM437x
 * because this device combines both master and slave probes in the LAT (mstr)
 * Statistic Collectors.
 */

struct sci_config_mstr my_config_emif = {SCI_MSTR_THROUGHPUT,
                                         SCI_EMIF_SYS,
                                         1,
                                         //1st filter parameters
                                         SCI_MSTID_IPU,
                                         0xff,
                                         SCI_SLVID_IPU, /* Since this is a master LAT the meta data gets the name of the filter from the slave match register*/
                                         0,
                                         SCI_WR_ONLY, // for M4_0,
                                         SCI_ERR_DONTCARE
                                        };


struct sci_config_mstr my_config_tc0_rd = {SCI_MSTR_THROUGHPUT,
                                         SCI_DSP1_EDMA,
                                         1,
                                         //1st filter parameters
                                         SCI_MSTID_DSP1_EDMA, /* Since the mask is 0 this is a don't care */
                                         0,
                                         SCI_SLVID_EMIF_SYS,
                                         0xff,
                                         SCI_RD_OR_WR_DONTCARE,
                                         SCI_ERR_DONTCARE
                                        };

char * msg[] = { "LAT0 - IPU to DDR Throughput Profile",
                 "LAT2 - EDMA to DDR Throughput Profile"};

struct sci_config_mstr * pmy_cfg[] =  { &my_config_emif, &my_config_tc0_rd };

#endif



//If the example is built with "C++", the callback function must be exported as a "C" function.
#ifdef __cplusplus
extern "C" {
#endif
void sci_errhandler(psci_handle phandle, const char * func, enum sci_err err)
{
    enum sci_err my_sci_err = SCI_SUCCESS;
    
    printf("SCILib failure %d in function: %s \n", err, func );
    
    if (NULL != phandle)
          my_sci_err = sci_close(&phandle);
          
    if ( my_sci_err )
        exit(-2);
    else
        exit (-3);
}
#ifdef __cplusplus
}
#endif

int main()
{
#ifdef _TDA3x
    Config_Device();
#endif
#ifdef _ETB
    // ETB parameters
    eETB_Error        etbRet;
    ETB_errorCallback pETBErrCallBack = NULL;
    ETBStatus         etbStatus;
    uint32_t          etbWidth;
    uint32_t          *pEtbBuffer = NULL;

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

    //STM declarations
    STMHandle * pSTMhdl;                                // STMLib handle pointer  
    STMBufObj * pSTMBufInfo = NULL;                     // Set pSTMBufInfo to NULL to set STM Lib for blocking IO 
    STMConfigObj STMConfigInfo;                         // STMLib configuration options struct 

    //SC declarations
    
    struct sci_config my_sci_config;
    psci_handle psci_hdl = NULL;
    enum sci_err my_sci_err;
    
    //////////////////////////////////////////////////////
    //Initialize STM Library                            //
    //////////////////////////////////////////////////////

    STMConfigInfo.xmit_printf_mode = eSend_optimized;
    STMConfigInfo.optimize_strings = true;               // Not forming any strings on the fly so optimized STM printf ok
    STMConfigInfo.STM_XportBaseAddr = STM_XPORT_BASE_ADDR; // Set STM Base address
    STMConfigInfo.STM_ChannelResolution = STM_CHAN_RESOLUTION; //Set STM Channel resolution
    STMConfigInfo.STM_CntlBaseAddr = STM_CONFIG_BASE_ADDR;
    STMConfigInfo.pCallBack = NULL;                     // Set STM Callback to NULL 

    // Open STMLib
    pSTMhdl = STMXport_open( pSTMBufInfo, &STMConfigInfo);
    if ( NULL == pSTMhdl ) exit(EXIT_FAILURE);
    
#ifdef _ETB
    //Setup routing to get STM data to the ETB
    Config_STMData_Routing();

    // In some cases the STM module can transport a SYNC message
    // thus the reason the ETB must be setup first.
    Config_STM_for_ETB(pSTMhdl);
#endif
    /////////////////////////////////////////////////////
    //Make sure DebugSS is powered up and enabled      //
    /////////////////////////////////////////////////////    
    my_sci_config.errhandler = sci_errhandler;
    my_sci_config.data_options = 0;         //Disable options
    my_sci_config.sdram_msg_rate = 8192;    //Counters will saturate if you set the value to high
#ifdef _TDA3x
    my_sci_config.mstr_msg_rate = 262144;
#else
    my_sci_config.mstr_msg_rate = 65536;
#endif
    my_sci_config.pstm_handle = pSTMhdl;
    my_sci_config.stm_ch = STM_SCILogCh;
    my_sci_config.stm_log_enable = false;   //Logging not supported yet
    my_sci_config.trigger_enable = false;
    my_sci_config.mode = SCI_MODE_STM;

    my_sci_err = sci_open(&psci_hdl, &my_sci_config);
    if (SCI_SUCCESS != my_sci_err) exit(-1);

    {                                                        
                                                                      
        int i;
        int num_use_cases = sizeof(pmy_cfg)/sizeof(struct sci_config_mstr *);
        psci_usecase_key my_usecase_key[MAX_USECASES] = {NULL, NULL, NULL, NULL, NULL, NULL};
        int valid_usecase_cnt = 0;

#if SAMPLE_MODE_EXAMPLE
        int total_cntrs = 0;
        int sample_cnt = 0;
#endif
            
        for (i=0; i<num_use_cases; i++) {                   
        
            STMXport_logMsg0(pSTMhdl, STM_HelperCh, msg[i] ); 
            my_sci_err = sci_reg_usecase_mstr(psci_hdl, pmy_cfg[i], &my_usecase_key[i] );
            
            if ( SCI_SUCCESS != my_sci_err) break; 
            
            valid_usecase_cnt++;
        }

#if SAMPLE_MODE_EXAMPLE
        sci_dump_info(psci_hdl, my_usecase_key, num_use_cases, &total_cntrs);

        printf("Number of counters is %d\n", total_cntrs);
        printf("Address of sample_buf is 0x%x\n", (uint32_t)sample_buf);
#endif
        
        // Proceed with application code
        if (valid_usecase_cnt == num_use_cases)
        {
            
            int i;
            
            my_sci_err = sci_global_enable(psci_hdl);

            for (i =0; i < NumDMAChUsed; i++) { 
                DMA_Channel_Config( i );
                STMXport_logMsg1(pSTMhdl, STM_HelperCh, "DMA %d Starts Now", i );
            }

            for (i =0; i < NumDMAChUsed; i++) {
                 DMA_Channel_Enable( i );
            }
            {   //Wait for the last DMA to complete
                int done[NumDMAChUsed];
                
                do
                {
                    int numDMAChDone = 0;
                    
                    for (i =0; i < NumDMAChUsed; i++) {
                    
                        if ( done[i] = DMA_Channel_Complete( i ))
                            STMXport_logMsg1(pSTMhdl, STM_HelperCh, "DMA %d Active",i );
                        else
                            STMXport_logMsg1(pSTMhdl, STM_HelperCh, "DMA %d Complete",i );
                    }

#if SAMPLE_MODE_EXAMPLE
                    if ((sample_cnt * (total_cntrs+1)) > (MAX_NUM_SAMPLES/(total_cntrs+1))){
                        printf("Max Sample Cnt exceeded\n");
                    }
                    else {
                        //Add samples to buffer     
                        *sample_buf++ = sample_cnt++; 
                        sci_dump_cntrs(psci_hdl, my_usecase_key, num_use_cases, sample_buf);
                        sample_buf += total_cntrs;
                    }
#endif
                    for (i =0; i < NumDMAChUsed; i++){
                        if ( done[i] == 0)
                            numDMAChDone++;
                    }
                    
                    if ( numDMAChDone == NumDMAChUsed ) break;

                }while (1);
            }

            /* Wait before disable to get last data from SC Module*/
            {
                volatile int waitCnt = 2048;
                while ( waitCnt-- != 0);
            }

            my_sci_err = sci_global_disable(psci_hdl);

#if SAMPLE_MODE_EXAMPLE
           printf("Number of samples is %d\n", sample_cnt);
#endif

        }       
        else{
            printf(" SCI Lib Error %d\n", my_sci_err);
        } 

        for (i=0; i<valid_usecase_cnt; i++) {  
            my_sci_err = sci_remove_usecase( psci_hdl, &my_usecase_key[i]);
        }
   
    }

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
			printf("Transporting ETB and DCM data\n");
            transferEtbData(etbStatus, pEtbBuffer);
            writeDcmFile(pSTMhdl, etbStatus.isWrapped);
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

    my_sci_err = sci_close(&psci_hdl);
    if (SCI_SUCCESS != my_sci_err) exit(-2);
    
    /////////////////////////////////////////////////////
    //Initialize SC Library                            //
    /////////////////////////////////////////////////////

    exit(0);
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

			printf("Successfully transported ETB data to: %s\n", pFileName);

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
        printf("Successfully transported DCM data to: %s\n", file_name);
    }
    else {
        printf("Error opening %s for writing\n", file_name);
    }

    return;
}

#endif
