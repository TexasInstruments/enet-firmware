/**
 * main.c
 */
#include "stdlib.h"
#include "stdio.h"
#include "ETBInterface.h"
#include "ETBDeviceSpecific.h"
#include "k3_rat.h"
#include "target_access.h"
#include "cpt2.h"
#include "cpt2_helper.h"
#include "trace_aggr.h"
#include "atbrep.h"
#include "gtc.h"


/****************************************************************************/
// Global Variables
/****************************************************************************/
//ETBHandle etbHandle;
ETBHandle* pETBHandle = NULL;
cpt2_handle_t cpt2Handle;
trace_aggr_handle_t traceAggrHandle;
atbrep_handle_t atbRepHandle;
gtc_handle_t gtcHandle;
cpt2_options_t cpt2Options;
uint8_t  cpt2_mst_id;

/****************************************************************************/
/* Private definitions and declarations */
/****************************************************************************/
#define BYTE_SWAP32(n) \
    ( ((((uint32_t) n) << 24) & 0xFF000000) |   \
      ((((uint32_t) n) <<  8) & 0x00FF0000) |   \
      ((((uint32_t) n) >>  8) & 0x0000FF00) |   \
      ((((uint32_t) n) >> 24) & 0x000000FF) )
#define DEFAULT_ATB_ID      0x30 // This value can be changed to anything with 0-127 range

typedef enum {
  eTransaction,
  eThroughput,
  eLatency
} cpt2_mode_t;

// RAT related definitions
#if defined(RAT_ON)

#define pulsar1_rat_cfg_base    (0x40f90000)
#define dmsc_rat_cfg_base       (0x44200000)

#define mkptr64(base,offset)      ((volatile uint64_t *)(base+(offset)))
#define mkptr32(base,offset)      ((volatile uint32_t *)(base+(offset)))
#define mkptr16(base,offset)      ((volatile uint16_t *)(base+(offset)))
#define mkptr8(base,offset)       ((volatile uint8_t *)(base+(offset)))
#define mkptr(base,offset)        mkptr32(base,offset)

static uint32_t set_rat_region(
        uint32_t rat_cfg_base,
        uint32_t region_id,
        uint32_t base_addr,
        uint64_t xlate_addr,
        uint32_t region_width);

#endif

//ETB and CPTracer2 Helper functions
static uint8_t transferETBData(ETBHandle* pETBHandle, ETBStatus etbStatus, uint32_t* pBuffer, char * pBinFileName);
static uint8_t transferCPT2Config(cpt2_options_t * p_options, uint32_t atb_id, char * pDcmFileName);
static uint8_t CPT2TBR_PostProcess(cpt2_options_t * p_options, ETBHandle* pETBHandle, uint32_t atb_id);

uint8_t trace_open(cpt2pb_id_t pb_id);
uint8_t trace_close();
uint8_t trace_enable(cpt2_options_t * p_options);
uint8_t trace_disable();

