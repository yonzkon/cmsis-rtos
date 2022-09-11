#ifndef __TASK_H
#define __TASK_H

#include <stdint.h>
#include <fs/fs.h>

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

    struct file files[8];
};

void task_init(void);

void reset_msp();
void switch_to_user_task(struct task_struct *task);
void switch_to(struct task_struct *task);

extern struct task_struct tasks[8];
extern struct task_struct *current;

#endif
