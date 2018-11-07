/****************************************************************************
CToolsLib - ETB Library 

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

/*! \file CSETB.c
    \version 1.3
*/

#include "ETBInterface.h"
#include "ETBAddr.h"

#if defined(TCI6486) || defined(TCI6488)
#include "c6x.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Private structs
//
////////////////////////////////////////////////////////////////////////////////////////////////

struct _ETBHandle_t
{
    uint32_t ulContext;    /*!< ETB context handle*/
    uint8_t id;            /*!< ETB core user ID*/
    uint8_t dnum;          /*!< Detected CPU ID*/
    ETB_errorCallback pCallBack; /*!< ETB error callback*/
    DMAConfig *pDmaConfig; /*!< ETB DMA pointer, NULL when dma not used */
    DMAStatus dmaStatus;   /*!< Copy of ETB DMA status, populated when
                                 ETB_flush_dma is called, used during ETB_read
                                 calls.
                            */
};

/* ETB module access handle - virtual for this library */
static ETBHandle stHandle[NUM_ETB_INSTANCES];

/*! RETURN_ETB_CALLBACK
    A macro to return API error.
*/
#define RETURN_ETB_CALLBACK(id,retValue)        if ( 0 != stHandle[id].pCallBack ) stHandle[id].pCallBack(retValue); return retValue

// ETBLib symbols for CCS ETB Receiver (Trace capture and decoding)
// ETBLib symbols need to be placed in external memory (MSMC or DDR3)
// for STM driver to read from these symbols

#pragma DATA_SECTION(etbLib_buffer_start_addr,"ETBLib_ExtMem");
#pragma DATA_SECTION(etbLib_buffer_size,"ETBLib_ExtMem");
#pragma DATA_SECTION(etbLib_buffer_data_start,"ETBLib_ExtMem");
volatile uint32_t etbLib_buffer_start_addr[NUM_ETB_INSTANCES]; //start of ETB buffer
volatile uint32_t etbLib_buffer_size[NUM_ETB_INSTANCES]; //number of Bytes
volatile uint32_t etbLib_buffer_data_start[NUM_ETB_INSTANCES]; //address after circular buffer wrap point, where oldest data starts

#if SYSETB_PRESENT

#pragma DATA_SECTION(etbLib_sys_etb_index,"ETBLib_ExtMem");
volatile uint32_t etbLib_sys_etb_index = SYS_ETB_ID; //symbol for SYS_ETB index

#endif

/**
* Handle_index(uint32_t id) - Get handle id for the core/sys ETB
*/
uint32_t Handle_index(uint32_t id) 
{
#if SYSETB_PRESENT
    if(id == SYS_ETB || id == SYS_ETB_ID)
    {
        id = NUM_ETB_INSTANCES -1;
    }
#endif
    return id;
}

/**
* Normalize_ID(uint32_t id) - Normalize core/sys ETB ID
*/
uint32_t Normalize_ID(uint32_t id) 
{
#if SYSETB_PRESENT
    if(id == SYS_ETB || id == SYS_ETB_ID)
    {
        id = SYS_ETB_ID;
    }
#endif
    return id;
}

/**
* ETB_open - open ETB programming module interface
*/
eETB_Error  ETB_open(ETB_errorCallback pErrCallBack, eETB_Mode mode, uint8_t coreID, ETBHandle** ppHandle, uint32_t* pETBSizeInWords)
{
    uint32_t dnum = 0;

#if defined(TCI6486) || defined(TCI6488)
    dnum = DNUM;
#endif

    coreID = Normalize_ID(coreID);
    
#if SYSETB_PRESENT
    // populate SYS_ETB index symbol
    if(coreID == SYS_ETB_ID)
    {
        etbLib_sys_etb_index = SYS_ETB_ID;
    }

#endif

    if(Handle_index(coreID) >= NUM_ETB_INSTANCES)
    {
        return eETB_Error_Bad_Param;
    }

    if((mode != eETB_Circular) || (ppHandle == 0))
    {
        return eETB_Error_Bad_Param;
    }
    
    /* Unlock ETB in order to enable accesses to any ETB registers below. */
    *((volatile uint32_t*)ETB_LOCK(coreID)) = ETB_UNLOCK_VAL;

	/* Size of the ETB. ETB_RDP contains number of 32 bit wide words. */
    *pETBSizeInWords = (*((volatile uint32_t*)ETB_RDP(coreID))); 
    
    /* Set ETB context in the handle*/
    stHandle[Handle_index(coreID)].ulContext = ETB_UNLOCK_VAL;
    stHandle[Handle_index(coreID)].id = coreID;
    stHandle[Handle_index(coreID)].dnum = dnum;
    stHandle[Handle_index(coreID)].pCallBack = pErrCallBack;
    
    *ppHandle = &stHandle[Handle_index(coreID)];
   
    return eETB_Success;
}

