#include "scheduler.h"
#include <kernel/multitasking/multitasking.h>
#include <kernel/mem/lib/main.h>
#include <kernel/mem/klime/klime.h>
#include <kernel/communication/serial.h>

u64 scheduler_quantum_for_priority(u64 priority) {
    if (priority == 0) priority = 1;

    u64 q = (priority * SCHED_BASE_QUANTUM) / 128;

    if (q < SCHED_MIN_QUANTUM) q = SCHED_MIN_QUANTUM;
    if (q > SCHED_MAX_QUANTUM) q = SCHED_MAX_QUANTUM;

    return q;
}

scheduler_t* scheduler_init(ulime_t *ulime, u64 base_quantum)
{
    if (!ulime || !ulime->klime) return NULL;

    scheduler_t *s = (scheduler_t*)klime_create(ulime->klime, sizeof(scheduler_t));
    if (!s) return NULL;

    s->ulime         = ulime;
    s->quantum       = (base_quantum > 0) ? base_quantum : SCHED_BASE_QUANTUM;
    s->ticks         = 0;
    s->last_idx      = -1;
    s->round_counter = 0;

    return s;
}

void scheduler_set_quantum(scheduler_t *s, u64 quantum) {
    if (!s) return;
    s->quantum = (quantum > 0) ? quantum : SCHED_BASE_QUANTUM;
}

int scheduler_pick_next(scheduler_t *s, void *mt_ptr, int current_idx)
{
    mt_t *mt = (mt_t*)mt_ptr;
    if (!mt || mt->task_count == 0) return -1;

    int count = mt->task_count;
    int start = (current_idx + 1) % count;
    int i = start;

    do
    {
        mt_task_t *t = &mt->tasks[i];

        if (!t->valid || !t->proc) {
            i = (i + 1) % count;
            continue;
        }

        u64 state = t->proc->state;
        if (state == PROC_READY || state == PROC_CREATED || state == PROC_RUNNING) {
            return i;
        }

        i = (i + 1) % count;
    } while (i != start);

    // second pass: if nothing ready, check if anything is just blocked
    // but has been waiting way too long (starvation prevention)
    // we skip this for ZOMBIE states though, dead is dead
    for (int j = 0; j < count; j++) {
        mt_task_t *t = &mt->tasks[j];
        if (!t->valid || !t->proc) continue;
        if (t->proc->state == PROC_ZOMBIE) continue;

        // if its blocked but not zombie, give it a shot
        // the syscall that blocked it might have been resolved by now
        if (t->proc->state == PROC_BLOCKED) {
            // only do this every STARVATION_LIMIT rounds so we dont spam
            if (s->round_counter % SCHED_STARVATION_LIMIT == 0) {
                return j;
            }
        }
    }

    return -1;
}

void scheduler_tick(scheduler_t *s) {
    if (!s) return;

    s->ticks++;

    if (s->ticks >= s->quantum) {
        ulime_schedule(s->ulime);
        s->ticks = 0;
        s->round_counter++;
    }
}

void scheduler_yield(scheduler_t *s) {
    if (!s) return;

    // reset ticks so we dont immediately preempt after resuming
    s->ticks = 0;
    ulime_schedule(s->ulime);
}