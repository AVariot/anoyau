#include <stdint.h>
#include "printk.h"
#include "vga.h"
#include "isr_handler.h"
#include "irq.h"

static void print_unsigned(uint64_t v, unsigned int base) {
    static const char digits[] = "0123456789abcdef";
    char buf[32];
    char *p = buf + sizeof(buf);

    do {
        *--p = digits[v % base];
        v /= base;
    } while (v);

    while (p < buf + sizeof(buf)) {
        vga_putchar(*p++);
    }
}

void printk(const char *fmt, ...) {
    uint64_t flags = irq_save();
    va_list ap;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            vga_putchar(*fmt++);
            continue;
        }

        fmt++;
        if (*fmt == '\0') {
            break;
        }

        /* optional length modifier: %lx, %lu, %ld are 64 bits */
        int is_long = 0;
        if (*fmt == 'l') {
            is_long = 1;
            fmt++;
            if (*fmt == '\0') {
                break;
            }
        }

        switch (*fmt) {
            case 'c': {
                char c = (char)va_arg(ap, int);
                vga_putchar(c);
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char *);
                vga_puts(s ? s : "(null)");
                break;
            }
            case 'd': {
                long n = is_long ? va_arg(ap, long) : va_arg(ap, int);
                if (n < 0) {
                    vga_putchar('-');
                    /* -(unsigned) avoid a depassment INT_MIN / LONG_MIN */
                    print_unsigned(-(uint64_t)n, 10);
                } else {
                    print_unsigned(n, 10);
                }
                break;
            }
            case 'u':
                print_unsigned(is_long ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int), 10);
                break;
            case 'x':
                print_unsigned(is_long ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int), 16);
                break;
            case 'p': {
                void *ptr = va_arg(ap, void *);
                vga_puts("0x");
                print_unsigned((uintptr_t)ptr, 16);
                break;
            }
            case '%':
                vga_putchar('%');
                break;
            default:
                vga_putchar('%');
                vga_putchar(*fmt);
                break;
        }

        fmt++;
    }

    va_end(ap);
    irq_restore(flags);
}
