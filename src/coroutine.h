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

#ifndef COROUTINE_H
#define COROUTINE_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ucontext.h>

#include "context.h"
#include "utils.h"

#define STACKSIZE 1024 * 128
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
    void *(*func)(void *arg);
    void *arg;
    char *stack;
    size_t stack_size;
#ifdef USE_DEBUG
    const char *name;  //用于调试
#endif
    bool is_detached;  //是否分离
    bool timeout;      //是否因timeout而被唤醒
    int fd;            //当前协程因哪个fd而挂起
    struct epoll_event *event;  //当前协程为了监听哪个事件而挂起；当前协程因为收到哪个事件而被唤醒
    void *return_val;             //协程返回值
    struct Coroutine *waited_co;  //等待自己结束的协程，用于join的通知
} Coroutine;

typedef struct CoroutineEnv {
    Coroutine *eventloop_coroutine;
    Coroutine *current_coroutine;
    HandleTable table;
} CoroutineEnv;

__thread static CoroutineEnv ENV;

void init_coroutine_table();
void eventloop_init();

void *coroutine_create(void *(*func)(void *), void *arg, size_t stack_size);
void coroutine_resume(void *handle);
void coroutine_yield();
void *coroutine_join(void *handle);
void coroutine_free(void *handle);

Coroutine *get_current_coroutine();

#endif
