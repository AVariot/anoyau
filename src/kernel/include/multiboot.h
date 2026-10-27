
#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>
#include <stddef.h>

#define MULTIBOOT_MAGIC 0x2BADB002
#define MULTIBOOT_INFO_MMAP (1 << 6)
#define MULTIBOOT_MEMORY_AVAILABLE 1

struct multiboot_info_s {
    uint32_t flags;
    uint32_t mem_lower; // lower memory (kb)
    uint32_t mem_upper; // lower memory (kb)
    uint32_t boot_device; // boot device id
    uint32_t cmdline; // address of kernel command line
    uint32_t mods_count; // number of module loaded
    uint32_t mods_addr; // address of first module structure
    
    uint32_t syms[4]; // symbol table information (not used)
    
    uint32_t mmap_length;  // memory map length
    uint32_t mmap_addr;    // memory map address
} __attribute__((packed));

struct multiboot_mmap_entry_s {
    uint32_t size;
    uint64_t base;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

_Static_assert(offsetof(struct multiboot_info_s, mmap_length) == 44, "mmap_length is at offset 44");
_Static_assert(offsetof(struct multiboot_info_s, mmap_addr) == 48, "mmap_addr is at offset 48");
_Static_assert(sizeof(struct multiboot_mmap_entry_s) == 24, "a mmap entry is 24 bytes");

void multiboot_print_mmap(uint32_t magic, uint32_t info);

#endif /* MULTIBOOT_H */
