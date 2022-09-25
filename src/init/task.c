#include <stdlib.h>
#include <string.h>
#include <sched.h>

LIST_HEAD(tasks);
struct task_struct *current;

static struct task_struct *task_create(void (*entry)(void))
{
    struct task_struct *task = calloc(1, sizeof(*task));

    task->stack = calloc(1, TASK_STACK_SIZE);
    memset(task->stack, TASK_STACK_MAGIC, TASK_STACK_MAGIC_LEN);
    task->pc = (uint8_t *)entry;
    task->psp = task->stack + TASK_STACK_SIZE - 0x20;
    task->control = 3;

    *(uint32_t *)(task->psp + 0x10) = 0; // R12
    *(uint32_t *)(task->psp + 0x14) = 0; // LR
    *(uint32_t *)(task->psp + 0x18) = (uint32_t)entry; // PC
    *(uint32_t *)(task->psp + 0x1c) = 0x1000000; // XPSR

    task->state = TASK_RUNNING;
    task->timeout_ms = 0;
    INIT_LIST_HEAD(&task->node);
    list_add_tail(&task->node, &tasks);

    return task;
}

void task_init(void)
{
    extern void idle(void);
    extern void shell_main(void);
    extern void apistt_main(void);

    current = task_create(idle);
    task_create(shell_main);
    task_create(apistt_main);
}
