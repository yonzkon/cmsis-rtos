#include "stm32f1xx_hal.h"
#include "syscall.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <ringbuf.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern struct ringbuf *huart1_rxbuf;
extern struct ringbuf *huart2_rxbuf;

static int fd_state[4];

int sys_read(int fd, char *buf, int len)
{
    if (fd == 1) {
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        int n = ringbuf_used(huart1_rxbuf);
        if (n > len) n = len;
        ringbuf_read(huart1_rxbuf, buf, len);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        return n;
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

int sys_write(int fd, char *buf, int len)
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

int sys_open(const char *pathname, int flags)
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

int sys_close(int fd)
{
    if (fd > 2) {
        errno = EBADF;
        return -1;
    }

    fd_state[fd] = 0;
    return 0;
}

int sys_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
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
    syscall_table[SYS_READ] = sys_read;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_OPEN] = sys_open;
    syscall_table[SYS_CLOSE] = sys_close;
    syscall_table[SYS_SELECT] = sys_select;
    syscall_table[SYS_NANOSLEEP] = sys_nanosleep;
}
