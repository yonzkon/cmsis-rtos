#include "stm32f1xx_ll_utils.h"
#include "syscall.h"
#include <stdint.h>
#include <sys/select.h>
#include <sched.h>

extern uint64_t sys_tick_ms;
extern uint64_t tim1_tick_ms;

int sys_gettimeofday(struct timeval *tp, void *tzp)
{
    tp->tv_sec = sys_tick_ms / 1000;
    tp->tv_usec = (sys_tick_ms % 1000) * 1000;
    return 0;
}

int sys_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    current->timeout_ms = tim1_tick_ms + rqtp->tv_sec*1000 + rqtp->tv_nsec/1000/1000;
    current->state = TASK_INTERRUPTIBLE;
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    return 0;
}

int sys_select(int nfds, fd_set *restrict readfds,
               fd_set *restrict writefds, fd_set *restrict exceptfds,
               struct timeval *restrict timeout)
{
    return 1;
}

void *syscall_table[512];

void sys_init(void)
{
    syscall_table[SYS_GETTIMEOFDAY] = sys_gettimeofday;
    syscall_table[SYS_NANOSLEEP] = sys_nanosleep;
    syscall_table[SYS_SELECT] = sys_select;
}
