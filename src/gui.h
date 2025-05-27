#ifndef GUI_H
#define GUI_H

#include "util.h"

// GUI mode functions
void gui_init(void);
void gui_main_loop(void);
void gui_shutdown(void);
bool gui_is_active(void);
void gui_handle_keyboard(char key);

#endif // GUI_H
