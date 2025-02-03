#include "co_cond.h"
#include <stdatomic.h>
#include "coroutine.h"
#include "event_manager.h"
#include "log.h"
#include "utils.h"
#include "coheader.h"

void* co_cond_alloc() {
    eventloop_init();
    Cond* cond = (Cond*)malloc(sizeof(Cond));
    atomic_init(&cond->cnt_signal, 0);
    atomic_init(&cond->waiting_co, 0);
    cond->cnt_broadcast = 0;
#ifdef USE_DEBUG
    log_debug("alloc cond %p", cond);
#endif
    return cond;
}

//通知一个
void co_cond_signal(void* handle) {
    Cond* cond = (Cond*)handle;
#ifdef USE_DEBUG
    log_debug("co %s signal cond %p", get_current_coroutine()->name, cond);
#endif
    atomic_fetch_add(&cond->cnt_signal, 1);
}

void co_cond_broadcast(void* handle) {
    Cond* cond = (Cond*)handle;
    cond->cnt_broadcast++;
}

int co_cond_wait(void* cond_handle, void* mutex_handle) {
    Cond* cond = (Cond*)cond_handle;
#ifdef USE_DEBUG
    log_debug("co %s wait cond %p", get_current_coroutine()->name, cond);
#endif
    awake_cond();
    add_cond_waiting(cond, get_current_coroutine());
    co_mutex_unlock(mutex_handle);
    coroutine_yield();
    co_mutex_lock(mutex_handle);
    return 0;
}

//超时时间单位是ms
int co_cond_timewait(void* cond_handle, void* mutex_handle, int timeout) {
    Cond* cond = (Cond*)cond_handle;
    CoNode* node = add_cond_waiting(cond, get_current_coroutine());
    co_mutex_unlock(mutex_handle);
    int ret = wait_cond(node, timeout) - 1;
    co_mutex_lock(mutex_handle);
    return ret;
}

//正在使用的话会返回-1;
int co_cond_free(void* handle) {
    Cond* cond = (Cond*)handle;
    free(cond);
    return 0;
}