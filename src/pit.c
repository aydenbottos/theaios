#include "pit.h"
#include "interrupts.h"
#include "idt.h"
#include "util.h"

#define PIT_FREQ       1193182u
#define PIT_TARGET_HZ  100u
#define PIT_DIVISOR    (PIT_FREQ / PIT_TARGET_HZ)

static volatile uint32_t ticks = 0;

// IRQ0 stub needs to be declared extern in your irq_table
extern void irq0();

void pit_handler(regs_t *r) {
    (void)r;
    ticks++;
}

void pit_init(void) {
    // register our handler for IRQ0 (index 32)
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    // set divisor
    outb(0x43, 0x36);
    outb(0x40, PIT_DIVISOR & 0xFF);
    outb(0x40, PIT_DIVISOR >> 8);
    // unmask IRQ0
    uint8_t mask = inb(0x21) & ~0x01;
    outb(0x21, mask);
}

uint32_t pit_get_ticks(void) {
    return ticks;
}