/****************************************************************************/
/* main() */
/****************************************************************************/
int main(void)
{
    uint8_t err = 0;
    uint64_t dbgcell_baddr_toMap;
    uint64_t cpt2_aggr_baddr_toMap;

    // Select which CPTracer2 probe to use, this is device specific
#if defined(AM654x)
    cpt2pb_id_t cpt2_probe_id = eCpTracer2_Probe_6; //eCpTracer2_Probe_6 is the probe for EMIF0 initiator, <USER CAN MODIFY>
#elif defined(J7ES)
    cpt2pb_id_t cpt2_probe_id = eCpTracer2_Probe_10; //eCpTracer2_Probe_10 is the probe for EMIF0 initiator, <USER CAN MODIFY>
#endif

    // Set up the CPTracer2 probe operations (transaction, throughput, latency etc.)
    cpt2_mode_t mode = eThroughput;//eTransaction;//;//eTransaction;//eLatency;//Change this for a different use case. <USER CAN MODIFY>

    /****************************************************************************/
    // For Cortex-R5 and M3, we need to configure RAT to map the address of debug
    // components in 64-bit address space in the system memory view to the
    // 32-bit address space in the CPU view.
    //
    // We need to map two regions:
    // 1) 64KB region for Debug Cell
    // 2) 256KB region for CPTracer2
    //
    // Note:
    // The following two addresses can be changed as needed for user application.
    // Definition can be found in common_J7ES.h file
    // 1) DBGCELL_BADDR_MAPPED
    // 2) CPT2_BADDR_MAPPED
    /****************************************************************************/
#if defined(RAT_ON)

    uint32_t region_id;
	uint32_t region_width;

	// Based on the CPT2 probe ID, get the base address for the corresponding Debug Cell and Trace Aggregator for RAT mapping
    err = CPT2H_get_addrToMap(cpt2_probe_id, &dbgcell_baddr_toMap, &cpt2_aggr_baddr_toMap);
    if (0 != err) {
        return 1;
    }


	// Configure RAT to map Debug Cell to 32-bit CPU memory view -- 64KB
	region_id = 0;
	region_width = 16;
    if (0 != set_rat_region(pulsar1_rat_cfg_base, region_id, DBGCELL_BADDR_MAPPED, dbgcell_baddr_toMap, region_width))
    {
        printf("Failed to configure RAT for Debug Cell access.\n");
        //exit (1);
        return 1;
    }

    // Configure RAT to map CPTracer2 trace aggregator to 32-bit CPU memory view -- 256KB
	region_id = 1;
	region_width = 18;
	if (0 != set_rat_region(pulsar1_rat_cfg_base, region_id, CPT2_BADDR_MAPPED, cpt2_aggr_baddr_toMap, region_width))
    {
        printf("Failed to configure RAT for CPTracer2 trace aggregator access.\n");
        //exit (1);
	    return 1;
    }

#endif


    err = trace_open(cpt2_probe_id);
    if (0!=err)
    {
        printf("trace_open() failed\n");
        //exit (1);
        return 1;
    }


    /****************************************************************************/
    // Enable Trace
    /****************************************************************************/
    // Configure and enable CPTracer2 probe
    cpt2Options = g_defaultCpt2Options;

    // Set up the CPTracer2 Probe based on the use case selected
    switch (mode)
    {
        case eThroughput:
            cpt2Options.mode = eCPT2_MODE_THROUGHGPUT;
            cpt2Options.sampling_period = 0x1fff;
            cpt2Options.mast_id_3_msbs = cpt2_mst_id >> 5;
            break;
        case eLatency:
            cpt2Options.mode = eCPT2_MODE_LATENCY;
            cpt2Options.sampling_period = 0x1fff;
            cpt2Options.mast_id_3_msbs = cpt2_mst_id >> 5;
            break;
        case eTransaction:
        default:
            //The address range to set up here needs to be the real physical address not the RAT mapped address
            //In this example, access to DDR 0x80000000 actually goes to physical address 0x8,0000,0000.
            cpt2Options.filt_addr_low_lsb = 0x0;
            cpt2Options.filt_addr_low_msb = 0x00000008;
            cpt2Options.filt_addr_high_lsb =0x20;
            cpt2Options.filt_addr_high_msb =0x00000008;
            cpt2Options.filt_addr_low_range_exclude = false;//track the transactions in the address range
            cpt2Options.mode = eCPT2_MODE_TRANSACTION;
            cpt2Options.mast_id_3_msbs = cpt2_mst_id >> 5; 
            break;
    }

    err = trace_enable(&cpt2Options);
    if (0!=err)
    {
        printf("trace_enable() failed\n");
        //exit (1);
        return 1;
    }

    /****************************************************************************/
    // Generate DDR transaction
    /****************************************************************************/

    uint32_t ddr_addr = 0x80000000;//0x41c30000;//0x80000000;
    uint32_t ddr_data = 0x100;
    uint32_t i;
    uint32_t size = 0x10;
    for (i=0; i<size; i++) {
        *(uint32_t *)ddr_addr = ddr_data;
        ddr_addr +=4;
        ddr_data++;
    }

    /****************************************************************************/
    // Disable Trace
    /****************************************************************************/
    err = trace_disable();
    if (0!=err)
    {
        printf("trace_disable() failed\n");
        //exit (1);
        return 1;
    }


    /****************************************************************************/
    // Do the post processing on the recorded CPTracer2 data in ETB.
    // This will generate the following files in your PC host:
    //  -- c:\temp\CPT2.bin
    //  -- c:\temp\CPT2.dcm
    /****************************************************************************/
    if (0 != CPT2TBR_PostProcess(&cpt2Options, pETBHandle, DEFAULT_ATB_ID))
    {
        printf("CPT2TBR_PostProcess() call failed!\n");
        return 1;
    }

    //Close trace components
    err = trace_close();
    if (0!=err)
    {
        printf("trace_close() failed\n");
        //exit (1);
        return 1;
    }


	return 0;

}

