#include <pthread.h>
#include <stdatomic.h>
#include "coheader.h"

#define MAX_COUNT 100000
#define NUM_COROUTINES 10000

static int shared_counter = 0;
co_mutex_t mutex;
co_cond_t cond_seven;
co_cond_t cond_nonseven;
coroutine_t coroutines[NUM_COROUTINES];

// 7倍修改者
void* seven_updater(void* arg) {
    while (1) {
        co_mutex_lock(mutex);
        while (shared_counter % 7 != 0) {  // 等待为7倍
            co_cond_wait(cond_seven, mutex);
        }
        shared_counter++;
        co_cond_signal(cond_nonseven);  // 唤醒非7倍修改者
        co_mutex_unlock(mutex);
    }
    return NULL;
}

// 非7倍修改者
void* nonseven_updater(void* arg) {
    while (1) {
        co_mutex_lock(mutex);
        while (shared_counter % 7 == 0) {  // 等待为非7倍
            co_cond_wait(cond_nonseven, mutex);
        }
        shared_counter++;
        co_cond_signal(cond_seven);  // 唤醒7倍修改者
        co_mutex_unlock(mutex);
    }
    return NULL;
}

void* co_creater(void* arg) {
    int id = *(int*)arg;
    for (int i = id; i < id + NUM_COROUTINES / 2; ++i) {
        void* func = (i % 2 == 0) ? seven_updater : nonseven_updater;
        coroutines[i] = coroutine_create(func, NULL, 0);
    }

    for (int i = id; i < id + NUM_COROUTINES / 2; ++i) {
        coroutine_join(coroutines[i]);
    }
    return NULL;
}

int main() {
    mutex = co_mutex_alloc();
    cond_seven = co_cond_alloc();
    cond_nonseven = co_cond_alloc();
    int id0 = 0, id1 = NUM_COROUTINES / 2;
    //因为测试机是2核的，所以只开了两个线程。
    pthread_t t1, t2;
    pthread_create(&t1, NULL, co_creater, &id0);
    pthread_create(&t2, NULL, co_creater, &id1);
    pthread_detach(t1);
    pthread_detach(t2);
    while (shared_counter < MAX_COUNT)
        ;
    return 0;
}