#include <stdint.h>
#include "vga.h"
#include "printk.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "shell.h"
#include "multiboot.h"
#include "pmm.h"

#ifdef RUN_TESTS
#include "tests.h"
#endif

void gdt_init(void);

extern char kernel_start[];
extern char kernel_end[];

void kmain(uint32_t magic, uint32_t info) {
    vga_init();
    vga_clear();
    gdt_init();
    idt_init();
    pic_init();
    timer_init(100);
    pmm_init(magic, info);
    printk("pmm: %lu frames libres (%lu KiB)\n", pmm_free_count(), pmm_free_count() * 4);
    uint64_t a = pmm_alloc_frame();
    uint64_t b = pmm_alloc_frame();
    printk("alloc: %lx %lx (libres: %lu)\n", a, b, pmm_free_count());
    pmm_free_frame(a);
    uint64_t c = pmm_alloc_frame();
    printk("apres free+alloc: %lx (doit etre = %lx)\n", c, a);

    /* from here on, interrupts (timer, keyboard) are delivered */
    __asm__ volatile ("sti");

#ifdef RUN_TESTS
    test_all();
    vga_clear();
#endif

    multiboot_print_mmap(magic, info);
    printk("kernel: %p - %p (%lu KiB)\n", kernel_start, kernel_end,
       ((uintptr_t)kernel_end - (uintptr_t)kernel_start) >> 10);

    printk("anoyau 64 bits\n");
    printk("magic = %x\n", magic);
    printk("info  = %p\n", (void *)(uintptr_t)info);
    sleep_ms(1000);

    vga_clear();
    shell();
}