// addr -- 32-bit address from CPU perspective
// xlat_addr -- 48-bit address in SoC memory map, this is the translated address after the RAT mapping
uint32_t set_rat_region(
        uint32_t rat_cfg_base,
		uint32_t region_id, 
		uint32_t base_addr, 
		uint64_t xlate_addr,
		uint32_t region_width)
{

        uint32_t xlate_addr_lower = xlate_addr & 0xffffffff;
        uint32_t xlate_addr_upper = xlate_addr >> 32;

		uint32_t base;
		uint32_t mask = ((1 << region_width) - 1);

		base = rat_cfg_base + region_id * IP_RAT_REGION_SIZE;

		if (((base_addr & mask) != 0) || ((xlate_addr_lower & mask) != 0)) {
			return 1;
		}
		*mkptr(base, IP_RAT_R0_CTRL) = 0;
		*mkptr(base, IP_RAT_R0_BASE) = base_addr;
		*mkptr(base, IP_RAT_R0_UPPER) = xlate_addr_upper;
		*mkptr(base, IP_RAT_R0_LOWER) = xlate_addr_lower;
		*mkptr(base, IP_RAT_R0_CTRL) = 0x80000000 | region_width;
		return 0;
}

/****************************************************************************/
// Open the drivers for CPTracer2, Trace Aggregator, ATB replicator and ETB
/****************************************************************************/
uint8_t trace_open(cpt2pb_id_t pb_id) {

    td_error_t err = e_ERR_NONE;
    uint8_t err1;

    uint8_t dbgcell_id;
    uint64_t dbgcell_baddr;
    uint64_t atbrep_baddr;
    uint64_t cpt2_aggr_baddr;
    uint64_t cpt2_probe_baddr;

    err1 = CPT2H_get_device_info(pb_id, &dbgcell_id, &dbgcell_baddr, &atbrep_baddr, &cpt2_aggr_baddr, &cpt2_probe_baddr, &cpt2_mst_id);
    if (0 != err1) {
        return 1;
    }


    // CPTracer2 Aggregator declarations
    target_access_t traceAggrTa = { 0, cpt2_aggr_baddr };
    trace_aggr_init_t traceAggrInit = { &traceAggrTa };

    // CPTracer2 Probe declarations
    target_access_t cpt2Ta = { 0, cpt2_probe_baddr };
    cpt2_init_t cpt2Init = { &cpt2Ta };

    // ATB Replicator declarations
    target_access_t atbRepTa = { 0, atbrep_baddr };
    atbrep_init_t atbRepInit = { &atbRepTa };

    // Global Timer Counter declarations
    uint32_t gtc_baddr = GTC_BADDR;
    target_access_t gtcTa = { 0, gtc_baddr };
    gtc_init_t gtcInit = { &gtcTa };

    //ETB declarations
    uint32_t etbWidth=0;
    eETB_Error  etbRet = eETB_Success;
    ETB_errorCallback pETBErrCallBack = NULL;

    // Open the IP drivers for above debug components
    printf("Opening CPT2 probe...\n");
    err = CPT2_open(&cpt2Init, &cpt2Handle);
    if (e_ERR_NONE != err) {
        printf("Failed to open CPT2 probe.\n");
        return 1;
    }

    printf("Opening trace aggregator...\n");
    err = TRACE_AGGR_open(&traceAggrInit, &traceAggrHandle);
    if (e_ERR_NONE != err) {
       // printf("Failed to open trace aggregator.\n");
        return 1;
    }

    printf("Opening ATB replicator...\n");
    err = ATBREP_open(&atbRepInit, &atbRepHandle);
    if (e_ERR_NONE != err) {
        printf("Failed to open ATB replicator.\n");
        return 1;
    }

    printf("Opening Global Time Counter (GTC)...\n");
    err = GTC_open(&gtcInit, &gtcHandle);
    if (e_ERR_NONE != err) {
        printf("Failed to open GTC.\n");
        return 1;
    }

    printf("Opening ETB...\n");
    if ( etbRet = ETB_open(pETBErrCallBack, eETB_Circular, dbgcell_id, &pETBHandle, &etbWidth) )
    {
        printf("Error %d opening ETB \n", etbRet);
        //exit(1);
        return 1;
    }

    return 0;

}

