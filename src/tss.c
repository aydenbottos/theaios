// src/tss.c

#include "tss.h"
#include "util.h"

/* Define the TSS and its size */
struct tss_entry tss;
uint32_t        tss_size = sizeof(tss);

/* Initialise the TSS and load TR */
void tss_init(void) {
    /* Zero out the TSS */
    memset(&tss, 0, tss_size);

    /* Capture the current kernel stack pointer as esp0 */
    uint32_t esp0;
    asm volatile("movl %%esp, %0" : "=r"(esp0));

    tss.ss0        = 0x10;   /* kernel data selector */
    tss.esp0       = esp0;
    tss.iomap_base = tss_size - 1;  /* no I/O bitmap, align within limit */

    /* Load TR with selector 5<<3 = 0x28 */
    asm volatile("ltr %%ax" :: "a"(0x28));
}
