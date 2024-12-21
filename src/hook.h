#ifndef HOOK_H
#define HOOK_H

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

void enable_hook();
void disable_hook();
bool is_hook_enabled();
void init_hook();
/********************************************************************************************/
ssize_t co_read(int fd, void *buf, size_t nbyte);
ssize_t co_write(int fd, const void *buf, size_t nbyte);
ssize_t co_sendto(int fd, const void *buf, size_t n, int flags, const struct sockaddr *addr,
                  socklen_t addrlen);
ssize_t co_recvfrom(int fd, void *buf, size_t n, int flags, struct sockaddr *addr,
                    socklen_t *addrlen);
ssize_t co_send(int fd, const void *buf, size_t n, int flags);
ssize_t co_recv(int fd, void *buf, size_t n, int flags);
int co_accept(int fd, struct sockaddr *addr, socklen_t *addrlen);
int co_connect(int fd, const struct sockaddr *address, socklen_t address_len);
int co_setsockopt(int fd, int level, int option_name, const void *option_value,
                  socklen_t option_len);
unsigned int co_sleep(unsigned int seconds);
int co_usleep(useconds_t useconds);

#endif