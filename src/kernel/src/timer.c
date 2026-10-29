
#include "timer.h"
#include "task.h"

/*

ms	    hz	 calcul	            tics
500	    100	 500 × 100 / 1000	50
2000	100	 2000 × 100 / 1000	200
10	    100	 10 × 100 / 1000	1
1	    100	 1 × 100 / 1000	    0 (pb)
*/

static volatile uint32_t ticks = 0;
static uint32_t timer_hz = 0;

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
    timer_hz = hz;
}

void timer_tick(void) {
    ticks++;
}

uint32_t timer_get_ticks(void) {
    return ticks;
}

uint32_t timer_ms_to_ticks(uint32_t ms) {
    // 1000 for ms
    // 999 = because we used 1000 so we don't want a rest
    return (ms * timer_hz + 999) / 1000;
}

void sleep_ms(uint32_t ms) {
    uint32_t wanted = timer_ms_to_ticks(ms);

    /* once tasks run, sleep without burning CPU: another task takes over */
    if (task_running()) {
        task_sleep_ticks(wanted);
        return;
    }

    /* before task_init(): busy wait */
    uint32_t start = ticks;

    while ((uint32_t)(ticks - start) < wanted) {
        __asm__ volatile ("hlt");
    }
}
