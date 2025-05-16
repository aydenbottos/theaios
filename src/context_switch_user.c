// src/context_switch_user.c
#include <stdint.h>
#include "gdt.h"

/*
 * Perform a far call into Ring 3 via the call‑gate we installed
 * in GDT slot 7 (selector 0x38).
 *
 * entry_point: linear address of the user entry (e.g. ELF e_entry)
 * user_stack : top of user‑mode stack (below 4 MiB identity map)
 */
void context_switch_user(uint32_t entry_point, uint32_t user_stack) {
    /* Switch to user mode via IRET: set data segments and push iret frame */
    /* switch to user mode via IRET; user_stack in %%ebx, entry_point in %%ecx */
    asm volatile(
        "cli\n\t"
        "movw $0x23, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        /* SS (user data selector) */
        "pushl $0x23\n\t"
        /* ESP (user stack pointer) */
        "pushl %%ebx\n\t"
        /* EFLAGS with IF=1 */
        "pushfl\n\t"
        "popl %%eax\n\t"
        "orl $0x200, %%eax\n\t"
        "pushl %%eax\n\t"
        /* CS (user code selector) */
        "pushl $0x1B\n\t"
        /* EIP */
        "pushl %%ecx\n\t"
        "iret\n\t"
        :
        : "b"(user_stack), "c"(entry_point)
        : "eax", "memory"
    );
}
