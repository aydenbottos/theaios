#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "window_manager.h"
#include "fs.h"

// File list entry
typedef struct {
    char name[13];
    uint32_t size;
    bool is_directory;
} file_entry_t;

// File manager state
typedef struct {
    window_t* window;
    char current_path[128];
    file_entry_t files[32];
    int file_count;
    int selected_index;
    int scroll_offset;
} file_manager_t;

// File manager functions
void file_manager_init(void);
void file_manager_open(const char* path);
void file_manager_close(void);
void file_manager_refresh(void);

#endif // FILE_MANAGER_H
