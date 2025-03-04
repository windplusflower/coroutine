/*
 * Copyright (c) 2025 windflower
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "co_mutex.h"
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include "coroutine.h"
#include "event_manager.h"
#include "log.h"
#include "utils.h"

void *co_mutex_alloc() {
    eventloop_init();
    Mutex *mutex = (Mutex *)malloc(sizeof(Mutex));
    pthread_mutex_init(&mutex->mutex, NULL);
    pthread_mutex_init(&mutex->mutex_pipe, NULL);
    mutex->pipe_list = make_empty_list();
#ifdef USE_DEBUG
    log_debug("alloc mutex %p", mutex);
#endif
    return mutex;
}

void co_mutex_lock(void *handle) {
    Mutex *mutex = (Mutex *)handle;
    if (pthread_mutex_trylock(&mutex->mutex) == 0) {
#ifdef USE_DEBUG
        log_debug("co %s get mutex %p", get_current_coroutine()->name, mutex);
#endif
        return;
    }
    add_lock_waiting(mutex, get_current_coroutine());
#ifdef USE_DEBUG
    log_debug("co %s wait mutex %p", get_current_coroutine()->name, mutex);
#endif
    coroutine_yield();
#ifdef USE_DEBUG
    log_debug("co %s get mutex %p", get_current_coroutine()->name, mutex);
#endif
}

void co_mutex_unlock(void *handle) {
    Mutex *mutex = (Mutex *)handle;
#ifdef USE_DEBUG
    log_debug("co %s unlock mutex %p", get_current_coroutine()->name, mutex);
#endif
    pthread_mutex_unlock(&mutex->mutex);
    pthread_mutex_lock(&mutex->mutex_pipe);
    WaitingPipe *p;
    while ((p = pop_front(mutex->pipe_list))) {
        if (atomic_exchange(&p->awakable, false)) {
            char buf[1];
            write(p->fd[1], buf, 1);
        }
    }
    pthread_mutex_unlock(&mutex->mutex_pipe);
}

int co_mutex_free(void *handle) {
    Mutex *mutex = (Mutex *)handle;
    int ret = pthread_mutex_destroy(&mutex->mutex);
    if (ret) return ret;
    free(mutex);
    return 0;
}

int co_mutex_trylock(void *handle) {
    Mutex *mutex = (Mutex *)handle;
    return pthread_mutex_trylock(&mutex->mutex);
}