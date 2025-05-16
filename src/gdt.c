// src/gdt.c

#include "util.h"
#include <stdint.h>
#include "gdt.h"
#include "tss.h"

/* Standard GDT entry */
struct __attribute__((packed)) gdt_entry {
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran; // Includes limit_hi, flags (G, D/B, L, AVL)
    uint8_t  base_hi;
};

/* Call‑gate descriptor */
struct __attribute__((packed)) call_gate {
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t  param_count:5;
    uint8_t  reserved:3;
    uint8_t  access;
    uint16_t offset_hi;
};

/* LGDT pointer */
struct __attribute__((packed)) gdt_ptr {
    uint16_t limit;
    uint32_t base;
};

/* GDT slots 0..7 */
static struct gdt_entry gdt[8];
static struct call_gate  cg;
static struct gdt_ptr    gp;

/* Helper to set a flat code/data descriptor */
static void set_entry(int idx, uint32_t base, uint32_t limit,
                      uint8_t access, uint8_t gran_flags) {
    gdt[idx].limit_lo = (uint16_t)(limit & 0xFFFF);
    gdt[idx].base_lo  = (uint16_t)(base  & 0xFFFF);
    gdt[idx].base_mid = (uint8_t)((base >> 16) & 0xFF);
    gdt[idx].access   = access;
    // Combine limit_hi (bits 19:16) and granularity flags (G, D/B, L, AVL)
    gdt[idx].gran     = (uint8_t)(((limit >> 16) & 0x0F) | (gran_flags & 0xF0));
    gdt[idx].base_hi  = (uint8_t)((base >> 24) & 0xFF);
}

/* Called early in kernel_main() */
void gdt_init(void) {
    /* zero all entries */
    memset(gdt, 0, sizeof(gdt));

    /* 0: Null descriptor */
    // Already zeroed by memset

    /* 1: kernel code (0x08) */
    set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* 2: kernel data (0x10) */
    set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* 3: user code   (0x18) */
    set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    /* 4: user data   (0x20) */
    set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    /* 5: TSS descriptor (0x28) */
    extern struct tss_entry tss;
    extern uint32_t        tss_size; // Defined in tss.c as sizeof(tss)
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = tss_size - 1; // Limit is size - 1
    set_entry(5, base, limit, 0x89, 0x00); // TSS: byte granularity, reserved bits zero

    // GDT entry 6 is unused/available

    /* Load GDT */
    gp.limit = sizeof(gdt) - 1; // Limit is size - 1
    gp.base  = (uint32_t)&gdt;
    asm volatile("lgdt (%0)" :: "r"(&gp));

    /* Reload segments (AT&T syntax) to use the new GDT */
    asm volatile(
        "movw $0x10, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss\n\t"
        "ljmp $0x08, $1f\n\t"
        "1:\n\t"
        : : : "ax" // Clobbers AX register
    );

    /* After segment reload, remove premature TSS load here; will load later in kernel_main */
}

/* Install call gate at GDT index 7 (selector 0x38) */
void gdt_install_call_gate(uint32_t target_entry) {
    // Access=P=1, DPL=3, S=0, Type=12 (32-bit Call Gate)=0xEC
    cg.offset_lo   = target_entry & 0xFFFF;
    cg.selector    = 0x1B;      /* Target User Code Selector (RPL=3) */
    cg.param_count = 0;         /* No parameters copied */
    cg.reserved    = 0;
    cg.access      = 0xEC;
    cg.offset_hi   = (target_entry >> 16) & 0xFFFF;

    memcpy(&gdt[7], &cg, sizeof(cg));
}
