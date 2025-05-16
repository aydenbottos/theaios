#include "util.h"
#include "serial.h"
#include "ata.h"
#include "fs.h"
#include "paging.h"
#include "pmm.h"
#include "kheap.h"
#include "idt.h"
#include "pit.h"
#include "task.h"
#include "shell.h"
#include "irq.h"
#include "keyboard.h"
#include "gdt.h"
#include "tss.h"

void kernel_main(void) {
    gdt_init();
    tss_init();
    idt_init();
    clear_screen();
    serial_init();

    ata_init();      // initialize PIO interface
    fs_init();       // read BPB & compute root/data offsets

    paging_init();   // turn on paging
    pmm_init();      // init physical memory manager
    kheap_init();    // init kernel heap

    pit_init();
    irq_install();
        
    puts("My-OS ready (FS mounted)\n");
    shell_init();

    for (;;) asm volatile("hlt");
}
