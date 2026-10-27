
#include "pmm.h"
#include "multiboot.h"
#include "string.h"
#include "printk.h"

/*
    PMM = Physical Memory Manager
    The kernel needs memory so he needs to know wich space is free and reserv one
*/

#define MEMORY_SIZE (1UL << 30)                    // 1 GiB : ce que le boot mappe
#define MAX_FRAMES  (MEMORY_SIZE / FRAME_SIZE)     // 262144

extern char kernel_start[];
extern char kernel_end[];

static uint8_t bitmap[MAX_FRAMES / 8];

// 2 >> 3 = 2^3 = 8 = 1 octet

static void bitmap_set(size_t n) {
    bitmap[n >> 3] |= (1 << (n & 7));
}

static void bitmap_clear(size_t n) {
    bitmap[n >> 3] &= ~(1 << (n & 7));
}

static int bitmap_test(size_t n) {
    return (bitmap[n >> 3] & (1 << (n & 7))) != 0;
}

void pmm_init(uint32_t magic, uint32_t info) {
    memset(bitmap, 0xFF, sizeof(bitmap));

    if (magic != MULTIBOOT_MAGIC) return;
    const struct multiboot_info_s *mbi = (const struct multiboot_info_s *)(uintptr_t)info;
    if (!(mbi->flags & MULTIBOOT_INFO_MMAP)) return;

    uintptr_t addr = mbi->mmap_addr;
    uintptr_t end  = addr + mbi->mmap_length;

    while (addr < end) {
        const struct multiboot_mmap_entry_s *e = (const struct multiboot_mmap_entry_s *)addr;

        if (e->type == MULTIBOOT_MEMORY_AVAILABLE) {
            uint64_t start = (e->base + FRAME_SIZE - 1) & ~(uint64_t)(FRAME_SIZE - 1);
            uint64_t stop  = (e->base + e->length)      & ~(uint64_t)(FRAME_SIZE - 1);

            for (uint64_t a = start; a < stop; a += FRAME_SIZE) {
                if (a < 0x100000 ||
                    (a >= (uintptr_t)kernel_start && a < (uintptr_t)kernel_end) ||
                    a >= MEMORY_SIZE) continue;

                bitmap_clear(a / FRAME_SIZE);
            }
        }
        addr += e->size + 4;
    }
}

uint64_t pmm_alloc_frame(void) {
    for (size_t n = 0; n < MAX_FRAMES; n++) {
        if (!bitmap_test(n)) {
            bitmap_set(n);
            return (uint64_t)n * FRAME_SIZE; 
        }
    }

    return 0;
}

void pmm_free_frame(uint64_t addr) {
    if (addr % FRAME_SIZE != 0) {
        printk("Error pmm free frame: not align\n");
        return;
    }
    size_t n = addr / FRAME_SIZE;
    if (n >=  MAX_FRAMES) {
        printk("Error pmm free frame: out of frame\n");
        return;
    }
    if (bitmap_test(n) == 0) {
        printk("Error pmm free frame: already free\n");
        return;
    }
    bitmap_clear(n);
}

size_t pmm_free_count(void) {
    size_t count = 0;

    for (size_t n = 0; n < MAX_FRAMES; n++) {
        if (!bitmap_test(n)) {
            count++;
        }
    }

    return count;
}
