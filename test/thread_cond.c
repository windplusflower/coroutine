/*
 * Copyright (c) 2025 windflower
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <pthread.h>
#include <stdatomic.h>

#define MAX_COUNT 100000
#define NUM_THREADS 10000

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