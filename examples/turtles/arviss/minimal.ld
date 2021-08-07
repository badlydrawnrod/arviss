OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")

OUTPUT_ARCH(riscv)
ENTRY(_start)

ROM_START = 0x0000;
MEMORY_SIZE = 0x2000;
STACK_TOP = (ROM_START + MEMORY_SIZE);

SECTIONS
{
    /* Startup code. */
    .init ROM_START :
    {
        KEEP (*(SORT_NONE(.init)))
    } 

    /* Code. */
    .text :
    {
        *(.text)
    } 

    /* Read-only data. */
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) } 

    /* Initialised data. */
    .data :
    {
        __DATA_BEGIN__ = .;
        *(.data)
    } 

    /* Small initialised data. */
    .sdata :
    {
        __SDATA_BEGIN__ = .;
        *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata .srodata.*)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
    } 

    _edata = .; PROVIDE (edata = .);

    /* Start of uninitialised data. */
    __bss_start = .;
    .sbss :
    {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
    } 

    .bss (NOLOAD) :
    {
        *(.bss .bss.* .gnu.linkonce.b.*)
    } 

    /DISCARD/ :
    {
        *(.eh_frame)
    }

    . = ALIGN(32 / 8);
    __BSS_END__ = .;

    __global_pointer$ = MIN(__SDATA_BEGIN__ + 0x800, MAX(__DATA_BEGIN__ + 0x800, __BSS_END__ - 0x800));

    _end = .; PROVIDE (end = .);

    PROVIDE (__stack_top = STACK_TOP);
}
