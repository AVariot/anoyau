#include <stdint.h>
#include "printk.h"
#include "vga.h"
#include "tests.h"

/* The odd cases below (%q, NULL, trailing %) are on purpose */
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-overflow"

void test_printk(void) {
    vga_clear();
    vga_puts("== test printk ==\n");

    printk("%d %d %d %d\n", 0, 123, -42, -2147483647 - 1);
    printk("%u %x %x %x\n", 4000000000u, 0, 255, 0xFFFFFFFFu);
    printk("%c%c %s|%s| 100%% %q\n", 'O', 'K', "chaine", (char *)0);
    printk("%p %p\n", (void *)0x10000, (void *)0);
    printk("fin avec pourcent: 100%");
    printk("\n");
}
