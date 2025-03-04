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

#ifndef event_manager_H
#define event_manager_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "coroutine.h"
#include "utils.h"
typedef struct Mutex Mutex;
typedef struct Cond Cond;
#define EVENTSIZE 1024
#define FDSIZE 1024

typedef struct epoll_event epoll_event;
typedef struct timeval timeval;

typedef struct WaitingPipe{
    int fd[2];
    struct epoll_event* event;
    atomic_bool awakable;
    //目标已经被唤醒时值为false，否则为true
    //避免重复唤醒或无效唤醒，这样可以减少管道读时花费的事件。
}WaitingPipe;

typedef struct EventManager {
    CoList* active_list;
    int epollfd, event_size;
    epoll_event* events;
    CoList* waiting_co[FDSIZE];                          // fd对应的协程列表
    epoll_event* flags[FDSIZE];                          // fd正在监听的事件
    timeval recv_timeout[FDSIZE], send_timeout[FDSIZE];  // fd对应的超时时间
    CoList* locking_co;                                  // 正在等待锁的协程
    Heap* time_heap;
    WaitingPipe* waiting_pipe;
} EventManager;

typedef struct LockPair {
    Coroutine* co;
    Mutex* mutex;
} LockPair;

typedef struct CondPair {
    Coroutine* co;
    int cnt_signal, cnt_broadcast;
    Cond* cond;
} CondPair;

__thread static EventManager EVENT_MANAGER;

void init_eventmanager();
EventManager* get_eventmanager();

bool wait_event(epoll_event* event, unsigned long long timeout);
bool wait_cond(CoNode* node, unsigned long long timeout);
void add_coroutine(Coroutine* co);

void event_loop();

void show_list(CoList* list);

void add_lock_waiting(Mutex* mutex, Coroutine* co);
CoNode* add_cond_waiting(Cond* cond, Coroutine* co);
void awake_cond();
#endif