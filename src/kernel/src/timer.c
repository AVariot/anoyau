
#include "timer.h"

static volatile uint32_t ticks = 0;

void timer_init(uint32_t hz) {
    // prescaler
    uint32_t n = PIT_BASE_FREQ / hz;

    // 00110110
    /*
        7 6 = canal
            00 = system timer
            01 = RAM refresh
            10 = haut parlur
        5 4 = access mod
        3 2 1 = fonction mode
        0 = count format
    */
    outb(PIT_CMD, 0x36);
    outb(PIT_CHANNEL0, n & 0xFF);
    outb(PIT_CHANNEL0, (n >> 8) & 0xFF);

}

void timer_tick(void) {
    ticks++;
}

uint32_t timer_get_ticks(void) {
    return ticks;
}
