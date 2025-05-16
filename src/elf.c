// src/elf.c
#include "elf.h"
#include "paging.h"
#include "gdt.h"
#include "util.h"
#include "fs.h"    // if you need to read the ELF from disk

#define USER_STACK_SIZE 0x1000

int load_elf(const void *elf_image) {
    const Elf32_Ehdr *eh = (const Elf32_Ehdr*)elf_image;
    /* 1) Validate ELF magic */
    if (memcmp(eh->e_ident, "\x7F""ELF", 4) != 0) {
        return -1;
    }

    /* 2) (Re-)enable identity paging so ring 3 can execute.
     *    The kernel already turned paging on during early boot, but the
     *    helper remains useful when load_elf() is called before that
     *    or after the paging tables have been modified. It is therefore
     *    kept – re-initialising the page directory is idempotent for the
     *    simple 1:1, user-accessible mapping that paging_init() installs.
     */
    paging_init();

    /* 3) Copy each PT_LOAD to its p_vaddr */
    const Elf32_Phdr *ph = (const Elf32_Phdr*)
        ((const uint8_t*)elf_image + eh->e_phoff);
    for (int i = 0; i < eh->e_phnum; i++, ph++) {
        if (ph->p_type != 1) continue;  /* only PT_LOAD */

        void *dst = (void*)(uintptr_t)ph->p_vaddr;
        memcpy(dst, (const uint8_t*)elf_image + ph->p_offset,
               ph->p_filesz);
        /* zero BSS */
        if (ph->p_memsz > ph->p_filesz) {
            memset((uint8_t*)dst + ph->p_filesz, 0,
                   ph->p_memsz - ph->p_filesz);
        }
    }

    /* 4) All segments are now in place.  Simply return the ELF's entry
     *    point so that the caller (typically the shell) can create a
     *    task and let the scheduler switch into user mode at its own
     *    convenience.  We *must not* perform the privilege transition
     *    here – doing so skips the scheduler and leaves kernel data
     *    structures (e.g. task list, kernel stack pointers) in an
     *    inconsistent state, which quickly leads to a General-Protection
     *    Fault when the first interrupt arrives.
     */

    return eh->e_entry;
}
