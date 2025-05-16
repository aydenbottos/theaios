; CPU exceptions 0‑31 - Corrected Error Code Handling
[BITS 32]

SECTION .text
extern  isr_handler          ; C handler we call from the common stub

; Macro for ISRs that do NOT push an error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push dword 0            ; Push dummy error code
    push dword %1           ; Push interrupt number
    jmp  isr_common
%endmacro

; Macro for ISRs that PUSH an error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    ; No dummy error code needed - CPU pushed one
    push dword %1           ; Push interrupt number
    jmp  isr_common
%endmacro

; Exceptions which DO NOT push an error code
ISR_NOERRCODE  0   ; Divide-by-zero
ISR_NOERRCODE  1   ; Debug
ISR_NOERRCODE  2   ; Non-maskable Interrupt
ISR_NOERRCODE  3   ; Breakpoint
ISR_NOERRCODE  4   ; Overflow
ISR_NOERRCODE  5   ; Bound Range Exceeded
ISR_NOERRCODE  6   ; Invalid Opcode
ISR_NOERRCODE  7   ; Device Not Available
; ISR 8 - Double Fault - Pushes Error Code
ISR_NOERRCODE  9   ; Coprocessor Segment Overrun (Reserved)
; ISR 10 - Invalid TSS - Pushes Error Code
; ISR 11 - Segment Not Present - Pushes Error Code
; ISR 12 - Stack Fault - Pushes Error Code
; ISR 13 - General Protection Fault - Pushes Error Code
; ISR 14 - Page Fault - Pushes Error Code
ISR_NOERRCODE 15   ; Reserved
ISR_NOERRCODE 16   ; x87 Floating-Point Exception
; ISR 17 - Alignment Check - Pushes Error Code
ISR_NOERRCODE 18   ; Machine Check
ISR_NOERRCODE 19   ; SIMD Floating-Point Exception
ISR_NOERRCODE 20   ; Virtualization Exception
; ISR 21 - Control Protection Exception - Pushes Error Code
ISR_NOERRCODE 22   ; Reserved
ISR_NOERRCODE 23   ; Reserved
ISR_NOERRCODE 24   ; Reserved
ISR_NOERRCODE 25   ; Reserved
ISR_NOERRCODE 26   ; Reserved
ISR_NOERRCODE 27   ; Reserved
ISR_NOERRCODE 28   ; Hypervisor Injection Exception
; ISR 29 - VMM Communication Exception - Pushes Error Code
; ISR 30 - Security Exception - Pushes Error Code
ISR_NOERRCODE 31   ; Reserved

; Exceptions which DO push an error code
ISR_ERRCODE  8   ; Double Fault
ISR_ERRCODE 10   ; Invalid TSS
ISR_ERRCODE 11   ; Segment Not Present
ISR_ERRCODE 12   ; Stack Fault
ISR_ERRCODE 13   ; General Protection Fault
ISR_ERRCODE 14   ; Page Fault
ISR_ERRCODE 17   ; Alignment Check
ISR_ERRCODE 21   ; Control Protection Exception
ISR_ERRCODE 29   ; VMM Communication Exception
ISR_ERRCODE 30   ; Security Exception


; ── common tail shared by every stub ───────────────────────────────
isr_common:
    pusha                   ; Save general registers (eax, ecx, edx, ebx, esp, ebp, esi, edi)
    mov eax, esp            ; Get current stack pointer

    push eax                ; Pass &regs_t struct pointer to the handler
    call isr_handler        ; Call the C handler
    add esp, 4              ; Remove the struct pointer from the stack

    popa                    ; Restore general registers
    add esp, 8              ; Discard interrupt number and error code (pushed by CPU or stub)
    sti                     ; Re-enable interrupts
    iretd                   ; Return from interrupt

; ── jump table used by idt_init() ─────────────────────────────────
; Note: This table might not be strictly necessary if idt_init() uses the
; extern isrX symbols directly, but kept for potential use.
SECTION .rodata
global isr_table
isr_table:
    dd isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7
    dd isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15
    dd isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    dd isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
