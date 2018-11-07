/* linker options */
__STACK_SIZE = 0x10000;
__TI_STACK_SIZE = __STACK_SIZE;

MEMORY
{
    BOOTVECTOR             : ORIGIN = 0x70000100, LENGTH = 0x00000f00  /* MSMC RAM INIT CODE (4 KB)				*/
    DDR0            (RWX)  : ORIGIN = 0x80000000, LENGTH = 0x01000000
    APP_LOG_MEM            : ORIGIN = 0x8C000000, LENGTH = 0x00040000
    TIOVX_OBJ_DESC_MEM     : ORIGIN = 0x8C040000, LENGTH = 0x00FC0000
    RESERVED_MEM           : ORIGIN = 0x8D000000, LENGTH = 0x01000000
    IPC_VRING_MEM          : ORIGIN = 0x8E000000, LENGTH = 0x01000000
    DDR_SHARED_MEM         : ORIGIN = 0x90000000, LENGTH = 0x30000000
}

REGION_ALIAS("REGION_TEXT", DDR0);
REGION_ALIAS("REGION_BSS", DDR0);
REGION_ALIAS("REGION_DATA", DDR0);
REGION_ALIAS("REGION_STACK", DDR0);
REGION_ALIAS("REGION_HEAP", DDR0);

SECTIONS {

    .vecs : {
        *(.vecs)
    } > BOOTVECTOR AT> BOOTVECTOR

    .text : {
        CREATE_OBJECT_SYMBOLS
        *(.text)
        *(.text.*)
        . = ALIGN(0x8);
        KEEP (*(.ctors))
        . = ALIGN(0x4);
        KEEP (*(.dtors))
        . = ALIGN(0x8);
        __init_array_start = .;
        KEEP (*(.init_array*))
        __init_array_end = .;
        *(.init)
        *(.fini*)
    } > REGION_TEXT AT> REGION_TEXT

    PROVIDE (__etext = .);
    PROVIDE (_etext = .);
    PROVIDE (etext = .);

    .rodata : {
        *(.rodata)
        *(.rodata*)
    } > REGION_TEXT AT> REGION_TEXT

    .data : ALIGN (8) {
        __data_load__ = LOADADDR (.data);
        __data_start__ = .;
        *(.data)
        *(.data*)
        . = ALIGN (8);
        __data_end__ = .;
    } > REGION_DATA AT> REGION_TEXT

    .bss : {
        __bss_start__ = .;
        *(.shbss)
        *(.bss)
        *(.bss.*)
        . = ALIGN (8);
        __bss_end__ = .;
        . = ALIGN (8);
        *(COMMON)
    } > REGION_BSS AT> REGION_BSS

    .heap : {
        __heap_start__ = .;
        end = __heap_start__;
        _end = end;
        __end = end;
        KEEP(*(.heap))
        __heap_end__ = .;
        __HeapLimit = __heap_end__;
    } > REGION_HEAP AT> REGION_HEAP

    .stack (NOLOAD) : ALIGN(16) {
        _stack = .;
        __stack = .;
        KEEP(*(.stack))
    } > REGION_STACK AT> REGION_STACK

	__TI_STACK_BASE = __stack;

    .bss:app_log_mem        (NOLOAD) : {} > APP_LOG_MEM
    .bss:tiovx_obj_desc_mem (NOLOAD) : {} > TIOVX_OBJ_DESC_MEM
    .bss:ipc_vring_mem      (NOLOAD) : {} > IPC_VRING_MEM
    .bss:ddr_shared_mem     (NOLOAD) : {} > DDR_SHARED_MEM
}
