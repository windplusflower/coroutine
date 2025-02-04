#include <pthread.h>
#include <stdatomic.h>

#define MAX_COUNT 100000
#define NUM_THREADS 5000

static int shared_counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_seven = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_nonseven = PTHREAD_COND_INITIALIZER;
pthread_t threads[NUM_THREADS];

// 7倍修改者
void* seven_updater(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (shared_counter % 7 != 0) {  // 等待为7倍
            pthread_cond_wait(&cond_seven, &mutex);
        }
        shared_counter++;
        pthread_cond_signal(&cond_nonseven);  // 唤醒非7倍修改者
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

// 非7倍修改者
void* nonseven_updater(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (shared_counter % 7 == 0) {  // 等待为非7倍
            pthread_cond_wait(&cond_nonseven, &mutex);
        }
        shared_counter++;
        pthread_cond_signal(&cond_seven);  // 唤醒7倍修改者
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    for (int i = 0; i < NUM_THREADS; ++i) {
        void* func = (i % 2 == 0) ? seven_updater : nonseven_updater;
        pthread_create(&threads[i], NULL, func, NULL);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_detach(threads[i]);
    }
    while (shared_counter < MAX_COUNT)
        ;
    return 0;
}