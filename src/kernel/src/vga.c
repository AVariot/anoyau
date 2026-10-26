
#include "vga.h"
#include "io.h"

static volatile uint16_t *vga_address = (volatile uint16_t *)VGA_MEMORY;
static int row = 0;
static int col = 0;
static uint8_t color = 0;

static void vga_cursor(void) {
    uint16_t pos = row * VGA_WIDTH + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);

}

void vga_init(void) {
    vga_setcolor(VGA_WHITE, VGA_BLACK);
}

void vga_clear(void) {
    for (int i = 0; i < (VGA_WIDTH * VGA_HEIGHT); i++) {
        vga_address[i] = ' ' | (color << 8);
    }
    row = 0;
    col = 0;

    vga_cursor();
}

void vga_setcolor(uint8_t fg, uint8_t bg) {
    color = fg | (bg << 4);
}

void vga_putchar(char c) {
    if (c == '\n') {
        row++;
        col = 0;
    } else if (c == '\b') {
        if (col == 0) {
            if (row == 0) {
                return;
            }
            row--;
            col = VGA_WIDTH - 1;
            while ((vga_address[(row * VGA_WIDTH) + col] & 0xFF) == ' ' && col != 0) {
                col--;
            }
        } else {
            col--;
        }
        vga_address[(row * VGA_WIDTH) + col] = ' ' | (color << 8);
    } else {
        // Cast because unsigned so can be negativ so fill of 1 in strng side
        vga_address[(row * VGA_WIDTH) + col] = (uint8_t)c | (color << 8);
        col++;
    }
    if (col == VGA_WIDTH) {
        col = 0;
        row++;
    }
    if (row == VGA_HEIGHT) {
        for (int i = 0; i < (VGA_WIDTH * (VGA_HEIGHT - 1)); i++) {
            vga_address[i] = vga_address[i + VGA_WIDTH];
        }
        for (int i = (VGA_WIDTH * (VGA_HEIGHT - 1)); i < (VGA_WIDTH * (VGA_HEIGHT)); i++) {
            vga_address[i] = ' ' | (color << 8);
            row = VGA_HEIGHT - 1;
        }
    }
    vga_cursor();
}

void vga_puts(const char *s) {
    while (*s) {
        vga_putchar(*s++);
    }
}