/**
* ETB_enable- Enable ETB to capture trace data
*/
eETB_Error  ETB_enable(ETBHandle* pHandle, uint32_t triggerCount)
{
    uint32_t etbControl;
    uint32_t waitCount = 100;
    uint8_t n=0;
    
  
    if(pHandle->ulContext != ETB_UNLOCK_VAL)
    {
        RETURN_ETB_CALLBACK(pHandle->id, eETB_Error_Program);
    }

    n =  pHandle->id;

    /* ETB FIFO reset by writing 0 to ETB RAM Write Pointer Register. */
    *((volatile uint32_t*)ETB_RWP(n)) = 0;
    
#ifdef ENABLE_ETB_FORMATTER
    *((volatile uint32_t*)ETB_FFCR(n)) = 1; /* EnFCont=0, EnFTC=1 */
#else
    /* Disable formatting and put ETB formatter into bypass mode. */
    *((volatile uint32_t*)ETB_FFCR(n)) = 0; /* EnFCont=0, EnFTC=0 */
#endif
    /* Setup Trigger counter. */
    *((volatile uint32_t*)ETB_TRIG(n)) = triggerCount; 

    /* Enable ETB data capture by writing ETB Control Register. */
    *((volatile uint32_t*)ETB_CTL(n)) = 1; /* TraceCaptEn =1 */

	/* Put some delays in here - make sure we can read back. */
	do
	{
		etbControl = *((volatile uint32_t*)ETB_CTL(n));
        waitCount--;
	} while ((etbControl != ETB_ENABLE) && (waitCount > 0));

    if(waitCount ==0)
    {
        RETURN_ETB_CALLBACK(pHandle->id, eETB_Error_Program);
    }
    return eETB_Success;
}

/**
* ETB_disable - Disable ETB to stop capturing trace data
*/
eETB_Error  ETB_disable(ETBHandle* pHandle)
{
    uint32_t etbControl;
    uint32_t waitCount = 100;
    uint8_t n=0;

    if(pHandle->ulContext != ETB_UNLOCK_VAL)
    {
        RETURN_ETB_CALLBACK(pHandle->id, eETB_Error_Program);
    }

    n =  pHandle->id;
    
    /* Flush */
    etbControl = *((volatile uint32_t*)ETB_FFCR(n));
    
#ifdef ENABLE_ETB_FORMATTER
    etbControl |= (1 << 12); /* Stop on flush bit */
    *((volatile uint32_t*)ETB_FFCR(n)) = etbControl;
#endif
    etbControl |= (1 << 6); /* Manual flush */
    *((volatile uint32_t*)ETB_FFCR(n)) = etbControl;
    
#ifdef ENABLE_ETB_FORMATTER
    /* Wait for formatter to stop */
    do
    {
        etbControl = *((volatile uint32_t*)ETB_FFSR(n));
        /* counter-- */
    } while((etbControl & 0x2) == 0);
#endif

    /* Disable ETB data capture by writing ETB Control Register. */
    *((volatile uint32_t*)ETB_CTL(n)) = 0; /* TraceCaptEn =0 */

	/* Put some delays in here - make sure we can read back. */
	do
	{
		etbControl = *((volatile uint32_t*)ETB_CTL(n));
		waitCount--;
	} while (etbControl != 0 && (waitCount > 0));


    if(waitCount ==0)
    {
    	RETURN_ETB_CALLBACK(pHandle->id, eETB_Error_Program);
    }
    
    return eETB_Success;
}


/**
* ETB_status- Get ETB status
*   
    ETB status register . 
    STS:
    RAMFull=1   [0], RAMEmpty=0     [0]
    Triggered=1 [1], NotTriggered=0 [1]
    AccqComp=1  [2], NotAccqComp=0  [2]
    FtEmpty=1   [3], NotFtEmpty=0   [3]
    x --> reserved
    v --> read value
    31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    ________________________________________________________________________________________________
    |x | x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| v| v| v| v|
    |__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|
    
*/
eETB_Error  ETB_status(ETBHandle* pHandle, ETBStatus* status)
{
    uint32_t etbStatus, etbControl=0;
    uint8_t n=0;

    if(pHandle->ulContext != ETB_UNLOCK_VAL)
    {
        RETURN_ETB_CALLBACK(pHandle->id, eETB_Error_Program);
    }
    
    if(status == 0 )
    {
        RETURN_ETB_CALLBACK(pHandle->id, eETB_Error_Bad_Param);
    }
    
    n =  pHandle->id;
    
    status->canRead=0;
    status->availableWords=0;
    status->isWrapped=0;
    
    etbControl = *((volatile uint32_t*)ETB_CTL(n));
	
    if(etbControl == ETB_ENABLE)
    {
        return eETB_Success;
    }
    
    etbStatus = *((volatile uint32_t*)ETB_STS(n));
    
    if((etbStatus & ETB_STS_ACQCOMP) == ETB_STS_ACQCOMP)
    {
        status->canRead = 1;
    }

    if ((etbStatus  & ETB_STS_FULL) == ETB_STS_FULL)
    {
        status->isWrapped = 1;
        status->availableWords = *((volatile uint32_t*)ETB_RDP(n));
    }
    else
    	status->availableWords = *((volatile uint32_t*)ETB_RWP(n));

    

   return eETB_Success;
}


