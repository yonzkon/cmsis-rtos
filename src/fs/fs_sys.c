#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <fs/fs.h>
#include <syscall/syscall.h>
#include <list.h>

int sys_read(int fd, char *buf, int len)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp->fd == fd) {
            assert(filp->f_ops->read);
            return filp->f_ops->read(filp, buf, len);
        }
    }

    errno = EBADF;
    return -1;
}

int sys_write(int fd, char *buf, int len)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp->fd == fd) {
            assert(filp->f_ops->write);
            return filp->f_ops->write(filp, buf, len);
        }
    }

    errno = EBADF;
    return -1;
}

int sys_open(const char *pathname, int flags)
{
    struct dentry *den = dentry_walk(pathname);
    if (den) {
        for (int i = 1; i < TASK_FILES; i++) {
            if (current->files[i]->dentry == den) {
                errno = EEXIST;
                return -1;
            }
        }

        for (int i = 1; i < TASK_FILES; i++) {
            if (current->files[i] == NULL) {
                struct file *filp = alloc_file(i, den, den->inode);
                assert(filp->f_ops->open);
                filp->f_ops->open(filp);
                current->files[i] = filp;
                return i;
            }
        }
    }

    errno = ENOENT;
    return -1;
}

int sys_close(int fd)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp->fd == fd) {
            assert(filp->f_ops->close);
            filp->f_ops->close(filp);
            free(filp);
            current->files[i] = NULL;
            return 0;
        }
    }

    errno = EBADF;
    return -1;
}

int sys_ioctl(int fd, unsigned int cmd, unsigned long arg)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp->fd == fd) {
            assert(filp->f_ops->ioctl);
            return filp->f_ops->ioctl(filp, cmd, arg);
        }
    }

    errno = EBADF;
    return -1;
}

void fs_sys_init(void)
{
    syscall_table[SYS_READ] = sys_read;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_OPEN] = sys_open;
    syscall_table[SYS_CLOSE] = sys_close;
    syscall_table[SYS_IOCTL] = sys_ioctl;
}
