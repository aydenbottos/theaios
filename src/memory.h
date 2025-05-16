#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// Allocate `size` bytes from the kernel’s bump heap
void* kmalloc(uint32_t size);

void setup_heap(void);
uint32_t get_free_mem(void);   // NEW: bytes free in bump heap

// Convert integer to hex string, null‑terminated
void utohex(uintptr_t val, char *out);

#endif // MEMORY_H
