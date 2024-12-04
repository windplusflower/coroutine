#ifndef COROUTINE_H
#define COROUTINE_H

#define STACKSIZE 1024 * 128

#include <stdbool.h>
#include <stdlib.h>
#include <sys/ucontext.h>
#include <ucontext.h>

typedef enum {
    COROUTINE_READY,
    COROUTINE_RUNNING,
    COROUTINE_SUSPENDED,
    COROUTINE_DEAD
} coroutine_status;

typedef struct Coroutine {
    ucontext_t context;
    coroutine_status status;
    void (*func)(void *arg);
    void *arg;
    char *stack;
    const char *name;
    bool auto_schedule;  //默认自动调度，由用户resume的话会改为手动调度
    size_t stack_size;
} Coroutine;

typedef struct CoroutineEnv {
    Coroutine *call_stack[128];
    int size;
    Coroutine *eventloop_coroutine;
} CoroutineEnv;

typedef struct WrappedArg {
    void (*func)(void *);
    void *arg;
} WrappedArg;

__thread static CoroutineEnv ENV;

void show_call_stack();

void start_eventloop();

void env_push(Coroutine *co);
Coroutine *env_pop();

void coroutine_init(Coroutine *co, void (*func)(void *), void *arg, size_t stack_size);
void coroutine_resume(Coroutine *co);
void coroutine_yield();
void coroutine_free(Coroutine *co);

#endif
