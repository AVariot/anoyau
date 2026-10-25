
#include "idt.h"

struct idt_gate_s idt[256];
struct idtr_s idtr;

void idt_set_gate(uint8_t index, uint32_t handler_address, uint16_t selector, uint8_t attribute) {
    idt[index].offset_low = handler_address & 0xFFFF;
    idt[index].offset_high = handler_address >> 16;
    idt[index].selector = selector;
    idt[index].reserved = 0;
    idt[index].attribute = attribute;
}

extern uint32_t isr_stub_table[48];

void idt_init(void) {
    for (int i = 0; i < 48; i++) {
        idt_set_gate(i, isr_stub_table[i], 0x08, 0x8E);
    }
    idtr.size = sizeof(idt) - 1;
    idtr.address = (uint32_t) &idt;
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}
