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
#include "serial.h"

// GUI state
static bool gui_active = false;
static uint32_t last_update_tick = 0;

// Mouse simulation for testing
static int simulated_x = VGA_WIDTH / 2;
static int simulated_y = VGA_HEIGHT / 2;

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
    uint32_t poll_status_timer = 0;
    
    while (gui_active) {
        // Get current tick
        uint32_t current_tick = pit_get_ticks();
        
        /* Poll mouse in case IRQ12 is not firing (works both ways). */
        mouse_poll();
        
        // TEMPORARY: If no mouse data after 100 ticks, use keyboard simulation
        poll_status_timer++;
        if (poll_status_timer > 100) {
            mouse_state_t* mouse = mouse_get_state();
            if (mouse->x == VGA_WIDTH/2 && mouse->y == VGA_HEIGHT/2) {
                // No mouse movement detected, enable keyboard control
                static bool warned = false;
                if (!warned) {
                    serial_puts("\nWARNING: No mouse data detected. Use arrow keys to move cursor.\n");
                    warned = true;
                }
            }
        }

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
            
            // TEMPORARY: Arrow keys move cursor if no mouse data
            mouse_state_t* mouse = mouse_get_state();
            static bool using_keyboard_mouse = false;
            
            // Check if we should use keyboard control
            if (poll_status_timer > 100 && mouse->x == VGA_WIDTH/2 && mouse->y == VGA_HEIGHT/2) {
                using_keyboard_mouse = true;
            }
            
            if (using_keyboard_mouse) {
                // Arrow key codes (scan codes)
                if (key == 72) { // Up arrow
                    simulated_y -= 5;
                    if (simulated_y < 0) simulated_y = 0;
                    mouse->y = simulated_y;
                } else if (key == 80) { // Down arrow
                    simulated_y += 5;
                    if (simulated_y >= VGA_HEIGHT) simulated_y = VGA_HEIGHT - 1;
                    mouse->y = simulated_y;
                } else if (key == 75) { // Left arrow
                    simulated_x -= 5;
                    if (simulated_x < 0) simulated_x = 0;
                    mouse->x = simulated_x;
                } else if (key == 77) { // Right arrow
                    simulated_x += 5;
                    if (simulated_x >= VGA_WIDTH) simulated_x = VGA_WIDTH - 1;
                    mouse->x = simulated_x;
                } else if (key == ' ') { // Space = click
                    mouse->buttons = 1;
                } else {
                    mouse->buttons = 0;
                }
            }
            
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
