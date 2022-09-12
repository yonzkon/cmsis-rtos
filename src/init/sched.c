#include <sched.h>

extern uint64_t tim1_tick_ms;

void schedule(void)
{
    struct task_struct *pos, *n;
    list_for_each_entry_safe(pos, n, &tasks, node) {
        if (pos == current)
            continue;

        if (pos->state == TASK_INTERRUPTIBLE ||
            pos->state == TASK_UNINTERRUPTIBLE) {
            if (tim1_tick_ms > pos->timeout_ms)
                pos->state = TASK_RUNNING;
        }

        if (pos->state == TASK_RUNNING) {
            list_del(&pos->node);
            list_add_tail(&pos->node, &tasks);
            current = pos;
            break;
        }
    }
}
