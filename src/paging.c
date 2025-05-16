// src/paging.c
#include <stdint.h>
#include "paging.h"

#define ENTRIES 1024
#define FLAGS   (1 | 2 | 4)  /* P=1,RW=1,US=1 */

static uint32_t pgdir[ENTRIES] __attribute__((aligned(4096)));
static uint32_t pgtab[ENTRIES] __attribute__((aligned(4096)));

void paging_init(void) {
    // Identity-map entire 4GB using 4MB pages
    for (int i = 0; i < ENTRIES; i++) {
        pgdir[i] = (i << 22) | FLAGS | 0x80; /* P=1,RW=1,US=1,PS=1 */
    }
    asm volatile("mov %0, %%cr3" :: "r"(pgdir));
    // Enable Page Size Extensions (PSE) for 4MB pages
    uint32_t cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 1 << 4;
    asm volatile("mov %0, %%cr4" :: "r"(cr4));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}
