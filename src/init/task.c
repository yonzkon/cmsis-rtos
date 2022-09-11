#include "task.h"
#include <stdlib.h>

#define TASK_STACK_SIZE 1024

struct task_struct tasks[8];
struct task_struct *current;

void task_create(struct task_struct *task, void (*entry)(void))
{
    task->stack = calloc(1, TASK_STACK_SIZE);
    task->pc = (uint8_t *)entry;
    task->psp = task->stack + TASK_STACK_SIZE - 0x20;
    task->control = 3;

    *(uint32_t *)(task->psp + 0x10) = 0; // R12
    *(uint32_t *)(task->psp + 0x14) = 0; // LR
    *(uint32_t *)(task->psp + 0x18) = (uint32_t)entry; // PC
    *(uint32_t *)(task->psp + 0x1c) = 0x1000000; // XPSR
}

extern void idle(void);
extern void shell_main(void);
extern void apistt_main(void);

void task_init(void)
{
    task_create(tasks + 0, idle);
    task_create(tasks + 1, shell_main);
    task_create(tasks + 2, apistt_main);
}
