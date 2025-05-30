#include "mouse.h"
#include "util.h"
#include "vga_graphics.h"
#include "irq.h"
#define MOUSE_DEBUG 1
#include "serial.h"

// PS/2 mouse ports
#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_CMD    0x64

// PS/2 mouse commands
#define MOUSE_CMD_ENABLE_AUX     0xA8
#define MOUSE_CMD_GET_COMPAQ    0x20
#define MOUSE_CMD_SET_COMPAQ    0x60
#define MOUSE_CMD_WRITE_MOUSE   0xD4
#define MOUSE_SET_DEFAULTS      0xF6
#define MOUSE_ENABLE_STREAMING  0xF4
#define MOUSE_SET_SAMPLE_RATE   0xF3
#define MOUSE_GET_DEVICE_ID     0xF2

// Mouse state
static mouse_state_t mouse_state = {
    .x = VGA_WIDTH / 2,
    .y = VGA_HEIGHT / 2,
    .dx = 0,
    .dy = 0,
    .buttons = 0,
    .visible = true
};

// Mouse packet buffer
static uint8_t mouse_cycle = 0;
static int8_t mouse_packet[3];

// Previous button states for click detection
static uint8_t prev_buttons = 0;

// Debug counters
static uint32_t interrupt_count = 0;
static uint32_t packet_count = 0;

// Helper to print number via serial
static void serial_putnum(int num) {
    char buf[12];
    itoa(num, buf, 10);
    serial_puts(buf);
}

/* -------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------*/

static void process_packet(void) {
    /* Translate 3-byte PS/2 packet into our mouse_state structure. */
    prev_buttons = mouse_state.buttons;
    mouse_state.buttons = mouse_packet[0] & 0x07;

    mouse_state.dx = (int8_t)mouse_packet[1];
    mouse_state.dy = -(int8_t)mouse_packet[2];   /* Y is negative up */

    mouse_state.x += mouse_state.dx;
    mouse_state.y += mouse_state.dy;

    /* Clamp to screen bounds */
    if (mouse_state.x < 0) mouse_state.x = 0;
    if (mouse_state.x >= VGA_WIDTH)  mouse_state.x = VGA_WIDTH - 1;
    if (mouse_state.y < 0) mouse_state.y = 0;
    if (mouse_state.y >= VGA_HEIGHT) mouse_state.y = VGA_HEIGHT - 1;
    
    packet_count++;
#ifdef MOUSE_DEBUG
    // Log packet info every 10 packets
    if (packet_count % 10 == 0) {
        serial_puts("\nPkt#");
        serial_putnum(packet_count);
        serial_puts(" dx=");
        serial_putnum(mouse_state.dx);
        serial_puts(" dy=");
        serial_putnum(mouse_state.dy);
        serial_puts(" pos=(");
        serial_putnum(mouse_state.x);
        serial_puts(",");
        serial_putnum(mouse_state.y);
        serial_puts(")\n");
    }
#endif
}

// Saved pixels under cursor
static uint8_t cursor_saved[CURSOR_WIDTH * CURSOR_HEIGHT];
static int cursor_saved_x = -1;
static int cursor_saved_y = -1;

// Mouse cursor bitmap (arrow pointer)
static const uint8_t cursor_bitmap[CURSOR_HEIGHT][CURSOR_WIDTH] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,1,1,1,1,0,0,0},
    {1,2,1,2,2,1,0,0,0,0,0,0},
    {1,1,0,1,1,1,0,0,0,0,0,0},
    {1,0,0,0,1,1,0,0,0,0,0,0}
};

// Wait for PS/2 controller
static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(MOUSE_STATUS) & 1) == 1) {
                return;
            }
        }
        return;
    } else {
        while (timeout--) {
            if ((inb(MOUSE_STATUS) & 2) == 0) {
                return;
            }
        }
        return;
    }
}

// Write to mouse
static void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(MOUSE_CMD, MOUSE_CMD_WRITE_MOUSE);
    mouse_wait(1);
    outb(MOUSE_PORT, data);
}

// Read from mouse
static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(MOUSE_PORT);
}

