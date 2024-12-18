#ifndef COROUTINE_H
#define COROUTINE_H

#include <stdbool.h>
#include <stdlib.h>
#include <sys/ucontext.h>

#include "context.h"

#define STACKSIZE 1024 * 128
#define STACKDEPTH 128
#define TABLESIZE 1024

typedef enum {
    COROUTINE_READY,
    COROUTINE_RUNNING,
    COROUTINE_SUSPENDED,
    COROUTINE_DEAD
} coroutine_status;

typedef struct Coroutine {
    Context context;
    coroutine_status status;
    void (*func)(const void *arg);
    const void *arg;
    char *stack;
    size_t stack_size;
    const char *name;    //用于调试
    bool auto_schedule;  //默认自动调度
    bool in_epoll;       //是否在等待事件
    bool timeout;        //是否因timeout而被唤醒
    int fd;              //当前协程因哪个fd而挂起
    struct epoll_event *event;  //当前协程为了监听哪个事件而挂起；当前协程因为收到哪个事件而被唤醒
    int handle;  //协程对应的句柄
} Coroutine;

typedef struct CoroutineEnv {
    Coroutine **call_stack;
    int size, capacity;
    Coroutine *eventloop_coroutine;
} CoroutineEnv;

//用于存储从句柄到协程的映射
typedef struct CoroutineTable {
    Coroutine **co_table;
    //未被使用的句柄
    int *unused;
    // size表示unused的大小
    int size, capacity;
} CoroutineTable;

__thread static CoroutineEnv ENV;
__thread static CoroutineTable TABLE;

void init_coroutine_table();
int alloc_id();
Coroutine *get_coroutine_by_id(int id);
void free_id(int id);

void start_eventloop();

int coroutine_init(void (*func)(const void *), const void *arg, size_t stack_size);
void coroutine_resume(int handle);
void coroutine_yield();
void coroutine_join(int handle);
void coroutine_free(int handle);
void coroutine_cancel(int handle);
void coroutine_finish();

Coroutine *get_current_coroutine();

#endif
