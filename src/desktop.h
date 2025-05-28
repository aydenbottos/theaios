#ifndef DESKTOP_H
#define DESKTOP_H

#include <stdbool.h>
#include "util.h"
#include "window_manager.h"

// Desktop icon size
#define ICON_WIDTH 48
#define ICON_HEIGHT 48
#define ICON_SPACING 64
#define ICON_TEXT_HEIGHT 16

// Icon types
typedef enum {
    ICON_TYPE_FILE,
    ICON_TYPE_FOLDER,
    ICON_TYPE_EXECUTABLE,
    ICON_TYPE_TEXT
} icon_type_t;

// Desktop icon structure
typedef struct {
    int x, y;
    char label[32];
    icon_type_t type;
    char path[64];
    bool selected;
} desktop_icon_t;

// Desktop functions
void desktop_init(void);
void desktop_draw(void);
void desktop_handle_mouse(void);
void desktop_add_icon(const char* label, icon_type_t type, const char* path);
void desktop_refresh_icons(void);
void desktop_open_icon(desktop_icon_t* icon);

#endif // DESKTOP_H
