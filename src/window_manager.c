#include "window_manager.h"
#include "vga_graphics.h"
#include "mouse.h"
#include "util.h"
#include "kheap.h"

// Window array and management
static window_t* windows[MAX_WINDOWS];
static int window_count = 0;
static int next_window_id = 1;
static window_t* focused_window = NULL;
static window_t* dragging_window = NULL;
static int drag_offset_x = 0;
static int drag_offset_y = 0;

// Initialize window manager
void window_manager_init(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i] = NULL;
    }
    window_count = 0;
    focused_window = NULL;
    dragging_window = NULL;
}

// Create a new window
window_t* window_create(int x, int y, int width, int height, const char* title, uint8_t flags) {
    if (window_count >= MAX_WINDOWS) {
        return NULL;
    }
    
    window_t* win = (window_t*)kmalloc(sizeof(window_t));
    if (!win) {
        return NULL;
    }
    
    win->id = next_window_id++;
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    strncpy(win->title, title, 63);
    win->title[63] = '\0';
    win->flags = flags;
    win->visible = true;
    win->minimized = false;
    win->draw_content = NULL;
    win->on_click = NULL;
    win->user_data = NULL;
    
    // Add to window list
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == NULL) {
            windows[i] = win;
            window_count++;
            break;
        }
    }
    
    // Set as focused window
    window_bring_to_front(win);
    
    return win;
}

// Destroy a window
void window_destroy(window_t* win) {
    if (!win) return;
    
    // Remove from window list
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == win) {
            windows[i] = NULL;
            window_count--;
            break;
        }
    }
    
    // Update focus
    if (focused_window == win) {
        focused_window = NULL;
        // Find new window to focus
        for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
            if (windows[i] && windows[i]->visible && !windows[i]->minimized) {
                focused_window = windows[i];
                break;
            }
        }
    }
    
    // kfree(win); // Would use this if kfree was implemented
}

// Draw a window
void window_draw(window_t* win) {
    if (!win || !win->visible || win->minimized) return;
    
    bool is_focused = (win == focused_window);
    
    // Draw window shadow
    vga_fill_rect(win->x + 2, win->y + 2, win->width, win->height, COLOR_DARK_GRAY);
    
    // Draw window border
    vga_fill_rect(win->x, win->y, win->width, win->height, WIN_COLOR_BORDER);
    
    // Draw title bar
    uint8_t titlebar_color = is_focused ? WIN_COLOR_TITLEBAR_ACTIVE : WIN_COLOR_TITLEBAR;
    vga_fill_rect(win->x + 1, win->y + 1, win->width - 2, TITLEBAR_HEIGHT, titlebar_color);
    
    // Draw title text
    vga_draw_string(win->x + 5, win->y + 6, win->title, COLOR_WHITE, 255);
    
    // Draw close button if closeable
    if (win->flags & WINDOW_CLOSEABLE) {
        int close_x = win->x + win->width - 18;
        int close_y = win->y + 4;
        vga_fill_rect(close_x, close_y, 12, 12, COLOR_RED);
        vga_draw_string(close_x + 2, close_y + 2, "X", COLOR_WHITE, 255);
    }
    
    // Draw content area
    vga_fill_rect(win->x + 1, win->y + TITLEBAR_HEIGHT + 1, 
                  win->width - 2, win->height - TITLEBAR_HEIGHT - 2, 
                  WIN_COLOR_BACKGROUND);
    
    // Draw custom content if handler exists
    if (win->draw_content) {
        win->draw_content(win);
    }
}

// Bring window to front
void window_bring_to_front(window_t* win) {
    if (!win) return;
    
    // Find window in array
    int win_index = -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == win) {
            win_index = i;
            break;
        }
    }
    
    if (win_index < 0) return;
    
    // Move to end of array (top of z-order)
    for (int i = win_index; i < MAX_WINDOWS - 1; i++) {
        windows[i] = windows[i + 1];
    }
    windows[MAX_WINDOWS - 1] = win;
    
    // Update focus
    if (focused_window) {
        focused_window->flags &= ~WINDOW_FOCUSED;
    }
    focused_window = win;
    win->flags |= WINDOW_FOCUSED;
}

// Set content draw handler
void window_set_content_handler(window_t* win, void (*handler)(window_t*)) {
    if (win) {
        win->draw_content = handler;
    }
}

// Set click handler
void window_set_click_handler(window_t* win, void (*handler)(window_t*, int, int)) {
    if (win) {
        win->on_click = handler;
    }
}

// Update window manager
void window_manager_update(void) {
    mouse_state_t* mouse = mouse_get_state();
    
    // Handle window dragging
    if (dragging_window) {
        if (mouse->buttons & 0x01) { // Left button still pressed
            dragging_window->x = mouse->x - drag_offset_x;
            dragging_window->y = mouse->y - drag_offset_y;
            
            // Keep window on screen
            if (dragging_window->x < 0) dragging_window->x = 0;
            if (dragging_window->y < 0) dragging_window->y = 0;
            if (dragging_window->x + dragging_window->width > VGA_WIDTH) {
                dragging_window->x = VGA_WIDTH - dragging_window->width;
            }
            if (dragging_window->y + dragging_window->height > VGA_HEIGHT) {
                dragging_window->y = VGA_HEIGHT - dragging_window->height;
            }
        } else {
            dragging_window = NULL;
        }
    }
}

// Handle mouse input for windows
void window_handle_mouse(void) {
    if (!mouse_left_clicked()) return;
    
    mouse_state_t* mouse = mouse_get_state();
    
    // Check windows from top to bottom
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        window_t* win = windows[i];
        if (!win || !win->visible || win->minimized) continue;
        
        // Check if click is on window
        if (mouse_in_rect(win->x, win->y, win->width, win->height)) {
            // Bring to front
            window_bring_to_front(win);
            
            // Check for close button click
            if ((win->flags & WINDOW_CLOSEABLE) && 
                mouse_in_rect(win->x + win->width - 18, win->y + 4, 12, 12)) {
                window_destroy(win);
                return;
            }
            
            // Check for title bar click (drag)
            if ((win->flags & WINDOW_MOVEABLE) && 
                mouse_in_rect(win->x, win->y, win->width, TITLEBAR_HEIGHT)) {
                dragging_window = win;
                drag_offset_x = mouse->x - win->x;
                drag_offset_y = mouse->y - win->y;
                return;
            }
            
            // Check for content click
            if (win->on_click && 
                mouse_in_rect(win->x + 1, win->y + TITLEBAR_HEIGHT + 1,
                            win->width - 2, win->height - TITLEBAR_HEIGHT - 2)) {
                int rel_x = mouse->x - win->x - 1;
                int rel_y = mouse->y - win->y - TITLEBAR_HEIGHT - 1;
                win->on_click(win, rel_x, rel_y);
            }
            
            return;
        }
    }
}

// Draw all windows
void window_manager_draw(void) {
    // Draw windows from bottom to top
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i]) {
            window_draw(windows[i]);
        }
    }
}

// Get window by ID
window_t* window_get_by_id(int id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] && windows[i]->id == id) {
            return windows[i];
        }
    }
    return NULL;
}