/**
* ETB_read- Read ETB data
*/
eETB_Error  ETB_read(ETBHandle* pHandle, uint32_t *pBuffer, uint32_t bufferLength, uint32_t startWord, uint32_t readLength, uint32_t* pRetLength)
{
    ETBStatus status;
    uint32_t i, startAddr, depth;
    uint8_t n=0;
    uint8_t *pTemp=0; 
    
    if(pHandle->ulContext != ETB_UNLOCK_VAL)
    {
        RETURN_ETB_CALLBACK(pHandle->id, eETB_Error_Program);
    }

    n =  pHandle->id;

    /* Check if we have proper buffer pointer */
    if(pBuffer == 0)
    {
        RETURN_ETB_CALLBACK(pHandle->id, eETB_Error_Bad_Param);
    }

    *pRetLength =0;
    
    /* Get the status of the ETB before we can start reading. */
	if(eETB_Success == ETB_status(pHandle, &status))
    {
        if(status.canRead == 0)
        {
            RETURN_ETB_CALLBACK(pHandle->id, eETB_Error_Cannot_Read);
        }
        
        // Update the 3 symbols which are required for CCS ETB receiver
		etbLib_buffer_start_addr[pHandle->id] = (uint32_t)pBuffer; //CCS ETB receiver will always get a linearized buffer for the non-EDMA ETB drain case
		etbLib_buffer_size[pHandle->id] = status.availableWords * 4; //Number of bytes available
		etbLib_buffer_data_start[pHandle->id] = etbLib_buffer_start_addr[pHandle->id]; //circular buffer wrap point, if equal to the start address, no unwrapping of the buffer is required


         /* ETB depth */
        depth = *((volatile uint32_t*)ETB_RDP(n));
        
        /* Check if the buffer is wrapped or not; set read pointers accordingly. */
	    if(status.isWrapped == 1)
        {
            startAddr = *((volatile uint32_t*)ETB_RWP(n)) + startWord;
        }
        else
        {
            startAddr = 0x0 + startWord;
        }
       	
       	/* Adjust the read size for the available user buffer and requested data. */
	    if(bufferLength < status.availableWords)
            status.availableWords = bufferLength;
    
        if(readLength < status.availableWords)
            status.availableWords = readLength;

        /* Adjust to accomodate the start word. */
        if(startWord < status.availableWords)
            status.availableWords = status.availableWords - startWord;
        else
            status.availableWords =0;

        /* Initialize the ETB RAM read pointer register with startAddr */
        *((volatile uint32_t*)ETB_RRP(n)) = startAddr;

        /* Now read trace data out of ETB */
        for (i=0; i<status.availableWords; i++)
        {
            /* Read the ETB RAM read data register to retrieve trace data. */
            /* This would cause the read pointer register value to auto-increment. */
            pBuffer[i] = *((volatile uint32_t*)ETB_RRD(n));
    	    
            (*pRetLength)++;

            startAddr++;
        	
            if(startAddr == depth)
	        {
	            /*Now wrap from begining */
	            startAddr = 0x0;
	            *((volatile uint32_t*)ETB_RRP(n)) = startAddr;
	        }
        }
        /* In Coresight ETB, it addes 0x1 0x0 0x0 (up to 15 bytes of 0) as postamble. This will cost problem in the case of
		 STM. So what we are going to do here is to travel the memory buffer backward and as soon as we find the first value
		0x1, we are going to replace it with 0. Several assumption made here. First is all the access are in byte size. 
		Second, although there is a specific number of 0s occurs before the 0x1 occurs, the number is not very consistant.
		So, we are just going to replace the first 0x1 we will see from backward. We may need to look back to it later on*/
        pTemp = (uint8_t*) &(pBuffer[status.availableWords-1]);
        
        for (i= 0; i<16; i++)
		{
            if (*pTemp != 0x0)
			{
				if (*pTemp == 0x1)
                {
                    (*pTemp) = 0x0;
                    break;
                }
            }
            pTemp--;
        }
    }

    return eETB_Success;
}


/**
* ETB_close- Close ETB programming module interface
*/
eETB_Error  ETB_close(ETBHandle* pHandle)
{
    if(pHandle->ulContext == ETB_UNLOCK_VAL)
    {
        pHandle->ulContext = 0;
    }
    
    return eETB_Success;
}


