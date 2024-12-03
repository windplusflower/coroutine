#include "coroutine.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>
#include <ucontext.h>

#include "epoll_manager.h"
#include "log.h"

void env_push(Coroutine *co) {
    assert(ENV.size < 128);
    ENV.call_stack[ENV.size++] = co;
}

Coroutine *env_pop() {
    assert(ENV.size > 0);
    return ENV.call_stack[--ENV.size];
}

Coroutine *get_current_coroutine() {
    assert(ENV.size > 0);
    return ENV.call_stack[ENV.size - 1];
}

void epoll_init() {
    __thread static bool has_inited = false;
    if (has_inited) return;
    has_inited = true;
    init_eventlist();
    ENV.eventloop_coroutine = (Coroutine *)malloc(sizeof(Coroutine));
    ENV.size = 0;
    coroutine_init(ENV.eventloop_coroutine, event_loop, "eventloop", STACKSIZE);
}
Coroutine *main_coroutine_init() {
    __thread static Coroutine *main_coroutine = NULL;
    if (main_coroutine != NULL) return main_coroutine;

    main_coroutine = (Coroutine *)malloc(sizeof(Coroutine));
    main_coroutine->stack_size = STACKSIZE;
    main_coroutine->stack = malloc(STACKSIZE);
    main_coroutine->context.uc_stack.ss_sp = main_coroutine->stack;
    main_coroutine->context.uc_stack.ss_size = main_coroutine->stack_size;
    main_coroutine->context.uc_link = NULL;
    main_coroutine->status = COROUTINE_READY;
    main_coroutine->arg = "main";

    env_push(main_coroutine);
    return main_coroutine;
}

void coroutine_init(Coroutine *co, void (*func)(void *), void *arg, size_t stack_size) {
    epoll_init();
    co->status = COROUTINE_READY;
    co->func = func;
    co->arg = arg;
    co->stack_size = stack_size;
    co->stack = malloc(stack_size);

    getcontext(&co->context);
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = stack_size;
    co->context.uc_link = &ENV.eventloop_coroutine->context;
    makecontext(&co->context, (void (*)(void))func, 1, arg);

    push_back(co, -1);
}

void start_eventloop() {
    Coroutine *co = main_coroutine_init();
    push_back(co, -1);
    env_push(ENV.eventloop_coroutine);
    swapcontext(&co->context, &ENV.eventloop_coroutine->context);
}

void coroutine_resume(Coroutine *co) {
    co->status = COROUTINE_RUNNING;
    Coroutine *cur = get_current_coroutine();
    env_push(co);
    log_debug("%s resume to %s", cur->arg, co->arg);
    swapcontext(&cur->context, &co->context);
}

void coroutine_yield() {
    Coroutine *current_coroutine = env_pop();
    Coroutine *upcoming_coroutine = get_current_coroutine();
    current_coroutine->status = COROUTINE_SUSPENDED;
    push_back(current_coroutine, -1);
    log_debug("%s yield to %s", current_coroutine->arg, upcoming_coroutine->arg);
    swapcontext(&current_coroutine->context, &upcoming_coroutine->context);
}

void coroutine_free(Coroutine *co) {
    free(co->stack);
    co->stack = NULL;
}
