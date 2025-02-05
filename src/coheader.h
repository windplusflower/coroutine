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


#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

typedef void *coroutine_t;
typedef void *co_cond_t;
typedef void *co_mutex_t;

coroutine_t coroutine_create(void *(*func)(const void *), const void *arg, size_t stack_size);
void *coroutine_join(coroutine_t handle);
void coroutine_detach(coroutine_t handle);
/***********************************************/
//协程同步

co_cond_t co_cond_alloc();
void co_cond_signal(co_cond_t handle);
void co_cond_broadcast(co_cond_t handle);
int co_cond_wait(co_cond_t cond_handle, co_mutex_t mutex_handle);
int co_cond_timewait(co_cond_t cond_handle, co_mutex_t mutex_handle,
                     int timeout);  //超时时间单位是毫秒
int co_cond_free(co_cond_t handle);

co_mutex_t co_mutex_alloc();
void co_mutex_lock(co_mutex_t handle);
void co_mutex_unlock(co_mutex_t handle);
int co_mutex_trylock(co_mutex_t handle);
int co_mutex_free(co_mutex_t handle);

/*********************************************/

// hook的函数：read,write,send,recv,sendto,recvfrom,accept,aonnect,setsockopt,sleep,usleep

void enable_hook();
void disable_hook();
bool is_hook_enabled();

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
