#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>
#include "interrupts.h"

// Special scan codes
#define SCANCODE_UP    0x48
#define SCANCODE_DOWN  0x50
#define SCANCODE_LEFT  0x4B
#define SCANCODE_RIGHT 0x4D
#define SCANCODE_SPACE 0x39
#define SCANCODE_ESC   0x01

void keyboard_init(void);
void keyboard_handler(regs_t* regs);
bool keyboard_has_input(void);
char keyboard_get_char(void);
bool keyboard_has_scancode(void);
uint8_t keyboard_get_scancode(void);

#endif // KEYBOARD_H
