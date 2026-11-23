ENTRY(Reset_Handler)
MEMORY {
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 2048K
    RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 192K
    CCMRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size = 0x400;
_Min_Stack_Size = 0x2000;
PHDRS { flash PT_LOAD FLAGS(5); ram PT_LOAD FLAGS(6); zero PT_LOAD FLAGS(6); }
SECTIONS {
    .isr_vector : { . = ALIGN(4); KEEP(*(.isr_vector)) . = ALIGN(4); } > FLASH :flash
    .text : { . = ALIGN(4); *(.text*) *(.glue_7) *(.glue_7t) *(.eh_frame) KEEP(*(.init)) KEEP(*(.fini)) . = ALIGN(4); _etext = .; } > FLASH :flash
    .rodata : { . = ALIGN(4); *(.rodata*) . = ALIGN(4); } > FLASH :flash
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH :flash
    .ARM.exidx : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } > FLASH :flash
    .preinit_array : { PROVIDE_HIDDEN(__preinit_array_start = .); KEEP(*(.preinit_array*)) PROVIDE_HIDDEN(__preinit_array_end = .); } > FLASH :flash
    .init_array : { PROVIDE_HIDDEN(__init_array_start = .); KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array*)) PROVIDE_HIDDEN(__init_array_end = .); } > FLASH :flash
    .fini_array : { PROVIDE_HIDDEN(__fini_array_start = .); KEEP(*(SORT(.fini_array.*))) KEEP(*(.fini_array*)) PROVIDE_HIDDEN(__fini_array_end = .); } > FLASH :flash
    _sidata = LOADADDR(.data);
    .data : { . = ALIGN(4); _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT> FLASH :ram
    .bss (NOLOAD) : AT(ADDR(.bss)) { . = ALIGN(4); _sbss = .; __bss_start__ = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; __bss_end__ = .; } > RAM :zero
    ._user_heap_stack (NOLOAD) : AT(ADDR(._user_heap_stack)) { . = ALIGN(8); PROVIDE(end = .); PROVIDE(_end = .); __heap_start = .; . += _Min_Heap_Size; __heap_end = .; . += _Min_Stack_Size; . = ALIGN(8); } > RAM :zero
    /DISCARD/ : { *(.note.GNU-stack) }
}
