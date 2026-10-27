#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#include "./io.h"

/* 64-bit interrupt gate: 16 bytes */
struct idt_gate_s {
    uint16_t offset_low;    // bits 0-15 of the handler address
    uint16_t selector;      // code segment in the gdt (0x08)
    uint8_t  ist;           // interrupt stack table index, 0 = not used
    uint8_t  attribute;     // present, ring, type
    uint16_t offset_mid;    // bits 16-31          (oldly offset_high)
    uint32_t offset_high;   // bits 32-63          (new)
    uint32_t reserved;      // always 0            (new)
} __attribute__((packed));

struct idtr_s {
    uint16_t size;
    uint64_t address;
} __attribute__((packed));

// Verify during the compilation if a structure has a good size
_Static_assert(sizeof(struct idt_gate_s) == 16, "an idt gate is 16 bytes");
_Static_assert(sizeof(struct idtr_s) == 10, "the idtr is 10 bytes");

extern struct idt_gate_s idt[256];
extern struct idtr_s idtr;

void idt_set_gate(uint8_t index, uint64_t handler_address, uint16_t selector, uint8_t attribute);
void idt_init(void);

#endif /* IDT_H */
