
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

enum task_state_e {
    READY,
    SLEEPING,
    DEAD,
};

struct task_s {
    uint64_t rsp;
    int id;
    void *stack;
    struct task_s *next;
    void (*entry)(void);
    enum task_state_e state;
    uint32_t wake_tick; 
};

void task_tick(void);

void switch_to(uint64_t *old_rsp, uint64_t new_rsp);
void task_init(void);
void task_yield(void);
void task_sleep_ticks(uint32_t ticks);
int task_running(void);     /* 1 once task_init() has run */

struct task_s *task_create(void (*entry)(void));

#endif /* TASK_H */
