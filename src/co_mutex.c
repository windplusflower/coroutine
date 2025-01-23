#include "co_mutex.h"
#include <pthread.h>
#include "coroutine.h"
#include "event_manager.h"
#include "log.h"
#include "utils.h"

void* co_mutex_alloc() {
    eventloop_init();
    Mutex* mutex = (Mutex*)malloc(sizeof(Mutex));
    pthread_mutex_init((pthread_mutex_t*)mutex, NULL);
#ifdef USE_DEBUG
    log_debug("alloc mutex %p", mutex);
#endif
    return mutex;
}

void co_mutex_lock(void* handle) {
    Mutex* mutex = (Mutex*)handle;
    if (pthread_mutex_trylock((pthread_mutex_t*)mutex) == 0) return;
    add_lock_waiting(mutex, get_current_coroutine());
    coroutine_yield();
}

void co_mutex_unlock(void* handle) {
    Mutex* mutex = (Mutex*)handle;
    pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

int co_mutex_free(void* handle) {
    Mutex* mutex = (Mutex*)handle;
    int ret = pthread_mutex_destroy((pthread_mutex_t*)mutex);
    if (ret) return ret;
    free(mutex);
    return 0;
}

int co_mutex_trylock(void* handle) {
    Mutex* mutex = (Mutex*)handle;
    return pthread_mutex_trylock((pthread_mutex_t*)mutex);
}