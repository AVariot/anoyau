
#include <stdint.h>
#include "string.h"

size_t strlen(const char *s) {
    const char *p = s;

    while (*p) {
        p++;
    }

    return p - s;
}

void *memset(void *dst, int c, size_t n) {

    uint8_t *d = dst;
    for (size_t i = 0; i < n; i++) {
        d[i] = (uint8_t)c;
    }

    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;

    if (s < d) {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    } else {
        for (size_t i = 0; i < n; i++)
            d[i] = s[i];
    }

    return dst;
}

int strcmp(const char *a, const char *b) {
    while (*a || *b) {
        if ((uint8_t)*a == (uint8_t)*b) {
            a++;
            b++;
        } else {
            return (uint8_t)*a - (uint8_t)*b;
        }
    }

    return 0;
}

int split_line(char *line, char **argv, int max) {
    int argc = 0;
    char *p = line;

    while (*p) {
        while (*p == ' ') {
            p++;
        }

        if (*p == '\0') {
            break;
        }
        if (argc < max) {
            argv[argc] = p;
            argc++;
        }

        while (*p != '\0' && *p != ' ') {
            p++;
        }

        if (*p == ' ') {
            *p = '\0';
            p++;
        }
    }

    return argc;
}
