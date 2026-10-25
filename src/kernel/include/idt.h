#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#include "./io.h"

struct idt_gate_s {
    uint16_t offset_low; // 16 lower bits of the function address
    uint16_t selector; // refere to gdt table segment
    uint8_t reserved; // always set to 0
    uint8_t attribute; // ring, type
    uint16_t offset_high; // interupt function
} __attribute__((packed));

struct idtr_s {
    uint16_t size;
    uint32_t address;
} __attribute__((packed));

extern struct idt_gate_s idt[256];
extern struct idtr_s idtr;

void idt_set_gate(uint8_t index, uint32_t handler_address, uint16_t selector, uint8_t attribute);
void idt_init(void);

#endif /* IDT_H */
