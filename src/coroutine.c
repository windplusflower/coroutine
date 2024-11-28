#include "coroutine.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>
#include <ucontext.h>

#include "epoll_manager.h"
#include "utils.h"

coroutine_t *get_main_coroutine() {
    return main_coroutine;
}
coroutine_t *get_running_coroutine() {
    return running_coroutine;
}

void epoll_init() {
    __thread static bool has_inited = false;
    if (has_inited) return;
    has_inited = true;
    init_eventlist();
    epoll_coroutine = (coroutine_t *)malloc(sizeof(coroutine_t));
    coroutine_init(epoll_coroutine, event_loop, NULL, STACKSIZE);
}
coroutine_t *main_coroutine_init() {
    coroutine_t *main_coroutine = (coroutine_t *)malloc(sizeof(coroutine_t));
    main_coroutine->stack_size = STACKSIZE;
    main_coroutine->stack = malloc(STACKSIZE);
    main_coroutine->context.uc_stack.ss_sp = main_coroutine->stack;
    main_coroutine->context.uc_stack.ss_size = main_coroutine->stack_size;
    main_coroutine->context.uc_link = &epoll_coroutine->context;
    main_coroutine->status = COROUTINE_READY;
    return main_coroutine;
}

void coroutine_init(coroutine_t *co, void (*func)(void *), void *arg, size_t stack_size) {
    epoll_init();
    co->status = COROUTINE_READY;
    co->func = func;
    co->arg = arg;
    co->stack_size = stack_size;
    co->stack = malloc(stack_size);

    getcontext(&co->context);
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = stack_size;
    co->context.uc_link = &epoll_coroutine->context;
    makecontext(&co->context, (void (*)(void))func, 1, arg);
    push_back(co, -1);
}

//开启自动调度
//目前不支持自动调度与手动调度混用，自动调度的程序不能用resume，不然有bug。
void start_eventloop() {
    coroutine_t *co = main_coroutine_init();
    push_back(co, -1);
    swapcontext(&co->context, &epoll_coroutine->context);
}

//暂时默认只由事件循环调用resume
void coroutine_resume(coroutine_t *co) {
    if (co->status == COROUTINE_READY || co->status == COROUTINE_SUSPENDED) {
        co->status = COROUTINE_RUNNING;
        running_coroutine = co;
        swapcontext(&epoll_coroutine->context, &co->context);
    }
}

void coroutine_yield() {
    if (running_coroutine->status == COROUTINE_RUNNING) {
        running_coroutine->status = COROUTINE_SUSPENDED;
        push_back(running_coroutine, -1);
        swapcontext(&running_coroutine->context, &epoll_coroutine->context);
    }
}

void coroutine_free(coroutine_t *co) {
    free(co->stack);
    co->stack = NULL;
}
