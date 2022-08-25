#include "stm32f1xx_hal.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <ringbuf.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern struct ringbuf *huart2_rxbuf;

static int fd_state[4];

int _write(int fd, char *buf, int len)
{
    if (fd == 1) {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, HAL_MAX_DELAY);
        return len - huart1.TxXferCount;
    }

    if (fd == 2) {
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);
        return len - huart2.TxXferCount;
    }

    errno = EBADF;
    return -1;
}

int _read(int fd, char *buf, int len)
{
    asm(
        "STMFD R13!, {R14};"
        //"push {lr};"
        "mov r7, #3;"
        "swi 0;"
        "LDMFD R13!, {R14};"
        //"pop {lr};"
        ::);
}

int sys_read(int fd, char *buf, int len)
{
    if (fd == 1) {
        HAL_UART_Receive(&huart1, (uint8_t *)buf, len, 500);
        return len - huart1.RxXferCount;
    }

    if (fd == 2) {
        HAL_NVIC_DisableIRQ(USART2_IRQn);
        int n = ringbuf_used(huart2_rxbuf);
        if (n > len) n = len;
        ringbuf_read(huart2_rxbuf, buf, len);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        return n;
    }

    errno = EBADF;
    return -1;
}

int _open(const char *pathname, int flags)
{
    if (strcmp(pathname, "uart1") == 0) {
        if (fd_state[1] != 0) {
            errno = EEXIST;
            return -1;
        } else {
            fd_state[1] = 1;
            return 1;
        }
    }

    if (strcmp(pathname, "uart2") == 0) {
        if (fd_state[2] != 0) {
            errno = EEXIST;
            return -1;
        } else {
            fd_state[2] = 1;
            return 2;
        }
    }

    errno = ENOENT;
    return -1;
}

int _close(int fd)
{
    if (fd > 2) {
        errno = EBADF;
        return -1;
    }

    fd_state[fd] = 0;
    return 0;
}

int usleep(useconds_t usec)
{
    HAL_Delay(usec / 1000);
    return 0;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    HAL_Delay(rqtp->tv_sec * 1000 + rqtp->tv_nsec / 1000 / 1000);
    return 0;
}

int select(int nfds, fd_set *restrict readfds,
           fd_set *restrict writefds, fd_set *restrict exceptfds,
           struct timeval *restrict timeout)
{
    return 1;
}
