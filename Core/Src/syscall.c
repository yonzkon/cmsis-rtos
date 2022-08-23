#include "stm32f1xx_hal.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

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
    if (fd == 1) {
        HAL_UART_Receive(&huart1, (uint8_t *)buf, len, 500);
        return len - huart1.RxXferCount;
    }

    if (fd == 2) {
        HAL_UART_Receive(&huart2, (uint8_t *)buf, len, 500);
        return len - huart2.RxXferCount;
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
