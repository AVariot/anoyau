#include <stdint.h>
#include "vga.h"
#include "printk.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "shell.h"

#ifdef RUN_TESTS
#include "tests.h"
#endif

void gdt_init(void);

void kmain(uint32_t magic, uint32_t info) {
    timer_init(100);
    vga_init();
    vga_clear();
    gdt_init();
    idt_init();
    pic_init();

    /* from here on, interrupts (timer, keyboard) are delivered */
    __asm__ volatile ("sti");

#ifdef RUN_TESTS
    test_all();
    vga_clear();
#endif

    printk("anoyau\n");
    printk("magic = %x\n", (unsigned int)magic);
    printk("info  = %p\n", (void *)info);
    sleep_ms(1000);

    vga_clear();
    shell();
}
