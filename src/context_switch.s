; src/context_switch.s – NASM syntax for 32‑bit elf

section .text
global context_switch

; void context_switch(uint32_t *old_sp_ptr, uint32_t new_sp);
context_switch:
    push ebp
    mov  ebp, esp
    mov  eax, [ebp + 8]    ; eax = old_sp_ptr
    mov  edx, [ebp + 12]   ; edx = new_sp
    mov  [eax], esp        ; *old_sp_ptr = current ESP
    mov  esp, edx          ; switch to new ESP
    pop  ebp
    ret
