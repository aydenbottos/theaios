#include "memory.h"
#include <stddef.h>

extern uint8_t _end;       // defined by linker right after .bss
static uintptr_t heap_ptr;

void setup_heap(void) {
    heap_ptr = (uintptr_t)&_end;
}

uint32_t get_free_mem(void) {
    uintptr_t start = (uintptr_t)&_end;
    return (uint32_t)(heap_ptr - start);
}

void utohex(uintptr_t val, char *out) {
    static const char hex[] = "0123456789ABCDEF";
    for (int i = (sizeof(val)*2)-1; i >= 0; i--) {
        out[i] = hex[val & 0xF];
        val >>= 4;
    }
    out[sizeof(val)*2] = '\0';
}