// Initialize PS/2 mouse
void mouse_init(void) {
    uint8_t status;
    uint8_t ack;

#ifdef MOUSE_DEBUG
    serial_puts("\nMouse init starting...\n");
#endif

    /* Flush any pending bytes from previous operations */
    while (inb(MOUSE_STATUS) & 1) {
        (void)inb(MOUSE_PORT);
    }
    
    // Enable auxiliary device (mouse port)
    mouse_wait(1);
    outb(MOUSE_CMD, MOUSE_CMD_ENABLE_AUX);
    
    // Enable interrupts
    mouse_wait(1);
    outb(MOUSE_CMD, MOUSE_CMD_GET_COMPAQ);
    mouse_wait(0);
    status = inb(MOUSE_PORT);
#ifdef MOUSE_DEBUG
    serial_puts("Initial compaq byte: 0x");
    char buf[12];
    utohex(status, buf);
    serial_puts(buf);
    serial_puts("\n");
#endif
    
    status |= 0x02;      /* enable IRQ12 */
    status &= ~(1 << 5); /* enable aux clock */
    
    mouse_wait(1);
    outb(MOUSE_CMD, MOUSE_CMD_SET_COMPAQ);
    mouse_wait(1);
    outb(MOUSE_PORT, status);

    // Re-enable auxiliary device
    mouse_wait(1);
    outb(MOUSE_CMD, MOUSE_CMD_ENABLE_AUX);
    
    // Reset mouse
    mouse_write(0xFF);  // Reset command
    ack = mouse_read(); // ACK
    if (ack == 0xFA) {
        mouse_read();   // 0xAA (self-test passed)
        mouse_read();   // 0x00 (mouse ID)
#ifdef MOUSE_DEBUG
        serial_puts("Mouse reset successful\n");
#endif
    }
    
    // Set defaults
    mouse_write(MOUSE_SET_DEFAULTS);
    ack = mouse_read();
#ifdef MOUSE_DEBUG
    serial_puts("Set defaults ACK: 0x");
    utohex(ack, buf);
    serial_puts(buf);
    serial_puts("\n");
#endif
    
    // Set sample rate to 100 Hz
    mouse_write(MOUSE_SET_SAMPLE_RATE);
    mouse_read();  // ACK
    mouse_write(100);
    mouse_read();  // ACK
    
    // Set resolution to 4 counts/mm
    mouse_write(0xE8);  // Set resolution
    mouse_read();       // ACK
    mouse_write(2);     // 4 counts/mm
    mouse_read();       // ACK
    
    // Enable data reporting
    mouse_write(MOUSE_ENABLE_STREAMING);
    ack = mouse_read();
#ifdef MOUSE_DEBUG
    serial_puts("Enable streaming ACK: 0x");
    utohex(ack, buf);
    serial_puts(buf);
    serial_puts("\n");
#endif

    // Get device ID to verify mouse is responding
    mouse_write(MOUSE_GET_DEVICE_ID);
    mouse_read();  // ACK
    uint8_t id = mouse_read();
#ifdef MOUSE_DEBUG
    serial_puts("Mouse ID: 0x");
    utohex(id, buf);
    serial_puts(buf);
    serial_puts("\n");
    serial_puts("Mouse init complete!\n");
#endif
}

// Mouse interrupt handler
void mouse_handler(void) {
    uint8_t data = inb(MOUSE_PORT);
    
    interrupt_count++;
    
#ifdef MOUSE_DEBUG
    if (interrupt_count <= 20) {  // Log first 20 interrupts
        serial_puts("\nIRQ12 #");
        serial_putnum(interrupt_count);
        serial_puts(" data=0x");
        char buf[12];
        utohex(data, buf);
        serial_puts(buf);
        serial_puts(" cycle=");
        serial_putnum(mouse_cycle);
    }
#endif

    switch (mouse_cycle) {
        case 0:
            mouse_packet[0] = data;
            if (!(data & 0x08)) {
#ifdef MOUSE_DEBUG
                serial_puts(" SYNC ERR!");
#endif
                // Try to resync
                mouse_cycle = 0;
                return;
            }
            mouse_cycle++;
            break;
            
        case 1:
            mouse_packet[1] = data;
            mouse_cycle++;
            break;
            
        case 2:
            mouse_packet[2] = data;
            mouse_cycle = 0;
            process_packet();
            break;
    }
}

/* -------------------------------------------------------------------
 * Polling fallback — called from the GUI main loop when no IRQs are
 * received (e.g. when running under a virtual machine whose PS/2 model
 * is disabled).  We reuse the same state-machine that assembles the
 * 3-byte packets, but driven by reads from the data port when the
 * output buffer is full.
 * -------------------------------------------------------------------*/
