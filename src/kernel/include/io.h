#ifndef IO_H
#define IO_H

#include <stdint.h>

/* static inline: the body is copied at each call, one single instruction */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t res;
    __asm__ volatile ("inb %1, %0" : "=a"(res) : "Nd"(port));
    return res;
}

#endif /* IO_H */
