#ifndef COROUTINE_H
#define COROUTINE_H

#define STACKSIZE 1024 * 8

#include <stdlib.h>
#include <sys/ucontext.h>
#include <ucontext.h>

typedef enum {
    COROUTINE_READY,
    COROUTINE_RUNNING,
    COROUTINE_SUSPENDED,
    COROUTINE_DEAD
} coroutine_status;

typedef struct coroutine {
    ucontext_t context;
    coroutine_status status;
    void (*func)(void *arg);
    void *arg;
    char *stack;
    size_t stack_size;
} coroutine_t;

static coroutine_t *running_coroutine = NULL, *main_coroutine = NULL, *epoll_coroutine = NULL;

void start_eventloop();
coroutine_t *get_running_coroutine();
void coroutine_init(coroutine_t *co, void (*func)(void *), void *arg, size_t stack_size);
void coroutine_resume(coroutine_t *co);
void coroutine_yield();
void coroutine_free(coroutine_t *co);

#endif
