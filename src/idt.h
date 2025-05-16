#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/*
 * Set the IDT entry `num` to point at `base` with selector `sel`
 * and flags byte `flags` (e.g. 0x8E for present, ring 0, 32‑bit interrupt gate).
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

/* Initialize the IDT (zero all entries, install exception stubs) and load it */
void idt_init(void);

#endif /* IDT_H */
