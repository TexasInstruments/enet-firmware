// ********************************************
// Sample ARM application used to showcase GEL 
// automation
// ********************************************

#include <stdio.h>

#define SYS_CONFIG             0x2c
#define GCR_OFFSET             0x78
#define CCR_OFFSET             0x80
#define CLCR_OFFSET            0x84
#define CICR_OFFSET            0x88
#define CSDP_OFFSET            0x90
#define CEN_OFFSET         0x94
#define CFN_OFFSET       0x98
#define CSSA_OFFSET            0x9c
#define CDSA_OFFSET      0x0A0

#define CCR_CHANNEL_ENABLE    (0x1 << 7)
#define CCR_RD_ACTIVE         (0x1 << 9 )
#define CCR_WR_ACTIVE         (0x1 << 10)

#define CCR_SRC_CONST_ADDR    (0x0 << 12)
#define CCR_SRC_POST_INC      (0x1 << 12)

#define CCR_DST_CONST_ADDR    (0x0 << 14)
#define CCR_DST_POST_INC      (0x1 << 14)


#define CSDP_8BIT_DATA        (0x0 << 0)
#define CSDP_16BIT_DATA       (0x1 << 0)
#define CSDP_32BIT_DATA       (0x2 << 0)

#define CSDP_SRC_PACKED       (0x1 << 6)
#define CSDP_SRC_NON_PACKED   (0x0 << 6)

#define CSDP_SRC_BURST_SINGLE (0x0 << 7)
#define CSDP_SRC_BURST_4_64   (0x1 << 7)
#define CSDP_SRC_BURST_8_64   (0x2 << 7)
#define CSDP_SRC_BURST_16_64  (0x3 << 7)

#define CSDP_DST_PACKED       (0x1 << 13)
#define CSDP_DST_NON_PACKED   (0x0 << 13)

#define CSDP_DST_BURST_SINGLE (0x0 << 14)
#define CSDP_DST_BURST_4_64   (0x1 << 14)
#define CSDP_DST_BURST_8_64   (0x2 << 14)
#define CSDP_DST_BURST_16_64  (0x3 << 14)

#define CSDP_WR_NON_POSTED    (0x0 << 16)
#define CSDP_WR_POSTED        (0x1 << 16)
#define CSDP_WR_POSTED_LAST_N (0x2 << 16)

#define CH_OFFSET                0x60

#define CH2                       2

 // testcase defines
//#define NUM_MEM_WORDS_TO_INIT    (200)

#define DMA_BASE_ADDRESS       0x4A056000
#define DMA_MEM_SRC_ADDRESS    0x80300000
#define DMA_MEM_DST_ADDRESS    0x80340000 //0x80302000
#define L4_PER_START           0x48000000

//#ifdef _ETB
#if 1
#define DMA_NUM_ELEMENTS_TO_TRANSFER 8096
#define DMA_NUM_FRAMES_TO_TRANSFER    1   
#else
#define DMA_NUM_ELEMENTS_TO_TRANSFER 65536 
#define DMA_NUM_FRAMES_TO_TRANSFER    8 
#endif

#define NUM_MEM_WORDS_TO_INIT    (DMA_NUM_ELEMENTS_TO_TRANSFER*DMA_NUM_FRAMES_TO_TRANSFER) //(200)

void DMA_Channel_Config( int chNum );
void DMA_Channel_Enable( int chNum );

void generate_dma_accesses()
{
    int i = 1;
    while( i-- != 0 ) 
    {
        DMA_Channel_Config( CH2 );
        DMA_Channel_Enable( CH2 );
//        unsigned long* pL4_per = (unsigned long*)L4_PER_START;
//        *pL4_per = 0x55555555;
    }
}

void DMA_Channel_Config( int chNum )
{
//  unsigned long *src_addr = (unsigned long*)DMA_MEM_SRC_ADDRESS;
//  unsigned long *dst_addr = (unsigned long*)DMA_MEM_DST_ADDRESS;
//  int i;

//  printf(" Channel %d  configuring .. \n", chNum );

#if 0
  for (i=0; i < NUM_MEM_WORDS_TO_INIT; i++)
  {
     src_addr[i] = (0xABCD0122 + i);
     dst_addr[i] = 0x0;
  }
#endif
    // issue a soft reset
  if ( 0 == chNum ) { 
    *(int*)(DMA_BASE_ADDRESS + SYS_CONFIG) = 0x2;

    // GCR register
    // FIFO depth allocated to one channel is 0x20 elements
    *(int*) (DMA_BASE_ADDRESS + GCR_OFFSET) = 0x01010020;
  }
   // src is post-incremented addressing mode
   // dst is post-incremented addressing mode
  *(int*)(DMA_BASE_ADDRESS + (chNum * CH_OFFSET) + CCR_OFFSET) = (CCR_DST_POST_INC | CCR_SRC_POST_INC);

  
    // disable channel linking mode
  *(int*)(DMA_BASE_ADDRESS + (chNum * CH_OFFSET) + CLCR_OFFSET) = 0x0;

   // disables all interrupts
  *(int*)(DMA_BASE_ADDRESS + (chNum * CH_OFFSET) + CICR_OFFSET) = 0x0;

   // 32-bit data transfer
   // src / dst packing enabled
   // 16x64-bit read burst
   // 16x64-bit write burst
  *(int*)(DMA_BASE_ADDRESS + (chNum * CH_OFFSET) + CSDP_OFFSET) = (CSDP_32BIT_DATA | CSDP_SRC_PACKED | CSDP_SRC_BURST_16_64 | CSDP_DST_PACKED  | CSDP_DST_BURST_16_64);

   // Configure the number of elements to transfer
  *(int*)(DMA_BASE_ADDRESS + (chNum * CH_OFFSET) + CEN_OFFSET) = DMA_NUM_ELEMENTS_TO_TRANSFER;

   // Configure the number of frames to transfer
  *(int*)(DMA_BASE_ADDRESS + (chNum * CH_OFFSET) + CFN_OFFSET) = DMA_NUM_FRAMES_TO_TRANSFER;

   // Configure the start address from which the transfer begins (this is a memory to memory transfer (unsynchronized)
  *(int*)(DMA_BASE_ADDRESS + (chNum * CH_OFFSET) + CSSA_OFFSET) = DMA_MEM_SRC_ADDRESS;

   // Configure the start address from which the transfer begins (this is a memory to memory transfer (unsynchronized)
  *(int*)(DMA_BASE_ADDRESS + (chNum * CH_OFFSET) + CDSA_OFFSET) = DMA_MEM_DST_ADDRESS;

//  printf(" Channel %d  configured \n", chNum);

}

void DMA_Channel_Enable( int chNum )
{
  *(int*)(DMA_BASE_ADDRESS + (chNum * CH_OFFSET) + CCR_OFFSET) |= CCR_CHANNEL_ENABLE;

//   printf(" Channel %d  enabled  \n", chNum);
}

int DMA_Complete ( int chNum )
{
    unsigned int ccr = *(int*)(DMA_BASE_ADDRESS + (chNum * CH_OFFSET) + CCR_OFFSET);
    if (( ccr & CCR_RD_ACTIVE) || ( ccr & CCR_WR_ACTIVE))
        return 1;
    else
        return 0; 
   
}

