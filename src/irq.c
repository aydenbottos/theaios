// src/irq.c

#include <stdint.h>
#include "util.h"     // inb(), outb()
#include "idt.h"      // idt_init(), idt_flush()
#include "pit.h"      // pit_handler()
#include "task.h"     // schedule()
#include "keyboard.h" // keyboard_handler()

/* 8259 ports */
#define PIC1_CMD   0x20
#define PIC1_DATA  0x21
#define PIC2_CMD   0xA0
#define PIC2_DATA  0xA1

/* ICW and OCW bits */
#define ICW1_INIT  0x11
#define ICW4_8086  0x01

// A 100 ns plus delay: out to port 0x80 is guaranteed ~150 ns on PC.
static inline void io_wait(void) {
    asm volatile("outb %%al, $0x80" : : "a"(0));
}

void irq_install(void) {

    // remap PIC
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);

    // mask all but timer (IRQ0) and keyboard (IRQ1)
    outb(0x21, 0xFC);
    outb(0xA1, 0xFF);

    // enable PS/2 keyboard port
    while (inb(0x64) & 0x02) { }
    outb(0x64, 0xAE);
    (void)inb(0x60);

    asm volatile("sti");  // finally turn on interrupts
}

void irq_handler(regs_t *r) {
    // acknowledge PIC
    if (r->int_no >= 0x28) outb(0xA0, 0x20);
    outb(0x20, 0x20);

    if (r->int_no == 0x20) {
        pit_handler(r);
        schedule();
    }
    else if (r->int_no == 0x21) {
        keyboard_handler(r);
    }
}
