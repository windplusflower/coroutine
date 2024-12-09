#ifndef event_manager_H
#define event_manager_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "coroutine.h"
#define EVENTSIZE 1024

typedef struct EventNode {
    struct EventNode* next;
    Coroutine* co;
} EventNode;

typedef struct EventList {
    EventNode* head;
    EventNode* tail;
} EventList;

typedef struct epoll_event epoll_event;
typedef struct EventManager {
    EventList* active_list;
    int epollfd, event_size;
    epoll_event* events;
    EventList* waiting_co[1024];  // fd对应的协程列表
    epoll_event* flags[1024];     // fd正在监听的事件
} EventManager;

__thread static EventManager EVENT_MANAGER;

void init_eventmanager();

EventNode* make_empty_node();
EventNode* make_node(Coroutine*);
void push_back(EventList*, Coroutine*);
Coroutine* pop_front(EventList*);
bool is_emptylist(EventList*);
void show_list(EventList*);

void wait_event(epoll_event*, unsigned int);
void add_coroutine(Coroutine* co);

void event_loop();

#endif