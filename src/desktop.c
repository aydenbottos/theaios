#include "desktop.h"
#include "vga_graphics.h"
#include "mouse.h"
#include "util.h"
#include "kheap.h"
#include "fs.h"

// Maximum desktop icons
#define MAX_DESKTOP_ICONS 32

// Desktop state
static desktop_icon_t* icons[MAX_DESKTOP_ICONS];
static int icon_count = 0;
static desktop_icon_t* selected_icon = NULL;

// Icon bitmaps (simplified 16x16 representations)
static const uint8_t folder_icon[16][16] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,6,6,6,6,6,0,0,0,0,0,0,0,0,0,0},
    {6,6,6,6,6,6,6,6,6,6,6,6,6,0,0,0},
    {6,14,14,14,14,14,14,14,14,14,14,14,6,0,0,0},
    {6,14,14,14,14,14,14,14,14,14,14,14,6,0,0,0},
    {6,14,14,14,14,14,14,14,14,14,14,14,6,0,0,0},
    {6,14,14,14,14,14,14,14,14,14,14,14,6,0,0,0},
    {6,14,14,14,14,14,14,14,14,14,14,14,6,0,0,0},
    {6,14,14,14,14,14,14,14,14,14,14,14,6,0,0,0},
    {6,14,14,14,14,14,14,14,14,14,14,14,6,0,0,0},
    {6,14,14,14,14,14,14,14,14,14,14,14,6,0,0,0},
    {6,14,14,14,14,14,14,14,14,14,14,14,6,0,0,0},
    {6,6,6,6,6,6,6,6,6,6,6,6,6,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static const uint8_t file_icon[16][16] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,15,15,15,15,15,15,15,15,0,0,0,0,0,0,0},
    {0,15,7,7,7,7,7,7,15,15,0,0,0,0,0,0},
    {0,15,7,7,7,7,7,7,7,15,15,0,0,0,0,0},
    {0,15,7,7,7,7,7,7,7,7,15,15,0,0,0,0},
    {0,15,7,7,7,7,7,7,7,7,7,15,0,0,0,0},
    {0,15,15,15,15,15,15,15,15,15,15,15,0,0,0,0},
    {0,15,7,7,7,7,7,7,7,7,7,15,0,0,0,0},
    {0,15,7,7,7,7,7,7,7,7,7,15,0,0,0,0},
    {0,15,7,7,7,7,7,7,7,7,7,15,0,0,0,0},
    {0,15,7,7,7,7,7,7,7,7,7,15,0,0,0,0},
    {0,15,7,7,7,7,7,7,7,7,7,15,0,0,0,0},
    {0,15,7,7,7,7,7,7,7,7,7,15,0,0,0,0},
    {0,15,15,15,15,15,15,15,15,15,15,15,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

static const uint8_t exe_icon[16][16] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {0,1,9,9,9,9,9,9,9,9,9,9,1,0,0,0},
    {0,1,9,15,15,15,15,15,15,15,15,9,1,0,0,0},
    {0,1,9,15,0,0,15,15,0,0,15,9,1,0,0,0},
    {0,1,9,15,0,0,15,15,0,0,15,9,1,0,0,0},
    {0,1,9,15,15,15,15,15,15,15,15,9,1,0,0,0},
    {0,1,9,15,15,15,15,15,15,15,15,9,1,0,0,0},
    {0,1,9,15,0,0,0,0,0,0,15,9,1,0,0,0},
    {0,1,9,15,0,0,0,0,0,0,15,9,1,0,0,0},
    {0,1,9,15,15,15,15,15,15,15,15,9,1,0,0,0},
    {0,1,9,9,9,9,9,9,9,9,9,9,1,0,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// Initialize desktop
void desktop_init(void) {
    for (int i = 0; i < MAX_DESKTOP_ICONS; i++) {
        icons[i] = NULL;
    }
    icon_count = 0;
    selected_icon = NULL;
    
    // Add some default icons
    desktop_add_icon("Files", ICON_TYPE_FOLDER, "/");
    desktop_add_icon("Editor", ICON_TYPE_EXECUTABLE, "editor");
    desktop_add_icon("README", ICON_TYPE_TEXT, "README.TXT");
}

// Add icon to desktop
void desktop_add_icon(const char* label, icon_type_t type, const char* path) {
    if (icon_count >= MAX_DESKTOP_ICONS) return;
    
    desktop_icon_t* icon = (desktop_icon_t*)kmalloc(sizeof(desktop_icon_t));
    if (!icon) return;
    
    // Calculate position (grid layout)
    int row = icon_count / 4;
    int col = icon_count % 4;
    icon->x = 10 + col * ICON_SPACING;
    icon->y = 10 + row * ICON_SPACING;
    
    strncpy(icon->label, label, 31);
    icon->label[31] = '\0';
    icon->type = type;
    strncpy(icon->path, path, 63);
    icon->path[63] = '\0';
    icon->selected = false;
    
    icons[icon_count++] = icon;
}

// Draw icon
static void draw_icon(desktop_icon_t* icon) {
    if (!icon) return;
    
    // Draw selection highlight
    if (icon->selected) {
        vga_fill_rect(icon->x - 2, icon->y - 2, 
                     ICON_WIDTH + 4, ICON_HEIGHT + ICON_TEXT_HEIGHT + 4,
                     COLOR_LIGHT_BLUE);
    }
    
    // Draw icon bitmap
    const uint8_t (*bitmap)[16] = NULL;
    switch (icon->type) {
        case ICON_TYPE_FOLDER:
            bitmap = folder_icon;
            break;
        case ICON_TYPE_EXECUTABLE:
            bitmap = exe_icon;
            break;
        case ICON_TYPE_FILE:
        case ICON_TYPE_TEXT:
        default:
            bitmap = file_icon;
            break;
    }
    
    // Scale up icon (16x16 to 48x48)
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint8_t color = bitmap[y][x];
            if (color != 0) {
                // Draw 3x3 block for each pixel
                for (int dy = 0; dy < 3; dy++) {
                    for (int dx = 0; dx < 3; dx++) {
                        vga_set_pixel(icon->x + x*3 + dx, icon->y + y*3 + dy, color);
                    }
                }
            }
        }
    }
    
    // Draw label
    int text_x = icon->x + (ICON_WIDTH - strlen(icon->label) * 8) / 2;
    int text_y = icon->y + ICON_HEIGHT + 2;
    vga_draw_string(text_x, text_y, icon->label, 
                   icon->selected ? COLOR_WHITE : COLOR_BLACK, 255);
}

