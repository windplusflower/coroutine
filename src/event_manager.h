#ifndef event_manager_H
#define event_manager_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "coroutine.h"
#include "utils.h"
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
    Heap* time_heap;
} EventManager;

__thread static EventManager EVENT_MANAGER;

void init_eventmanager();
EventManager* get_eventmanager();

bool wait_event(epoll_event* event, unsigned long long timeout);
bool wait_cond(CoNode* node, unsigned long long timeout);
void add_coroutine(Coroutine* co);

void event_loop();

void show_list(CoList* list);
#endif