
#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

static inline uint64_t irq_save(void) {
    uint64_t flags;
    __asm__ volatile ("pushfq; cli; pop %0" : "=r"(flags) :: "memory");
    return flags;
}

static inline void irq_restore(uint64_t flags) {
    __asm__ volatile ("push %0; popfq" :: "r"(flags) : "memory", "cc");
}

#endif /* IRQ_H */
