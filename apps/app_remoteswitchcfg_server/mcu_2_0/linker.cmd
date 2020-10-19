/* linker options */
--fill_value=0

#define ATCM_START 0x00000000

-e __VECS_ENTRY_POINT
--retain="*(.utilsCopyVecsToAtcm)"

SECTIONS
{
    .vecs       : {
        __VECS_ENTRY_POINT = .;
    } palign(8) > ATCM_START

    .text_boot {
        *boot.aer5f*<*boot.o*>(.text)
        -lsysbios.aer5f*<BIOS.o*>(.text:ti_sysbios_family_arm_v7r_keystone3_Hwi_*)
        -lti.targets.arm.rtsarm.aer5f*<*.o*>(.text:xdc_runtime_*)
        -lsysbios.aer5f*<BIOS.o*>(.text:ti_sysbios_family_arm_v7r_Cache*)
        -lsysbios.aer5f*<BIOS.o*>(.text:ti_sysbios_family_arm_MPU*)
        -lsysbios.aer5f*<BIOS.o*>(.text:ti_sysbios_family_arm_exc*)
        *(*:xdc_runtime_Startup*)
        *(*:ti_sysbios_family_arm_v7r*)
        *(*:ti_sysbios_family_arm_MPU*)
    }    >> R5F_TCMB0

    .utilsCopyVecsToAtcm : {} palign(8) > R5F_TCMB0

    .text_fast {
        *(.text:EnetDma_retrieveRxPktQ*)
        *(.text:EnetDma_retrieveTxPktQ*)
        *(.text:EnetUdma_ringDequeue*)
        *(.text:EnetUdma_ringEnqueue*)
        *(.text:EnetUdma_submitPkts*)
        *(.text:EnetDma_submitRxPktQ*)
        *(.text:EnetDma_submitTxPktQ*)
        *(.text:EnetQueue_append*)
        *(.text:EnetQueue_copyQ*)
        *(.text:EnetQueue_deq*)
        *(.text:EnetQueue_enq*)
        *(.text:EnetQueue_enqHead*)
        *(.text:EnetQueue_getQCount*)
        *(.text:EnetUtils_physToVirt*)
        *(.text:EnetUtils_virtToPhys*)
        *(.text:NIMUPacketService*)
        *(.text:NIMUReceivePacket*)
        *(.text:NIMUSendPacket*)
        *(.text:NIMUCreatePacket*)
        *(.text:Ndk2Enet_sendTxPackets*)
        *(.text:PBM_alloc*)
        *(.text:PBMQ_enq*)
        *(.text:PBMQ_deq*)
        *(.text:Udma_ringQueueRaw*)
        *(.text:Udma_ringDequeueRaw*)
        *(.text:Udma_virtToPhyFxn*)
     }     > DDR_MCU2_0

     .irqStackSection
    {
       *(*:ti_sysbios_family_arm_v7r_keystone3_Hwi_Module_State_0_irqStack__A)
    } palign(8) > R5F_TCMB0

    .text_rest{
       _text_rest_begin = .;
       *(.text)
       _text_rest_end = .;
    } palign(32)    >  DDR_MCU2_0

    .const_sect {
       *(.const)
    } palign(32)    >  DDR_MCU2_0

    .data_sect {
       *(.data)
    } palign(128)   >  DDR_MCU2_0

    .cinit      : {} palign(8)      > DDR_MCU2_0
    .pinit      : {} palign(8)      > R5F_TCMB0

    /* For NDK packet memory*/
    .bss:CPSW_DMA_DESC_MEMPOOL (NOLOAD) {} ALIGN (128) > DDR_MCU2_0
    .bss:CPSW_DMA_RING_MEMPOOL (NOLOAD) {} ALIGN (128) > DDR_MCU2_0
    .bss:CPSW_DMA_PKT_MEMPOOL  (NOLOAD) {} ALIGN (128) > DDR_MCU2_0
    .bss:CPSW_DMA_OBJ_MEM      (NOLOAD) {} ALIGN (128) > DDR_MCU2_0

    .bss:NDK_MMBUFFER  (NOLOAD) {} ALIGN (128) > DDR_MCU2_0
    .bss:NDK_PACKETMEM (NOLOAD) {} ALIGN (128) > DDR_MCU2_0

    /* Used in Switch configuration tool */
    .serialContext     (NOLOAD) {} ALIGN (128) > DDR_MCU2_0

    .bss:app_log_mem        (NOLOAD) : {} > APP_LOG_MEM
    .bss:ipc_vring_mem      (NOLOAD) : {} > IPC_VRING_MEM
    .bss:ddr_shared_mem     (NOLOAD) : {} > DDR_MCU2_0
    .text       : {} palign(8)   > DDR_MCU2_0
    .cinit      : {} palign(8)   > DDR_MCU2_0
    .bss        : {} align(8)    > DDR_MCU2_0
    .far        : {} align(4)    > DDR_MCU2_0
    .const      : {} palign(8)   > DDR_MCU2_0
    .data       : {} palign(128) > DDR_MCU2_0
    .sysmem     : {} align(8)    > DDR_MCU2_0
    .stack      : {} align(4)    > DDR_MCU2_0
    .data_buffer: {} palign(128) > DDR_MCU2_0

    .benchmark_buffer: {} palign(128) > DDR_MCU2_0

    ipc_data_buffer (NOINIT) : {} palign(128) > DDR_MCU2_0
    .resource_table : {
        __RESOURCE_TABLE = .;
    } > DDR_MCU2_0_RESOURCE_TABLE

    .tracebuf   : {} > DDR_MCU2_0

}  /* end of SECTIONS */

