
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "pic.h"

// Data gate 0
#define PIT_CHANNEL0 0x40
// Command gate
#define PIT_CMD 0x43
// Hz
#define PIT_BASE_FREQ 1193182

void timer_init(uint32_t hz);
void timer_tick(void);
uint32_t timer_get_ticks(void);
uint32_t timer_ms_to_ticks(uint32_t ms);
void sleep_ms(uint32_t ms);

#endif /* TIMER_H */
