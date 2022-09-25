#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <fs/fs.h>
#include <sys/select.h>
#include <syscall/syscall.h>
#include <sys/socket.h>

int sys_socket(int domain, int type, int protocol)
{
    // only support tcp ...
    assert(domain == PF_INET);
    assert(type == SOCK_STREAM);
    //assert(protocol == 0);

    for (int i = 1; i < TASK_FILES; i++) {
        if (current->files[i] == NULL) {
            struct file *filp = sock_open_file(i);
            assert(filp->f_ops->open);
            filp->f_ops->open(filp);
            current->files[i] = filp;
            return i;
        }
    }

    errno = ENOENT;
    return -1;
}

int sys_bind(int socket, const struct sockaddr *address, socklen_t address_len)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp && filp->fd == socket) {
            struct socket *sock =
                container_of(filp->inode, struct socket, inode);
            assert(sock->s_ops->bind);
            return sock->s_ops->bind(sock, address, address_len);
        }
    }

    errno = EBADF;
    return -1;
}

int sys_connect(int socket, const struct sockaddr *address, socklen_t address_len)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp && filp->fd == socket) {
            struct socket *sock =
                container_of(filp->inode, struct socket, inode);
            assert(sock->s_ops->connect);
            return sock->s_ops->connect(sock, address, address_len, 0);
        }
    }

    errno = EBADF;
    return -1;
}

int sys_accept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp && filp->fd == socket) {
            struct socket *sock =
                container_of(filp->inode, struct socket, inode);
            assert(sock->s_ops->accept);
            struct socket newsock = {0};
            int rc = sock->s_ops->accept(sock, &newsock, 0);
            // TODO: set address & address_len
            return rc;
        }
    }

    errno = EBADF;
    return -1;
}

int sys_listen(int socket, int backlog)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp && filp->fd == socket) {
            struct socket *sock =
                container_of(filp->inode, struct socket, inode);
            assert(sock->s_ops->listen);
            return sock->s_ops->listen(sock, backlog);
        }
    }

    errno = EBADF;
    return -1;
}

int sys_shutdown(int socket, int how)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp && filp->fd == socket) {
            struct socket *sock =
                container_of(filp->inode, struct socket, inode);
            assert(sock->s_ops->shutdown);
            return sock->s_ops->shutdown(sock, how);
        }
    }

    errno = EBADF;
    return -1;
}

int sys_recv(int socket, char *buf, int len)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp && filp->fd == socket) {
            struct socket *sock =
                container_of(filp->inode, struct socket, inode);
            assert(sock->s_ops->recv);
            return sock->s_ops->recv(sock, buf, len);
        }
    }

    errno = EBADF;
    return -1;
}

int sys_send(int socket, char *buf, int len)
{
    for (int i = 1; i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp && filp->fd == socket) {
            struct socket *sock =
                container_of(filp->inode, struct socket, inode);
            assert(sock->s_ops->send);
            return sock->s_ops->send(sock, buf, len);
        }
    }

    errno = EBADF;
    return -1;
}

#include "wizchip_socket.h"
int sys_select(int nfds, fd_set *restrict readfds,
               fd_set *restrict writefds, fd_set *restrict exceptfds,
               struct timeval *restrict timeout)
{
    int rc = 0;

    for (int i = 1; nfds && i < TASK_FILES; i++) {
        struct file *filp = current->files[i];
        if (filp && FD_ISSET(filp->fd, readfds)) {
            nfds--;
            uint8_t sr = getSn_SR(filp->fd);
            if (sr == SOCK_CLOSED) {
                rc++;
            } else if (sr == SOCK_INIT) {
                rc++;
            } else if (sr == SOCK_ESTABLISHED) {
                uint16_t len = getSn_RX_RSR(filp->fd);
                if (len) rc++;
            } else if (sr == SOCK_CLOSE_WAIT) {
                rc++;
            }
        }
    }

    return rc;
}

void net_sys_init(void)
{
    syscall_table[SYS_SOCKET] = sys_socket;
    syscall_table[SYS_BIND] = sys_bind;
    syscall_table[SYS_CONNECT] = sys_connect;
    syscall_table[SYS_ACCEPT] = sys_accept;
    syscall_table[SYS_LISTEN] = sys_listen;
    syscall_table[SYS_SHUTDOWN] = sys_shutdown;
    syscall_table[SYS_RECV] = sys_recv;
    syscall_table[SYS_SEND] = sys_send;
    //syscall_table[SYS_SENDTO] = sys_setsockopt;
    //syscall_table[SYS_RECVFROM] = sys_getsockopt;
    syscall_table[SYS_SELECT] = sys_select;
}
