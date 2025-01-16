
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

typedef int coroutine_t;
typedef int co_cond_t;
typedef int co_mutex_t;

coroutine_t coroutine_create(void *(*func)(const void *), const void *arg, size_t stack_size);
void *coroutine_join(coroutine_t handle);
void coroutine_detach(coroutine_t handle);
/***********************************************/
//协程同步

co_cond_t co_cond_alloc();
void co_cond_signal(co_cond_t handle);
void co_cond_broadcast(co_cond_t handle);
bool co_cond_wait(co_cond_t handle, int timeout);  //超时时间单位是毫秒
void co_cond_free(co_cond_t handle);

co_mutex_t co_mutex_alloc();
void co_mutex_lock(co_mutex_t handle);
void co_mutex_unlock(co_mutex_t handle);

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
