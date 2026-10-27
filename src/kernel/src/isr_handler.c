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

/*
 * Page fault (vector 14): the CPU could not translate an address.
 *
 * CR2 holds the virtual address that caused the fault; it is a control
 * register, so it can only be read with an instruction (like CR3).
 * The pushed error code says why, bit by bit:
 *   bit 0: 0 = page not present, 1 = protection violation on a present page
 *   bit 1: 0 = read,             1 = write
 *   bit 2: 0 = kernel (ring 0),  1 = user (ring 3)
 */
static void page_fault_info(const struct regs_s *r) {
    uint64_t cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));

    printk("fault address = %lx\n", cr2);
    printk("cause: %s, %s, %s mode\n",
           (r->err_code & 1) ? "protection violation" : "page not present",
           (r->err_code & 2) ? "write" : "read",
           (r->err_code & 4) ? "user" : "kernel");
}

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
    if (r->int_no == 14) {
        page_fault_info(r);
    }
    printk("err_code=%lx rip=%lx cs=%lx rflags=%lx\n",
           r->err_code, r->rip, r->cs, r->rflags);
    printk("rsp=%lx ss=%lx\n", r->rsp, r->ss);
    printk("rax=%lx rbx=%lx\n", r->rax, r->rbx);
    printk("rcx=%lx rdx=%lx\n", r->rcx, r->rdx);
    printk("rsi=%lx rdi=%lx rbp=%lx\n", r->rsi, r->rdi, r->rbp);

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
