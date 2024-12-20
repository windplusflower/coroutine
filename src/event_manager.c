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

//创建空链表
EventList* make_empty_list() {
    EventList* res = (EventList*)malloc(sizeof(EventList));
    res->head = (EventNode*)calloc(1, sizeof(EventNode));
    res->tail = res->head;
    return res;
}

//获取EVENTMANAGER，在其他文件调用
//因为EVENT_MANAGER有static属性，所以在其他文件只能通过函数调用
EventManager* get_eventmanager() {
    return &EVENT_MANAGER;
}

//初始化EVENTMANAGER
void init_eventmanager() {
    EVENT_MANAGER.active_list = make_empty_list();
    EVENT_MANAGER.epollfd = epoll_create1(0);
    EVENT_MANAGER.event_size = EVENTSIZE;
    EVENT_MANAGER.events = (epoll_event*)malloc(EVENTSIZE * sizeof(epoll_event));
    //初始大小设多大无妨，因为会动态调整大小
    EVENT_MANAGER.time_heap = heap_create(1024);

#ifdef USE_DEBUG
    log_debug("Event manager init finished");
#endif
}

//生成结点
EventNode* make_node(Coroutine* co) {
    EventNode* res = (EventNode*)calloc(1, sizeof(EventNode));
    res->co = co;
    res->valid = 1;
    res->free_times = 1;
    return res;
}

//不需要释放co指向的内存，因为会转移这块内存的所有权，它之后还要用到，在协程结束时才由调度器销毁。
void free_node(EventNode* node) {
    node->free_times--;
    assert(node->free_times >= 0);
    //只要释放一次后，就一定是无效的了
    node->valid = 0;
    if (node->free_times == 0) free(node);
}

//添加到链表尾部
EventNode* push_back(EventList* list, Coroutine* co) {
    EventNode* node = make_node(co);
    list->tail->next = node;
    list->tail = node;
    return node;
}

//从链表头部弹出
Coroutine* pop_front(EventList* list) {
    assert(list->head->next != NULL);
    EventNode* node = list->head->next;
    list->head->next = node->next;
    Coroutine* co = node->co;
    if (list->head->next == NULL) list->tail = list->head;
    free_node(node);
    return co;
}

//移除node的下一个节点
void remove_next(EventList* list, EventNode* node) {
    assert(node->next != NULL);
    EventNode* tmp = node->next;
    node->next = tmp->next;
    free_node(tmp);
    if (node->next == NULL) list->tail = node;
}

//判断链表是否为空
bool is_emptylist(EventList* list) {
    return list == NULL || list->head == list->tail;
}

void show_list(EventList* list) {
#ifdef USE_DEBUG
    EventNode* p = list->head;
    char buf[1024];
    buf[0] = '\0';
    while (p->next) {
        p = p->next;
        strcat(buf, p->co->name);
        strcat(buf, " ");
        if (strlen(buf) > 1000) break;
    }

    log_debug("event_list: %s", buf);
#endif
}

void show_epoll() {
#ifdef USE_DEBUG
    log_debug("****************epoll****************");
    for (int i = 0; i < 1024; i++) {
        if (EVENT_MANAGER.waiting_co[i] == NULL) continue;
        show_list(EVENT_MANAGER.waiting_co[i]);
    }
    log_debug("***************epoll****************");
#endif
}

//将协程添加到准备队列
void add_coroutine(Coroutine* co) {
    push_back(EVENT_MANAGER.active_list, co);
}

//加入epoll,合并不同协程对相同fd的等待
EventNode* push_in_epoll(Coroutine* co) {
    if (co->fd < 0) return NULL;
    co->in_epoll = true;
    int fd = co->fd;
    //初次访问，分配内存
    if (EVENT_MANAGER.waiting_co[fd] == NULL) {
        EVENT_MANAGER.waiting_co[fd] = make_empty_list();
        EVENT_MANAGER.flags[fd] = (epoll_event*)malloc(sizeof(epoll_event));
    }
    //第一次监听该fd,直接添加进epoll。
    if (is_emptylist(EVENT_MANAGER.waiting_co[fd])) {
        EVENT_MANAGER.flags[fd]->events = co->event->events;
        EVENT_MANAGER.flags[fd]->data.fd = fd;
        epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_ADD, fd, EVENT_MANAGER.flags[fd]);
        return push_back(EVENT_MANAGER.waiting_co[fd], co);
    }

    //已经监听了fd,且需要监听的事件是已监听事件的子集，无需操作epoll
    if ((EVENT_MANAGER.flags[fd]->events & co->event->events) == co->event->events)
        return push_back(EVENT_MANAGER.waiting_co[fd], co);
    //剩下的就是已经监听了fd,且需要监听的事件至少有一部分没有监听
    EVENT_MANAGER.flags[fd]->events |= co->event->events;
    epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_MOD, fd, EVENT_MANAGER.flags[fd]);
    return push_back(EVENT_MANAGER.waiting_co[fd], co);
}

