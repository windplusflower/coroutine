#include "hook.h"

#include <dlfcn.h>
#include <errno.h>
#include <sys/epoll.h>

#include "event_manager.h"
#include "log.h"
#include "utils.h"
/*****************************************************************************/
typedef ssize_t (*read_t)(int fildes, void *buf, size_t nbyte);
typedef ssize_t (*write_t)(int fildes, const void *buf, size_t nbyte);

typedef ssize_t (*sendto_t)(int socket, const void *message, size_t length, int flags,
                            const struct sockaddr *dest_addr, socklen_t dest_len);

typedef ssize_t (*recvfrom_t)(int socket, void *buffer, size_t length, int flags,
                              struct sockaddr *address, socklen_t *address_len);

typedef ssize_t (*send_t)(int socket, const void *buffer, size_t length, int flags);
typedef ssize_t (*recv_t)(int socket, void *buffer, size_t length, int flags);

typedef int (*accept_t)(int fd, struct sockaddr *addr, socklen_t *addrlen);
typedef int (*connect_t)(int socket, const struct sockaddr *address, socklen_t address_len);
typedef int (*setsockopt_t)(int socket, int level, int option_name, const void *option_value,
                            socklen_t option_len);
typedef int (*poll_t)(struct pollfd fds[], nfds_t nfds, int timeout);

typedef void (*sleep_t)(unsigned int seconds);
typedef void (*usleep_t)(useconds_t useconds);
/**********************************************************************************/
static read_t sys_read;
static write_t sys_write;

static sendto_t sys_sendto;
static recvfrom_t sys_recvfrom;

static send_t sys_send;
static recv_t sys_recv;

static accept_t sys_accept;
static connect_t sys_connect;
static setsockopt_t sys_setsockopt;
static poll_t sys_poll;

static sleep_t sys_sleep;
static usleep_t sys_usleep;
/***********************************************************************************/

__thread static bool is_hooked = false;

void init_hook() {
    static bool has_inited = false;
    if (has_inited) return;
    has_inited = true;

    sys_read = (read_t)dlsym(RTLD_NEXT, "read");
    sys_write = (write_t)dlsym(RTLD_NEXT, "write");

    sys_sendto = (sendto_t)dlsym(RTLD_NEXT, "sendto");
    sys_recvfrom = (recvfrom_t)dlsym(RTLD_NEXT, "recvfrom");

    sys_send = (send_t)dlsym(RTLD_NEXT, "send");
    sys_recv = (recv_t)dlsym(RTLD_NEXT, "recv");

    sys_accept = (accept_t)dlsym(RTLD_NEXT, "accept");
    sys_connect = (connect_t)dlsym(RTLD_NEXT, "connect");
    sys_setsockopt = (setsockopt_t)dlsym(RTLD_NEXT, "setsockopt");
    sys_poll = (poll_t)dlsym(RTLD_NEXT, "poll");

    sys_sleep = (sleep_t)dlsym(RTLD_NEXT, "sleep");
    sys_usleep = (usleep_t)dlsym(RTLD_NEXT, "usleep");
}

void enable_hook() {
    is_hooked = true;
}

void unable_hook() {
    is_hooked = false;
}

bool is_hook_enabled() {
    return is_hooked;
}
ssize_t co_read(int fd, void *buf, size_t nbyte) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return sys_read(fd, buf, nbyte);
    epoll_event event;
    event.data.fd = fd;
    //不保证一次能读全，所以不能ET
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    //如果fd不是套接字，那么get_timeout会返回-1，下面几个函数同理
    if (!wait_event(&event, get_timeout(&get_eventmanager()->recv_timeout[fd]))) {
        errno = EAGAIN;
        return -1;
    }
    return sys_read(fd, buf, nbyte);
}
//写跟读不一样，可能出现目标可写，但是需要写的内容大于容量的情况
//此时如果写对象是管道或套接字，会陷入阻塞
//所以需要使用非阻塞的写操作
ssize_t co_write(int fd, const void *buf, size_t nbyte) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return sys_write(fd, buf, nbyte);
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
    int ret;
    while (1) {
        if (!wait_event(&event, get_timeout(&get_eventmanager()->send_timeout[fd]))) {
            errno = EAGAIN;
            return -1;
        }
        fcntl(fd, F_SETFL, flag | O_NONBLOCK);
        ret = sys_write(fd, buf, nbyte);
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
    if (flag & O_NONBLOCK) return sys_sendto(fd, buf, n, flags, addr, addrlen);

    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
    int ret;
    while (1) {
        if (!wait_event(&event, get_timeout(&get_eventmanager()->send_timeout[fd]))) {
            errno = EAGAIN;
            return -1;
        }
        fcntl(fd, F_SETFL, flag | O_NONBLOCK);
        ret = sys_sendto(fd, buf, n, flags, addr, addrlen);
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
    if (flag & O_NONBLOCK) return sys_recvfrom(fd, buf, n, flags, addr, addrlen);
    epoll_event event;
    event.data.fd = fd;
    //不保证一次能读全，所以不能ET
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    if (!wait_event(&event, get_timeout(&get_eventmanager()->recv_timeout[fd]))) {
        errno = EAGAIN;
        return -1;
    }
    return sys_recvfrom(fd, buf, n, flags, addr, addrlen);
}

ssize_t co_send(int fd, const void *buf, size_t n, int flags) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return sys_send(fd, buf, n, flags);

    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
    int ret;
    while (1) {
        if (!wait_event(&event, get_timeout(&get_eventmanager()->send_timeout[fd]))) {
            errno = EAGAIN;
            return -1;
        }
        fcntl(fd, F_SETFL, flag | O_NONBLOCK);
        ret = sys_send(fd, buf, n, flags);
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
    if (flag & O_NONBLOCK) return sys_recv(fd, buf, n, flags);
    epoll_event event;
    event.data.fd = fd;
    //不保证一次能读全，所以不能ET
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    if (!wait_event(&event, get_timeout(&get_eventmanager()->recv_timeout[fd]))) {
        errno = EAGAIN;
        return -1;
    }
    return sys_recv(fd, buf, n, flags);
}

int co_accept(int fd, struct sockaddr *addr, socklen_t *addrlen) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return sys_accept(fd, addr, addrlen);
    epoll_event event;
    event.data.fd = fd;
    //不保证一次能读全，所以不能ET
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    // accept不受setsockopt设置的超时时间影响
    wait_event(&event, -1);
    return sys_accept(fd, addr, addrlen);
}

