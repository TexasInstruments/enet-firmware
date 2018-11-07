#include "sci.h"
#include "STMLibrary.h"
#ifdef _DRA7xx
#include "DeviceSpecific_DRA7xx.h"
#endif
#ifdef _OMAP5430
#include "DeviceSpecific_OMAP5430.h"
#endif
#if defined(_OMAP4430) || defined(_OMAP4460)
#include "DeviceSpecific_OMAP44xx.h"
#endif
#include <stdio.h>


#define NumDMAChUsed 1
#define MAX_USECASES 6

//STM addresses
extern uint32_t STM_BaseAddress;
extern uint32_t STM_ChannelResolution; 

//STM channel assignements
#define STM_HelperCh 0
#define STM_SCILogCh 1


//Example Type - also note that examples can change with processor type
#define NO_FILTER_EXAMPLE 1   // Set to 0 to enable filter example
#define SAMPLE_MODE_EXAMPLE 0 // Set to 1 to enable application sampling
                              // Note: only valid for OMAP5430, and OMAP4470

#if SAMPLE_MODE_EXAMPLE
// Counter samples for each usecase placed in buf in the following order:
// sample index, usecase 0 counter, usecase 1 counter, ...
// sample index incremented for each sample.
#define MAX_NUM_SAMPLES 256
uint32_t buf[MAX_NUM_SAMPLES];
uint32_t * sample_buf = &buf[0];
#endif

//SDRAM usecase configuration
struct sci_config_sdram my_config_emif0 = {SCI_SDRAM_THROUGHPUT,
                                         SCI_EMIF1, 
                                         0}; 
                                         
struct sci_config_sdram my_config_emif1 = {SCI_SDRAM_THROUGHPUT,
                                         SCI_EMIF2, 
                                         0};
#if defined(_OMAP5430) || defined(_OMAP4460) || defined(_OMAP4470) || defined(_DRA7xx)
struct sci_config_sdram my_config_emif0a = {SCI_SDRAM_THROUGHPUT,
                                         SCI_MA_MPU_P1, 
                                         0};
                                         
struct sci_config_sdram my_config_emif1a = {SCI_SDRAM_THROUGHPUT,
                                         SCI_MA_MPU_P2, 
                                         0};       
#endif
struct sci_config_sdram my_config_emif3 = {SCI_SDRAM_THROUGHPUT,
                                         SCI_EMIF1, 
                                         1,
                                         //1st filter parameters
                                         SCI_MSTID_SDMA_WR, 
                                         0xff,
                                         SCI_WR_ONLY,
                                         SCI_ERR_DONTCARE,
                                         //2nd filter parameters - just filling in to avoid warnings
                                         SCI_MSTID_MPU, 
                                         0,
                                         SCI_RD_ONLY,
                                         SCI_ERR_DONTCARE 
#if defined(_OMAP5430) || defined(_OMAP4460) || defined(_OMAP4470) || defined(_DRA7xx)
                                         ,true,
                                         0x80340000, //Filter address must be aligned to 4K boundarys 
                                         0x80342000, //So SCILib shifts these 12 bits right
                                         0,0,false //Alarm parameters
#endif
                                         };

struct sci_config_sdram my_config_emif4 = {SCI_SDRAM_THROUGHPUT,
                                         SCI_EMIF2, 
                                         1,
                                         //1st filter parameters
                                         SCI_MSTID_SDMA_WR,
                                         0xff,
                                         SCI_WR_ONLY,
                                         SCI_ERR_DONTCARE,
                                         //2nd filter parameters - just filling in to avoid warnings
                                         SCI_MSTID_MPU,
                                         0,
                                         SCI_RD_ONLY,
                                         SCI_ERR_DONTCARE 
#if defined(_OMAP5430) || defined(_OMAP4460) || defined(_OMAP4470) || defined(_DRA7xx)
                                         ,true,
                                         0x80340000,
                                         0x80342000,
                                         0,0,false //Alarm parameters
#endif                                         
                                         };

struct sci_config_sdram my_config_emif5 = {SCI_SDRAM_THROUGHPUT,
                                         SCI_EMIF1, 
                                         1,
                                         SCI_MSTID_SDMA_RD,
                                         0xff,
                                         SCI_RD_ONLY,
                                         SCI_ERR_DONTCARE};

struct sci_config_sdram my_config_emif6 = {SCI_SDRAM_THROUGHPUT,
                                         SCI_EMIF2, 
                                         1,
                                         SCI_MSTID_SDMA_RD,
                                         0xff,
                                         SCI_RD_ONLY,
                                         SCI_ERR_DONTCARE};



#if NO_FILTER_EXAMPLE
#ifdef _OMAP4430
char * msg[] = { "SDRAM EMIF1 Test - Total SDRAM Throughput (DMA and MPU)", 
                 "SDRAM EMIF2 Test - Total SDRAM Throughput (DMA and MPU)" }; 

struct sci_config_sdram * pmy_cfg[] =  { &my_config_emif0,
                                         &my_config_emif1 };
