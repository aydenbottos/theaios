#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* Install flat code/data segments + TSS (entries 0–6) */
void gdt_init(void);

/* Install a Ring 3 call gate at GDT slot 7 targeting `target` */
void gdt_install_call_gate(uint32_t target_entry);

#endif /* GDT_H */
