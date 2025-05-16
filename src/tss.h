#ifndef TSS_H
#define TSS_H

#include <stdint.h>

/* 104‑byte TSS structure */
struct __attribute__((packed)) tss_entry {
    uint32_t prev, esp0, ss0;
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint16_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
};

/* Export the TSS and its size */
extern struct tss_entry tss;
extern uint32_t        tss_size;

/* Initialise TSS and load TR */
void tss_init(void);

#endif /* TSS_H */
