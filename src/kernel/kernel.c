#include <stdint.h>
#include "vga.h"
#include "printk.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"

#ifdef RUN_TESTS
#include "tests.h"
#endif

void gdt_init(void);

void kmain(uint32_t magic, uint32_t info) {
    vga_init();
    vga_clear();
    gdt_init();
    idt_init();
    pic_init();
    timer_init(100);

    /* from here on, interrupts (timer, keyboard) are delivered */
    __asm__ volatile ("sti");

#ifdef RUN_TESTS
    test_all();
    vga_clear();
#endif

    printk("anoyau\n");
    printk("magic = %x\n", (unsigned int)magic);
    printk("info  = %p\n", (void *)info);

    vga_clear();

    for (;;) {
        char c = keyboard_get_char();
        if (c) {
            vga_putchar(c);
        }
    }
}