// close CPTracers/Aggregator/ATB replicator etc.
uint8_t trace_close() {

    td_error_t err = e_ERR_NONE;
    eETB_Error etbRet;

    // Close the trace drivers.
    printf("Closing CPT2 probe...\n");
    err = CPT2_close(cpt2Handle);
    if (e_ERR_NONE != err) {
        printf("Failed to close CPT2 probe.\n");
        return 1;
    }

    printf("Closing trace aggregator...\n");
    err = TRACE_AGGR_close(traceAggrHandle);
    if (e_ERR_NONE != err) {
        printf("Failed to close trace aggregator.\n");
        return 1;
    }

    printf("Closing ATB replicator...\n");
    ATBREP_close(atbRepHandle);

    printf("Closing Global Time Counter (GTC)...\n");
    err = GTC_close(gtcHandle);
    if (e_ERR_NONE != err) {
        printf("Failed to close GTC.\n");
        return 1;
    }

    etbRet  = ETB_close(pETBHandle);
    if(etbRet != eETB_Success)
    {
        printf("Error closing ETB\n");
        //exit(1);
        return 1;
    }


    return 0;

}


/****************************************************************************/
// Configure and Enable CPTracer2, Trace Aggregator, ATB replicator and ETB
// Please follow the order in this function to enable the debug components!!!
/****************************************************************************/
uint8_t trace_enable(cpt2_options_t * p_options)
{

    td_error_t err = e_ERR_NONE;
    eETB_Error etbRet;

    // Configure and enable GTC
    err = GTC_configure(gtcHandle, false);
    if (e_ERR_NONE != err) {
        printf("Failed to configure GTC.\n");
        return 1;
    }

    err = GTC_enable(gtcHandle);
    if (e_ERR_NONE != err) {
        printf("Failed to enable GTC.\n");
        return 1;
    }

    // Configure and enable ETB
    etbRet = ETB_enable(pETBHandle, 0);
    if(etbRet != eETB_Success)
    {
        printf("Error %d enabling ETB\n", etbRet);
        //exit(1);
        return 1;
    }

    // Configure and enable CPTracer2 aggregator
    trace_aggr_options_t traceAggrOptions;
    traceAggrOptions.atb_id = DEFAULT_ATB_ID;
    err = TRACE_AGGR_configure(traceAggrHandle, &traceAggrOptions);
    if (e_ERR_NONE != err) {
        printf("Failed to configure trace aggregator.\n");
        return 1;
    }

    err = TRACE_AGGR_enable(traceAggrHandle);
    if (e_ERR_NONE != err) {
        printf("Failed to enable trace aggregator.\n");
        return 1;
    }

    err = CPT2_enable(cpt2Handle, p_options);
    if (e_ERR_NONE != err) {
        printf("Failed to enable CPTracer2 probe.\n");
        return 1;
    }


    return 0;

}

/****************************************************************************/
// Disable CPTracer2, Trace Aggregator,  ETB
// Please follow the order in this function to disable the debug components!!!
/****************************************************************************/
uint8_t trace_disable() {

    td_error_t err = e_ERR_NONE;
    eETB_Error etbRet;

    // Disable CPTracer2 probe
    err = CPT2_disable(cpt2Handle);
    if (e_ERR_NONE != err) {
        printf("Failed to disable CPTracer2 probe.\n");
        return 1;
    }

    // Disable CPTracer2 aggregator
    err = TRACE_AGGR_disable(traceAggrHandle);
    if (e_ERR_NONE != err) {
        printf("Failed to disable trace aggregator.\n");
        return 1;
    }

    /* Disable ETB to stop capturing STM trace. ETB must be disabled before reading contents */
    etbRet = ETB_disable(pETBHandle);
    if(etbRet != eETB_Success)
    {
        printf("Error %d disabling ETB\n", etbRet);
        //exit(1);
        return 1;
    }

    return 0;

}

