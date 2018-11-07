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
     }  palign(8)   > MSMC3
    .text:xdc_runtime_Startup_reset__I     : {} palign(8) > MSMC3
    .text:ti_sysbios_family_arm_v7r_Cache* : {} palign(8) > MSMC3
    .text:ti_sysbios_family_arm_MPU*       : {} palign(8) > MSMC3

    .text       : {} palign(8)   > DDR0
    .cinit      : {} palign(8)   > DDR0
    .bss        : {} align(8)    > DDR0
    .const      : {} palign(8)   > DDR0
    .data       : {} palign(128) > DDR0
    .sysmem     : {} align(8)    > DDR0
    .stack      : {} align(4)    > DDR0

    .bss:app_log_mem        (NOLOAD) : {} > APP_LOG_MEM
    .bss:tiovx_obj_desc_mem (NOLOAD) : {} > TIOVX_OBJ_DESC_MEM
    .bss:ipc_vring_mem      (NOLOAD) : {} > IPC_VRING_MEM
    .bss:ddr_shared_mem     (NOLOAD) : {} > DDR_SHARED_MEM
}
