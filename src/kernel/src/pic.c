
#include "pic.h"

void pic_init(void) {

    /*
        ICWx Initialization Command Word 4 octes send to pic 
    */

    // ICW1: Start init
    outb(PIC_MASTER_CMD, 0x10 + 0x01);
    outb(PIC_SLAVE_CMD, 0x10 + 0x01);
    // ICW2: Setup vector offset
    outb(PIC_MASTER_DATA, 0x20);
    outb(PIC_SLAVE_DATA, 0x28);
    // ICW3: tell to master that it has a slave
    outb(PIC_MASTER_DATA, 4); 
    outb(PIC_SLAVE_DATA, 2);
    // ICW4: enabke 8086 mode
    outb(PIC_MASTER_DATA, 0x01); 
    outb(PIC_SLAVE_DATA, 0x01);
    // only IRQ 0 / IRQ 1 (timer / keyboard) enabled
    outb(PIC_MASTER_DATA, 0xFC);
    // all slave IRQs disabled
    outb(PIC_SLAVE_DATA, 0xFF);
}

// EOI = End of interupt
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC_SLAVE_CMD, PIC_CMD_EOI);
    }
    outb(PIC_MASTER_CMD, PIC_CMD_EOI);
}
