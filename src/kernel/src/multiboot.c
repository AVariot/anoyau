
#include "multiboot.h"
#include "printk.h"

// info is the address of GRUB structure == usually 0x10000
void multiboot_print_mmap(uint32_t magic, uint32_t info) {
    if (magic != MULTIBOOT_MAGIC) {
        printk("Not booted by a multiboot loader\n");
        return;
    }

    const struct multiboot_info_s *mbi = (const struct multiboot_info_s *)(uintptr_t)info;

    if (!(mbi->flags & MULTIBOOT_INFO_MMAP)) {
        printk("No memory map\n");
        return;
    }

    uintptr_t addr = mbi->mmap_addr;
    uintptr_t end = addr + mbi->mmap_length;
    uint64_t total = 0;

    while (addr < end) {
        const struct multiboot_mmap_entry_s *e = (const struct multiboot_mmap_entry_s *)addr;
        printk("base=%lx  length=%lx  type=%u\n", e->base, e->length, e->type);
        if (e->type == MULTIBOOT_MEMORY_AVAILABLE) {
            total += e->length;
        }
        addr += e->size + 4;
    }

    printk("usable: %lu KiB\n", total >> 10);
}
