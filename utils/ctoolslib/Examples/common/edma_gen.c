// ********************************************
// Sample ARM application used to showcase GEL 
// automation
// ********************************************

#include <stdio.h>
#include <stdint.h>

#ifdef _AM437x
#define EDMA_BASE_ADDRESS       0x49000000
#define DMA_MEM_SRC_ADDRESS     0x80300000 //OCM 0x40310000 //OCM
#define DMA_MEM_DST_ADDRESS     0x80400000 //0x80340000 //DDR

//prcm
#define CM_PER_TPCC_CLKCTRL     0x44DF8878
#define CM_PER_TPTC0_CLKCTRL	0x44DF8880
#define CM_PER_TPTC1_CLKCTRL	0x44DF8888
#define CM_PER_TPTC2_CLKCTRL	0x44DF8890

#define CLKEN					2

inline void edma_prcm_setup()
{
	*(volatile uint32_t *)(CM_PER_TPCC_CLKCTRL) = CLKEN;
	*(volatile uint32_t *)(CM_PER_TPTC0_CLKCTRL) = CLKEN;
	*(volatile uint32_t *)(CM_PER_TPTC1_CLKCTRL) = CLKEN;
	*(volatile uint32_t *)(CM_PER_TPTC2_CLKCTRL) = CLKEN;
}

#ifdef _ETB
#define DMA_NUM_ELEMENTS_TO_TRANSFER 8192
#define DMA_NUM_FRAMES_TO_TRANSFER   16
#define DMA_NUM_BYTES_PER_ELEMENT 4
#else
#define DMA_NUM_ELEMENTS_TO_TRANSFER 65536
#define DMA_NUM_FRAMES_TO_TRANSFER    8
#define DMA_NUM_BYTES_PER_ELEMENT 4
#endif
#endif

#if defined(_66AK2Exx) || defined(_66AK2Gxx)
#define EDMA_BASE_ADDRESS       0x02700000
#define DMA_MEM_SRC_ADDRESS     0x10800000 //First 32K of L2_0
#define DMA_MEM_DST_ADDRESS     0x10808000 // Second 32k of L2_0

inline void edma_prcm_setup()
{
	return;
}

#define DMA_NUM_ELEMENTS_TO_TRANSFER 8192
#define DMA_NUM_FRAMES_TO_TRANSFER    1
#define DMA_NUM_BYTES_PER_ELEMENT 4

#endif

#ifdef _TDA3x
#define EDMA_BASE_ADDRESS       0x40d10000
#define DMA_MEM_SRC_ADDRESS     0x80080000  //DDR Source  // 0x40300000 //First 32K of OCM
#define DMA_MEM_DST_ADDRESS     0x80088000  //DDR Dest    // 0x40308000 //Second 32k of OCM

inline void edma_prcm_setup()
{
    return;
}

#define DMA_NUM_ELEMENTS_TO_TRANSFER 8192
#define DMA_NUM_FRAMES_TO_TRANSFER    1
#define DMA_NUM_BYTES_PER_ELEMENT 4

#endif

#define EDMA_DCHMAP0			(EDMA_BASE_ADDRESS + 0x100)
#define EDMA_DMAQNUM0			(EDMA_BASE_ADDRESS + 0x240)
#define EDMA_QUEPRI				(EDMA_BASE_ADDRESS + 0x284)
#define EDMA_DRAE0				(EDMA_BASE_ADDRESS + 0x340)

#define EDMA_PRAM_ADDRESS(n)    (EDMA_BASE_ADDRESS + 0x4000 + (n * 0x20))
#define EDMA_ESR				(EDMA_BASE_ADDRESS + 0x1010)                   // Event set register
#define EDMA_IESR				(EDMA_BASE_ADDRESS + 0x1060)
#define EDMA_IPR                (EDMA_BASE_ADDRESS + 0x1068)                   // Interrupt pending register
#define EDMA_ICR				(EDMA_BASE_ADDRESS + 0x1070)				   // Interrupt clear register


