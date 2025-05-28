#include "vga_graphics.h"
#include "util.h"

// Helper for integer absolute value (replacement for abs)
static int abs_int(int x) {
    return x < 0 ? -x : x;
}

// VGA memory address for mode 13h
static uint8_t* vga_buffer = (uint8_t*)0xA0000;

// Double buffer for smoother graphics
static uint8_t back_buffer[VGA_SIZE];

/*
 * Write a complete VGA register dump.  The layout is borrowed from the
 * "FreeVGA" tables: one byte for the Misc Output register, followed by
 * 5 Sequencer, 25 CRTC, 9 Graphics-controller and 21 Attribute-
 * controller registers (61 bytes total).
 */
static void vga_write_regs(const uint8_t *regs) {
    /* 1) Misc output register */
    outb(0x3C2, *regs++);

    /* 2) Sequencer */
    for (uint8_t i = 0; i < 5; i++) {
        outb(0x3C4, i);
        outb(0x3C5, *regs++);
    }

    /* 3) Unlock CRTC registers by clearing bit 7 of index 0x11 */
    outb(0x3D4, 0x11);
    uint8_t prev = inb(0x3D5);
    outb(0x3D5, prev & 0x7F);

    /* 4) CRTC */
    for (uint8_t i = 0; i < 25; i++) {
        outb(0x3D4, i);
        outb(0x3D5, *regs++);
    }

    /* 5) Graphics controller */
    for (uint8_t i = 0; i < 9; i++) {
        outb(0x3CE, i);
        outb(0x3CF, *regs++);
    }

    /* 6) Attribute controller */
    for (uint8_t i = 0; i < 21; i++) {
        (void)inb(0x3DA);            /* reset flip-flop */
        outb(0x3C0, i);
        outb(0x3C0, *regs++);
    }

    /* 7) Enable video output */
    (void)inb(0x3DA);
    outb(0x3C0, 0x20);
}

// Basic 8x8 font (simplified, only uppercase and digits)
static const uint8_t font8x8[][8] = {
    // Space
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! to / (simplified set)
    {0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00}, // !
    // Numbers 0-9
    {0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00}, // 0
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, // 1
    {0x7C, 0xC6, 0x06, 0x0C, 0x30, 0x60, 0xFE, 0x00}, // 2
    {0x7C, 0xC6, 0x06, 0x3C, 0x06, 0xC6, 0x7C, 0x00}, // 3
    {0x0C, 0x1C, 0x3C, 0x6C, 0xFE, 0x0C, 0x0C, 0x00}, // 4
    {0xFE, 0xC0, 0xFC, 0x06, 0x06, 0xC6, 0x7C, 0x00}, // 5
    {0x7C, 0xC6, 0xC0, 0xFC, 0xC6, 0xC6, 0x7C, 0x00}, // 6
    {0xFE, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00}, // 7
    {0x7C, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0x7C, 0x00}, // 8
    {0x7C, 0xC6, 0xC6, 0x7E, 0x06, 0xC6, 0x7C, 0x00}, // 9
    // Letters A-Z
    {0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0x00}, // A
    {0xFC, 0xC6, 0xC6, 0xFC, 0xC6, 0xC6, 0xFC, 0x00}, // B
    {0x7C, 0xC6, 0xC0, 0xC0, 0xC0, 0xC6, 0x7C, 0x00}, // C
    {0xF8, 0xCC, 0xC6, 0xC6, 0xC6, 0xCC, 0xF8, 0x00}, // D
    {0xFE, 0xC0, 0xC0, 0xFC, 0xC0, 0xC0, 0xFE, 0x00}, // E
    {0xFE, 0xC0, 0xC0, 0xFC, 0xC0, 0xC0, 0xC0, 0x00}, // F
    {0x7C, 0xC6, 0xC0, 0xCE, 0xC6, 0xC6, 0x7C, 0x00}, // G
    {0xC6, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0x00}, // H
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, // I
    {0x06, 0x06, 0x06, 0x06, 0x06, 0xC6, 0x7C, 0x00}, // J
    {0xC6, 0xCC, 0xD8, 0xF0, 0xD8, 0xCC, 0xC6, 0x00}, // K
    {0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xFE, 0x00}, // L
    {0xC6, 0xEE, 0xFE, 0xD6, 0xC6, 0xC6, 0xC6, 0x00}, // M
    {0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00}, // N
    {0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00}, // O
    {0xFC, 0xC6, 0xC6, 0xFC, 0xC0, 0xC0, 0xC0, 0x00}, // P
    {0x7C, 0xC6, 0xC6, 0xC6, 0xD6, 0xCE, 0x7C, 0x06}, // Q
    {0xFC, 0xC6, 0xC6, 0xFC, 0xD8, 0xCC, 0xC6, 0x00}, // R
    {0x7C, 0xC6, 0xC0, 0x7C, 0x06, 0xC6, 0x7C, 0x00}, // S
    {0xFE, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // T
    {0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00}, // U
    {0xC6, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x10, 0x00}, // V
    {0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00}, // W
    {0xC6, 0x6C, 0x38, 0x38, 0x38, 0x6C, 0xC6, 0x00}, // X
    {0xC6, 0xC6, 0x6C, 0x38, 0x18, 0x18, 0x18, 0x00}, // Y
    {0xFE, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFE, 0x00}  // Z
};

// Initialize VGA graphics mode
void vga_init_graphics(void) {
    /*
     * We used to call the BIOS via "int 0x10" to switch to video
     * mode 13h.  In protected mode the BIOS is long gone and vector
     * 0x10 is reserved for the x87-FPU exception (#MF) — the call
     * therefore crashed the kernel.  We now program the VGA hardware
     * directly with the canonical register set for 320×200 256-colour
     * chain-4 graphics (mode 13h).
     */

    static const uint8_t mode13h_regs[] = {
        /* MISC */
        0x63,

        /* Sequencer */
        0x03, 0x01, 0x0F, 0x00, 0x0E,

        /* CRTC */
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x41,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x28,
        0x40, 0x96, 0xB9, 0xA3, 0xFF,

        /* Graphics controller */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF,

        /* Attribute controller */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00, 0x0F, 0x00,
        0x00
    };

    vga_write_regs(mode13h_regs);

    /* Clear the back buffer so that the first swap shows a black
       screen. */
    vga_clear_screen(COLOR_BLACK);
}

// Return to text mode
void vga_return_to_text_mode(void) {
    /* 80×25 16-colour text mode (BIOS mode 03) */

    static const uint8_t mode03_regs[] = {
        /* MISC */
        0x67,

        /* Sequencer */
        0x03, 0x00, 0x03, 0x00, 0x02,

        /* CRTC */
        0x5B, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F, 0x00, 0x4F,
        0x0D, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x9C, 0x0E, 0x8F, 0x28,
        0x1F, 0x96, 0xB9, 0xA3, 0xFF,

        /* Graphics controller */
        0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0E, 0x00, 0xFF,

        /* Attribute controller */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39,
        0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08,
        0x00
    };

    vga_write_regs(mode03_regs);
}

// Set a pixel (to back buffer)
void vga_set_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        back_buffer[y * VGA_WIDTH + x] = color;
    }
}

