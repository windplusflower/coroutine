#include "hook.h"

#include <sys/epoll.h>

#include "event_manager.h"

ssize_t co_read(int fd, void *buf, size_t nbyte) {
    int flag = fcntl(fd, F_GETFL);
    if (flag & O_NONBLOCK) return read(fd, buf, nbyte);
    epoll_event event;
    event.data.fd = fd;
    //不保证一次能读全，所以不能ET
    event.events = EPOLLIN;
    wait_event(&event, -1);
    return read(fd, buf, nbyte);
}
ssize_t co_write(int fd, const void *buf, size_t nbyte) {
    return 0;
}
