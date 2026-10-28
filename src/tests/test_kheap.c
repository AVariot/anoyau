#include <stdint.h>
#include "vga.h"
#include "kheap.h"

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

/* Fixed seed: the same run every boot, so a failure can be reproduced */
static uint32_t rng_state = 12345;

static uint32_t rnd(void) {
    rng_state = rng_state * 1103515245u + 12345u;
    return (rng_state >> 16) & 0x7FFF;
}

static void fill(void *p, size_t n, uint8_t tag) {
    uint8_t *b = p;
    for (size_t i = 0; i < n; i++) {
        b[i] = tag;
    }
}

/* 1 if the whole block still holds the tag it was filled with */
static int intact(const void *p, size_t n, uint8_t tag) {
    const uint8_t *b = p;
    for (size_t i = 0; i < n; i++) {
        if (b[i] != tag) {
            return 0;
        }
    }
    return 1;
}

#define SLOTS 64
#define ROUNDS 3000

void test_kheap(void) {
    struct kheap_stats_s s;

    vga_clear();
    vga_puts("== test kheap ==\n");
    fails = 0;

    /* basics: distinct, aligned, reused after a free */
    void *a = kmalloc(100);
    void *b = kmalloc(100);
    check("kmalloc rend des pointeurs non nuls et distincts", a && b && a != b);
    check("pointeurs alignes sur 16", ((uintptr_t)a & 15) == 0 && ((uintptr_t)b & 15) == 0);
    kfree(a);
    void *c = kmalloc(100);
    check("un bloc libere est reutilise", c == a);
    kfree(b);
    kfree(c);

    /* two free neighbours must merge into one block big enough for 200 */
    void *x = kmalloc(100);
    void *y = kmalloc(100);
    void *z = kmalloc(100);
    kfree(x);
    kfree(y);
    void *w = kmalloc(200);
    check("deux blocs libres voisins fusionnent", w == x);
    kfree(w);
    kfree(z);

    /* refused requests */
    check("kmalloc(0) == NULL", kmalloc(0) == NULL);
    check("kmalloc(~0) == NULL", kmalloc(~0UL) == NULL);
    check("kmalloc(plus que le plafond) == NULL", kmalloc(65UL << 20) == NULL);
    check("le tas est sain apres les refus", kheap_check() == 0);

    /* misuse is reported (messages printed above) and changes nothing */
    kheap_stats(&s);
    size_t blocks_before = s.blocks;
    size_t used_before = s.used_bytes;
    void *d = kmalloc(50);
    kfree(d);
    kfree(d);
    kfree((void *)0x1234);
    kfree((void *)((uintptr_t)d + 1));
    kfree(NULL);
    kheap_stats(&s);
    check("double free et pointeurs invalides ignores",
          kheap_check() == 0 && s.used_bytes == used_before && s.blocks >= blocks_before);

    /* stress: random sizes, random order, data checked before each free */
    static void *ptr[SLOTS];
    static size_t size[SLOTS];
    static uint8_t tag[SLOTS];
    int corrupted = 0;
    int check_failed = 0;
    int alloc_failed = 0;

    for (int i = 0; i < ROUNDS; i++) {
        uint32_t k = rnd() % SLOTS;

        if (ptr[k]) {
            if (!intact(ptr[k], size[k], tag[k])) {
                corrupted++;
            }
            kfree(ptr[k]);
            ptr[k] = NULL;
        } else {
            size[k] = 1 + rnd() % 3000;
            tag[k] = (uint8_t)(rnd() | 1);
            ptr[k] = kmalloc(size[k]);
            if (ptr[k] == NULL) {
                alloc_failed++;
            } else {
                fill(ptr[k], size[k], tag[k]);
            }
        }
        if (i % 50 == 0 && kheap_check() != 0) {
            check_failed++;
        }
    }
    check("stress: aucune allocation refusee", alloc_failed == 0);
    check("stress: aucun bloc ecrase par un autre", corrupted == 0);
    check("stress: tas sain a chaque verification", check_failed == 0);

    /* at the end every block is freed: all of them must have merged */
    for (int k = 0; k < SLOTS; k++) {
        if (ptr[k]) {
            kfree(ptr[k]);
            ptr[k] = NULL;
        }
    }
    kheap_stats(&s);
    check("tout libere: plus aucun bloc utilise", kheap_check() == 0 && s.used_blocks == 0);
    check("tout libere: tous les blocs ont fusionne en un", s.blocks == 1);

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
