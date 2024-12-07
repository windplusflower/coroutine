#include "event_manager.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

#include "coroutine.h"
#include "log.h"
#include "utils.h"

void init_eventmanager() {
    EVENT_MANAGER.active_list = (EventList*)malloc(sizeof(EventList));
    EVENT_MANAGER.active_list->head = (EventNode*)calloc(1, sizeof(EventNode));
    EVENT_MANAGER.active_list->tail = EVENT_MANAGER.active_list->head;
    EVENT_MANAGER.epollfd = epoll_create1(0);
    EVENT_MANAGER.event_size = EVENTSIZE;
    EVENT_MANAGER.events = (epoll_event*)malloc(EVENTSIZE * sizeof(epoll_event));
}

EventNode* make_node(Coroutine* co) {
    EventNode* res = (EventNode*)calloc(1, sizeof(EventNode));
    res->co = co;
    return res;
}

//不需要释放co指向的内存，因为会转移这块内存的所有权，它之后还要用到，在协程结束时才由调度器销毁。
void free_node(EventNode* node) {
    free(node);
}

void push_back(EventList* list, Coroutine* co) {
    EventNode* node = make_node(co);
    list->tail->next = node;
    list->tail = node;
}

Coroutine* pop_front(EventList* list) {
    assert(EVENT_LIST->head->next != NULL);
    EventNode* node = list->head->next;
    list->head->next = node->next;
    Coroutine* co = node->co;
    if (list->head->next == NULL) list->tail = list->head;
    free_node(node);
    return co;
}

bool is_emptylist(EventList* list) {
    return list->head == list->tail;
}

void show_list(EventList* list) {
    EventNode* p = list->head;
    char buf[1024];
    buf[0] = '\0';
    while (p->next) {
        p = p->next;
        strcat(buf, p->co->name);
        strcat(buf, " ");
    }
    log_debug("event_list: %s", buf);
}

//事件相关的协程；监听的事件；超时时间（毫秒）
//这个函数只由库内的函数调用，保证event会传fd
// co为NULL表示为当前协程添加事件。
void add_event(Coroutine* co, epoll_event* event, unsigned int timeout) {
    if (co == NULL) co = get_current_coroutine();
    if (event == NULL || timeout == 0) {
        push_back(EVENT_MANAGER.active_list, co);
        return;
    }
    int fd = event->data.fd;
    event->data.ptr = co;
    epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_ADD, fd, event);
    coroutine_yield();
}

// active_list为空时调用
//意味着此时所有的协程都处于等待IO状态
//因此可以阻塞直到有可运行的协程加入运行队列
void awake() {
    int nfds = epoll_wait(EVENT_MANAGER.epollfd, EVENT_MANAGER.events, EVENT_MANAGER.event_size, -1);
    epoll_event* event = EVENT_MANAGER.events;
    for (int i = 0; i < nfds; i++) {
        add_event(event[i].data.ptr, NULL, 0);
    }
}
void event_loop() {
    while (1) {
        show_list(EVENT_MANAGER.active_list);
        if (is_emptylist(EVENT_MANAGER.active_list)) awake();
        Coroutine* co = pop_front(EVENT_MANAGER.active_list);
        if (co->status == COROUTINE_DEAD) {
            coroutine_free(co);
            continue;
        }
        if (co->auto_schedule == false) continue;
        coroutine_resume(co);
    }
}