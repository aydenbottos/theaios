#ifndef VGA_GRAPHICS_H
#define VGA_GRAPHICS_H

#include "util.h"

// VGA 320x200 256-color mode (mode 13h)
#define VGA_WIDTH 320
#define VGA_HEIGHT 200
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT)

// Colors (basic palette)
#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_BROWN 6
#define COLOR_LIGHT_GRAY 7
#define COLOR_DARK_GRAY 8
#define COLOR_LIGHT_BLUE 9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN 11
#define COLOR_LIGHT_RED 12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW 14
#define COLOR_WHITE 15

// Function declarations
void vga_init_graphics(void);
void vga_set_pixel(int x, int y, uint8_t color);
uint8_t vga_get_pixel(int x, int y);
void vga_clear_screen(uint8_t color);
void vga_draw_rect(int x, int y, int width, int height, uint8_t color);
void vga_fill_rect(int x, int y, int width, int height, uint8_t color);
void vga_draw_line(int x0, int y0, int x1, int y1, uint8_t color);
void vga_draw_char(int x, int y, char c, uint8_t fg_color, uint8_t bg_color);
void vga_draw_string(int x, int y, const char* str, uint8_t fg_color, uint8_t bg_color);
void vga_swap_buffers(void);
void vga_return_to_text_mode(void);

// Direct VGA buffer access (for cursor drawing)
void vga_set_pixel_direct(int x, int y, uint8_t color);
uint8_t vga_get_pixel_direct(int x, int y);

#endif // VGA_GRAPHICS_H
