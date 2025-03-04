/*
 * Copyright (c) 2025 windflower
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "event_manager.h"

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "coroutine.h"
#include "log.h"
#include "utils.h"
#include "co_mutex.h"
#include "co_cond.h"

//获取EVENTMANAGER，在其他文件调用
//因为EVENT_MANAGER有static属性，所以在其他文件只能通过函数调用
EventManager *get_eventmanager() {
    return &EVENT_MANAGER;
}

//初始化EVENTMANAGER
void init_eventmanager() {
    EVENT_MANAGER.active_list = make_empty_list();
    EVENT_MANAGER.epollfd = epoll_create1(0);
    EVENT_MANAGER.event_size = EVENTSIZE;
    EVENT_MANAGER.events = (epoll_event *)malloc(EVENTSIZE * sizeof(epoll_event));
    //初始大小设多大无妨，因为会动态调整大小
    EVENT_MANAGER.time_heap = heap_create(1024);
    EVENT_MANAGER.locking_co = make_empty_list();
    EVENT_MANAGER.waiting_pipe = (WaitingPipe *)malloc(sizeof(WaitingPipe));
    atomic_init(&EVENT_MANAGER.waiting_pipe->awakable, false);
    int ret = pipe(EVENT_MANAGER.waiting_pipe->fd);
    if (ret == -1) {
        log_error("Init eventmanager failed. Please enlarge fd limits!");
        return;
    }
    EVENT_MANAGER.waiting_pipe->event = (struct epoll_event *)malloc(sizeof(struct epoll_event));
    EVENT_MANAGER.waiting_pipe->event->data.fd = EVENT_MANAGER.waiting_pipe->fd[0];
    EVENT_MANAGER.waiting_pipe->event->events = EPOLLIN;
    int flags = fcntl(EVENT_MANAGER.waiting_pipe->fd[0], F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(EVENT_MANAGER.waiting_pipe->fd[0], F_SETFL, flags);
    flags = fcntl(EVENT_MANAGER.waiting_pipe->fd[1], F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(EVENT_MANAGER.waiting_pipe->fd[1], F_SETFL, flags);
#ifdef USE_DEBUG
    log_debug("Event manager init finished");
#endif
}

void show_list(CoList *list) {
#ifdef USE_DEBUG
    CoNode *p = list->head;
    char buf[1024];
    buf[0] = '\0';
    while (p->next) {
        p = p->next;
        strcat(buf, ((Coroutine *)p->data)->name);
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
void add_coroutine(Coroutine *co) {
    push_back(EVENT_MANAGER.active_list, co);
}

//加入epoll,合并不同协程对相同fd的等待
CoNode *push_in_epoll(Coroutine *co) {
    if (co->fd < 0) return NULL;
    int fd = co->fd;
    //初次访问，分配内存
    if (EVENT_MANAGER.waiting_co[fd] == NULL) {
        EVENT_MANAGER.waiting_co[fd] = make_empty_list();
        EVENT_MANAGER.flags[fd] = (epoll_event *)malloc(sizeof(epoll_event));
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

void epoll_add_pipe() {
    epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_ADD, EVENT_MANAGER.waiting_pipe->fd[0],
              EVENT_MANAGER.waiting_pipe->event);
}
void epoll_del_pipe() {
    epoll_ctl(EVENT_MANAGER.epollfd, EPOLL_CTL_DEL, EVENT_MANAGER.waiting_pipe->fd[0], NULL);
}
void clear_pipe() {
    //管道里不会有太多数据，不用很大的缓冲区。
    char buf[32];
    while (read(EVENT_MANAGER.waiting_pipe->fd[0], buf, 32) > 0)
        ;
}

//参数分别为：等待的事件；等待的时间(ms)
//这个函数只由库内的函数调用，保证event会传fd
//返回是否成功等到事件
bool wait_event(epoll_event *event, unsigned long long timeout) {
    Coroutine *co = get_current_coroutine();
    if (event == NULL) {
        CoNode *node = make_node(co);
        heap_push(EVENT_MANAGER.time_heap, timeout + get_now(), node);
    } else {
        int fd = event->data.fd;
        event->data.ptr = co;
        co->fd = fd;
        co->event = event;
        CoNode *node = push_in_epoll(co);
        if (timeout != -1) {
            //同时加入超时队列和等待队列，所以需要释放两次
            node->free_times = 2;
            heap_push(EVENT_MANAGER.time_heap, timeout + get_now(), node);
        }
    }

#ifdef USE_DEBUG
    log_debug("%s wait event(%lums) and yield", co->name, timeout);
#endif
    // add_event是由重写的系统函数调用的，因此需要yield，当描述符可用时由调度器唤醒。
    coroutine_yield();
    bool res = co->timeout ^ 1;
    co->timeout = 0;
    return res;
}

//参数分别为：等待的条件变量；等待的时间(ms)
//返回是否成功等到条件变量
bool wait_cond(CoNode *node, unsigned long long timeout) {
    Coroutine *co = get_current_coroutine();
    heap_push(EVENT_MANAGER.time_heap, timeout + get_now(), node);
#ifdef USE_DEBUG
    log_debug("%s wait event(%dms) and yield", co->name, timeout);
#endif
    coroutine_yield();
    bool res = co->timeout ^ 1;
    co->timeout = 0;
    return res;
}

//唤醒收到响应的协程
void awake_epoll(int timeout) {
    //把收到响应的进程加入执行队列
    int nfds = epoll_wait(EVENT_MANAGER.epollfd, EVENT_MANAGER.events, EVENT_MANAGER.event_size, timeout);
    epoll_event *events = EVENT_MANAGER.events;
    for (int i = 0; i < nfds; i++) {
        int fd = events[i].data.fd;
        if (fd == EVENT_MANAGER.waiting_pipe->fd[0]) continue;
        uint32_t flag = events[i].events;
        CoNode *p = EVENT_MANAGER.waiting_co[fd]->head;
        bool find_event = false;
        while (p->next) {
            if (!p->next->valid) {
                //已经因为超时被移除，直接跳过
                remove_next(EVENT_MANAGER.waiting_co[fd], p);
            } else if ((((Coroutine *)p->next->data)->event->events & flag) != 0) {
                ((Coroutine *)p->next->data)->event->events = flag;
                push_back(EVENT_MANAGER.active_list, p->next->data);
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
}

//唤醒超时的协程
void awake_timeout() {
    //把超时的进程加入执行队列
    long long now = get_now();
    CoNode *node;
    while (!heap_isempty(EVENT_MANAGER.time_heap) && now > heap_top(EVENT_MANAGER.time_heap)->w) {
        node = heap_pop(EVENT_MANAGER.time_heap).data;
        // valid为0，说明已经被释放过了
        if (!node->valid) {
            free_node(node);
            continue;
        }
#ifdef USE_DEBUG
        log_debug("%s time out", ((Coroutine *)node->data)->name);
#endif
        ((Coroutine *)node->data)->timeout = 1;
        add_coroutine(node->data);
        free_node(node);
    }
}

//唤醒收到锁的协程
void awake_mutex() {
    CoNode *p = EVENT_MANAGER.locking_co->head;
    while (p->next) {
        LockPair *lp = p->next->data;
        if (pthread_mutex_trylock(&lp->mutex->mutex)) {
            p = p->next;
            continue;
        }
        add_coroutine(lp->co);
        free(lp);
        remove_next(EVENT_MANAGER.locking_co, p);
    }
}

//唤醒收到响应的协程、超时的协程和获得锁的协程，加入执行队列
void awake(int timeout) {
    awake_epoll(timeout);
    awake_timeout();
    awake_mutex();
}

//事件循环
void event_loop() {
    int timeout = 0;
    while (1) {
        // if (EVENT_MANAGER.time_heap->size > 1024) awake_timeout();
        if (is_emptylist(EVENT_MANAGER.active_list)) {
            //此时还不确定是否需要pipe，但是必须提前开启
            //不然从awake_lock到epoll_add_pipe之间这段时间，其它线程的解锁会被忽略。
            clear_pipe();
            atomic_store(&EVENT_MANAGER.waiting_pipe->awakable, true);
            awake(0);
            //当前没有可以执行的协程，则通过阻塞调用挂起当前线程
            if (is_emptylist(EVENT_MANAGER.active_list)) {
                if (heap_isempty(EVENT_MANAGER.time_heap))
                    timeout = -1;
                else {
                    int delt = heap_top(EVENT_MANAGER.time_heap)->w - get_now();
                    timeout = delt > 0 ? delt : 0;
                }
                epoll_add_pipe();
                awake(timeout);
                //直接从epoll中删除pipe，因为即便线程不是由pipe唤醒的，此时已经有可执行的协程了，所以也需要删除pipe。
                //并不是等到pipe的读端有响应了再删除。
                epoll_del_pipe();
            }
            atomic_store(&EVENT_MANAGER.waiting_pipe->awakable, false);
            continue;
        }
        show_epoll();
        show_list(EVENT_MANAGER.active_list);
        Coroutine *co = pop_front(EVENT_MANAGER.active_list);
        if (co->status == COROUTINE_DEAD) {
            //分离的需要释放
            if (co->is_detached) coroutine_free(co);
            //其它不需要释放内存，等待join时释放
            continue;
        }
        coroutine_resume(co);
    }
}

void add_lock_waiting(Mutex *mutex, Coroutine *co) {
    LockPair *p = (LockPair *)malloc(sizeof(LockPair));
    p->mutex = mutex;
    p->co = co;
    push_back(EVENT_MANAGER.locking_co, p);
    pthread_mutex_lock(&mutex->mutex_pipe);
    push_back(mutex->pipe_list, EVENT_MANAGER.waiting_pipe);
    pthread_mutex_unlock(&mutex->mutex_pipe);
}