#include "co_mutex.h"
#include "coroutine.h"
#include "event_manager.h"
#include "log.h"
#include "utils.h"
//初始化hanlde与mutex之间的映射表，可动态扩容
void init_mutex_table() {
    ut_init_handle_table(&MUTEX_TABLE);
}

//分配Handle
int alloc_mutex_id() {
    return ut_alloc_id(&MUTEX_TABLE);
}

//根据handle获取Mutex
Mutex* get_mutex_by_id(int id) {
    return ut_get_item_by_id(&MUTEX_TABLE, id);
}

//释放Handle
void free_mutex_id(int id) {
    ut_free_id(&MUTEX_TABLE, id);
}

int co_mutex_alloc() {
    eventloop_init();
    Mutex* mutex = (Mutex*)malloc(sizeof(Mutex));
    mutex->list = make_empty_list();
    mutex->is_locked = false;
    int handle = alloc_mutex_id();
    MUTEX_TABLE.table[handle] = mutex;
#ifdef USE_DEBUG
    log_debug("alloc mutex %d", handle);
#endif
    return handle;
}

void co_mutex_lock(int handle) {
    Mutex* mutex = get_mutex_by_id(handle);
    if (mutex == NULL) {
        log_error("mutex %d not exist!", handle);
        return;
    }
    if (mutex->is_locked) {
        push_back(mutex->list, get_current_coroutine());
        coroutine_yield();
    } else
        mutex->is_locked = true;
}

void co_mutex_unlock(int handle) {
    Mutex* mutex = get_mutex_by_id(handle);
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
int co_mutex_free(int handle) {
    Mutex* mutex = get_mutex_by_id(handle);
    if (mutex == NULL) {
        log_error("mutex %d not exist!", handle);
        return -1;
    }
    if (mutex->is_locked) return -1;
    free_list(mutex->list);
    free(mutex);
    free_mutex_id(handle);
    return 0;
}

int co_mutex_trylock(int handle) {
    Mutex* mutex = get_mutex_by_id(handle);
    if (mutex == NULL) {
        log_error("mutex %d not exist!", handle);
        return -1;
    }
    if (mutex->is_locked) return -1;
    mutex->is_locked = true;
    return 0;
}