#endif
#if defined(_OMAP5430) || defined(_OMAP4460) || defined(_OMAP4470) || defined(_DRA7xx)
char * msg[] = { "SDRAM EMIF1 Test - Total SDRAM Throughput (DMA cycles)", 
                 "SDRAM EMIF2 Test - Total SDRAM Throughput (DMA cycles)",
                 "SDRAM MPU_P1 Test - Total SDRAM Throughput (MPU cycles)",
                 "SDRAM MPU_P2 Test - Total SDRAM Throughput (MPU cycles)"}; 

struct sci_config_sdram * pmy_cfg[] =  { &my_config_emif0,
                                         &my_config_emif1,
                                         &my_config_emif0a,
                                         &my_config_emif1a };                                         

#endif
#else                                        
char * msg[] = { "SDRAM EMIF1 Test - SDRAM Throughput with sDMA_WR filter, WR Only",
                 "SDRAM EMIF2 Test - SDRAM Throughput with sDMA_WR filter, WR Only",
                 "SDRAM EMIF1 Test - SDRAM Throughput with sDMA_RD filter, RD Only",
                 "SDRAM EMIF2 Test - SDRAM Throughput with sDMA_RD filter, RD Only" };
                  
struct sci_config_sdram * pmy_cfg[] =  { &my_config_emif3,
                                         &my_config_emif4,
                                         &my_config_emif5,
                                         &my_config_emif6};
#endif

void generate_dma_accesses();
void DMA_Channel_Config( int chNum );
void DMA_Channel_Enable( int chNum );
int DMA_Complete ( int chNum );

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
    
        
    /////////////////////////////////////////////////////
    //Make sure DebugSS is powered up and enabled      //
    /////////////////////////////////////////////////////    
    my_sci_config.errhandler = sci_errhandler;
    my_sci_config.data_options = 0;         //Disable options
    my_sci_config.sdram_msg_rate = 8192;    //Counters will saturate if you set the value to high
    my_sci_config.mstr_msg_rate = 256;      
    my_sci_config.pstm_handle = pSTMhdl;
    my_sci_config.stm_ch = STM_SCILogCh;
    my_sci_config.stm_log_enable = false;   //Logging not supported yet
    my_sci_config.trigger_enable = false;
    my_sci_config.mode = SCI_MODE_STM;

    my_sci_err = sci_open(&psci_hdl, &my_sci_config);
    if (SCI_SUCCESS != my_sci_err) exit(-1);

    {                                                        
                                                                      
        int i;
        int num_use_cases = sizeof(pmy_cfg)/sizeof(struct sci_config_sdram *);
        psci_usecase_key my_usecase_key[MAX_USECASES] = {NULL, NULL, NULL, NULL, NULL, NULL};
        int valid_usecase_cnt = 0;

#if SAMPLE_MODE_EXAMPLE
        int total_cntrs = 0;
        int sample_cnt = 0;
#endif
            
        for (i=0; i<num_use_cases; i++) {                   
        
            STMXport_logMsg0(pSTMhdl, STM_HelperCh, msg[i] ); 
            my_sci_err = sci_reg_usecase_sdram(psci_hdl, pmy_cfg[i], &my_usecase_key[i] );
            
            if ( SCI_SUCCESS != my_sci_err) break; 
            
            valid_usecase_cnt++;
        }

#if SAMPLE_MODE_EXAMPLE
        sci_dump_info(psci_hdl, my_usecase_key, num_use_cases, &total_cntrs);

        STMXport_logMsg1(pSTMhdl, STM_HelperCh, "Number of counters is %d", total_cntrs);
        STMXport_logMsg1(pSTMhdl, STM_HelperCh, "Address of sample_buf is %x", (uint32_t)sample_buf);
#endif
        
        // Proceed with application code
        if (valid_usecase_cnt == num_use_cases)
        {
            
            int i;
            
            my_sci_err = sci_global_enable(psci_hdl);
            
            /* Wait before enable to show data with no DMA */
            {
                volatile int waitCnt = 2000;
                while ( waitCnt-- != 0);
            }
            
            
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
                    
                        if ( done[i] = DMA_Complete( i ))
                            STMXport_logMsg1(pSTMhdl, STM_HelperCh, "DMA %d Active",i );
                        else
                            STMXport_logMsg1(pSTMhdl, STM_HelperCh, "DMA %d Complete",i );
                    }

#if SAMPLE_MODE_EXAMPLE
                    if ((sample_cnt * (total_cntrs+1)) > (MAX_NUM_SAMPLES - (total_cntrs+1))){
                        printf("Max Sample Cnt exceeded");
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
                volatile int waitCnt = 2000;
                while ( waitCnt-- != 0);
            }
            my_sci_err = sci_global_disable(psci_hdl);
        }       
        else{
            printf(" SCI Lib Error %d\n", my_sci_err);
        } 

        for (i=0; i<valid_usecase_cnt; i++) {  
            my_sci_err = sci_remove_usecase( psci_hdl, &my_usecase_key[i]);
        }
   
    }

    my_sci_err = sci_close(&psci_hdl);
    if (SCI_SUCCESS != my_sci_err) exit(-2);
    
    /////////////////////////////////////////////////////
    //Initialize SC Library                            //
    /////////////////////////////////////////////////////

    exit(0);
}
