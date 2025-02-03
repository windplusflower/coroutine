#include <pthread.h>
#include <stdatomic.h>
#include "coheader.h"

#define NUM_COROUTINES 1000
#define MAX_COUNT 100000

static int shared_counter = 0;
co_mutex_t mutex;
co_cond_t cond_even;
co_cond_t cond_odd;
coroutine_t coroutines[NUM_COROUTINES];

// 偶数修改者
void* even_updater(void* arg) {
    while (1) {
        co_mutex_lock(mutex);
        if (shared_counter >= MAX_COUNT) {
            co_mutex_unlock(mutex);
            return NULL;
        }
        while (shared_counter % 2 != 0) {  // 等待为偶数
            co_cond_wait(cond_even, mutex);
        }
        shared_counter++;
        co_cond_signal(cond_odd);  // 唤醒奇数修改者
        co_mutex_unlock(mutex);
    }
    return NULL;
}

// 奇数修改者
void* odd_updater(void* arg) {
    while (1) {
        co_mutex_lock(mutex);
        if (shared_counter >= MAX_COUNT) {
            co_mutex_unlock(mutex);
            return NULL;
        }
        while (shared_counter % 2 != 1) {  // 等待为奇数
            co_cond_wait(cond_odd, mutex);
        }
        shared_counter++;
        co_cond_signal(cond_even);  // 唤醒偶数修改者
        co_mutex_unlock(mutex);
    }
    return NULL;
}

void* co_creater(void* arg) {
    int id = *(int*)arg;
    for (int i = id; i < id + NUM_COROUTINES / 2; ++i) {
        void* func = (i % 2 == 0) ? even_updater : odd_updater;
        coroutines[i] = coroutine_create(func, NULL, 0);
    }

    for (int i = id; i < id + NUM_COROUTINES / 2; ++i) {
        coroutine_join(coroutines[i]);
    }
    return NULL;
}

int main() {
    mutex = co_mutex_alloc();
    cond_even = co_cond_alloc();
    cond_odd = co_cond_alloc();
    int id0 = 0, id1 = NUM_COROUTINES / 2;
    pthread_t t1, t2;
    pthread_create(&t1, NULL, co_creater, &id0);
    pthread_create(&t2, NULL, co_creater, &id1);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    co_mutex_free(mutex);
    co_cond_free(cond_even);
    co_cond_free(cond_odd);
    return 0;
}