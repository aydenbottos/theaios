#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "interrupts.h"  // for struct regs

// Called by the IRQ stub on vector 0x21
void keyboard_handler(struct regs *r);

// No special init here—handled in irq_install()
#endif /* KEYBOARD_H */
