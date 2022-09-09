#include "stm32f1xx_ll_utils.h"
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

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    syscall2(SYS_NANOSLEEP, rqtp, rmtp);
    return 0;
}

int usleep(useconds_t usec)
{
    struct timespec rqtp = {0};
    rqtp.tv_sec = usec / (1000 * 1000);
    rqtp.tv_nsec = usec % (1000 * 1000) * 1000;
    return nanosleep(&rqtp, NULL);
}

int select(int nfds, fd_set *restrict readfds,
           fd_set *restrict writefds, fd_set *restrict exceptfds,
           struct timeval *restrict timeout)
{
    syscall5(SYS_SELECT, nfds, readfds, writefds, exceptfds, timeout);
    register int retval __asm__("r0");
    return retval;
}
