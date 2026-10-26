#ifndef TESTS_H
#define TESTS_H

/* Each test_* clears the screen and prints its own results */
void test_string(void);
void test_printk(void);
void test_vga(void);

/* Endless output, never returns: not part of test_all() */
void test_vga_stress(void);

/* Pause so the screen can be read. Needs interrupts enabled (uses the timer) */
void test_wait(void);

/* string, printk, vga, one after the other */
void test_all(void);

#endif /* TESTS_H */
