#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

/* CPU register state pushed by ISR/IRQ stubs */
typedef struct regs {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t int_no;
    uint32_t err;
} regs_t;

#endif /* INTERRUPTS_H */
