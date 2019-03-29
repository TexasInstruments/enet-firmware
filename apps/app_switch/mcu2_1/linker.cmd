/* linker options */
--fill_value=0
--entry_point=ti_sysbios_family_arm_v7r_keystone3_Hwi_vectors /* Default BIOS */

#define ATCM_START 0x00000000

SECTIONS
{
    .text_boot {
        *boot.aer5f<*boot.o*>(.text)
        -lsysbios.aer5f<BIOS.o*>(.text:ti_sysbios_family_arm_v7r_keystone3_Hwi_*)
        -lti.targets.arm.rtsarm.aer5f<*.o*>(.text:xdc_runtime_*)
        -lsysbios.aer5f<BIOS.o*>(.text:ti_sysbios_family_arm_v7r_Cache*)
        -lsysbios.aer5f<BIOS.o*>(.text:ti_sysbios_family_arm_MPU*)
        -lsysbios.aer5f<BIOS.o*>(.text:ti_sysbios_family_arm_exc*)
        *(*:xdc_runtime_Startup*)
        *(*:ti_sysbios_family_arm_v7r*)
        *(*:ti_sysbios_family_arm_MPU*)
    }    >> R5F_TCMB0
    .text_fast {
		*(.text:CpswDma_retrieveRxPackets*)    
		*(.text:CpswDma_retrieveTxDonePackets*)
		*(.text:CpswDma_ringDequeue*)          
		*(.text:CpswDma_ringEnqueue*)          
		*(.text:CpswDma_rxFlowIdxInit*)        
		*(.text:CpswDma_submitPkts*)           
		*(.text:CpswDma_submitRxPackets*)      
		*(.text:CpswDma_submitTxReadyPackets*) 
		*(.text:CpswUtils_appendQ*)
		*(.text:CpswUtils_copyQ*)
		*(.text:CpswUtils_deQ*)
		*(.text:CpswUtils_disableIntr*)
		*(.text:CpswUtils_enQ*)
		*(.text:CpswUtils_enQHead*)
		*(.text:CpswUtils_enableIntr*)
		*(.text:CpswUtils_getQCount*)
		*(.text:CpswUtils_mutexLock*)
		*(.text:CpswUtils_mutexUnlock*)
		*(.text:CpswUtils_phyToVirtFxn*)
		*(.text:CpswUtils_virtToPhyFxn*)
		*(.text:Udma_ringQueueRaw*)
		*(.text:Udma_ringDequeueRaw*)
		*(.text:Udma_virtToPhyFxn*)
     }     > DDR_MCU2_1
    .irqStackSection
    {
       *(*:ti_sysbios_family_arm_v7r_keystone3_Hwi_Module_State_0_irqStack__A)
    } palign(8) > R5F_TCMB0
    
    .text_rest{
       _text_rest_begin = .;
       *(.text)
       _text_rest_end = .;
    } palign(32)    >  DDR_MCU2_1
    .const_sect {
       *(.const)
    } palign(32)    >  DDR_MCU2_1

    .data_sect {
       *(.data)
    } palign(128)   >  DDR_MCU2_1
    
    .cinit      : {} palign(8)      > R5F_TCMB0 
    .pinit      : {} palign(8)      > R5F_TCMB0

    /* For NDK packet memory, we need to map this sections before .bss*/
    /* For NDK packet memory, we need to map this sections before .bss*/
    .far:CPSW_DMA_DESC_MEMPOOL  (NOLOAD) {} ALIGN (128) > DDR_MCU2_1
    .far:CPSW_DMA_RING_MEMPOOL (NOLOAD) {} ALIGN (128) > DDR_MCU2_1
    .far:CPSW_DMA_PKT_MEMPOOL (NOLOAD) {} ALIGN (128) > DDR_MCU2_1
    .bss:NDK_MMBUFFER  (NOLOAD) {} ALIGN (128) > DDR_MCU2_1
    .bss:NDK_PACKETMEM (NOLOAD) {} ALIGN (128) > DDR_MCU2_1

    .bss        : {} align(4)       > DDR_MCU2_1
    .far        : {} align(4)       > DDR_MCU2_1
    .boardcfg_data        : {} palign(128)           > DDR_MCU2_1
    .sysmem     : {}                > DDR_MCU2_1
    .stack      : {} align(8)       > R5F_TCMB0

    /* USB or any other LLD buffer for benchmarking */
    .benchmark_buffer (NOLOAD) {} ALIGN (8) > DDR_MCU2_1

/* Additional sections settings     */

}  /* end of SECTIONS */

/*----------------------------------------------------------------------------*/
/* Misc linker settings                                                       */


/*-------------------------------- END ---------------------------------------*/