// Draw desktop
void desktop_draw(void) {
    // Draw desktop background (gradient effect)
    for (int y = 0; y < VGA_HEIGHT; y++) {
        uint8_t color = COLOR_CYAN + (y * 2 / VGA_HEIGHT);
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_set_pixel(x, y, color);
        }
    }
    
    // Draw all icons
    for (int i = 0; i < icon_count; i++) {
        if (icons[i]) {
            draw_icon(icons[i]);
        }
    }
}

// Handle mouse for desktop
void desktop_handle_mouse(void) {
    if (!mouse_left_clicked()) return;
    
    mouse_state_t* mouse = mouse_get_state();
    
    // Check icon clicks
    for (int i = 0; i < icon_count; i++) {
        desktop_icon_t* icon = icons[i];
        if (!icon) continue;
        
        if (mouse_in_rect(icon->x, icon->y, ICON_WIDTH, ICON_HEIGHT + ICON_TEXT_HEIGHT)) {
            // Deselect previous
            if (selected_icon) {
                selected_icon->selected = false;
            }
            
            // Select this icon
            icon->selected = true;
            selected_icon = icon;
            
            // Check for double-click (simplified - just open on click for now)
            desktop_open_icon(icon);
            return;
        }
    }
    
    // Click on empty space - deselect
    if (selected_icon) {
        selected_icon->selected = false;
        selected_icon = NULL;
    }
}

// Forward declarations for file manager and editor
void file_manager_open(const char* path);
void text_editor_open(const char* filename);

// Open an icon
void desktop_open_icon(desktop_icon_t* icon) {
    if (!icon) return;
    
    switch (icon->type) {
        case ICON_TYPE_FOLDER:
            file_manager_open(icon->path);
            break;
            
        case ICON_TYPE_EXECUTABLE:
            if (strcmp(icon->path, "editor") == 0) {
                text_editor_open(NULL);
            }
            break;
            
        case ICON_TYPE_TEXT:
            text_editor_open(icon->path);
            break;
            
        default:
            break;
    }
}

// Refresh desktop icons from filesystem
void desktop_refresh_icons(void) {
    // Clear existing icons (except system ones)
    int system_icons = 2; // Files and Editor
    while (icon_count > system_icons) {
        // kfree(icons[--icon_count]); // Would use if kfree existed
        icon_count--;
    }
    
    // Read files from root directory and add icons
    fat_directory_t dir;
    if (fat_read_directory("/", &dir) == 0) {
        for (int i = 0; i < dir.count && icon_count < MAX_DESKTOP_ICONS; i++) {
            // Skip system files and directories
            if (strcmp(dir.entries[i].filename, ".") == 0 ||
                strcmp(dir.entries[i].filename, "..") == 0) {
                continue;
            }
            
            // Determine icon type based on extension
            icon_type_t type = ICON_TYPE_FILE;
            int len = strlen(dir.entries[i].filename);
            if (len > 4) {
                if (strcmp(&dir.entries[i].filename[len-4], ".TXT") == 0) {
                    type = ICON_TYPE_TEXT;
                } else if (strcmp(&dir.entries[i].filename[len-4], ".ELF") == 0) {
                    type = ICON_TYPE_EXECUTABLE;
                }
            }
            
            desktop_add_icon(dir.entries[i].filename, type, dir.entries[i].filename);
        }
    }
}
