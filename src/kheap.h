#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>

/* Initialise the kernel heap to start right after the kernel image */
void kheap_init(void);

/* Allocate `size` bytes from the kernel’s bump heap */
void *kmalloc(uint32_t size);

#endif /* KHEAP_H */
