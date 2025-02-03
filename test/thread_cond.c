#include <pthread.h>
#include <stdatomic.h>

#define NUM_THREADS 1000
#define MAX_COUNT 100000

static int shared_counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_even = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_odd = PTHREAD_COND_INITIALIZER;
pthread_t threads[NUM_THREADS];

// 偶数修改者
void* even_updater(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        if (shared_counter >= MAX_COUNT) {
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        while (shared_counter % 2 != 0) {  // 等待为偶数
            pthread_cond_wait(&cond_even, &mutex);
        }
        shared_counter++;
        pthread_cond_signal(&cond_odd);  // 唤醒奇数修改者
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

// 奇数修改者
void* odd_updater(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        if (shared_counter >= MAX_COUNT) {
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        while (shared_counter % 2 != 1) {  // 等待为奇数
            pthread_cond_wait(&cond_odd, &mutex);
        }
        shared_counter++;
        pthread_cond_signal(&cond_even);  // 唤醒偶数修改者
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    // 创建各角色线程（示例：各占50%）
    for (int i = 0; i < NUM_THREADS; ++i) {
        void* func = (i % 2 == 0) ? even_updater : odd_updater;
        pthread_create(&threads[i], NULL, func, NULL);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_even);
    pthread_cond_destroy(&cond_odd);
    return 0;
}