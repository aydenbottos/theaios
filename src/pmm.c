#include "pmm.h"
#include <stddef.h>

#define MAX_FRAMES   (16 * 1024 * 1024 / 0x1000)  // first 16 MiB
static uint8_t frame_bitmap[MAX_FRAMES/8];
static uint32_t last_frame = 0;

void pmm_init(void) {
    for (size_t i = 0; i < sizeof(frame_bitmap); i++)
        frame_bitmap[i] = 0;
}

uint32_t pmm_alloc_frame(void) {
    for (uint32_t i = last_frame; i < MAX_FRAMES; i++) {
        uint32_t idx = i / 8, bit = i % 8;
        if (!(frame_bitmap[idx] & (1 << bit))) {
            frame_bitmap[idx] |= (1 << bit);
            last_frame = i + 1;
            return i;
        }
    }
    return (uint32_t)-1;
}

void pmm_free_frame(uint32_t frame) {
    uint32_t idx = frame / 8, bit = frame % 8;
    frame_bitmap[idx] &= ~(1 << bit);
    if (frame < last_frame) last_frame = frame;
}
