#include "file_manager.h"
#include "vga_graphics.h"
#include "kheap.h"
#include "util.h"
#include "desktop.h"

// Global file manager instance
static file_manager_t* file_manager = NULL;

// File manager window content drawing
static void file_manager_draw_content(window_t* win) {
    if (!file_manager) return;
    
    int x = win->x + 5;
    int y = win->y + TITLEBAR_HEIGHT + 5;
    int width = win->width - 10;
    int height = win->height - TITLEBAR_HEIGHT - 10;
    
    // Draw current path
    vga_fill_rect(x, y, width, 16, COLOR_WHITE);
    vga_draw_string(x + 2, y + 4, "Path: ", COLOR_BLACK, COLOR_WHITE);
    vga_draw_string(x + 50, y + 4, file_manager->current_path, COLOR_BLACK, COLOR_WHITE);
    
    y += 20;
    
    // Draw file list header
    vga_fill_rect(x, y, width, 16, COLOR_DARK_GRAY);
    vga_draw_string(x + 2, y + 4, "Name", COLOR_WHITE, COLOR_DARK_GRAY);
    vga_draw_string(x + 150, y + 4, "Size", COLOR_WHITE, COLOR_DARK_GRAY);
    
    y += 18;
    
    // Draw files
    int visible_files = (height - 38) / 16;
    for (int i = 0; i < visible_files && i + file_manager->scroll_offset < file_manager->file_count; i++) {
        int file_idx = i + file_manager->scroll_offset;
        file_entry_t* file = &file_manager->files[file_idx];
        
        // Draw selection
        uint8_t bg_color = (file_idx == file_manager->selected_index) ? COLOR_LIGHT_BLUE : COLOR_WHITE;
        uint8_t fg_color = (file_idx == file_manager->selected_index) ? COLOR_WHITE : COLOR_BLACK;
        
        vga_fill_rect(x, y + i * 16, width, 16, bg_color);
        
        // Draw icon
        if (file->is_directory) {
            // Simple folder icon
            vga_fill_rect(x + 2, y + i * 16 + 2, 12, 10, COLOR_YELLOW);
            vga_fill_rect(x + 2, y + i * 16 + 1, 6, 2, COLOR_BROWN);
        } else {
            // Simple file icon
            vga_fill_rect(x + 2, y + i * 16 + 2, 10, 12, COLOR_WHITE);
            vga_draw_rect(x + 2, y + i * 16 + 2, 10, 12, COLOR_BLACK);
        }
        
        // Draw name
        vga_draw_string(x + 18, y + i * 16 + 4, file->name, fg_color, bg_color);
        
        // Draw size (for files only)
        if (!file->is_directory) {
            char size_str[16];
            itoa(file->size, size_str, 10);
            strcat(size_str, " B");
            vga_draw_string(x + 150, y + i * 16 + 4, size_str, fg_color, bg_color);
        }
    }
}

// File manager click handler
static void file_manager_on_click(window_t* win, int x, int y) {
    if (!file_manager) return;
    
    // Check if click is in file list area
    if (y >= 38) {
        int file_idx = (y - 38) / 16 + file_manager->scroll_offset;
        if (file_idx < file_manager->file_count) {
            file_manager->selected_index = file_idx;
            
            // Double-click simulation (open on click)
            file_entry_t* file = &file_manager->files[file_idx];
            if (file->is_directory) {
                // Change directory
                if (strcmp(file->name, "..") == 0) {
                    // Go up one level
                    char* last_slash = strrchr(file_manager->current_path, '/');
                    if (last_slash && last_slash != file_manager->current_path) {
                        *last_slash = '\0';
                    }
                } else if (strcmp(file->name, ".") != 0) {
                    // Enter subdirectory
                    if (strlen(file_manager->current_path) > 1) {
                        strcat(file_manager->current_path, "/");
                    }
                    strcat(file_manager->current_path, file->name);
                }
                file_manager_refresh();
            } else {
                // Open file (for now, just try to open with text editor)
                char full_path[128];
                strcpy(full_path, file_manager->current_path);
                if (strlen(full_path) > 1) {
                    strcat(full_path, "/");
                }
                strcat(full_path, file->name);
                
                // Check file extension
                int len = strlen(file->name);
                if (len > 4 && strcmp(&file->name[len-4], ".TXT") == 0) {
                    // Forward declaration
                    void text_editor_open(const char* filename);
                    text_editor_open(full_path);
                }
            }
        }
    }
}

// Initialize file manager
void file_manager_init(void) {
    if (!file_manager) {
        file_manager = (file_manager_t*)kmalloc(sizeof(file_manager_t));
        if (!file_manager) return;
    }
    
    strcpy(file_manager->current_path, "/");
    file_manager->file_count = 0;
    file_manager->selected_index = 0;
    file_manager->scroll_offset = 0;
    file_manager->window = NULL;
}

// Open file manager window
void file_manager_open(const char* path) {
    file_manager_init();
    
    // Close existing window if open
    if (file_manager->window) {
        window_destroy(file_manager->window);
    }
    
    // Create window
    file_manager->window = window_create(50, 50, 300, 200, "File Manager", 
                                       WINDOW_MOVEABLE | WINDOW_CLOSEABLE);
    if (!file_manager->window) return;
    
    // Set handlers
    window_set_content_handler(file_manager->window, file_manager_draw_content);
    window_set_click_handler(file_manager->window, file_manager_on_click);
    
    // Set initial path
    if (path) {
        strncpy(file_manager->current_path, path, 127);
        file_manager->current_path[127] = '\0';
    }
    
    // Load directory
    file_manager_refresh();
}

// Close file manager
void file_manager_close(void) {
    if (file_manager && file_manager->window) {
        window_destroy(file_manager->window);
        file_manager->window = NULL;
    }
}

// Refresh file list
void file_manager_refresh(void) {
    if (!file_manager) return;
    
    file_manager->file_count = 0;
    file_manager->selected_index = 0;
    file_manager->scroll_offset = 0;
    
    // Read directory
    fat_directory_t dir;
    if (fat_read_directory(file_manager->current_path, &dir) == 0) {
        // Add entries
        for (int i = 0; i < dir.count && file_manager->file_count < 32; i++) {
            file_entry_t* entry = &file_manager->files[file_manager->file_count];
            strncpy(entry->name, dir.entries[i].filename, 12);
            entry->name[12] = '\0';
            entry->size = dir.entries[i].size;
            entry->is_directory = (dir.entries[i].attributes & 0x10) != 0;
            file_manager->file_count++;
        }
    }
    
    // Sort entries (directories first)
    for (int i = 0; i < file_manager->file_count - 1; i++) {
        for (int j = i + 1; j < file_manager->file_count; j++) {
            bool swap = false;
            if (file_manager->files[i].is_directory && !file_manager->files[j].is_directory) {
                continue;
            } else if (!file_manager->files[i].is_directory && file_manager->files[j].is_directory) {
                swap = true;
            } else if (strcmp(file_manager->files[i].name, file_manager->files[j].name) > 0) {
                swap = true;
            }
            
            if (swap) {
                file_entry_t temp = file_manager->files[i];
                file_manager->files[i] = file_manager->files[j];
                file_manager->files[j] = temp;
            }
        }
    }
}
