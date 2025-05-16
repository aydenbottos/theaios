; user_stubs.S – NASM syntax for 32‑bit ELF
; Provides write(fd,buf,len) and exit(code) via int 0x80 for user programs

[BITS 32]
section .text
global write
global exit

; int write(int fd, const char *buf, unsigned int len)
write:
    ; Stack layout on entry: [retaddr][fd][buf][len]
    mov   eax, 1           ; SYS_WRITE
    mov   ebx, [esp+4]     ; fd
    mov   ecx, [esp+8]     ; buf
    mov   edx, [esp+12]    ; len
    int   0x80
    ret

; void exit(int code)
exit:
    mov   eax, 2           ; SYS_EXIT
    mov   ebx, [esp+4]     ; code
    int   0x80

halt_loop:
    jmp   halt_loop
