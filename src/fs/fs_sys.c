#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <fs/fs.h>
#include <syscall/syscall.h>
#include <list.h>

static LIST_HEAD(files);

int sys_read(int fd, char *buf, int len)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *fp = current->files[i];
        if (fp->fd == fd) {
            assert(fp->f_ops.read);
            return fp->f_ops.read(fp, buf, len);
        }
    }

    errno = EBADF;
    return -1;
}

int sys_write(int fd, char *buf, int len)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *fp = current->files[i];
        if (fp->fd == fd) {
            assert(fp->f_ops.write);
            return fp->f_ops.write(fp, buf, len);
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
                struct file *fp = calloc(1, sizeof(*fp));
                fp->fd = i;
                fp->dentry = den;
                fp->f_ops = den->inode->f_ops;
                INIT_LIST_HEAD(&fp->node);
                list_add(&fp->node, &files);
                fp->private_data = NULL;
                assert(fp->f_ops.open);
                fp->f_ops.open(fp);
                current->files[i] = fp;
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
        struct file *fp = current->files[i];
        if (fp->fd == fd) {
            assert(fp->f_ops.close);
            fp->f_ops.close(fp);
            list_del(&fp->node);
            free(fp);
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
        struct file *fp = current->files[i];
        if (fp->fd == fd) {
            assert(fp->f_ops.ioctl);
            return fp->f_ops.ioctl(fp, cmd, arg);
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
