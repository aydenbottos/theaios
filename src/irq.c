// src/irq.c

#include <stdint.h>
#include "util.h"     // inb(), outb()
#include "idt.h"      // idt_init(), idt_flush()
#include "pit.h"      // pit_handler()
#include "task.h"     // schedule()
#include "keyboard.h" // keyboard_handler()
#include "irq.h"

/* 8259 ports */
#define PIC1_CMD   0x20
#define PIC1_DATA  0x21
#define PIC2_CMD   0xA0
#define PIC2_DATA  0xA1

/* ICW and OCW bits */
#define ICW1_INIT  0x11
#define ICW4_8086  0x01

// IRQ handlers array
static irq_handler_t irq_handlers[16] = {0};

// A 100 ns plus delay: out to port 0x80 is guaranteed ~150 ns on PC.
static inline void io_wait(void) {
    asm volatile("outb %%al, $0x80" : : "a"(0));
}

void irq_install(void) {
    // remap PIC
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);

    // mask all but timer (IRQ0), keyboard (IRQ1), and PS/2 mouse (IRQ12)
    outb(0x21, 0xF8);  // Enable IRQ0, IRQ1, IRQ2 (cascade)
    outb(0xA1, 0xEF);  // Enable IRQ12 (PS/2 mouse)

    // enable PS/2 keyboard port
    while (inb(0x64) & 0x02) { }
    outb(0x64, 0xAE);
    (void)inb(0x60);

    asm volatile("sti");  // finally turn on interrupts
}

void irq_install_handler(int irq, irq_handler_t handler) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = handler;
    }
}

void irq_uninstall_handler(int irq) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = NULL;
    }
}

void irq_handler(regs_t *r) {
    // acknowledge PIC
    if (r->int_no >= 0x28) outb(0xA0, 0x20);
    outb(0x20, 0x20);

    // Handle IRQ based on interrupt number
    int irq = r->int_no - 0x20;
    
    if (irq == 0) {
        pit_handler(r);
        schedule();
    }
    else if (irq == 1) {
        keyboard_handler(r);
    }
    else if (irq >= 0 && irq < 16 && irq_handlers[irq]) {
        // Call custom handler if installed
        irq_handlers[irq]();
    }
}
