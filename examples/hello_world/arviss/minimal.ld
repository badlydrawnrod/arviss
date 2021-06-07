OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")

OUTPUT_ARCH(riscv)
ENTRY(_start)

/* Configuration variables to be used by the MEMORY command. */
ROM_START = 0x00000000;
ROM_SIZE = 0x4000;  /* 16KiB  */
RAM_SIZE = 0x4000;  /* 16KiB */

/* The stack grows down from the end of RAM. */
STACK_TOP = ROM_START + ROM_SIZE + RAM_SIZE;

/* Memory space definition. These blocks of memory will be used in the SECTIONS command.
 *
 * The robot is a bare-metal machine, i.e., there is no operating system, so its program is loaded into ROM.
 */
MEMORY
{
    rom (rx) :  ORIGIN = ROM_START, LENGTH = ROM_SIZE
    ram (rwx) : ORIGIN = ROM_SIZE,  LENGTH = RAM_SIZE
}

SECTIONS
{
    PROVIDE (__stack_top = STACK_TOP);

    /* Startup code */
    .init ROM_START :
    {
        KEEP (*(SORT_NONE(.init)))
    } > rom

    /* Code. */
    .text :
    {
        *(.text)
    } > rom

    /* Read-only data. */
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) } > rom

    . = (ROM_START + ROM_SIZE);

    /* Initialised data. */
    .data :
    {
        __DATA_BEGIN__ = .;
        *(.data)
    } > ram

    /* Small initialised data. */
    .sdata :
    {
        __SDATA_BEGIN__ = .;
        *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata .srodata.*)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
    } > ram

    _edata = .; PROVIDE (edata = .);

    /* Start of uninitialised data. */
    __bss_start = .;
    .sbss :
    {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
    } > ram

    .bss (NOLOAD) :
    {
        *(.bss .bss.* .gnu.linkonce.b.*)
    } > ram

    /DISCARD/ :
    {
        *(.eh_frame)
    }

    . = ALIGN(32 / 8);
    __BSS_END__ = .;

    __global_pointer$ = MIN(__SDATA_BEGIN__ + 0x800, MAX(__DATA_BEGIN__ + 0x800, __BSS_END__ - 0x800));

    _end = .; PROVIDE (end = .);
}
