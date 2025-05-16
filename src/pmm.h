#ifndef PMM_H
#define PMM_H
#include <stdint.h>
/* Initialize frame bitmap, allocate/free 4 KiB frames */
void pmm_init(void);
uint32_t pmm_alloc_frame(void);
void     pmm_free_frame(uint32_t frame);
#endif
