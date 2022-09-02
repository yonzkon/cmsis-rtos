#include <assert.h>
#include <string.h>
#include <fs/fs.h>
#include <stdio.h>

static struct inode i_root;
static struct dentry d_root;

static struct inode i_dev;
static struct dentry d_dev;

int root_open(struct inode *inode)
{
    return -1;
}

int root_close(struct inode *inode)
{
    return -1;
}

int root_ioctl(struct inode *inode, unsigned int cmd, unsigned long arg)
{
    return -1;
}

int root_write(struct inode *inode, const void *buf, uint32_t len)
{
    return -1;
}

int root_read(struct inode *inode, void *buf, uint32_t size)
{
    return -1;
}

static inode_ops_t ops_root = {
    .open = root_open,
    .close = root_close,
    .ioctl = root_ioctl,
    .write = root_write,
    .read = root_read,
};

struct dentry *fs_get_root(void)
{
    return &d_root;
}

void fs_init(void)
{
    // root
    i_root.type = INODE_TYPE_TMP;
    i_root.ops = ops_root;
    INIT_LIST_HEAD(&i_root.node);

    bzero(d_root.name, sizeof(d_root.name));
    d_root.type = DENTRY_TYPE_DIR;
    d_root.parent = NULL;
    INIT_LIST_HEAD(&d_root.childs);
    INIT_LIST_HEAD(&d_root.child_node);
    d_root.inode = NULL;

    // dev
    i_dev.type = INODE_TYPE_TMP;
    i_dev.ops = ops_root;
    INIT_LIST_HEAD(&i_dev.node);

    snprintf(d_dev.name, sizeof(d_dev.name), "dev");
    d_dev.type = DENTRY_TYPE_DIR;
    d_dev.parent = &d_root;
    INIT_LIST_HEAD(&d_dev.childs);
    INIT_LIST_HEAD(&d_dev.child_node);
    list_add(&d_dev.child_node, &d_root.childs);
    d_dev.inode = NULL;

    fs_sys_init();
}

static struct dentry *__dentry_walk(const char *path, struct dentry *dentry)
{
    assert(path[0] == '/');
    assert(path[1] != 0);
    assert(dentry);

    if (list_empty(&dentry->childs))
        return NULL;

    char *path_next = strchr(path + 1, '/');

    // recursive fininshed
    if (path_next == NULL) {
        int len = strlen(path) - 1;
        assert(len < PATH_MAX);
        struct dentry *pos;
        list_for_each_entry(pos, &dentry->childs, child_node) {
            if (memcmp(pos->name, path + 1, len) == 0) {
                return pos;
            }
        }
        return NULL;
    } else {
        int len = path_next - path - 1;
        assert(len < PATH_MAX);
        struct dentry *pos;
        list_for_each_entry(pos, &dentry->childs, child_node) {
            if (memcmp(pos->name, path + 1, len) == 0) {
                return __dentry_walk(path_next, pos);
            }
        }
        return NULL;
    }
}

struct dentry *dentry_walk(const char *path)
{
    return __dentry_walk(path, &d_root);
}

int dentry_add(const char *path_parent, struct dentry *child)
{
    struct dentry *den = dentry_walk(path_parent);
    if (den == NULL) return -1;
    list_add(&child->child_node, &den->childs);
    child->parent = den;
    if (child->ops.create)
        child->ops.create(child);
    return 0;
}

int dentry_del(const char *path)
{
    struct dentry *den = dentry_walk(path);
    list_del(&den->child_node);
    den->parent = NULL;
    if (den->ops.unlink)
        den->ops.unlink(den);
    return 0;
}
