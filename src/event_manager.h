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

typedef struct EventNode {
    struct EventNode* next;
    Coroutine* co;
    unsigned char free_times;  //释放次数，超时的节点被两个队列持有，需要释放两次
    bool valid;                //是否是有效节点，用于超时机制
} EventNode;

typedef struct EventList {
    EventNode* head;
    EventNode* tail;
} EventList;

typedef struct epoll_event epoll_event;
typedef struct timeval timeval;
typedef struct EventManager {
    EventList* active_list;
    int epollfd, event_size;
    epoll_event* events;
    EventList* waiting_co[FDSIZE];                       // fd对应的协程列表
    epoll_event* flags[FDSIZE];                          // fd正在监听的事件
    timeval recv_timeout[FDSIZE], send_timeout[FDSIZE];  // fd对应的超时时间
    Heap* time_heap;
} EventManager;

__thread static EventManager EVENT_MANAGER;

void init_eventmanager();
EventManager* get_eventmanager();

bool wait_event(epoll_event* event, int timeout);
void add_coroutine(Coroutine* co);

void event_loop();

EventList* make_empty_list();
EventNode* make_node(Coroutine* co);
void free_node(EventNode* node);
EventNode* push_back(EventList* list, Coroutine* co);
Coroutine* pop_front(EventList* list);
void remove_next(EventList* list, EventNode* node);
bool is_emptylist(EventList* list);
void show_list(EventList* list);

#endif