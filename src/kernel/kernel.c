#include <stdint.h>
#include "vga.h"
#include "printk.h"

#ifdef RUN_TESTS
#include "tests.h"
#endif

void gdt_init(void);

void kmain(uint32_t magic, uint32_t info) {
    vga_init();
    vga_clear();

    gdt_init();

#ifdef RUN_TESTS
    test_all();
    vga_clear();
#endif

    printk("anoyau\n");
    printk("magic = %x\n", (unsigned int)magic);
    printk("info  = %p\n", (void *)info);
}
