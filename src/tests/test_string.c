#include <stdint.h>
#include "vga.h"
#include "string.h"

/* Mettre à 1 quand strcmp est écrite */
#define TEST_STRCMP 1

static int fails;

static void check(const char *name, int ok) {
    if (ok) {
        vga_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_puts("[ OK ] ");
    } else {
        vga_setcolor(VGA_LIGHT_RED, VGA_BLACK);
        vga_puts("[FAIL] ");
        fails++;
    }
    vga_setcolor(VGA_WHITE, VGA_BLACK);
    vga_puts(name);
    vga_puts("\n");
}

/* Compare nos tests sans dépendre de strcmp/memcmp */
static int same(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

void test_string(void) {
    char buf[16];

    vga_clear();
    vga_puts("== test string ==\n");

    /* strlen */
    check("strlen(\"\") == 0", strlen("") == 0);
    check("strlen(\"abc\") == 3", strlen("abc") == 3);
    check("strlen s'arrete au premier \\0", strlen("ab\0cd") == 2);

    /* memset */
    memset(buf, 'x', 8);
    check("memset remplit 8 octets", same(buf, "xxxxxxxx", 8));

    memset(buf, 'a', 16);
    memset(buf, 'z', 4);
    check("memset n'ecrit pas au-dela de n", buf[4] == 'a' && buf[3] == 'z');

    memset(buf, 'a', 4);
    memset(buf, 'b', 0);
    check("memset n=0 ne change rien", same(buf, "aaaa", 4));

    memset(buf, 0x1FF, 2);
    check("memset ne garde que l'octet bas", (uint8_t)buf[0] == 0xFF && (uint8_t)buf[1] == 0xFF);

    check("memset retourne dst", memset(buf, 0, 1) == buf);

    /* memcpy */
    {
        char src[] = "ABCD";
        char dst[8];
        memset(dst, '.', 8);
        memcpy(dst + 2, src, 4);
        check("memcpy copie 4 octets", same(dst, "..ABCD..", 8));
        check("memcpy retourne dst", memcpy(dst, src, 0) == dst);
    }
    {
        unsigned char hi[3] = {0x80, 0xFF, 0x01};
        unsigned char out[3];
        memcpy(out, hi, 3);
        check("memcpy octets >= 128 intacts", out[0] == 0x80 && out[1] == 0xFF && out[2] == 0x01);
    }

    /* memmove */
    {
        char t[] = "ABCDEF";
        memmove(t + 2, t, 4);
        check("memmove d > s (chevauche) -> ABABCD", same(t, "ABABCD", 6));
    }
    {
        char t[] = "ABCDEF";
        memmove(t, t + 2, 4);
        check("memmove d < s (chevauche) -> CDEFEF", same(t, "CDEFEF", 6));
    }
    {
        char t[] = "ABCDEF";
        memmove(t, t, 6);
        check("memmove d == s ne change rien", same(t, "ABCDEF", 6));
    }
    {
        char t[] = "ABCDEF";
        memmove(t + 1, t, 0);
        check("memmove n=0 ne change rien", same(t, "ABCDEF", 6));
    }
    {
        char a[] = "ABCD";
        char b[] = "....";
        memmove(b, a, 4);
        check("memmove zones separees", same(b, "ABCD", 4));
        check("memmove retourne dst", memmove(b, a, 1) == b);
    }
    {
        char t[] = "ABCDEFGH";
        memmove(t + 1, t, 7);
        check("memmove decalage de 7 (d = s+1)", same(t, "AABCDEFG", 8));
    }

#if TEST_STRCMP
    check("strcmp egales == 0", strcmp("abc", "abc") == 0);
    check("strcmp \"\" == \"\"", strcmp("", "") == 0);
    check("strcmp a < b -> negatif", strcmp("abc", "abd") < 0);
    check("strcmp b > a -> positif", strcmp("abd", "abc") > 0);
    check("strcmp prefixe plus court < ", strcmp("ab", "abc") < 0);
    check("strcmp prefixe plus long >", strcmp("abc", "ab") > 0);
    check("strcmp unsigned: 0x80 > 'a'", strcmp("\x80", "a") > 0);
#endif

    vga_puts("\n");
    if (fails == 0) {
        vga_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_puts("TOUS LES TESTS PASSENT\n");
    } else {
        vga_setcolor(VGA_LIGHT_RED, VGA_BLACK);
        vga_puts("DES TESTS ECHOUENT\n");
    }
    vga_setcolor(VGA_WHITE, VGA_BLACK);
}
