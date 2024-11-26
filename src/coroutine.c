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
void main_coroutine_init() {
    __thread static bool has_inited = false;
    if (has_inited) return;
    has_inited = true;
    main_coroutine = (coroutine_t *)malloc(sizeof(coroutine_t));
    main_coroutine->stack_size = STACKSIZE;
    main_coroutine->stack = malloc(STACKSIZE);
    main_coroutine->context.uc_stack.ss_sp = main_coroutine->stack;
    main_coroutine->context.uc_stack.ss_size = main_coroutine->stack_size;
}

void coroutine_init(coroutine_t *co, void (*func)(void *), void *arg, size_t stack_size) {
    epoll_init();
    main_coroutine_init();
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
}

void coroutine_resume(coroutine_t *co) {
    if (co->status == COROUTINE_READY || co->status == COROUTINE_SUSPENDED) {
        co->status = COROUTINE_RUNNING;
        running_coroutine = co;
        if (is_main_running())
            swapcontext(&main_coroutine->context, &co->context);
        else {
            swapcontext(&epoll_coroutine->context, &co->context);
        }
    }
}

void coroutine_yield() {
    if (running_coroutine->status == COROUTINE_RUNNING) {
        running_coroutine->status = COROUTINE_SUSPENDED;
        push_back(running_coroutine, -1);
        swapcontext(&running_coroutine->context, &epoll_coroutine->context);
    }
}

void yield_to_main() {
    swapcontext(&epoll_coroutine->context, &main_coroutine->context);
}

void coroutine_free(coroutine_t *co) {
    free(co->stack);
    co->stack = NULL;
}
