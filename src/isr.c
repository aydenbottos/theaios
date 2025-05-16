#include <stdint.h>
#include "interrupts.h"
#include "util.h"

/* Human‑readable names for CPU exceptions 0–31 */
static const char *exception_msg[] = {
    "Divide‑by‑zero",
    "Debug",
    "Non‑maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound‑range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 floating‑point",
    "Alignment check",
    "Machine check",
    "SIMD floating‑point",
    "Virtualisation",
    "Control‑protection",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security exception",
    "Reserved",
    "Triple fault"
};

void isr_handler(regs_t *r) {
    puts("\n*** CPU Exception ");
    char num[4]; itoa(r->int_no, num, 10);
    puts(num);
    puts(": ");
    puts(exception_msg[r->int_no]);
    puts(" ***\n");
    while (1) asm volatile("hlt");
}
