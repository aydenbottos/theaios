[BITS 32]
SECTION .multiboot
ALIGN 8
multiboot_header:
    dd 0xe85250d6                ; magic
    dd 0                         ; architecture = i386
    dd header_end - multiboot_header
    dd -(0xe85250d6 + 0 + (header_end - multiboot_header)) ; checksum

    ; minimal required end‑tag
    dw 0 ; type
    dw 0 ; flags
    dd 8 ; size
header_end:

SECTION .text
extern kernel_main
global _start
_start:
    cli
    ; initialize FPU: clear TS (bit3) and EM (bit2), set MP (bit1) and NE (bit5)
    fninit
    mov eax, cr0
    and eax, 0xfffffff3
    or eax, 0x22
    mov cr0, eax
    call kernel_main
.hang:  hlt
    jmp .hang
