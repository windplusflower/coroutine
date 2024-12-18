
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

typedef int coroutine_t;

void log_set_level_from_env();

int coroutine_init(void (*func)(const void *), const void *arg, size_t stack_size);
void coroutine_resume(int handle);
void coroutine_yield();
void coroutine_join(int handle);
void coroutine_free(int handle);
void coroutine_cancel(int handle);
/*********************************************/

// hook的函数：read,write,send,recv,sendto,recvfrom,accept,aonnect,setsockopt

void enable_hook();
void unable_hook();
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