void mouse_poll(void) {
    static uint32_t poll_count = 0;
    
    while (inb(MOUSE_STATUS) & 1) {
        uint8_t data = inb(MOUSE_PORT);
        
        poll_count++;
#ifdef MOUSE_DEBUG
        if (poll_count <= 20) {  // Log first 20 polls
            serial_puts("\nPoll #");
            serial_putnum(poll_count);
            serial_puts(" data=0x");
            char buf[12];
            utohex(data, buf);
            serial_puts(buf);
        }
#endif

        switch (mouse_cycle) {
            case 0:
                mouse_packet[0] = data;
                if (!(data & 0x08)) {
                    mouse_cycle = 0;  // Reset on sync error
                    return;
                }
                mouse_cycle++;
                break;
            case 1:
                mouse_packet[1] = data;
                mouse_cycle++;
                break;
            case 2:
                mouse_packet[2] = data;
                mouse_cycle = 0;
                process_packet();
                break;
        }
    }
}

// Get mouse state
mouse_state_t* mouse_get_state(void) {
    return &mouse_state;
}

// Show mouse cursor
void mouse_show_cursor(void) {
    mouse_state.visible = true;
}

// Hide mouse cursor
void mouse_hide_cursor(void) {
    mouse_state.visible = false;
    
    // Restore saved pixels
    if (cursor_saved_x >= 0 && cursor_saved_y >= 0) {
        int idx = 0;
        for (int y = 0; y < CURSOR_HEIGHT; y++) {
            for (int x = 0; x < CURSOR_WIDTH; x++) {
                int px = cursor_saved_x + x;
                int py = cursor_saved_y + y;
                if (px >= 0 && px < VGA_WIDTH && py >= 0 && py < VGA_HEIGHT) {
                    vga_set_pixel_direct(px, py, cursor_saved[idx]);
                }
                idx++;
            }
        }
    }
}

// Update mouse cursor on screen
void mouse_update_cursor(void) {
    if (!mouse_state.visible) return;
    
    // Restore old cursor position
    if (cursor_saved_x >= 0 && cursor_saved_y >= 0) {
        int idx = 0;
        for (int y = 0; y < CURSOR_HEIGHT; y++) {
            for (int x = 0; x < CURSOR_WIDTH; x++) {
                int px = cursor_saved_x + x;
                int py = cursor_saved_y + y;
                if (px >= 0 && px < VGA_WIDTH && py >= 0 && py < VGA_HEIGHT) {
                    vga_set_pixel_direct(px, py, cursor_saved[idx]);
                }
                idx++;
            }
        }
    }
    
    // Save pixels under new cursor position
    cursor_saved_x = mouse_state.x;
    cursor_saved_y = mouse_state.y;
    int idx = 0;
    for (int y = 0; y < CURSOR_HEIGHT; y++) {
        for (int x = 0; x < CURSOR_WIDTH; x++) {
            int px = cursor_saved_x + x;
            int py = cursor_saved_y + y;
            if (px >= 0 && px < VGA_WIDTH && py >= 0 && py < VGA_HEIGHT) {
                cursor_saved[idx] = vga_get_pixel_direct(px, py);
            }
            idx++;
        }
    }
    
    // Draw cursor
    for (int y = 0; y < CURSOR_HEIGHT; y++) {
        for (int x = 0; x < CURSOR_WIDTH; x++) {
            int px = mouse_state.x + x;
            int py = mouse_state.y + y;
            if (px >= 0 && px < VGA_WIDTH && py >= 0 && py < VGA_HEIGHT) {
                uint8_t pixel = cursor_bitmap[y][x];
                if (pixel == 1) {
                    vga_set_pixel_direct(px, py, COLOR_BLACK);
                } else if (pixel == 2) {
                    vga_set_pixel_direct(px, py, COLOR_WHITE);
                }
            }
        }
    }
}

// Check for left click
bool mouse_left_clicked(void) {
    return (mouse_state.buttons & 0x01) && !(prev_buttons & 0x01);
}

// Check for right click
bool mouse_right_clicked(void) {
    return (mouse_state.buttons & 0x02) && !(prev_buttons & 0x02);
}

// Check if mouse is in rectangle
bool mouse_in_rect(int x, int y, int width, int height) {
    return (mouse_state.x >= x && mouse_state.x < x + width &&
            mouse_state.y >= y && mouse_state.y < y + height);
}
