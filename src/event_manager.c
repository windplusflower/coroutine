#include "event_manager.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

#include "coroutine.h"
#include "log.h"
#include "utils.h"

EventList* make_empty_list() {
    EventList* res = (EventList*)malloc(sizeof(EventList));
    res->head = (EventNode*)calloc(1, sizeof(EventNode));
    res->tail = res->head;
    return res;
}

void init_eventmanager() {
    EVENT_MANAGER.active_list = make_empty_list();
    EVENT_MANAGER.epollfd = epoll_create1(0);
    EVENT_MANAGER.event_size = EVENTSIZE;
    EVENT_MANAGER.events = (epoll_event*)malloc(EVENTSIZE * sizeof(epoll_event));
    log_debug("Event manager init finished");
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
    assert(list->head->next != NULL);
    EventNode* node = list->head->next;
    list->head->next = node->next;
    Coroutine* co = node->co;
    if (list->head->next == NULL) list->tail = list->head;
    free_node(node);
    return co;
}

void remove_next(EventList* list, EventNode* node) {
    assert(node->next != NULL);
    EventNode* tmp = node->next;
    node->next = tmp->next;
    free_node(tmp);
    if (node->next == NULL) list->tail = node;
}

bool is_emptylist(EventList* list) {
    return list == NULL || list->head == list->tail;
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

void show_epoll() {
    log_debug("****************epoll****************");
    for (int i = 0; i < 1024; i++) {
        if (EVENT_MANAGER.waiting_co[i] == NULL) continue;
        log_debug("fd: %d", i);
        show_list(EVENT_MANAGER.waiting_co[i]);
    }
    log_debug("***************epoll****************");
}

//将协程添加到准备队列
void add_coroutine(Coroutine* co) {
    push_back(EVENT_MANAGER.active_list, co);
}

//加入epoll,合并不同协程对相同fd的等待
void push_in_epoll(Coroutine* co) {
    co->in_epoll = true;
    int fd = co->fd;
    //初次访问，分配内存
    if (EVENT_MANAGER.waiting_co[fd] == NULL) {
        EVENT_MANAGER.waiting_co[fd] = make_empty_list();
        EVENT_MANAGER.flags[fd] = (epoll_event*)malloc(sizeof(epoll_event));
    }
    //第一次监听该fd,直接添加进epoll。
    if (is_emptylist(EVENT_MANAGER.waiting_co[fd])) {
        push_back(EVENT_MANAGER.waiting_co[fd], co);
        EVENT_MANAGER.flags[fd]->events = co->event->events;
        EVENT_MANAGER.flags[fd]->data.fd = fd;
        epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_ADD, fd, EVENT_MANAGER.flags[fd]);
        return;
    }

    push_back(EVENT_MANAGER.waiting_co[fd], co);
    //已经监听了fd,且需要监听的事件是已监听事件的子集，无需操作epoll
    if ((EVENT_MANAGER.flags[fd]->events & co->event->events) == co->event->events) return;
    //剩下的就是已经监听了fd,且需要监听的事件至少有一部分没有监听
    EVENT_MANAGER.flags[fd]->events |= co->event->events;
    epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_MOD, fd, EVENT_MANAGER.flags[fd]);
}

//参数分别为：等待的事件；超时时间（毫秒）
//这个函数只由库内的函数调用，保证event会传fd
//超时相关暂未实现
void wait_event(epoll_event* event, unsigned int timeout) {
    Coroutine* co = get_current_coroutine();
    int fd = event->data.fd;
    event->data.ptr = co;
    co->fd = fd;
    co->event = event;
    push_in_epoll(co);
    log_debug("%s wait event", co->name);
    // add_event是由重写的IO调用的，因此需要yield，当描述符可用时由调度器唤醒。
    coroutine_yield();
}

//唤醒收到响应的协程，加入执行队列
//这里不能等执行队列为空时才调用，因为可能有的协程是在忙等待,此时执行队列永远不会空
void awake() {
    int nfds = epoll_wait(EVENT_MANAGER.epollfd, EVENT_MANAGER.events, EVENT_MANAGER.event_size, 0);
    if (nfds <= 0) return;
    epoll_event* events = EVENT_MANAGER.events;
    Coroutine* co;
    for (int i = 0; i < nfds; i++) {
        int fd = events[i].data.fd;
        uint32_t flag = events[i].events;
        EventNode* p = EVENT_MANAGER.waiting_co[fd]->head;
        bool find_event = false;
        while (p->next) {
            if ((p->next->co->event->events & flag) != 0) {
                push_back(EVENT_MANAGER.active_list, p->next->co);
                p->next->co->in_epoll = false;
                remove_next(EVENT_MANAGER.waiting_co[fd], p);
                //同一个fd一次只弹出一个协程，因为同时弹出多个的话，可能一个协程的操作使另一个陷入阻塞。
                //弹出时是弹出最早的，如果该协程再次加入则会进入队尾，实现轮转，不会导致其它协程饥饿。
                find_event = true;
                break;
            } else
                p = p->next;
        }
        if (find_event) continue;
        //运行到这说明该fd的该事件已经没有协程需要监听了
        // flag是已监听成功的事件，不需要继续监听了
        EVENT_MANAGER.flags[fd]->events &= ~flag;
        if (EVENT_MANAGER.flags[fd]->events)
            //还有剩余事件则继续监听
            epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_MOD, fd, EVENT_MANAGER.flags[fd]);
        else
            //否则不监听
            epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_DEL, fd, NULL);
    }
}
void event_loop() {
    while (1) {
        awake();
        show_epoll();
        show_list(EVENT_MANAGER.active_list);
        if (is_emptylist(EVENT_MANAGER.active_list)) continue;
        Coroutine* co = pop_front(EVENT_MANAGER.active_list);
        if (co->status == COROUTINE_DEAD) {
            coroutine_free(co);
            continue;
        }
        if (co->auto_schedule == false) continue;
        coroutine_resume(co);
    }
}