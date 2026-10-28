#include <stdint.h>
#include "tests.h"
#include "timer.h"

void test_wait(void) {
    sleep_ms(2000);
}

void test_all(void) {
    test_string();
    test_wait();

    test_printk();
    test_wait();

    test_vga();
    test_wait();

    test_kheap();
    test_wait();
}
