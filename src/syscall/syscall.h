#ifndef __SYSCALL_H
#define __SYSCALL_H

#define SYS_EXIT 1
#define SYS_FORK 2
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_OPEN 5
#define SYS_CLOSE 6
#define SYS_IOCTL 7
#define SYS_SELECT 24
#define SYS_SOCKET 30
#define SYS_BIND 31
#define SYS_CONNECT 32
#define SYS_ACCEPT 33
#define SYS_LISTEN 34
#define SYS_SHUTDOWN 35
#define SYS_RECV 36
#define SYS_SEND 37
#define SYS_SETSOCKOPT 38
#define SYS_GETSOCKOPT 39
#define SYS_GETTIMEOFDAY 161
#define SYS_NANOSLEEP 162

#define syscall0(no)                            \
    __syscall(0, 0, 0, 0, 0, 0, no)
#define syscall1(no, arg1)                      \
    __syscall((void *)arg1, 0, 0, 0, 0, 0, no)
#define syscall2(no, arg1, arg2)                        \
    __syscall((void *)arg1, (void *)arg2, 0, 0, 0, 0, no)
#define syscall3(no, arg1, arg2, arg3)                              \
    __syscall((void *)arg1, (void *)arg2, (void *)arg3, 0, 0, 0, no)
#define syscall4(no, arg1, arg2, arg3, arg4)                            \
    __syscall((void *)arg1, (void *)arg2, (void *)arg3, (void *)arg4, 0, 0, no)
#define syscall5(no, arg1, arg2, arg3, arg4, arg5)                      \
    __syscall((void *)arg1, (void *)arg2, (void *)arg3, (void *)arg4, (void *)arg5, 0, no)
#define syscall6 __syscall

int __syscall(void *arg1, void *arg2, void *arg3, void *arg4,
              void *arg5, void *arg6, int no);

extern void *syscall_table[512];

void sys_init(void);

#endif
