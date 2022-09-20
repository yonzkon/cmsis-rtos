#include "stm32f1xx_ll_utils.h"
#include "syscall.h"
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>

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

int ioctl(int fd, unsigned int cmd, unsigned long arg)
{
    syscall3(SYS_IOCTL, fd, cmd, arg);
    register int retval __asm__("r0");
    return retval;
}

int _gettimeofday(struct timeval *tp, void *tzp)
{
    syscall2(SYS_GETTIMEOFDAY, tp, tzp);
    register int retval __asm__("r0");
    return retval;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    syscall2(SYS_NANOSLEEP, rqtp, rmtp);
    register int retval __asm__("r0");
    return retval;
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

int socket(int domain, int type, int protocol)
{
    syscall3(SYS_SOCKET, domain, type, protocol);
    register int retval __asm__("r0");
    return retval;
}

int bind(int socket, const struct sockaddr *address, socklen_t address_len)
{
    syscall3(SYS_BIND, socket, address, address_len);
    register int retval __asm__("r0");
    return retval;
}

int connect(int socket, const struct sockaddr *address,
           socklen_t address_len)
{
    syscall3(SYS_CONNECT, socket, address, address_len);
    register int retval __asm__("r0");
    return retval;
}

int accept(int socket, struct sockaddr *restrict address,
           socklen_t *restrict address_len)
{
    syscall3(SYS_ACCEPT, socket, address, address_len);
    register int retval __asm__("r0");
    return retval;
}

int listen(int socket, int backlog)
{
    syscall2(SYS_LISTEN, socket, backlog);
    register int retval __asm__("r0");
    return retval;
}

int recv(int socket, void *buffer, size_t length, int flags)
{
    syscall3(SYS_RECV, socket, buffer, length);
    register int retval __asm__("r0");
    return retval;
}

int send(int socket, const void *buffer, size_t length, int flags)
{
    syscall3(SYS_SEND, socket, buffer, length);
    register int retval __asm__("r0");
    return retval;
}

int setsockopt(int socket, int level, int option_name,
               const void *option_value, socklen_t option_len)
{
    syscall5(SYS_SETSOCKOPT, socket, level, option_name, option_value, option_len);
    register int retval __asm__("r0");
    return retval;
}

int getsockopt(int socket, int level, int option_name,
           void *restrict option_value, socklen_t *restrict option_len)
{
    syscall5(SYS_GETSOCKOPT, socket, level, option_name, option_value, option_len);
    register int retval __asm__("r0");
    return retval;
}
