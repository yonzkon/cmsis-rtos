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

static LIST_HEAD(files);
static char fd_state[64];

int sys_read(int fd, char *buf, int size)
{
    struct file *pos;
    list_for_each_entry(pos, &files, node) {
        if (pos->fd == fd) {
            assert(pos->inode->ops.read);
            return pos->inode->ops.read(pos->inode, buf, size);
        }
    }

    errno = EBADF;
    return -1;
}

int sys_write(int fd, char *buf, int len)
{
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
    struct dentry *den = dentry_walk(pathname);
    if (den) {
        struct file *pos;
        list_for_each_entry(pos, &files, node) {
            if (pos->dentry == den) {
                errno = EBUSY;
                return -1;
            }
        }
        for (int i = 1; i < 64; i++) {
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
