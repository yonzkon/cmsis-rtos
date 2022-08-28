#include "stm32f1xx_hal.h"
#include "syscall.h"
#include <sys/time.h>
#include <sys/select.h>

int _read(int fd, char *buf, int len)
{
    syscall3(SYS_READ, fd, buf, len);
    register int retval __asm__("r0");
    return retval;
}

int _write(int fd, char *buf, int len)
{
    syscall3(SYS_WRITE, fd, buf, len);
    register int retval __asm__("r0");
    return retval;
}

int _open(const char *pathname, int flags)
{
    syscall2(SYS_OPEN, pathname, flags);
    register int retval __asm__("r0");
    return retval;
}

int _close(int fd)
{
    syscall1(SYS_CLOSE, fd);
    register int retval __asm__("r0");
    return retval;
}

int usleep(useconds_t usec)
{
    HAL_Delay(usec / 1000);
    return 0;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    //syscall2(SYS_NANOSLEEP, rqtp, rmtp);
    HAL_Delay(rqtp->tv_sec * 1000 + rqtp->tv_nsec / 1000 / 1000);
    return 0;
}

int select(int nfds, fd_set *restrict readfds,
           fd_set *restrict writefds, fd_set *restrict exceptfds,
           struct timeval *restrict timeout)
{
    syscall5(SYS_SELECT, nfds, readfds, writefds, exceptfds, timeout);
    register int retval __asm__("r0");
    return retval;
}
