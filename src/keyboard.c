#include <stdint.h>
#include <stdbool.h>
#include "util.h"
#include "interrupts.h"
#include "shell.h"
#include "gui.h"

// Circular buffer for keyboard input
#define KEYBOARD_BUFFER_SIZE 256
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static uint32_t buffer_read_index = 0;
static uint32_t buffer_write_index = 0;

/* PS/2 Set‑1 scancodes → ASCII (normal & shifted) */
static const char normal_map[128] = {
    [0x02] = '1',[0x03] = '2',[0x04] = '3',[0x05] = '4',
    [0x06] = '5',[0x07] = '6',[0x08] = '7',[0x09] = '8',
    [0x0A] = '9',[0x0B] = '0',[0x0C] = '-',[0x0D] = '=',
    [0x10] = 'q',[0x11] = 'w',[0x12] = 'e',[0x13] = 'r',
    [0x14] = 't',[0x15] = 'y',[0x16] = 'u',[0x17] = 'i',
    [0x18] = 'o',[0x19] = 'p',[0x1A] = '[',[0x1B] = ']',
    [0x1E] = 'a',[0x1F] = 's',[0x20] = 'd',[0x21] = 'f',
    [0x22] = 'g',[0x23] = 'h',[0x24] = 'j',[0x25] = 'k',
    [0x26] = 'l',[0x27] = ';',[0x28] = '\'',[0x29] = '`',
    [0x2C] = 'z',[0x2D] = 'x',[0x2E] = 'c',[0x2F] = 'v',
    [0x30] = 'b',[0x31] = 'n',[0x32] = 'm',[0x33] = ',',
    [0x34] = '.', [0x35] = '/', [0x39] = ' ', [0x0F] = '\t', [0x0E] = '\b', [0x1C] = '\n'

};

static const char shift_map[128] = {
    [0x02] = '!',[0x03] = '@',[0x04] = '#',[0x05] = '$',
    [0x06] = '%',[0x07] = '^',[0x08] = '&',[0x09] = '*',
    [0x0A] = '(',[0x0B] = ')',[0x0C] = '_',[0x0D] = '+',
    [0x10] = 'Q',[0x11] = 'W',[0x12] = 'E',[0x13] = 'R',
    [0x14] = 'T',[0x15] = 'Y',[0x16] = 'U',[0x17] = 'I',
    [0x18] = 'O',[0x19] = 'P',[0x1A] = '{',[0x1B] = '}',
    [0x1E] = 'A',[0x1F] = 'S',[0x20] = 'D',[0x21] = 'F',
    [0x22] = 'G',[0x23] = 'H',[0x24] = 'J',[0x25] = 'K',
    [0x26] = 'L',[0x27] = ':',[0x28] = '"',[0x29] = '~',
    [0x2C] = 'Z',[0x2D] = 'X',[0x2E] = 'C',[0x2F] = 'V',
    [0x30] = 'B',[0x31] = 'N',[0x32] = 'M',[0x33] = '<',
    [0x34] = '>', [0x35] = '?' , [0x39] = ' ', [0x0F] = '\t', [0x0E] = '\b', [0x1C] = '\n'

};

static int shift_down = 0;

// Add character to keyboard buffer
static void keyboard_buffer_add(char c) {
    uint32_t next_write_index = (buffer_write_index + 1) % KEYBOARD_BUFFER_SIZE;
    
    // Don't overflow the buffer
    if (next_write_index != buffer_read_index) {
        keyboard_buffer[buffer_write_index] = c;
        buffer_write_index = next_write_index;
    }
}

void keyboard_handler(regs_t* regs) {
    uint8_t sc = inb(0x60);

    // Key release?
    if (sc & 0x80) {
        uint8_t code = sc & 0x7F;
        if (code == 0x2A || code == 0x36) shift_down = 0;
        return;
    }
    // Key press of Shift?
    if (sc == 0x2A || sc == 0x36) {
        shift_down = 1;
        return;
    }

    // Lookup in appropriate map
    char c = shift_down ? shift_map[sc] : normal_map[sc];
    if (!c) return;
    
    // If GUI is active, add to buffer; otherwise feed to shell
    if (gui_is_active()) {
        keyboard_buffer_add(c);
    } else {
        shell_feed(c);
    }
}

void keyboard_init(void) {
    // Wait for 8042 input buffer empty
    while (inb(0x64) & 0x02) { /* spin */ }
    // Enable PS/2 port #1 (keyboard)
    outb(0x64, 0xAE);

    // Flush stale data
    (void)inb(0x60);
    
    // Initialize buffer indices
    buffer_read_index = 0;
    buffer_write_index = 0;
}

bool keyboard_has_input(void) {
    return buffer_read_index != buffer_write_index;
}

char keyboard_get_char(void) {
    if (!keyboard_has_input()) {
        return 0;  // No input available
    }
    
    char c = keyboard_buffer[buffer_read_index];
    buffer_read_index = (buffer_read_index + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}
