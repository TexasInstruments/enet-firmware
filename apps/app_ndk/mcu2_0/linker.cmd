/* linker options */
--fill_value=0
--stack_size=0x2000
--heap_size=0x1000

MEMORY
{
    /* R5F Core 0 local view  */
    R5F_TCMA (X)          : origin=0x0,        length=0x8000
    R5F_TCMB0 (RWIX)      : origin=0x41010000, length=0x8000

    MSMC3	(RWIX)        : origin=0x70040800, length=0x00000800
    DDR0    (RWIX)        : origin=0x82000000, length=0x01000000

    APP_LOG_MEM            : ORIGIN = 0x8C000000, LENGTH = 0x00040000
    TIOVX_OBJ_DESC_MEM     : ORIGIN = 0x8C040000, LENGTH = 0x00FC0000
    RESERVED_MEM           : ORIGIN = 0x8D000000, LENGTH = 0x01000000
    IPC_VRING_MEM          : ORIGIN = 0x8E000000, LENGTH = 0x01000000
    DDR_SHARED_MEM         : ORIGIN = 0x90000000, LENGTH = 0x30000000
}

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
    }	 >> R5F_TCMB0
    .text_fast {
		*cpsw.aer5f<cpsw_packet.o*>(.text)
		*cpsw.aer5f<cpsw_dma.o*>(.text)
		*nimucpsw.aer5f(.text)
		*udma.aer5f<udma_event.o*>(.text)
		*udma.aer5f<udma_priv.o*>(.text)
		*udma.aer5f<udma_api.o*>(.text)
     }     >> DDR0
    .irqStackSection
	{
	   *(*:ti_sysbios_family_arm_v7r_keystone3_Hwi_Module_State_0_irqStack__A)
	} palign(8) > R5F_TCMB0
	

    .text       : {} palign(8)   > DDR0
    .cinit      : {} palign(8)   > DDR0
    .bss        : {} align(8)    > DDR0
    .const      : {} palign(8)   > DDR0
    .data       : {} palign(128) > DDR0
    .sysmem     : {} align(8)    > DDR0
    .stack      : {} align(4)    > DDR0

    .bss:app_log_mem        (NOLOAD) : {} > APP_LOG_MEM
    .bss:ipc_vring_mem      (NOLOAD) : {} > IPC_VRING_MEM
    .bss:ddr_shared_mem     (NOLOAD) : {} > DDR_SHARED_MEM
}
