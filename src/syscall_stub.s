; syscall_stub.S – trampoline for user int 0x80 syscalls

[BITS 32]
section .text
global syscall_stub
extern syscall_handler

syscall_stub:
    cli
    pusha
    mov  eax, esp
    push eax
    call syscall_handler
    add  esp, 4
    popa
    sti
    iretd
