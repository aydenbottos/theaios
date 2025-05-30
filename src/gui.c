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
static bool using_keyboard_mouse = false;

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
        
        // TEMPORARY: If no mouse data after 300 ticks, use keyboard simulation
        poll_status_timer++;
        if (poll_status_timer == 300) {
            mouse_state_t* mouse = mouse_get_state();
            if (mouse->x == VGA_WIDTH/2 && mouse->y == VGA_HEIGHT/2) {
                // No mouse movement detected, enable keyboard control
                using_keyboard_mouse = true;
                serial_puts("\nWARNING: No mouse data detected. Use arrow keys to move cursor.\n");
                serial_puts("Arrow keys = move, Space = click, Enter = right-click\n");
                
                // Initialize simulated position to current mouse position
                simulated_x = mouse->x;
                simulated_y = mouse->y;
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
            gui_handle_keyboard(key);
        }
        
        // Handle special keys (arrow keys, etc.)
        if (keyboard_has_scancode()) {
            uint8_t scancode = keyboard_get_scancode();
            
            if (using_keyboard_mouse) {
                mouse_state_t* mouse = mouse_get_state();
                static bool space_was_pressed = false;
                
                switch (scancode) {
                    case SCANCODE_UP:
                        simulated_y -= 5;
                        if (simulated_y < 0) simulated_y = 0;
                        mouse->y = simulated_y;
                        break;
                        
                    case SCANCODE_DOWN:
                        simulated_y += 5;
                        if (simulated_y >= VGA_HEIGHT) simulated_y = VGA_HEIGHT - 1;
                        mouse->y = simulated_y;
                        break;
                        
                    case SCANCODE_LEFT:
                        simulated_x -= 5;
                        if (simulated_x < 0) simulated_x = 0;
                        mouse->x = simulated_x;
                        break;
                        
                    case SCANCODE_RIGHT:
                        simulated_x += 5;
                        if (simulated_x >= VGA_WIDTH) simulated_x = VGA_WIDTH - 1;
                        mouse->x = simulated_x;
                        break;
                        
                    case SCANCODE_SPACE:
                        // Simulate left click
                        if (!space_was_pressed) {
                            mouse->buttons = 1;  // Left button down
                            space_was_pressed = true;
                        } else {
                            mouse->buttons = 0;  // Left button up
                            space_was_pressed = false;
                        }
                        break;
                        
                    case 0x1C:  // Enter key
                        // Simulate right click
                        mouse->buttons = 2;  // Right button
                        break;
                        
                    case SCANCODE_ESC:
                        gui_shutdown();
                        break;
                }
            }
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