struct edma_param_t {
	uint32_t opt;
	uint32_t src;
	uint16_t acnt;
	uint16_t bcnt;
	uint32_t dst;
	uint16_t srcbidx;
	uint16_t dstbidx;
	uint16_t link;
	uint16_t bcntrld;
	uint16_t srccidx;
	uint16_t dstcidx;
	uint16_t ccnt;
	uint16_t rsvd;
};

//opt bits
#define EDMA_OPT_SAM_INC 	(0 << 0)
#define EDMA_OPT_DAM_INC 	(0 << 1)
#define EDMA_OPT_SYNCDIM_A 	(0 << 2)
#define EDMA_OPT_SYNCDIM_AB (1 << 2)
#define EDMA_OPT_STATIC     (1 << 3)
#define EDMA_OPT_TCC_0 		(0 << 12)
#define EDMA_OPT_FWID_32    (2 << 8)
#define EDMA_OPT_TCINTEN    (1 << 20)

#define NULL_LINK 0xFFFF


#define NUM_MEM_64_WORDS_TO_INIT    ((DMA_NUM_ELEMENTS_TO_TRANSFER*DMA_NUM_FRAMES_TO_TRANSFER)/2)

//Note: These functions only support EDMA channels 0 - 31
void DMA_Channel_Config( int chNum )
{
    unsigned long long *src_addr = (unsigned long long*)DMA_MEM_SRC_ADDRESS;
    unsigned long long *dst_addr = (unsigned long long*)DMA_MEM_DST_ADDRESS;
    int i;

//  printf(" Channel %d  configuring .. \n", chNum );
    edma_prcm_setup();

    for (i=0; i < NUM_MEM_64_WORDS_TO_INIT; i++)
    {
       src_addr[i] = (0xABCD0000 + i);
       dst_addr[i] = 0x0;
    }
  
    //set up the EDMA Parameter RAM
    struct edma_param_t * pram = (struct edma_param_t *)EDMA_PRAM_ADDRESS(chNum);

    *(volatile uint32_t *)(EDMA_QUEPRI) = 0;
    *(volatile uint32_t *)(EDMA_DRAE0) = 1;
    *(volatile uint32_t *)(EDMA_DMAQNUM0) = 0;
    *(volatile uint32_t *)(EDMA_DCHMAP0) = 0;

    if (DMA_NUM_FRAMES_TO_TRANSFER == 1) {
    	pram->opt = EDMA_OPT_SYNCDIM_A | EDMA_OPT_TCINTEN | EDMA_OPT_FWID_32;  //A-Sync Transfer
    } else {
    	pram->opt = EDMA_OPT_SYNCDIM_AB | EDMA_OPT_TCINTEN | EDMA_OPT_FWID_32; // AB-Sync Transfer
    }
    pram->src = DMA_MEM_SRC_ADDRESS;
    pram->dst = DMA_MEM_DST_ADDRESS;
    pram->acnt = DMA_NUM_ELEMENTS_TO_TRANSFER * DMA_NUM_BYTES_PER_ELEMENT;
    pram->bcnt = DMA_NUM_FRAMES_TO_TRANSFER;
    pram->ccnt = 1;
    pram->link = NULL_LINK;
    pram->srcbidx = 0;
    pram->dstbidx = 0;
    pram->bcntrld = 0;
    pram->srccidx = 0;
    pram->dstcidx = 0;

	//Clear the interrupt pending register just in case
    *(volatile uint32_t *)(EDMA_ICR) = 1 << chNum;
    //Set the Interrupt enable register
    *(volatile uint32_t *)(EDMA_IESR) = 1 << chNum;

//   printf(" Channel %d  configured \n", chNum);

}

void DMA_Channel_Enable( int chNum )
{

    *(volatile uint32_t *)(EDMA_ESR) = 1 << chNum;

//   printf(" Channel %d  enabled  \n", chNum);
}

int DMA_Channel_Complete ( int chNum )
{

    uint32_t ipr = *(volatile uint32_t *)(EDMA_IPR);
    if (ipr & (1 << chNum))
        return 0;
    else
        return 1;

}

