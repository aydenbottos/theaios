#include <stdint.h>
#include "kheap.h"

/* Provided by the linker script, just after the end of .bss */
extern uint8_t _end;  
static uintptr_t heap_end;

/* Kick off the heap right at &_end */
void kheap_init(void) {
    heap_end = (uintptr_t)&_end;
}

/* Simple bump allocator with 8‑byte alignment */
void *kmalloc(uint32_t size) {
    uintptr_t addr = heap_end;
    /* Round up to the next multiple of 8 */
    uintptr_t next = (addr + size + 7) & ~(uintptr_t)7;
    heap_end = next;
    return (void*)addr;
}