/*********************************************************************************/
// Read and Transfer ETB data to a Bin file, also record CPTracer2 configuration
// to a DCM file. We will then use both files for further decode and analysis
/*********************************************************************************/
static uint8_t CPT2TBR_PostProcess(cpt2_options_t * p_options, ETBHandle* pETBHandle, uint32_t atb_id)
{
#if defined(Linux) //Linux host
    char *pBinFileName = "/home/xgemuadm/temp/CPT2.bin";
    char *pDcmFileName = "/home/xgemuadm/temp/CPT2.dcm";
#else //Windows host
    char *pBinFileName = "C:\\temp\\CPT2.bin";
    char *pDcmFileName = "C:\\temp\\CPT2.dcm";
#endif

    eETB_Error  etbRet;
    ETBStatus etbStatus;
    uint32_t* pBuffer;

    /* Check the ETB status */
    etbRet= ETB_status(pETBHandle, &etbStatus);
    if(etbRet != eETB_Success)
    {
        printf("Error %d getting ETB status\n", etbRet);
        //exit(1);
        return 1;
    }

    /* Allocate buffer */
    pBuffer = (uint32_t*) malloc(etbStatus.availableWords * 4); // ETB words are 32 bit long
    if(pBuffer ==0)
    {
        printf("Cannot allocate %d byte buffer to read ETB\n", etbStatus.availableWords * 4);
        return 1;
    }

    /* Now read and transfer the ETB contents to a Bin file for further decode and analysis*/
    if( 0 != transferETBData(pETBHandle, etbStatus, pBuffer, pBinFileName) )
    {
        if(pBuffer)
            free(pBuffer);
        return 1;
    }

    /* Free buffer */
    if(pBuffer)
        free(pBuffer);

    /* Now transfer the CPTracer2 configuration to a DCM file to help decode */
    if (0 != transferCPT2Config(p_options, atb_id, pDcmFileName)) {
        return 1;
    }

    return 0;

}

/****************************************************************************/
/* Read and Transfer ETB data*/
/* This examples uses CCS CIO functionality to create a*/
/* ETB bin file on the PC. Apps would replace this with a specific data*/
/* transport mechanism to move the binary data to PC host for further */
/* decoding and analysis*/
/****************************************************************************/
static uint8_t transferETBData(ETBHandle* pETBHandle, ETBStatus etbStatus, uint32_t* pBuffer, char * pBinFileName)
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
        FILE* fp = fopen(pBinFileName, "wb");
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
                    printf("Error writing data to  - %s \n", pBinFileName);
                    return 1;
                }
                sz++;

            }
            printf("Successfully transported ETB data - %s \n", pBinFileName);

            fclose(fp);
        }
        else
        {
            printf("Error opening file - %s \n", pBinFileName);
            return 1;
        }
    }

    return 0;
}

/*************************************************************************************/
// Status of the CPTracer2 DCM data - this is required to decode the compressed data
/*************************************************************************************/
static uint8_t transferCPT2Config(cpt2_options_t * p_options, uint32_t atb_id, char * pDcmFileName)
{
    char buf[1024];

    FILE* fp = fopen(pDcmFileName, "w");
    if(fp)
    {
        td_error_t err = e_ERR_NONE;

        err = CPT2_get_options_string(p_options, buf, 1024);
        if (err != e_ERR_NONE)
        {
            printf("Error getting CPT2 option string");
            return 1;
        }

        fprintf(fp, "CPT2_%d=%s\n", cpt2_mst_id, buf);
        fprintf(fp, "STM_ATB_Id=%d\n", atb_id);
        fprintf(fp, "STM_Buffer_Wrapped=0\n");
        fprintf(fp, "STM_STP_Version=2\n");
        fprintf(fp, "TWP_Protocol=1\n");
        fclose(fp);
        printf("Successfully transported CPT2 config - %s \n", pDcmFileName);
    }
    else
    {
        printf("Error opening file - %s \n", pDcmFileName);
        return 1;
    }
    return 0;

}
