#ifndef COROUTINE_H
#define COROUTINE_H

#include "context.h"
#define STACKSIZE 1024 * 128

#include <stdbool.h>
#include <stdlib.h>
#include <sys/ucontext.h>

typedef enum {
    COROUTINE_READY,
    COROUTINE_RUNNING,
    COROUTINE_SUSPENDED,
    COROUTINE_DEAD
} coroutine_status;

typedef struct Coroutine {
    Context context;
    coroutine_status status;
    void (*func)(void *arg);
    void *arg;
    char *stack;
    size_t stack_size;
    const char *name;    //用于调试
    bool auto_schedule;  //默认自动调度，由用户resume的话会改为手动调度
    bool in_epoll;       //是否在等待事件
    int fd;              //当前协程因哪个fd而挂起
    struct epoll_event *event;  //当前协程为了监听哪个事件而挂起；当前协程因为收到哪个事件而被唤醒
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
Coroutine *get_current_coroutine();

#endif
