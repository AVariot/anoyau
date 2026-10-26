
#include "tty.h"
#include "keyboard.h"
#include "vga.h"

int tty_readline(char *buf, int max) {
    int len = 0;

    for (;;) {
        char c = keyboard_get_char();

        if (c == 0) {
            __asm__ volatile ("hlt");
        } else {
            if (c == '\b' && len > 0) {
                vga_putchar(c);
                len--;
            } else if (c == '\n') {
                vga_putchar(c);
                buf[len] = '\0';
                return len;
            } else {
                if (len < max - 1 && (c >= 32 && c <= 126)) {
                    vga_putchar(c);
                    buf[len] = c;
                    len++;
                }
                // TODO: pensez a une gestion d'erreur
            }
        }
    }
}
