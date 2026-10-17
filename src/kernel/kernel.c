#include <stdint.h>
#include "vga.h"

static void wait(void) {
    for (volatile uint32_t i = 0; i < 500000000; i++) {}
}

static void phase(const char *title) {
    vga_clear();
    vga_puts(title);
    vga_puts("\n");
}

void kmain(uint32_t magic, uint32_t info) {
    (void)magic;
    (void)info;

    vga_init();

    /* 1. Caractère simple + \n */
    phase("[1] chars et newline");
    vga_puts("abc\n");
    vga_puts("def\n\n");
    vga_puts("ligne apres une ligne vide\n");
    wait();

    /* 2. Retour automatique : exactement 80, puis 81 caracteres */
    phase("[2] wrap auto (80 colonnes)");
    for (int i = 0; i < 80; i++) vga_putchar('=');
    vga_puts("<-- apres 80 '=', cette phrase commence en colonne 0\n");
    for (int i = 0; i < 81; i++) vga_putchar('#');
    vga_puts("\n");
    wait();

    /* 3. Couleurs */
    phase("[3] couleurs");
    for (int c = 1; c < 16; c++) {
        vga_setcolor(c, VGA_BLACK);
        vga_puts("texte ");
    }
    vga_setcolor(VGA_BLACK, VGA_LIGHT_GRAY);
    vga_puts("\nnoir sur gris");
    vga_setcolor(VGA_WHITE, VGA_BLUE);
    vga_puts("\nblanc sur bleu\n");
    vga_setcolor(VGA_WHITE, VGA_BLACK);
    wait();

    /* 4. Caracteres >= 128 : la couleur ne doit pas etre corrompue */
    phase("[4] chars >= 128 (cast uint8_t)");
    vga_putchar((char)0xB0);
    vga_putchar((char)0xDB);
    vga_putchar((char)0xE0);
    vga_puts(" <-- blocs, couleur blanche intacte\n");
    wait();

    /* 5. Defilement par newline : 40 lignes numerotees */
    phase("[5] defilement par \\n (40 lignes)");
    for (int i = 0; i < 40; i++) {
        vga_putchar('0' + (i / 10));
        vga_putchar('0' + (i % 10));
        vga_puts(" ligne\n");
    }
    vga_puts("fin: la ligne 39 doit etre la derniere visible\n");
    wait();

    /* 6. Defilement par wrap (sans \n) */
    phase("[6] defilement par wrap");
    for (int i = 0; i < 30 * 80; i++) {
        vga_putchar('A' + (i / 80) % 26);
    }
    wait();

    /* 7. Bords : \n sur la derniere ligne */
    phase("[7] bords");
    for (int i = 0; i < 22; i++) vga_puts("\n");
    vga_puts("derniere ligne\n");
    vga_puts("apres \\n sur la derniere ligne\n");
    wait();

    /* 8. Stress : jamais de crash ni d'ecriture hors ecran */
    phase("[8] stress infini");
    for (uint32_t n = 0;; n++) {
        vga_putchar('a' + n % 26);
        if (n % 37 == 0) vga_putchar('\n');
    }
}
