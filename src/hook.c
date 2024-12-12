#include "hook.h"

#include <sys/epoll.h>
#include <unistd.h>

#include "event_manager.h"
#include "log.h"

ssize_t co_read(int fd, void *buf, size_t nbyte) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return read(fd, buf, nbyte);
    epoll_event event;
    event.data.fd = fd;
    //不保证一次能读全，所以不能ET
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    wait_event(&event, -1);
    return read(fd, buf, nbyte);
}
//写跟读不一样，可能出现目标可写，但是需要写的内容大于容量的情况
//此时如果写对象是管道或套接字，会陷入阻塞
//所以需要使用非阻塞的写操作
ssize_t co_write(int fd, const void *buf, size_t nbyte) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return write(fd, buf, nbyte);
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
    int ret;
    while (1) {
        wait_event(&event, -1);
        fcntl(fd, F_SETFL, flag | O_NONBLOCK);
        ret = write(fd, buf, nbyte);
        //需要改回阻塞
        fcntl(fd, F_SETFL, flag);
        //成功写，则返回
        if (ret >= 0) break;
        //发生了错误，则返回
        if (ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK) break;
        //继续等待fd变为可写
    }
    return ret;
}

ssize_t co_sendto(int fd, const void *buf, size_t n, int flags, const struct sockaddr *addr,
                  socklen_t addrlen) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return sendto(fd, buf, n, flags, addr, addrlen);

    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
    int ret;
    while (1) {
        wait_event(&event, -1);
        fcntl(fd, F_SETFL, flag | O_NONBLOCK);
        ret = sendto(fd, buf, n, flags, addr, addrlen);
        //需要改回阻塞
        fcntl(fd, F_SETFL, flag);
        //成功写，则返回
        if (ret >= 0) break;
        //发生了错误，则返回
        if (ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK) break;
        //继续等待fd变为可写
    }
    return ret;
}

ssize_t co_recvfrom(int fd, void *buf, size_t n, int flags, struct sockaddr *addr,
                    socklen_t *addrlen) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return recvfrom(fd, buf, n, flags, addr, addrlen);
    epoll_event event;
    event.data.fd = fd;
    //不保证一次能读全，所以不能ET
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    wait_event(&event, -1);
    return recvfrom(fd, buf, n, flags, addr, addrlen);
}

ssize_t co_send(int fd, const void *buf, size_t n, int flags) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return send(fd, buf, n, flags);

    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
    int ret;
    while (1) {
        wait_event(&event, -1);
        fcntl(fd, F_SETFL, flag | O_NONBLOCK);
        ret = send(fd, buf, n, flags);
        //需要改回阻塞
        fcntl(fd, F_SETFL, flag);
        //成功写，则返回
        if (ret >= 0) break;
        //发生了错误，则返回
        if (ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK) break;
        //继续等待fd变为可写
    }
    return ret;
}

ssize_t co_recv(int fd, void *buf, size_t n, int flags) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return recv(fd, buf, n, flags);
    epoll_event event;
    event.data.fd = fd;
    //不保证一次能读全，所以不能ET
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    wait_event(&event, -1);
    return recv(fd, buf, n, flags);
}

int co_accept(int fd, struct sockaddr *addr, socklen_t *addrlen) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return accept(fd, addr, addrlen);
    epoll_event event;
    event.data.fd = fd;
    //不保证一次能读全，所以不能ET
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    wait_event(&event, -1);
    return accept(fd, addr, addrlen);
}
