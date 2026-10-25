#include "isr_handler.h"
#include "vga.h"
#include "timer.h"
#include "keyboard.h"
#include "printk.h"

static const char *exception_names[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved",
};

static void panic(const struct regs_s *r) __attribute__((noreturn));

/* Fatal CPU exception: show what we know and stop */
static void panic(const struct regs_s *r) {
    const char *name = "Unknown";

    if (r->int_no < 32) {
        name = exception_names[r->int_no];
    }

    vga_setcolor(VGA_LIGHT_RED, VGA_BLACK);
    printk("\n*** KERNEL PANIC ***\n");
    vga_setcolor(VGA_WHITE, VGA_BLACK);

    printk("Error: %d (%s)\n", (int)r->int_no, name);
    printk("err_code=%x eip=%x cs=%x eflags=%x\n",
           (unsigned int)r->err_code, (unsigned int)r->eip,
           (unsigned int)r->cs, (unsigned int)r->eflags);
    printk("eax=%x ebx=%x ecx=%x edx=%x\n",
           (unsigned int)r->eax, (unsigned int)r->ebx,
           (unsigned int)r->ecx, (unsigned int)r->edx);
    printk("esi=%x edi=%x ebp=%x\n",
           (unsigned int)r->esi, (unsigned int)r->edi, (unsigned int)r->ebp);

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

void isr_handler(struct regs_s *r) {
    /* vectors 0-31: CPU exceptions, never resumed */
    if (r->int_no < 32) {
        panic(r);
    }

    /* vectors 32-47: hardware IRQs */
    if (r->int_no == 32) {
        timer_tick();
    }
    if (r->int_no == 33) {
        keyboard_handler();
    }
    pic_send_eoi(r->int_no - 32);
}
