#include "coroutine.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>
#include <ucontext.h>

#include "epoll_manager.h"
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
    init_eventlist();

    ENV.eventloop_coroutine = (Coroutine *)malloc(sizeof(Coroutine));
    ENV.size = 0;
    Coroutine *co = ENV.eventloop_coroutine;
    co->stack_size = STACKSIZE;
    co->stack = malloc(STACKSIZE);
    co->name = "event_loop";

    getcontext(&co->context);
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = STACKSIZE;
    co->context.uc_link = NULL;
    makecontext(&co->context, (void (*)(void))event_loop, 0);
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
    main_coroutine->arg = NULL;
    main_coroutine->name = "main";
    main_coroutine->auto_schedule = true;

    env_push(main_coroutine);
    return main_coroutine;
}
void func_wrapper() {
    Coroutine *co = get_current_coroutine();
    co->func(co->arg);
    co->status = COROUTINE_DEAD;
    env_pop();
    assert(co->context.uc_link == &get_current_coroutine()->context);
    log_debug("%s finished and return to %s", co->name, get_current_coroutine()->name);
}
void coroutine_init(Coroutine *co, void (*func)(void *), void *arg, size_t stack_size) {
    eventloop_init();
    co->status = COROUTINE_READY;
    co->func = func;
    co->arg = arg;
    co->stack_size = stack_size;
    co->stack = malloc(stack_size);
    co->auto_schedule = true;

    getcontext(&co->context);
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = stack_size;
    co->context.uc_link = &ENV.eventloop_coroutine->context;
    makecontext(&co->context, (void (*)(void))func_wrapper, 0);

    push_back(co, -1);
    co->name = arg;  //仅作调试用，用arg来辨识不同协程
}

void start_eventloop() {
    Coroutine *co = main_coroutine_init();
    push_back(co, -1);
    // start_eventloop不需要将主进程加入调用栈，因为此时主进程是受eventloop调度的。
    env_pop();
    env_push(ENV.eventloop_coroutine);
    swapcontext(&co->context, &ENV.eventloop_coroutine->context);
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
        //将程序结束后的目标改为父协程
        getcontext(&co->context);
        co->context.uc_stack.ss_sp = co->stack;
        co->context.uc_stack.ss_size = co->stack_size;
        co->context.uc_link = &cur->context;
        makecontext(&co->context, (void (*)(void))func_wrapper, 0);
    }
    log_debug("%s resume to %s", cur->name, co->name);
    swapcontext(&cur->context, &co->context);
}

void coroutine_yield() {
    show_call_stack();
    Coroutine *current_coroutine = env_pop();
    Coroutine *upcoming_coroutine = get_current_coroutine();
    current_coroutine->status = COROUTINE_SUSPENDED;
    if (current_coroutine->auto_schedule) push_back(current_coroutine, -1);
    log_debug("%s yield to %s", current_coroutine->name, upcoming_coroutine->name);
    swapcontext(&current_coroutine->context, &upcoming_coroutine->context);
}

void coroutine_free(Coroutine *co) {
    free(co->stack);
    co->stack = NULL;
}
