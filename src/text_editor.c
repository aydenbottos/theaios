#include "text_editor.h"
#include "vga_graphics.h"
#include "kheap.h"
#include "util.h"
#include "fs.h"
#include "keyboard.h"

// Global text editor instance
static text_editor_t* text_editor = NULL;

// Calculate cursor position from text position
static void update_cursor_position(void) {
    if (!text_editor) return;
    
    text_editor->cursor_line = 0;
    text_editor->cursor_col = 0;
    
    for (int i = 0; i < text_editor->cursor_pos && i < text_editor->text_length; i++) {
        if (text_editor->text[i] == '\n') {
            text_editor->cursor_line++;
            text_editor->cursor_col = 0;
        } else {
            text_editor->cursor_col++;
        }
    }
}

// Draw text editor content
static void text_editor_draw_content(window_t* win) {
    if (!text_editor) return;
    
    int x = win->x + 5;
    int y = win->y + TITLEBAR_HEIGHT + 5;
    int width = win->width - 10;
    int height = win->height - TITLEBAR_HEIGHT - 25;
    
    // Draw status bar
    int status_y = win->y + win->height - 20;
    vga_fill_rect(win->x + 1, status_y, win->width - 2, 19, COLOR_DARK_GRAY);
    
    // Status text
    char status[128];
    sprintf(status, "Line %d, Col %d", text_editor->cursor_line + 1, text_editor->cursor_col + 1);
    vga_draw_string(win->x + 5, status_y + 6, status, COLOR_WHITE, COLOR_DARK_GRAY);
    
    if (text_editor->modified) {
        vga_draw_string(win->x + 150, status_y + 6, "Modified", COLOR_YELLOW, COLOR_DARK_GRAY);
    }
    
    // Draw text
    int line = 0;
    int col = 0;
    int visible_lines = height / 10;
    
    for (int i = 0; i < text_editor->text_length; i++) {
        // Skip lines before scroll
        if (line < text_editor->scroll_line) {
            if (text_editor->text[i] == '\n') {
                line++;
            }
            continue;
        }
        
        // Stop if we've drawn all visible lines
        if (line >= text_editor->scroll_line + visible_lines) {
            break;
        }
        
        // Draw character
        if (text_editor->text[i] == '\n') {
            line++;
            col = 0;
        } else if (col < CHARS_PER_LINE) {
            int char_x = x + col * 8;
            int char_y = y + (line - text_editor->scroll_line) * 10;
            
            // Draw cursor background
            if (i == text_editor->cursor_pos) {
                vga_fill_rect(char_x, char_y, 8, 10, COLOR_BLUE);
                vga_draw_char(char_x, char_y + 1, text_editor->text[i], COLOR_WHITE, COLOR_BLUE);
            } else {
                vga_draw_char(char_x, char_y + 1, text_editor->text[i], COLOR_BLACK, WIN_COLOR_BACKGROUND);
            }
            col++;
        }
    }
    
    // Draw cursor at end of text
    if (text_editor->cursor_pos == text_editor->text_length && 
        line >= text_editor->scroll_line && 
        line < text_editor->scroll_line + visible_lines) {
        int char_x = x + col * 8;
        int char_y = y + (line - text_editor->scroll_line) * 10;
        vga_fill_rect(char_x, char_y, 8, 10, COLOR_BLUE);
    }
}

// Handle keyboard input
void text_editor_handle_key(char key) {
    if (!text_editor || !text_editor->window) return;
    
    if (key == '\b') { // Backspace
        if (text_editor->cursor_pos > 0) {
            // Remove character
            for (int i = text_editor->cursor_pos - 1; i < text_editor->text_length - 1; i++) {
                text_editor->text[i] = text_editor->text[i + 1];
            }
            text_editor->text_length--;
            text_editor->cursor_pos--;
            text_editor->modified = true;
            update_cursor_position();
        }
    } else if (key == '\n' || (key >= 32 && key < 127)) { // Printable or newline
        if (text_editor->text_length < MAX_TEXT_SIZE - 1) {
            // Insert character
            for (int i = text_editor->text_length; i > text_editor->cursor_pos; i--) {
                text_editor->text[i] = text_editor->text[i - 1];
            }
            text_editor->text[text_editor->cursor_pos] = key;
            text_editor->text_length++;
            text_editor->cursor_pos++;
            text_editor->modified = true;
            update_cursor_position();
        }
    }
    
    // Update scroll if cursor moved off screen
    int visible_lines = (text_editor->window->height - TITLEBAR_HEIGHT - 25) / 10;
    if (text_editor->cursor_line < text_editor->scroll_line) {
        text_editor->scroll_line = text_editor->cursor_line;
    } else if (text_editor->cursor_line >= text_editor->scroll_line + visible_lines) {
        text_editor->scroll_line = text_editor->cursor_line - visible_lines + 1;
    }
}

// Initialize text editor
void text_editor_init(void) {
    if (!text_editor) {
        text_editor = (text_editor_t*)kmalloc(sizeof(text_editor_t));
        if (!text_editor) return;
    }
    
    memset(text_editor, 0, sizeof(text_editor_t));
}

// Open text editor
void text_editor_open(const char* filename) {
    text_editor_init();
    
    // Close existing window
    if (text_editor->window) {
        window_destroy(text_editor->window);
    }
    
    // Create window
    char title[80];
    if (filename) {
        sprintf(title, "Text Editor - %s", filename);
        strncpy(text_editor->filename, filename, 63);
        text_editor->filename[63] = '\0';
    } else {
        strcpy(title, "Text Editor - Untitled");
        text_editor->filename[0] = '\0';
    }
    
    text_editor->window = window_create(100, 50, 350, 250, title, 
                                      WINDOW_MOVEABLE | WINDOW_CLOSEABLE);
    if (!text_editor->window) return;
    
    window_set_content_handler(text_editor->window, text_editor_draw_content);
    
    // Load file if specified
    if (filename) {
        uint8_t* buffer = (uint8_t*)kmalloc(MAX_TEXT_SIZE);
        if (buffer) {
            uint32_t size;
            if (fat_read_file(filename, buffer, &size) == 0 && size < MAX_TEXT_SIZE) {
                memcpy(text_editor->text, buffer, size);
                text_editor->text_length = size;
                text_editor->text[size] = '\0';
            }
            // kfree(buffer); // Would use if kfree existed
        }
    }
    
    text_editor->cursor_pos = 0;
    text_editor->modified = false;
    update_cursor_position();
}

// Save file
void text_editor_save(void) {
    if (!text_editor || !text_editor->filename[0]) return;
    
    if (fat_write_file(text_editor->filename, (uint8_t*)text_editor->text, 
                      text_editor->text_length) == 0) {
        text_editor->modified = false;
    }
}

// Close editor
void text_editor_close(void) {
    if (text_editor && text_editor->window) {
        window_destroy(text_editor->window);
        text_editor->window = NULL;
    }
}
