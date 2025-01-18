#include "co_mutex.h"
#include "coroutine.h"
#include "event_manager.h"
#include "log.h"
#include "utils.h"

void* co_mutex_alloc() {
    eventloop_init();
    Mutex* mutex = (Mutex*)malloc(sizeof(Mutex));
    mutex->list = make_empty_list();
    mutex->is_locked = false;
#ifdef USE_DEBUG
    log_debug("alloc mutex %p", mutex);
#endif
    return mutex;
}

void co_mutex_lock(void* handle) {
    Mutex* mutex = (Mutex*)handle;
    if (mutex == NULL) {
        log_error("mutex %p not exist!", handle);
        return;
    }
    if (mutex->is_locked) {
        push_back(mutex->list, get_current_coroutine());
        coroutine_yield();
    } else
        mutex->is_locked = true;
}

void co_mutex_unlock(void* handle) {
    Mutex* mutex = (Mutex*)handle;
    if (mutex == NULL) {
        log_error("mutex %d not exist!", handle);
        return;
    }
    if (is_emptylist(mutex->list)) {
        mutex->is_locked = false;
        return;
    }
    Coroutine* co = pop_front(mutex->list);
    add_coroutine(co);
}

//正在使用的话会返回-1
int co_mutex_free(void* handle) {
    Mutex* mutex = (Mutex*)handle;
    if (mutex == NULL) {
        log_error("mutex %d not exist!", handle);
        return -1;
    }
    if (mutex->is_locked) return -1;
    free_list(mutex->list);
    free(mutex);
    return 0;
}

int co_mutex_trylock(void* handle) {
    Mutex* mutex = (Mutex*)handle;
    if (mutex == NULL) {
        log_error("mutex %d not exist!", handle);
        return -1;
    }
    if (mutex->is_locked) return -1;
    mutex->is_locked = true;
    return 0;
}