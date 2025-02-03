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
typedef struct EventManager {
    CoList* active_list;
    int epollfd, event_size;
    epoll_event* events;
    CoList* waiting_co[FDSIZE];                          // fd对应的协程列表
    epoll_event* flags[FDSIZE];                          // fd正在监听的事件
    timeval recv_timeout[FDSIZE], send_timeout[FDSIZE];  // fd对应的超时时间
    CoList* locking_co;                                  // 正在等待锁的协程
    CoList* cond_co;                                     // 等待中的条件变量
    Heap *epoll_time_heap, *cond_time_heap;
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