#ifndef PIT_H
#define PIT_H

#include <stdint.h>
#include "interrupts.h"    // for regs_t

// Initialise the PIT to fire IRQ0 at 100 Hz
void pit_init(void);

// Handler called by your IRQ stub on vector 0x20
void pit_handler(regs_t *r);

// Return number of ticks since boot
uint32_t pit_get_ticks(void);

#endif /* PIT_H */
