#include "stm32f1xx_hal.h"
#include "syscall.h"
#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <ringbuf.h>
#include <fs.h>
#include <list.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern struct ringbuf *huart1_rxbuf;
extern struct ringbuf *huart2_rxbuf;

static LIST_HEAD(files);
static char fd_state[64];

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

    struct file *pos;
    list_for_each_entry(pos, &files, node) {
        if (pos->fd == fd) {
            assert(pos->inode->ops.read);
            return pos->inode->ops.read(pos->inode, buf, len);
        }
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

    struct file *pos;
    list_for_each_entry(pos, &files, node) {
        if (pos->fd == fd) {
            assert(pos->inode->ops.write);
            return pos->inode->ops.write(pos->inode, buf, len);
        }
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

    struct dentry *den = dentry_walk(pathname);
    if (den) {
        struct file *pos;
        list_for_each_entry(pos, &files, node) {
            if (pos->dentry == den) {
                errno = EBUSY;
                return -1;
            }
        }
        for (int i = 0; i < 64; i++) {
            if (fd_state[i] == 0) {
                pos = calloc(1, sizeof(*pos));
                fd_state[i] = 1;
                pos->fd = i;
                pos->dentry = den;
                pos->inode = den->inode;
                if (den->inode->ops.open)
                    den->inode->ops.open(den->inode);
                list_add(&pos->node, &files);
                return i;
            }
        }
    }

    errno = ENOENT;
    return -1;
}

int sys_close(int fd)
{
    if (fd == 1 || fd == 2) {
        fd_state[fd] = 0;
        return 0;
    }

    struct file *pos;
    list_for_each_entry(pos, &files, node) {
        if (pos->fd == fd) {
            list_del(&pos->node);
            free(pos);
            assert(fd_state[fd] == 1);
            fd_state[fd] = 0;
            return 0;
        }
    }

    errno = EBADF;
    return -1;
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
