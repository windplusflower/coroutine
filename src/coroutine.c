#include "coroutine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

static coroutine_t *running_coroutine = NULL, *epoll_coroutine = NULL;
static ucontext_t return_context;

void epoll_init() {
}
void running_coroutine_init() {
    running_coroutine = (coroutine_t *)malloc(sizeof(coroutine_t));
}

void coroutine_init(coroutine_t *co, void (*func)(void *), void *arg, size_t stack_size) {
    epoll_init();
    running_coroutine_init();
    co->status = COROUTINE_READY;
    co->func = func;
    co->arg = arg;
    co->stack_size = stack_size;
    co->stack = malloc(stack_size);

    getcontext(&co->context);
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = stack_size;
    co->context.uc_link = NULL;
    makecontext(&co->context, (void (*)(void))func, 1, arg);
}

//简易实现，暂时不用epoll，默认由father调用resume
void coroutine_resume(coroutine_t *co) {
    if (co->status == COROUTINE_READY || co->status == COROUTINE_SUSPENDED) {
        co->status = COROUTINE_RUNNING;
        running_coroutine = co;
        swapcontext(&return_context, &co->context);
    }
}

void coroutine_yield() {
    if (running_coroutine->status == COROUTINE_RUNNING) {
        running_coroutine->status = COROUTINE_SUSPENDED;
        swapcontext(&running_coroutine->context, &return_context);
    }
}

void coroutine_free(coroutine_t *co) {
    free(co->stack);
    co->stack = NULL;
}
