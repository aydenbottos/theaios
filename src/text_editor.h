#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include "window_manager.h"

// Text buffer settings
#define MAX_TEXT_SIZE 4096
#define MAX_LINES 100
#define CHARS_PER_LINE 40

// Text editor state
typedef struct {
    window_t* window;
    char filename[64];
    char text[MAX_TEXT_SIZE];
    int text_length;
    int cursor_pos;
    int cursor_line;
    int cursor_col;
    int scroll_line;
    bool modified;
} text_editor_t;

// Text editor functions
void text_editor_init(void);
void text_editor_open(const char* filename);
void text_editor_close(void);
void text_editor_save(void);
void text_editor_handle_key(char key);

#endif // TEXT_EDITOR_H
