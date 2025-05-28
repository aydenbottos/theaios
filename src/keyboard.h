#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include "interrupts.h"  // for struct regs

// Called by the IRQ stub on vector 0x21
void keyboard_handler(struct regs *r);

// Initialize keyboard
void keyboard_init(void);

// Check if keyboard input is available
bool keyboard_has_input(void);

// Get next character from keyboard buffer
char keyboard_get_char(void);

#endif /* KEYBOARD_H */
