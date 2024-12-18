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

void show_call_stack() {
    char buf[1024];
    buf[0] = '\0';
    for (int i = 0; i < ENV.size; i++) {
        strcat(buf, ENV.call_stack[i]->name);
        if (i + 1 != ENV.size) strcat(buf, "->");
    }
    log_debug("call_stack: %s", buf);
}

void init_coroutine_table() {
    TABLE.capacity = TABLESIZE;
    TABLE.size = TABLESIZE;
    TABLE.co_table = (Coroutine **)calloc(TABLESIZE, sizeof(Coroutine *));
    TABLE.unused = (int *)malloc(TABLESIZE * sizeof(int));
    for (int i = 0; i < TABLESIZE; i++) TABLE.unused[i] = i;
}

int alloc_id() {
    if (TABLE.size == 0) {
        int n = TABLE.capacity;
        TABLE.capacity = n * 2;
        TABLE.size = n;
        TABLE.co_table = realloc(TABLE.co_table, n * 2 * sizeof(Coroutine *));
        //虽然现在unused只需要n的空间，但是后续可能会有新的句柄从co_table中释放，最大可以到2*n
        TABLE.unused = realloc(TABLE.unused, n * 2 * sizeof(int));
        for (int i = n; i < n * 2; i++) TABLE.unused[i - n] = i;
    }
    TABLE.size--;
    return TABLE.unused[TABLE.size];
}

Coroutine *get_coroutine_by_id(int id) {
    return TABLE.co_table[id];
}

void free_id(int id) {
    TABLE.co_table[id] = NULL;
    TABLE.unused[TABLE.size++] = id;
}

void env_push(Coroutine *co) {
    if (ENV.size >= ENV.capacity) {
        ENV.capacity *= 2;
        ENV.call_stack = realloc(ENV.call_stack, ENV.capacity * sizeof(Coroutine *));
    }
    ENV.call_stack[ENV.size++] = co;
}

Coroutine *env_pop() {
    assert(ENV.size > 0);
    //有大量空余时释放内存，最少减到STACKSEPTH
    if (ENV.size * 4 < ENV.capacity && ENV.capacity > STACKDEPTH) {
        ENV.capacity /= 2;
        ENV.call_stack = realloc(ENV.call_stack, ENV.capacity * sizeof(Coroutine *));
    }
    return ENV.call_stack[--ENV.size];
}

Coroutine *get_current_coroutine() {
    assert(ENV.size > 0);
    return ENV.call_stack[ENV.size - 1];
}

Coroutine *main_coroutine_init() {
    __thread static Coroutine *main_coroutine = NULL;
    if (main_coroutine != NULL) return main_coroutine;

    main_coroutine = (Coroutine *)malloc(sizeof(Coroutine));
    //主协程用的进程的栈空间，故不需要手动分配和指定。
    main_coroutine->status = COROUTINE_RUNNING;
    main_coroutine->arg = NULL;
    main_coroutine->name = "main";
    main_coroutine->auto_schedule = true;
    main_coroutine->in_epoll = false;
    main_coroutine->handle = alloc_id();
    TABLE.co_table[main_coroutine->handle] = main_coroutine;
    env_push(main_coroutine);
    return main_coroutine;
}

void eventloop_init() {
    __thread static bool has_inited = false;
    if (has_inited) return;
    has_inited = true;
    log_set_level_from_env();
    init_coroutine_table();
    init_eventmanager();
    init_hook();

    ENV.eventloop_coroutine = (Coroutine *)malloc(sizeof(Coroutine));
    ENV.size = 0;
    ENV.capacity = STACKDEPTH;
    ENV.call_stack = (Coroutine **)malloc(STACKDEPTH * sizeof(Coroutine *));

    Coroutine *co = ENV.eventloop_coroutine;
    co->stack_size = STACKSIZE;
    co->stack = malloc(STACKSIZE);
    co->name = "event_loop";

    co->context.ss_sp = co->stack;
    co->context.ss_size = STACKSIZE;
    make_context(&co->context, event_loop);
    env_push(co);
    main_coroutine_init();

    log_debug("eventloop init finished");
}
void func_wrapper() {
    Coroutine *co = get_current_coroutine();
    co->func(co->arg);
    coroutine_finish();
}
int coroutine_init(void (*func)(const void *), const void *arg, size_t stack_size) {
    eventloop_init();
    if (stack_size <= 0) stack_size = STACKSIZE;
    Coroutine *co = (Coroutine *)malloc(sizeof(Coroutine));
    co->status = COROUTINE_READY;
    co->func = func;
    co->arg = arg;
    co->stack_size = stack_size;
    co->stack = malloc(stack_size);
    co->auto_schedule = true;
    co->in_epoll = false;
    co->timeout = false;

    co->context.ss_sp = co->stack;
    co->context.ss_size = stack_size;
    make_context(&co->context, func_wrapper);
    co->name = arg;  //仅作调试用，用arg来辨识不同协程
    if (co->name == NULL) co->name = "NoName";
    add_coroutine(co);

    int handle = alloc_id();
    TABLE.co_table[handle] = co;
    co->handle = handle;
    return handle;
}

void coroutine_resume(int handle) {
    show_call_stack();
    Coroutine *co = get_coroutine_by_id(handle);
    if (co == NULL) {
        log_error("Handle %d not exist!", handle);
        return;
    }
    if (co->status == COROUTINE_DEAD) {
        log_error("You can't resume a finished coroutine %d", handle);
        return;
    }
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
    assert(current_coroutine->status == COROUTINE_RUNNING);
    current_coroutine->status = COROUTINE_SUSPENDED;
    if (current_coroutine->auto_schedule && !current_coroutine->in_epoll)
        add_coroutine(current_coroutine);
    log_debug("%s yield to %s", current_coroutine->name, upcoming_coroutine->name);
    swap_context(&current_coroutine->context, &upcoming_coroutine->context);
}

void coroutine_finish() {
    show_call_stack();
    Coroutine *current_coroutine = env_pop();
    Coroutine *upcoming_coroutine = get_current_coroutine();
    current_coroutine->status = COROUTINE_DEAD;
    log_debug("%s finished and yield to %s", current_coroutine->name, upcoming_coroutine->name);
    swap_context(&current_coroutine->context, &upcoming_coroutine->context);
}

void coroutine_free(int handle) {
    Coroutine *co = get_coroutine_by_id(handle);
    if (co == NULL) {
        log_error("Handle %d not exist!", handle);
        return;
    }
    if (co->status != COROUTINE_DEAD) {
        log_error("Coroutine %d has not finished!", handle);
        return;
    }
    free(co->stack);
    free(co);
    free_id(handle);
}

void coroutine_join(int handle) {
    Coroutine *co = get_coroutine_by_id(handle);
    if (co == NULL) {
        log_error("Handle %d not exist!", handle);
        return;
    }
    if (co->auto_schedule == false) {
        log_error("You can't join a no auto schedule coroutine!");
        return;
    }
    while (co->status != COROUTINE_DEAD) coroutine_yield();
    coroutine_free(handle);
}

void coroutine_cancel(int handle) {
    Coroutine *co = get_coroutine_by_id(handle);
    if (co == NULL) {
        log_error("Handle %d not exist!", handle);
        return;
    }
    co->status = COROUTINE_DEAD;
}
