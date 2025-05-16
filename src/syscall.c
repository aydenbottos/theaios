// src/syscall.c

#include <stdint.h>
#include "util.h"     // for putc(), puts()
#include "serial.h"   // for serial_putc()
#include "syscall.h"  // for SYS_WRITE, SYS_EXIT

/*
 * Kernel entry point for int 0x80 syscalls.
 *  esp points at [eax, ebx, ecx, edx, ...] pushed by syscall_stub.
 *  We dispatch on eax and return a value in eax.
 */
/* The stack layout produced by the syscall stub (pushad) is:
 *   esp[0] : EDI
 *   esp[1] : ESI
 *   esp[2] : EBP
 *   esp[3] : old ESP (ignored)
 *   esp[4] : EBX
 *   esp[5] : EDX
 *   esp[6] : ECX
 *   esp[7] : EAX  (syscall number)
 *
 * We use these constants for clarity.
 */

#define REG_EDI 0
#define REG_ESI 1
#define REG_EBP 2
#define REG_OLDESP 3
#define REG_EBX 4
#define REG_EDX 5
#define REG_ECX 6
#define REG_EAX 7

void syscall_handler(uint32_t *esp) {
    uint32_t num = esp[REG_EAX];

    switch (num) {

      case SYS_WRITE: {
        int fd            = (int)esp[REG_EBX];
        const char *buf   = (const char*)esp[REG_ECX];
        uint32_t len      =  esp[REG_EDX];
        uint32_t written  = 0;
        (void)fd;  // for now we ignore fd and always write to screen+serial

        for (uint32_t i = 0; i < len; i++) {
            char c = buf[i];
            putc(c, 7);
            serial_putc(c);
            written++;
        }
        esp[REG_EAX] = written;
        break;
      }

      case SYS_EXIT: {
        int code = (int)esp[REG_EBX];
        putc('\n', 7);
        puts("[process exited]\n");
        (void)code;
        // kill the task (in a real scheduler you'd remove it). Here we halt.
        while (1) {
            asm volatile("hlt");
        }
        break;
      }

      default:
        // unknown syscall: return -1
        esp[REG_EAX] = (uint32_t)-1;
        break;
    }
}

/*
 * Userspace stub for write(fd, buf, len).
 * Traps into int 0x80 with:
 *   eax = SYS_WRITE, ebx = fd, ecx = buf, edx = len
 * Returns: number of bytes written (in eax).
 */
int write(int fd, const char *buf, uint32_t len) {
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)               /* output: ret ← EAX */
        : "a"(SYS_WRITE),         /* input: EAX = SYS_WRITE */
          "b"(fd),                /* EBX = fd */
          "c"(buf),               /* ECX = buf */
          "d"(len)                /* EDX = len */
        : "memory"
    );
    return ret;
}

/*
 * Userspace stub for exit(code).
 * Traps into int 0x80 with:
 *   eax = SYS_EXIT, ebx = code
 * Does not return.
 */
void exit(int code) {
    asm volatile(
        "int $0x80"
        :
        : "a"(SYS_EXIT),          /* EAX = SYS_EXIT */
          "b"(code)               /* EBX = code */
        : "memory"
    );
    // should never get here
    for (;;) asm volatile("hlt");
}
