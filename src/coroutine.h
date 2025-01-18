#ifndef COROUTINE_H
#define COROUTINE_H

#include <stdbool.h>
#include <stdlib.h>
#include <sys/ucontext.h>

#include "context.h"
#include "utils.h"

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
    void *(*func)(const void *arg);
    const void *arg;
    char *stack;
    size_t stack_size;
#ifdef USE_DEBUG
    const char *name;  //用于调试
#endif
    bool is_detached;  //是否分离
    bool timeout;      //是否因timeout而被唤醒
    int fd;            //当前协程因哪个fd而挂起
    struct epoll_event *event;  //当前协程为了监听哪个事件而挂起；当前协程因为收到哪个事件而被唤醒
    //    int handle;                   //协程对应的句柄
    void *return_val;             //协程返回值
    struct Coroutine *waited_co;  //等待自己结束的协程，用于join的通知
} Coroutine;

// //用于存储从句柄到协程的映射
// typedef struct CoroutineTable {
//     Coroutine **co_table;
//     //未被使用的句柄
//     int *unused;
//     // size表示unused的大小
//     int size, capacity;
// } CoroutineTable;

typedef struct CoroutineEnv {
    Coroutine *eventloop_coroutine;
    Coroutine *current_coroutine;
    HandleTable table;
} CoroutineEnv;

__thread static CoroutineEnv ENV;

void init_coroutine_table();
void eventloop_init();

void *coroutine_create(void *(*func)(const void *), const void *arg, size_t stack_size);
void coroutine_resume(void *handle);
void coroutine_yield();
void *coroutine_join(void *handle);
void coroutine_free(void *handle);

Coroutine *get_current_coroutine();

#endif
