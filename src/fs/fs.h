#ifndef __FS_H
#define __FS_H

#include <stdint.h>
#include <list.h>

#define PATH_MAX 32

#define DENTRY_TYPE_DIR 0
#define DENTRY_TYPE_FILE 1

#define INODE_TYPE_TMP 0
#define INODE_TYPE_DIR 1
#define INODE_TYPE_FILE 2
#define INODE_TYPE_CHAR 3
#define INODE_TYPE_BLOCK 4

struct file {
    struct inode *inode;
    struct dentry *dentry;
    int fd;
    struct list_head node;
};

typedef struct dentry_ops {
    int (*create)(struct dentry *dentry);
    int (*unlink)(struct dentry *dentry);
} dentry_ops_t;

struct dentry {
    char name[PATH_MAX];
    int type;
    dentry_ops_t ops;
    struct dentry *parent;
    struct list_head childs;
    struct list_head child_node;
    struct inode *inode;
};

typedef struct inode_ops {
    int (*open)(struct inode *inode);
    int (*close)(struct inode *inode);
    int (*ioctl)(struct inode *inode, unsigned int cmd, unsigned long arg);
    int (*write)(struct inode *inode, const void *buf, uint32_t len);
    int (*read)(struct inode *inode, void *buf, uint32_t len);
} inode_ops_t;

struct inode {
    int type;
    inode_ops_t ops;
    struct list_head node;
};

struct dentry *dentry_walk(const char *path);
int dentry_add(const char *path, struct dentry *child);
int dentry_del(const char *path);

struct dentry *fs_get_root(void);
void fs_init(void);
void fs_sys_init(void);

#endif
