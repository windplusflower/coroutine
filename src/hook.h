/*
 * Copyright (c) 2025 windflower
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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