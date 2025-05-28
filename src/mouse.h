#ifndef MOUSE_H
#define MOUSE_H

#include <stdbool.h>
#include "util.h"

// Mouse state structure
typedef struct {
    int x, y;           // Current position
    int dx, dy;         // Delta movement
    uint8_t buttons;    // Button states (bit 0 = left, bit 1 = right, bit 2 = middle)
    bool visible;       // Cursor visibility
} mouse_state_t;

// Mouse cursor bitmap (12x12)
#define CURSOR_WIDTH 12
#define CURSOR_HEIGHT 12

// Function declarations
void mouse_init(void);
void mouse_handler(void);
mouse_state_t* mouse_get_state(void);
void mouse_show_cursor(void);
void mouse_hide_cursor(void);
void mouse_update_cursor(void);
bool mouse_left_clicked(void);
bool mouse_right_clicked(void);
bool mouse_in_rect(int x, int y, int width, int height);

/* Polling fallback (optional) */
void mouse_poll(void);

#endif // MOUSE_H
