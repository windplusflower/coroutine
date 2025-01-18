#include "co_cond.h"
#include "coroutine.h"
#include "event_manager.h"
#include "log.h"
#include "utils.h"
#include "coheader.h"

void* co_cond_alloc() {
    eventloop_init();
    Cond* cond = (Cond*)malloc(sizeof(Cond));
    cond->list = make_empty_list();
#ifdef USE_DEBUG
    log_debug("alloc cond %p", cond);
#endif
    return cond;
}

//通知一个
void co_cond_signal(void* handle) {
    Cond* cond = (Cond*)handle;
    if (cond == NULL) {
        log_error("cond %p not exist!", handle);
        return;
    }
    //移除已经因超时而无效的协程
    while (!is_emptylist(cond->list) && !cond->list->head->next->valid)
        remove_next(cond->list, cond->list->head);
    if (is_emptylist(cond->list)) return;
    cond->list->head->next->valid = 0;
    Coroutine* co = pop_front(cond->list);
#ifdef USE_DEBUG
    log_debug("signal coroutine %s", co->name);
#endif
    add_coroutine(co);
}

void co_cond_broadcast(void* handle) {
    Cond* cond = (Cond*)handle;
    if (cond == NULL) {
        log_error("cond %d not exist!", handle);
        return;
    }
    while (!is_emptylist(cond->list)) {
        if (!cond->list->head->next->valid)
            remove_next(cond->list, cond->list->head);
        else {
            cond->list->head->next->valid = 0;
            Coroutine* co = pop_front(cond->list);
            add_coroutine(co);
        }
    }
}

int co_cond_wait(void* cond_handle, void* mutex_handle) {
    Cond* cond = (Cond*)cond_handle;
    if (cond == NULL) {
        log_error("cond %d not exist!", cond_handle);
        return -1;
    }
    co_mutex_unlock(mutex_handle);
    push_back(cond->list, get_current_coroutine());
    coroutine_yield();
    co_mutex_lock(mutex_handle);
    return 0;
}

//超时时间单位是ms
int co_cond_timewait(void* cond_handle, void* mutex_handle, int timeout) {
    Cond* cond = (Cond*)cond_handle;
    if (cond == NULL) {
        log_error("cond %d not exist!", cond_handle);
        return -1;
    }
    co_mutex_unlock(mutex_handle);
    push_back(cond->list, get_current_coroutine());
    int ret = wait_cond(cond->list->tail, timeout) - 1;
    co_mutex_lock(mutex_handle);
    return ret;
}

//正在使用的话会返回-1;
int co_cond_free(void* handle) {
    Cond* cond = (Cond*)handle;
    if (cond == NULL) {
        log_error("cond %d not exist!", handle);
        return -1;
    };
    if (!is_emptylist(cond->list)) return -1;
    free_list(cond->list);
    free(cond);
    return 0;
}