#ifndef EPOLL_MANAGER_H
#define EPOLL_MANAGER_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "coroutine.h"

typedef struct EventNode {
    struct EventNode* next;
    Coroutine* co;
    int fd;  //等待响应的描述符,暂时不用
} EventNode;

typedef struct EventList {
    EventNode* head;
    EventNode* tail;
} EventList;

__thread static EventList* EVENT_LIST = NULL;
__thread static bool started = false;

void init_eventlist();
bool is_main_running();

EventNode* make_empty_node();
EventNode* make_node(Coroutine*, int);
EventList* make_empty_list();
void push_back(Coroutine*, int);
void pop_front(Coroutine**, int*);
void is_empty();

void event_loop();

#endif