#include <stdint.h>
#include "vga.h"
#include "printk.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "tty.h"

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

    char line[64];
    for (;;) {
        printk("> ");
        tty_readline(line, sizeof(line));
        printk("tu as tape : %s\n", line);
    }

}
