#ifndef EPOLL_MANAGER_H
#define EPOLL_MANAGER_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "coroutine.h"

typedef struct EventNode {
    struct EventNode* next;
    coroutine_t* co;
    int fd;  //等待响应的描述符,暂时不用
} EventNode;

typedef struct EventList {
    EventNode* head;
    EventNode* tail;
} EventList;

__thread static EventList* EVENT_LIST = NULL;
__thread static bool main_context_running = true;

void init_eventlist();
bool is_main_running();

EventNode* make_empty_node();
EventNode* make_node(coroutine_t*, int);
EventList* make_empty_list();
void push_back(coroutine_t*, int);
void pop_front(coroutine_t**, int*);
void is_empty();

void event_loop();

#endif