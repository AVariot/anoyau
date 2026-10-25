
#ifndef KEYBOARD_H
#define KEYBOARD_H

// Keyboard = controler PS/2 port = 0x60
// data = 0x60 and status/cmd 0x64

#include "io.h"
#include <stdint.h>

void keyboard_handler(void);
int keyboard_get_scancode(void);
char keyboard_get_char(void);

#endif /* KEYBOARD_H */
