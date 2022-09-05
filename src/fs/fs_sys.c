#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fs/fs.h>
#include <syscall/syscall.h>
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
                INIT_LIST_HEAD(&pos->node);
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

void fs_sys_init(void)
{
    syscall_table[SYS_READ] = sys_read;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_OPEN] = sys_open;
    syscall_table[SYS_CLOSE] = sys_close;
}
