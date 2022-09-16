#ifndef __SCHED_H
#define __SCHED_H

#include <stdint.h>
#include <fs/fs.h>

#define TASK_RUNNING         0
#define TASK_INTERRUPTIBLE   1
#define TASK_UNINTERRUPTIBLE 2
#define TASK_STOPPED         3

#define TASK_FILES 8

struct task_struct {
    uint32_t r[13];
    uint8_t *sp;
    uint8_t *lr;
    uint8_t *pc;
    uint32_t xpsr;
    uint8_t *psp;
    uint32_t primask;
    uint32_t basepri;
    uint32_t faultmask;
    uint8_t control;

    uint8_t *stack;
    uint8_t state;
    uint64_t timeout_ms;

    struct file *files[TASK_FILES];
    struct list_head node;
};

void task_init(void);

void reset_msp();
void move_to_user_mode();
void schedule(void);

extern struct list_head tasks;
extern struct task_struct *current;

#endif
