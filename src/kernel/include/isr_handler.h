#ifndef ISR_HANDLER_H
#define ISR_HANDLER_H

#include <stdint.h>

struct regs_s {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

void isr_handler(struct regs_s *r);

#endif /* ISR_HANDLER_H */
