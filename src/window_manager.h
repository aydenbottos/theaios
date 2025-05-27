#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <stdbool.h>
#include "util.h"

// Window flags
#define WINDOW_MOVEABLE    0x01
#define WINDOW_RESIZABLE   0x02
#define WINDOW_CLOSEABLE   0x04
#define WINDOW_MINIMIZABLE 0x08
#define WINDOW_FOCUSED     0x10

// Window colors
#define WIN_COLOR_TITLEBAR      COLOR_BLUE
#define WIN_COLOR_TITLEBAR_ACTIVE COLOR_LIGHT_BLUE
#define WIN_COLOR_BORDER        COLOR_DARK_GRAY
#define WIN_COLOR_BACKGROUND    COLOR_LIGHT_GRAY
#define WIN_COLOR_TEXT          COLOR_BLACK

// Title bar height
#define TITLEBAR_HEIGHT 20

// Maximum windows
#define MAX_WINDOWS 16

// Window structure
typedef struct window {
    int id;
    int x, y;
    int width, height;
    char title[64];
    uint8_t flags;
    bool visible;
    bool minimized;
    void (*draw_content)(struct window* win);
    void (*on_click)(struct window* win, int x, int y);
    void* user_data;
} window_t;

// Window manager functions
void window_manager_init(void);
void window_manager_update(void);
void window_manager_draw(void);
window_t* window_create(int x, int y, int width, int height, const char* title, uint8_t flags);
void window_destroy(window_t* win);
void window_draw(window_t* win);
void window_bring_to_front(window_t* win);
void window_set_content_handler(window_t* win, void (*handler)(window_t*));
void window_set_click_handler(window_t* win, void (*handler)(window_t*, int, int));
void window_handle_mouse(void);
window_t* window_get_by_id(int id);

#endif // WINDOW_MANAGER_H
