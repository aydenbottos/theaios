#include "gui.h"
#include "vga_graphics.h"
#include "mouse.h"
#include "window_manager.h"
#include "desktop.h"
#include "file_manager.h"
#include "text_editor.h"
#include "keyboard.h"
#include "pit.h"
#include "irq.h"

// GUI state
static bool gui_active = false;
static uint32_t last_update_tick = 0;

// Initialize GUI system
void gui_init(void) {
    // Switch to graphics mode
    vga_init_graphics();
    
    /* Install mouse interrupt handler *before* the device starts
       sending data so that we do not lose the very first packets. */
    irq_install_handler(12, mouse_handler);
    mouse_init();
    
    // Initialize GUI components
    window_manager_init();
    desktop_init();
    file_manager_init();
    text_editor_init();
    
    gui_active = true;
    
    // Initial desktop refresh
    desktop_refresh_icons();
}

// Main GUI event loop
void gui_main_loop(void) {
    uint32_t frame_count = 0;
    
    while (gui_active) {
        // Get current tick
        uint32_t current_tick = pit_get_ticks();
        
        /* Poll mouse in case IRQ12 is not firing (works both ways). */
        mouse_poll();

        // Update at ~30 FPS (every 3 ticks at 100Hz)
        if (current_tick - last_update_tick >= 3) {
            last_update_tick = current_tick;
            
            // Clear screen
            vga_clear_screen(COLOR_BLACK);
            
            // Draw desktop
            desktop_draw();
            
            // Update and draw windows
            window_manager_update();
            window_manager_draw();
            
            // Swap buffers
            vga_swap_buffers();
            
            // Update mouse cursor AFTER buffer swap
            // This draws directly to VGA memory, not the back buffer
            mouse_update_cursor();
            
            // Handle mouse events
            mouse_state_t* mouse = mouse_get_state();
            if (mouse->buttons || mouse->dx || mouse->dy) {
                window_handle_mouse();
                desktop_handle_mouse();
            }
            
            frame_count++;
        }
        
        // Handle keyboard input
        if (keyboard_has_input()) {
            char key = keyboard_get_char();
            gui_handle_keyboard(key);
        }
        
        // Yield CPU
        __asm__ volatile("hlt");
    }
}

// Handle keyboard input in GUI mode
void gui_handle_keyboard(char key) {
    // Send to focused window's text editor if applicable
    text_editor_handle_key(key);
    
    // Global hotkeys
    if (key == 27) { // ESC - return to text mode
        gui_shutdown();
    } else if (key == 19) { // Ctrl+S - save in text editor
        text_editor_save();
    }
}

// Shutdown GUI and return to text mode
void gui_shutdown(void) {
    gui_active = false;
    
    // Clean up windows
    file_manager_close();
    text_editor_close();
    
    // Return to text mode
    vga_return_to_text_mode();
    
    // Uninstall mouse handler
    irq_uninstall_handler(12);
}

// Check if GUI is active
bool gui_is_active(void) {
    return gui_active;
}
