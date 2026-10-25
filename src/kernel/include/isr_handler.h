#ifndef ISR_HANDLER_H
#define ISR_HANDLER_H

#include "printk.h"

#include <stdint.h>

struct regs_s {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
};

void isr_handler(struct regs_s *r);


#endif /* ISR_HANDLER */