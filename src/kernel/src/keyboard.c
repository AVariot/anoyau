
#include "keyboard.h"

static volatile uint8_t last_scancode = 0;
static volatile uint8_t new_key = 0;
static const char scancode_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ',
};


void keyboard_handler(void) {
    last_scancode = inb(0x60);
    new_key = 1;
}

int keyboard_get_scancode(void) {
    if (!new_key) {
        return -1;
    }
    new_key = 0;
    return last_scancode;
}

char keyboard_get_char(void) {
    int key = keyboard_get_scancode();

    if (key == -1 || key & 0x80) {
        return 0;
    }

    return scancode_to_ascii[key];
}
