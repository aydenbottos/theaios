ENTRY(_start)
SECTIONS
{
    . = 1M;                       /* load kernel at 0x0010 0000 */
    .text :  { *(.multiboot) *(.text*) }
    .rodata : { *(.rodata*) }
    .data :   { *(.data*) }
    .bss  :   { *(.bss*)  *(COMMON) }
    _end = .;

    /DISCARD/ : { *(.eh_frame) *(.note.gnu*) }
}
