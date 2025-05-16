#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

#define EI_NIDENT 16

/* ELF32 file header */
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint32_t      e_entry;     /* entry point */
    uint32_t      e_phoff;     /* program header table offset */
    uint32_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;     /* number of program headers */
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} Elf32_Ehdr;

/* ELF32 program header */
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;  /* offset in file image */
    uint32_t p_vaddr;   /* virtual address in memory */
    uint32_t p_paddr;
    uint32_t p_filesz;  /* bytes in file image */
    uint32_t p_memsz;   /* bytes in memory (BSS) */
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

/*
 * Copy all PT_LOAD segments of the provided 32-bit ELF image to the
 * virtual addresses they were linked for (the kernel uses an identity
 * mapping, so this is a straight memcpy).
 *
 * On success the function returns the program's entry point (e_entry),
 * allowing the caller to spawn a user-mode task at that address.  A
 * negative value is returned on error (e.g. invalid ELF magic).
 */
int load_elf(const void *elf_image);

#endif /* ELF_H */