//参数分别为：等待的事件；等待的时间(ms)
//这个函数只由库内的函数调用，保证event会传fd
//返回是否成功等到事件
bool wait_event(epoll_event* event, int timeout) {
    Coroutine* co = get_current_coroutine();
    if (event == NULL && timeout != -1) {
        EventNode* node = make_node(co);
        co->in_epoll = true;
        heap_push(EVENT_MANAGER.time_heap, timeout + get_now(), node);
    } else {
        int fd = event->data.fd;
        event->data.ptr = co;
        co->fd = fd;
        co->event = event;
        EventNode* node = push_in_epoll(co);
        if (timeout != -1) {
            //同时加入超时队列和等待队列，所以需要释放两次
            node->free_times = 2;
            heap_push(EVENT_MANAGER.time_heap, timeout + get_now(), node);
        }
    }

#ifdef USE_DEBUG
    log_debug("%s wait event(%dms) and yield", co->name, timeout);
#endif
    // add_event是由重写的系统函数调用的，因此需要yield，当描述符可用时由调度器唤醒。
    coroutine_yield();
    bool res = co->timeout ^ 1;
    co->timeout = 0;
    return res;
}

//唤醒收到响应的协程和超时的协程，加入执行队列
//这里不能等执行队列为空时才调用，因为可能有的协程是在忙等待,此时执行队列永远不会空
void awake() {
    //把收到响应的进程加入执行队列
    int nfds = epoll_wait(EVENT_MANAGER.epollfd, EVENT_MANAGER.events, EVENT_MANAGER.event_size, 0);
    epoll_event* events = EVENT_MANAGER.events;
    for (int i = 0; i < nfds; i++) {
        int fd = events[i].data.fd;
        uint32_t flag = events[i].events;
        EventNode* p = EVENT_MANAGER.waiting_co[fd]->head;
        bool find_event = false;
        while (p->next) {
            if (!p->next->valid) {
                //已经因为超时被移除，直接跳过
                remove_next(EVENT_MANAGER.waiting_co[fd], p);
            } else if ((p->next->co->event->events & flag) != 0) {
                p->next->co->event->events = flag;
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
        if (EVENT_MANAGER.flags[fd]->events && !is_emptylist(EVENT_MANAGER.waiting_co[fd]))
            //还有剩余事件则继续监听
            epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_MOD, fd, EVENT_MANAGER.flags[fd]);
        else
            //否则不监听
            epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_DEL, fd, NULL);
    }
    //把超时的进程加入执行队列
    long long now = get_now();
    EventNode* node;
    while (!heap_isempty(EVENT_MANAGER.time_heap) && now > heap_top(EVENT_MANAGER.time_heap)->w) {
        node = heap_pop(EVENT_MANAGER.time_heap).data;
        // valid为0，说明已经在epoll中被释放过了
        if (!node->valid) {
            free_node(node);
            continue;
        }
#ifdef USE_DEBUG
        log_debug("%s time out", node->co->name);
#endif
        node->co->timeout = 1;
        node->co->in_epoll = false;
        add_coroutine(node->co);
        free_node(node);
    }
}

//事件循环
void event_loop() {
    while (1) {
        awake();
        show_epoll();
        show_list(EVENT_MANAGER.active_list);
        if (is_emptylist(EVENT_MANAGER.active_list)) continue;
        Coroutine* co = pop_front(EVENT_MANAGER.active_list);
        if (co->auto_schedule == false) continue;
        if (co->status == COROUTINE_DEAD) {
            //不需要释放内存，等待join时释放
            continue;
        }
        if (co->status == COROUTINE_CANCELED) {
            //需要释放内存
            coroutine_free(co->handle);
            continue;
        }
        coroutine_resume(co->handle);
    }
}