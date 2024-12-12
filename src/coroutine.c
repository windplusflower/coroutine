#include "coroutine.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>

#include "context.h"
#include "event_manager.h"
#include "log.h"

void show_call_stack() {
    char buf[1024];
    buf[0] = '\0';
    for (int i = 0; i < ENV.size; i++) {
        strcat(buf, ENV.call_stack[i]->name);
        if (i + 1 != ENV.size) strcat(buf, "->");
    }
    log_debug("call_stack: %s", buf);
}

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

void eventloop_init() {
    __thread static bool has_inited = false;
    if (has_inited) return;
    has_inited = true;
    init_eventmanager();

    ENV.eventloop_coroutine = (Coroutine *)malloc(sizeof(Coroutine));
    ENV.size = 0;
    Coroutine *co = ENV.eventloop_coroutine;
    co->stack_size = STACKSIZE;
    co->stack = malloc(STACKSIZE);
    co->name = "event_loop";

    co->context.ss_sp = co->stack;
    co->context.ss_size = STACKSIZE;
    make_context(&co->context, event_loop);
    log_debug("eventloop init finished");
}
Coroutine *main_coroutine_init() {
    __thread static Coroutine *main_coroutine = NULL;
    if (main_coroutine != NULL) return main_coroutine;

    main_coroutine = (Coroutine *)malloc(sizeof(Coroutine));
    //主协程用的进程的栈空间，故不需要手动分配和指定。
    main_coroutine->status = COROUTINE_READY;
    main_coroutine->arg = NULL;
    main_coroutine->name = "main";
    main_coroutine->auto_schedule = true;
    main_coroutine->in_epoll = false;

    env_push(main_coroutine);
    return main_coroutine;
}
void func_wrapper() {
    Coroutine *co = get_current_coroutine();
    co->func(co->arg);
    co->status = COROUTINE_DEAD;
    log_debug("%s finished and yield", co->name);
    coroutine_yield();
}
void coroutine_init(Coroutine *co, void (*func)(void *), void *arg, size_t stack_size) {
    eventloop_init();
    if (stack_size <= 0) stack_size = STACKSIZE;
    co->status = COROUTINE_READY;
    co->func = func;
    co->arg = arg;
    co->stack_size = stack_size;
    co->stack = malloc(stack_size);
    co->auto_schedule = true;
    co->in_epoll = false;

    co->context.ss_sp = co->stack;
    co->context.ss_size = stack_size;
    make_context(&co->context, func_wrapper);
    add_coroutine(co);
    co->name = arg;  //仅作调试用，用arg来辨识不同协程
    if (co->name == NULL) co->name = "NoName";
}

void start_eventloop() {
    eventloop_init();
    Coroutine *co = main_coroutine_init();
    add_coroutine(co);
    // start_eventloop不需要将主进程加入调用栈，因为此时主进程是受eventloop调度的。
    env_pop();
    env_push(ENV.eventloop_coroutine);
    swap_context(&co->context, &ENV.eventloop_coroutine->context);
}

void coroutine_resume(Coroutine *co) {
    main_coroutine_init();
    show_call_stack();
    co->status = COROUTINE_RUNNING;
    Coroutine *cur = get_current_coroutine();
    //即便是主进程，resume时也需要加入调用栈，因为此时是由主进程来调度目标协程。
    env_push(co);
    //并不是由eventloop调用的resume，说明目标协程需要手动调度。
    if (cur != ENV.eventloop_coroutine) {
        co->auto_schedule = false;
    }
    log_debug("%s resume to %s", cur->name, co->name);
    swap_context(&cur->context, &co->context);
}

void coroutine_yield() {
    show_call_stack();
    Coroutine *current_coroutine = env_pop();
    Coroutine *upcoming_coroutine = get_current_coroutine();
    current_coroutine->status = COROUTINE_SUSPENDED;
    if (current_coroutine->auto_schedule && !current_coroutine->in_epoll)
        add_coroutine(current_coroutine);
    log_debug("%s yield to %s", current_coroutine->name, upcoming_coroutine->name);
    swap_context(&current_coroutine->context, &upcoming_coroutine->context);
}

void coroutine_free(Coroutine *co) {
    free(co->stack);
    co->stack = NULL;
}
