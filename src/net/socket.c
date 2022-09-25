#include <assert.h>
#include <stdlib.h>
#include <fs/fs.h>
#include <sys/socket.h>
#include "wizchip_socket.h"

static int sock_open(struct file *filp)
{
    __socket(filp->fd, Sn_MR_TCP, 0, Sn_MR_ND);
    return 0;
}

static int sock_close(struct file *filp)
{
    __disconnect(filp->fd);
    return __close(filp->fd);
}

static int sock_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return -1;
}

static int sock_read(struct file *filp, void *buf, uint32_t len)
{
    return -1;
}

static int sock_write(struct file *filp, const void *buf, uint32_t len)
{
    return -1;
}

static const struct file_operations f_ops_sock = {
    .open = sock_open,
    .close = sock_close,
    .ioctl = sock_ioctl,
    .read = sock_read,
    .write = sock_write,
};

static int sock_bind(struct socket *sock, const struct sockaddr *myaddr, int sockaddr_len)
{
    struct sockaddr_in *addr = (struct sockaddr_in *)myaddr;
    return __bind(sock->fd, addr->sin_port, Sn_MR_ND);
}

static int sock_connect(struct socket *sock, const struct sockaddr *vaddr,
                        int sockaddr_len, int flags)
{
    struct sockaddr_in *addr = (struct sockaddr_in *)vaddr;
    return __connect(sock->fd, (uint8_t *)addr->sin_addr.s_addr, addr->sin_port);
}

static int sock_accept(struct socket *sock, struct socket *newsock, int flags)
{
    return -1;
}

static int sock_listen(struct socket *sock, int backlog)
{
    return __listen(sock->fd);
}

static int sock_shutdown(struct socket *sock, int how)
{
    //return __disconnect(sock->fd);
    return 0;
}

static int sock_recv(struct socket *sock, void *buf, uint32_t len)
{
    uint8_t sr = getSn_SR(sock->fd);
    if (sr == SOCK_CLOSED) {
        return -1;
    } else if (sr == SOCK_INIT) {
        assert(0);
        return -1;
    } else if (sr == SOCK_ESTABLISHED) {
        if (getSn_IR(sock->fd) & Sn_IR_CON)
            setSn_IR(sock->fd, Sn_IR_CON);
        return __recv(sock->fd, buf, len);
    } else if (sr == SOCK_CLOSE_WAIT) {
        return 0;
    }
    assert(0);
    return -1;
}

static int sock_send(struct socket *sock, const void *buf, uint32_t len)
{
    return __send(sock->fd, (uint8_t *)buf, len);
}

static const struct sock_operations sock_sops = {
    .bind = sock_bind,
    .connect = sock_connect,
    .accept = sock_accept,
    .listen = sock_listen,
    .shutdown = sock_shutdown,
    .recv = sock_recv,
    .send = sock_send,
};

struct file *sock_open_file(int fd)
{
    struct socket *sock = calloc(1, sizeof(*sock));
    sock->fd = fd;
    sock->inode.type = INODE_TYPE_SOCKET;
    sock->inode.f_ops = &f_ops_sock;
    sock->s_ops = &sock_sops;
    assert(sock);

    struct file *filp = alloc_file(fd, NULL, &sock->inode);
    assert(filp);
    return filp;
}
