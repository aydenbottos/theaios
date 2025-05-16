; Hardware IRQs 0‑15 (remapped to INT 32‑47)
[BITS 32]

SECTION .text
extern  irq_handler

%macro IRQ 2
global irq%1
irq%1:
    cli
    push dword 0            ; dummy error‑code
    push dword %2           ; interrupt number (32+irq)
    jmp  irq_common
%endmacro

%assign i 0
%rep 16
IRQ i, 32+i
%assign i i+1
%endrep

irq_common:
    pusha
    mov eax, esp
    push eax
    call irq_handler
    add esp, 4
    popa
    add esp, 8
    sti
    iretd

SECTION .rodata
global irq_table
irq_table:
    dd irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7
    dd irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15
