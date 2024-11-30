#include "epoll_manager.h"

#include <stdio.h>

#include "coroutine.h"
#include "utils.h"

void init_eventlist() {
    if (EVENT_LIST == NULL) EVENT_LIST = make_empty_list();
}

//是否是第一次运行协程
bool is_coroutine_started() {
    return started;
}
//标志协程已经开始
void set_coroutine_stared() {
    started = true;
}
//协程无法主动结束，所以没有结束标志

//暂时用先进先出队列实现一版对称式协程调度
EventNode* make_empty_node() {
    EventNode* res = (EventNode*)calloc(1, sizeof(EventNode));
    res->fd = -1;
    return res;
}

EventNode* make_node(Coroutine* co, int fd) {
    EventNode* res = make_empty_node();
    res->co = co;
    res->fd = fd;
    return res;
}

//不需要释放co指向的内存，因为会转移这块内存的所有权，它之后还要用到，在协程结束时才由调度器销毁。
void free_node(EventNode* node) {
    free(node);
}

EventList* make_empty_list() {
    EventList* res = calloc(1, sizeof(EventList));
    res->head = calloc(1, sizeof(EventNode));
    res->tail = res->head;
    return res;
}

void push_back(Coroutine* co, int fd) {
    EventNode* node = make_node(co, fd);
    EVENT_LIST->tail->next = node;
    EVENT_LIST->tail = node;
}

void pop_front(Coroutine** co, int* fd) {
    assert(EVENT_LIST->head->next != NULL);
    EventNode* node = EVENT_LIST->head->next;
    EVENT_LIST->head->next = node->next;
    *co = node->co;
    *fd = node->fd;
    if (EVENT_LIST->head->next == NULL) EVENT_LIST->tail = EVENT_LIST->head;
    free_node(node);
}
Coroutine* seek_front() {
    assert(EVENT_LIST->head->next != NULL);
    return EVENT_LIST->head->next->co;
}

bool list_is_empty() {
    return EVENT_LIST->head == EVENT_LIST->tail;
}

void event_loop() {
    while (1) {
        Coroutine* co;
        int fd;
        pop_front(&co, &fd);
        //只有程序运行结束时来到eventloop才会是running状态。
        if (co->status == COROUTINE_RUNNING) {
            continue;
        }
        coroutine_resume(co);
    }
}