// Get a pixel color (from back buffer)
uint8_t vga_get_pixel(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        return back_buffer[y * VGA_WIDTH + x];
    }
    return 0;
}

// Set a pixel directly to VGA buffer (for cursor)
void vga_set_pixel_direct(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_buffer[y * VGA_WIDTH + x] = color;
    }
}

// Get a pixel color directly from VGA buffer (for cursor)
uint8_t vga_get_pixel_direct(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        return vga_buffer[y * VGA_WIDTH + x];
    }
    return 0;
}

// Clear the screen
void vga_clear_screen(uint8_t color) {
    memset(back_buffer, color, VGA_SIZE);
}

// Draw a rectangle outline
void vga_draw_rect(int x, int y, int width, int height, uint8_t color) {
    // Top and bottom
    for (int i = 0; i < width; i++) {
        vga_set_pixel(x + i, y, color);
        vga_set_pixel(x + i, y + height - 1, color);
    }
    // Left and right
    for (int i = 0; i < height; i++) {
        vga_set_pixel(x, y + i, color);
        vga_set_pixel(x + width - 1, y + i, color);
    }
}

// Fill a rectangle
void vga_fill_rect(int x, int y, int width, int height, uint8_t color) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            vga_set_pixel(x + i, y + j, color);
        }
    }
}

// Draw a line (Bresenham's algorithm)
void vga_draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = abs_int(x1 - x0);
    int dy = abs_int(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        vga_set_pixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Draw a character
void vga_draw_char(int x, int y, char c, uint8_t fg_color, uint8_t bg_color) {
    int char_index = 0;
    
    // Map character to font index
    if (c == ' ') char_index = 0;
    else if (c == '!') char_index = 1;
    else if (c >= '0' && c <= '9') char_index = 2 + (c - '0');
    else if (c >= 'A' && c <= 'Z') char_index = 12 + (c - 'A');
    else if (c >= 'a' && c <= 'z') char_index = 12 + (c - 'a'); // Treat lowercase as uppercase
    else return; // Character not in font
    
    // Draw the character
    for (int row = 0; row < 8; row++) {
        uint8_t font_row = font8x8[char_index][row];
        for (int col = 0; col < 8; col++) {
            if (font_row & (0x80 >> col)) {
                vga_set_pixel(x + col, y + row, fg_color);
            } else if (bg_color != 255) { // 255 = transparent
                vga_set_pixel(x + col, y + row, bg_color);
            }
        }
    }
}

// Draw a string
void vga_draw_string(int x, int y, const char* str, uint8_t fg_color, uint8_t bg_color) {
    int x_pos = x;
    while (*str) {
        vga_draw_char(x_pos, y, *str, fg_color, bg_color);
        x_pos += 8;
        str++;
    }
}

// Swap buffers (double buffering)
void vga_swap_buffers(void) {
    memcpy(vga_buffer, back_buffer, VGA_SIZE);
}
