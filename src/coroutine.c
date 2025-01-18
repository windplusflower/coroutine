#include "coroutine.h"

#include <assert.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>

#include "context.h"
#include "event_manager.h"
#include "log.h"
#include "hook.h"
#include "utils.h"
#include "co_cond.h"
#include "co_mutex.h"

//获取当前协程
Coroutine *get_current_coroutine() {
    return ENV.current_coroutine;
}
//获取当前协程
Coroutine *get_eventloop_coroutine() {
    return ENV.eventloop_coroutine;
}

//初始化主协程
Coroutine *main_coroutine_init() {
    __thread static Coroutine *main_coroutine = NULL;
    if (main_coroutine != NULL) return main_coroutine;

    main_coroutine = (Coroutine *)malloc(sizeof(Coroutine));
    //主协程用的进程的栈空间，故不需要手动分配和指定。
    main_coroutine->status = COROUTINE_RUNNING;
    main_coroutine->arg = NULL;
#ifdef USE_DEBUG
    main_coroutine->name = "main";
#endif
    ENV.current_coroutine = main_coroutine;
    return main_coroutine;
}

//初始化事件循环
void eventloop_init() {
    __thread static bool has_inited = false;
    if (has_inited) return;
    has_inited = true;
    log_set_level_from_env();
    init_eventmanager();

    ENV.eventloop_coroutine = (Coroutine *)malloc(sizeof(Coroutine));

    Coroutine *co = ENV.eventloop_coroutine;
    co->stack_size = STACKSIZE;
    co->stack = malloc(STACKSIZE);
#ifdef USE_DEBUG
    co->name = "event_loop";
#endif

    co->context.ss_sp = co->stack;
    co->context.ss_size = STACKSIZE;
    make_context(&co->context, event_loop);
    main_coroutine_init();

#ifdef USE_DEBUG
    log_debug("eventloop init finished");
#endif
}

//结束协程
void coroutine_finish() {
    Coroutine *current_coroutine = get_current_coroutine();
    Coroutine *upcoming_coroutine = get_eventloop_coroutine();
    current_coroutine->status = COROUTINE_DEAD;
    if (current_coroutine->waited_co != NULL) {
        add_coroutine(current_coroutine->waited_co);
    }
#ifdef USE_DEBUG
    log_debug("%s finished and yield to %s", current_coroutine->name, upcoming_coroutine->name);
#endif
    ENV.current_coroutine = get_eventloop_coroutine();
    swap_context(&current_coroutine->context, &upcoming_coroutine->context);
}
//函数封装
void func_wrapper() {
    Coroutine *co = get_current_coroutine();
    co->return_val = co->func(co->arg);
    coroutine_finish();
}

//创建协程
void *coroutine_create(void *(*func)(const void *), const void *arg, size_t stack_size) {
    eventloop_init();
    if (stack_size <= 0) stack_size = STACKSIZE;
    Coroutine *co = (Coroutine *)malloc(sizeof(Coroutine));
    co->status = COROUTINE_READY;
    co->func = func;
    co->arg = arg;
    co->stack_size = stack_size;
    co->stack = malloc(stack_size);
    co->is_detached = false;
    co->timeout = false;
    co->waited_co = NULL;

    co->context.ss_sp = co->stack;
    co->context.ss_size = stack_size;
    make_context(&co->context, func_wrapper);
#ifdef USE_DEBUG
    char *buf = malloc(7);
    for (int i = 0; i < 5; i++) {
        buf[i] = 'a' + rand() % 26;
    }
    buf[5] = 0;
    co->name = buf;  //仅作调试用，来辨识不同协程
    log_debug("create coroutine %s", co->name);
#endif
    add_coroutine(co);
    return co;
}

//唤醒协程
void coroutine_resume(void *handle) {
    Coroutine *co = (Coroutine *)handle;
    Coroutine *cur = get_current_coroutine();
    ENV.current_coroutine = co;
    co->status = COROUTINE_RUNNING;

#ifdef USE_DEBUG
    log_debug("%s resume to %s", cur->name, co->name);
#endif
    swap_context(&cur->context, &co->context);
}

//挂起协程
void coroutine_yield() {
    eventloop_init();
    Coroutine *current_coroutine = get_current_coroutine();
    Coroutine *upcoming_coroutine = get_eventloop_coroutine();
    assert(current_coroutine->status == COROUTINE_RUNNING);
    current_coroutine->status = COROUTINE_SUSPENDED;

#ifdef USE_DEBUG
    log_debug("%s yield to %s", current_coroutine->name, upcoming_coroutine->name);
#endif
    ENV.current_coroutine = get_eventloop_coroutine();
    swap_context(&current_coroutine->context, &upcoming_coroutine->context);
}

//释放协程
void coroutine_free(void *handle) {
    Coroutine *co = (Coroutine *)handle;
    free(co->stack);
    free(co);
}

//等待协程结束,返回返回值
void *coroutine_join(void *handle) {
    Coroutine *co = (Coroutine *)handle;
    if (co->is_detached) {
        log_error("You can't join a detached coroutine %d!", handle);
        return NULL;
    }
    if (co->status != COROUTINE_DEAD) {
        if (co->waited_co != NULL) {
            log_error("Coroutine %d has been join by other coroutine!", handle);
            return NULL;
        }
        co->waited_co = get_current_coroutine();
        coroutine_yield();
    }
    assert(co->status == COROUTINE_DEAD);
    void *res = co->return_val;
    coroutine_free(handle);
    return res;
}

void coroutine_detach(void *handle) {
    Coroutine *co = (Coroutine *)handle;
    if (co == NULL) {
        log_error("Handle %d not exist!", handle);
        return;
    }
    if (co->is_detached) {
        log_error("Coroutine %d has already detached!", handle);
        return;
    }
    co->is_detached = 1;
}
