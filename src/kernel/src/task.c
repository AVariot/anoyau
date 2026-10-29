
#include "task.h"
#include "kheap.h"
#include "irq.h"
#include "timer.h"

#define STACK_SIZE 8192

_Static_assert(STACK_SIZE % 16 == 0, "stack size needs to be a multiple of 16");

static int next_id = 1;

static struct task_s main_task;
static struct task_s *current;

/*
 * Runs only when no other task is ready. It is NOT in the circular list:
 * otherwise it would get a full time slice even while others have work.
 */
static struct task_s *idle_task;

/* Reached when a task's function returns. Parks the CPU for now. */
static void task_exit(void) {
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

/*
 * First code run by every new task. We get here from a switch_to(), often
 * inside the timer handler, so interrupts are still off: turn them on, or
 * this task could never be preempted.
 */
static void task_start(void) {
    __asm__ volatile ("sti");
    current->entry();
    task_exit();
}

static void idle_entry(void) {
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

#define TIME_SLICE_TICKS 10

static unsigned slice = 0;

/*
 * Picks the next task able to run: one lap of the list starting after prev,
 * first ready task wins. Sleepers whose time has come are woken on the way.
 * prev is visited last, so if it is the only ready task it picks itself.
 */
static struct task_s *pick_next(struct task_s *prev) {
    struct task_s *start = prev->next;
    struct task_s *c = start;

    do {
        /* difference, not comparison: still right when the counter wraps */
        if (c->state == SLEEPING &&
            (int32_t)(timer_get_ticks() - c->wake_tick) >= 0) {
            c->state = READY;
        }
        if (c->state == READY) {
            return c;
        }
        c = c->next;
    } while (c != start);

    return idle_task;
}

/* Switches to the next ready task. Interrupts MUST be off. */
static void schedule(void) {
    struct task_s *prev = current;
    struct task_s *next = pick_next(prev);

    if (next == prev) {
        return;
    }
    current = next;
    switch_to(&prev->rsp, next->rsp);
}

void task_yield(void) {
    uint64_t flags = irq_save();
    schedule();
    irq_restore(flags);
}

void task_sleep_ticks(uint32_t ticks) {
    if (ticks == 0) {
        return;
    }
    /* off for the whole sequence: a tick between "I sleep" and "I yield"
     * would reschedule a task that is already marked as sleeping */
    uint64_t flags = irq_save();
    current->wake_tick = timer_get_ticks() + ticks;
    current->state = SLEEPING;
    schedule();                 /* comes back here once we are woken up */
    irq_restore(flags);
}

int task_running(void) {
    return current != NULL;
}

void task_tick(void) {
    if (current == NULL) {
        return;
    }
    /* idle: look for a task to wake up at every tick, not every 100 ms */
    if (current == idle_task || ++slice >= TIME_SLICE_TICKS) {
        slice = 0;
        schedule();
    }
}

/* Builds a task with a forged stack. Not in the list: see task_create. */
static struct task_s *task_new(void (*entry)(void)) {
    struct task_s *t = kmalloc(sizeof(*t));

    if (t == NULL) {
        return NULL;
    }

    t->stack = kmalloc(STACK_SIZE);
    if (t->stack == NULL) {
        kfree(t);
        return NULL;
    }

    uint64_t *s = (uint64_t *)((uintptr_t)t->stack + STACK_SIZE);

    s[-1] = (uint64_t)(uintptr_t)task_exit;
    s[-2] = (uint64_t)(uintptr_t)task_start;
    s[-3] = 0; // rbp
    s[-4] = 0; // rbx
    s[-5] = 0; // r12
    s[-6] = 0; // r13
    s[-7] = 0; // r14
    s[-8] = 0; // r15

    t->rsp = (uint64_t)(uintptr_t)&s[-8];
    t->id = next_id++;
    t->entry = entry;
    t->state = READY;
    t->wake_tick = 0;
    t->next = NULL;
    return t;
}

void task_init(void) {
    main_task.id = 0;
    main_task.stack = NULL;
    main_task.state = READY;
    main_task.next = &main_task;

    idle_task = task_new(idle_entry);
    /* never in the list, but its next must lead into it: pick_next(idle) starts there */
    if (idle_task != NULL) {
        idle_task->next = &main_task;
    }
    current = &main_task;
}

struct task_s *task_create(void (*entry)(void)) {
    struct task_s *t = task_new(entry);

    if (t == NULL) {
        return NULL;
    }
    /* only now that t is fully built does it become visible to the timer */
    uint64_t flags = irq_save();
    t->next = current->next;
    current->next = t;
    irq_restore(flags);
    return t;
}