int co_connect(int fd, const struct sockaddr *address, socklen_t address_len) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return sys_connect(fd, address, address_len);

    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    int ret = sys_connect(fd, address, address_len);
    fcntl(fd, F_SETFL, flag);

    if (!(ret < 0 && errno == EINPROGRESS)) {
        return ret;
    }

    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
    // linux默认超时时间为75s
    if (!wait_event(&event, 75000)) {
        //超时
        errno = ETIMEDOUT;
        return -1;
    }
    //这里使用阻塞的connect，因为EPOLLOUT | EPOLLERR | EPOLLHUP
    //时不会阻塞，并且能给用户正确的返回值和errno
    ret = sys_connect(fd, address, address_len);
    //连接成功
    if (ret == -1 && errno == EISCONN) return 0;
    //出现错误
    return ret;
}

int co_setsockopt(int fd, int level, int option_name, const void *option_value,
                  socklen_t option_len) {
    int res = sys_setsockopt(fd, level, option_name, option_value, option_len);
    //操作失败的话就不需要保存超时信息
    EventManager *event_manager = get_eventmanager();
    if (res) return res;
    if (SOL_SOCKET == level) {
        struct timeval *val = (struct timeval *)option_value;

#ifdef USE_DEBUG
        log_debug("fd %d:set time out %d", fd, get_timeout(val) + 1);
#endif
        if (SO_RCVTIMEO == option_name) {
            set_timeout(&event_manager->recv_timeout[fd], (struct timeval *)option_value);
        } else if (SO_SNDTIMEO == option_name) {
            set_timeout(&event_manager->send_timeout[fd], (struct timeval *)option_value);
        }
    }
    return res;
}

unsigned int co_sleep(unsigned int seconds) {
    wait_event(NULL, seconds * 1000);
    return 0;
}
int co_usleep(useconds_t useconds) {
    wait_event(NULL, useconds / 1000);
    return 0;
}

/***********************************************************************************/
ssize_t read(int fd, void *buf, size_t nbyte) {
    init_hook();
    if (is_hook_enabled())
        return co_read(fd, buf, nbyte);
    else
        return sys_read(fd, buf, nbyte);
}
ssize_t write(int fd, const void *buf, size_t nbyte) {
    init_hook();
    if (is_hook_enabled())
        return co_write(fd, buf, nbyte);
    else
        return sys_write(fd, buf, nbyte);
}

ssize_t sendto(int fd, const void *buf, size_t n, int flags, const struct sockaddr *addr,
               socklen_t addrlen) {
    init_hook();
    if (is_hook_enabled())
        return co_sendto(fd, buf, n, flags, addr, addrlen);
    else
        return sys_sendto(fd, buf, n, flags, addr, addrlen);
}

ssize_t recvfrom(int fd, void *buf, size_t n, int flags, struct sockaddr *addr,
                 socklen_t *addrlen) {
    init_hook();
    if (is_hook_enabled())
        return co_recvfrom(fd, buf, n, flags, addr, addrlen);
    else
        return sys_recvfrom(fd, buf, n, flags, addr, addrlen);
}

ssize_t send(int fd, const void *buf, size_t n, int flags) {
    init_hook();
    if (is_hook_enabled())
        return co_send(fd, buf, n, flags);
    else
        return sys_send(fd, buf, n, flags);
}

ssize_t recv(int fd, void *buf, size_t n, int flags) {
    init_hook();
    if (is_hook_enabled())
        return co_recv(fd, buf, n, flags);
    else
        return sys_recv(fd, buf, n, flags);
}

int accept(int fd, struct sockaddr *addr, socklen_t *addrlen) {
    init_hook();
    if (is_hook_enabled())
        return co_accept(fd, addr, addrlen);
    else
        return sys_accept(fd, addr, addrlen);
}

int connect(int fd, const struct sockaddr *address, socklen_t address_len) {
    init_hook();
    if (is_hook_enabled())
        return co_connect(fd, address, address_len);
    else
        return sys_connect(fd, address, address_len);
}

int setsockopt(int fd, int level, int option_name, const void *option_value, socklen_t option_len) {
    init_hook();
    if (is_hook_enabled())
        return co_setsockopt(fd, level, option_name, option_value, option_len);
    else
        return sys_setsockopt(fd, level, option_name, option_value, option_len);
}

unsigned int sleep(unsigned int seconds) {
    init_hook();
    if (is_hook_enabled())
        co_sleep(seconds);
    else
        sys_sleep(seconds);
}

int usleep(useconds_t useconds) {
    init_hook();
    if (is_hook_enabled())
        co_usleep(useconds);
    else
        sys_usleep(useconds);
}
