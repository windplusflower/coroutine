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

#include "co_cond.h"
#include <stdatomic.h>
#include "coroutine.h"
#include "event_manager.h"
#include "log.h"
#include "utils.h"
#include "coheader.h"

void *co_cond_alloc() {
    eventloop_init();
    Cond *cond = (Cond *)malloc(sizeof(Cond));
    cond->waiting_co = make_empty_list();
    cond->mutex = co_mutex_alloc();
#ifdef USE_DEBUG
    log_debug("alloc cond %p", cond);
#endif
    return cond;
}

//通知一个
void co_cond_signal(void *handle) {
    Cond *cond = (Cond *)handle;
    Coroutine *co;
    co_mutex_lock(cond->mutex);
    if ((co = pop_front(cond->waiting_co))) add_coroutine(co);
#ifdef USE_DEBUG
    log_debug("co %s signal cond %p awake co %s", get_current_coroutine()->name,
              cond, co);
#endif
    co_mutex_unlock(cond->mutex);
}

void co_cond_broadcast(void *handle) {
    Cond *cond = (Cond *)handle;
    Coroutine *co;
    co_mutex_lock(cond->mutex);
    while ((co = pop_front(cond->waiting_co))) add_coroutine(co);
    co_mutex_unlock(cond->mutex);
}

int co_cond_wait(void *cond_handle, void *mutex_handle) {
    Cond *cond = (Cond *)cond_handle;
#ifdef USE_DEBUG
    log_debug("co %s wait cond %p", get_current_coroutine()->name, cond);
#endif
    co_mutex_lock(cond->mutex);
    push_back(cond->waiting_co, get_current_coroutine());
    co_mutex_unlock(cond->mutex);
    co_mutex_unlock(mutex_handle);
    coroutine_yield();
    co_mutex_lock(mutex_handle);
    return 0;
}

//超时时间单位是ms
int co_cond_timewait(void *cond_handle, void *mutex_handle, int timeout) {
    Cond *cond = (Cond *)cond_handle;
    co_mutex_lock(cond->mutex);
    CoNode *node = push_back(cond->waiting_co, get_current_coroutine());
    node->free_times = 2;
    co_mutex_unlock(cond->mutex);
    co_mutex_unlock(mutex_handle);
    int ret = wait_cond(node, timeout) - 1;
    co_mutex_lock(mutex_handle);
    return ret;
}

int co_cond_free(void *handle) {
    Cond *cond = (Cond *)handle;
    free(cond);
    return 0;
}