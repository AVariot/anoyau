#include <stdint.h>
#include "tests.h"

void test_wait(void) {
    for (volatile uint32_t i = 0; i < 500000000; i++) {}
}

void test_all(void) {
    test_string();
    test_wait();

    test_printk();
    test_wait();

    test_vga();
}
