#include <stdint.h>
#include "printk.h"
#include "vga.h"

static void print_unsigned(unsigned int v, unsigned int base) {
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
                int n = va_arg(ap, int);
                if (n < 0) {
                    vga_putchar('-');
                    /* -(unsigned) avoid a depassment INT_MIN */
                    print_unsigned(-(unsigned int)n, 10);
                } else {
                    print_unsigned(n, 10);
                }
                break;
            }
            case 'u':
                print_unsigned(va_arg(ap, unsigned int), 10);
                break;
            case 'x':
                print_unsigned(va_arg(ap, unsigned int), 16);
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
}
