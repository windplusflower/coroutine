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
#include "coheader.h"

#define TARGET_VALUE 1000
#define MAX_COROUTINES 10000
#define MUTEX_NUM 100

co_mutex_t mutex[MUTEX_NUM];
int counter[MUTEX_NUM];
int done[MUTEX_NUM];
int ids[MAX_COROUTINES];
coroutine_t coroutines[MAX_COROUTINES];

void* increment_counter(const void* arg) {
    int id = *(int*)arg;
    id %= MUTEX_NUM;
    while (1) {
        co_mutex_lock(mutex[id]);
        counter[id]++;
        if (counter[id] >= TARGET_VALUE) {
            done[id] = 1;
        }
        co_mutex_unlock(mutex[id]);
        if (done[id]) break;
    }
    return NULL;
}

void* creater(void* arg) {
    int num = *(int*)arg;
    for (int i = num; i < num + MAX_COROUTINES / 2; i++) {
        coroutines[i] = coroutine_create(increment_counter, ids + i, 0);
    }
    for (int i = num; i < num + MAX_COROUTINES / 2; i++) {
        coroutine_join(coroutines[i]);
    }
    return NULL;
}

int main() {
    for (int i = 0; i < MAX_COROUTINES; i++) ids[i] = i;
    for (int i = 0; i < MUTEX_NUM; i++) mutex[i] = co_mutex_alloc();
    int num0 = 0, num1 = MAX_COROUTINES / 2;
    pthread_t t1, t2;
    pthread_create(&t1, NULL, creater, &num0);
    pthread_create(&t2, NULL, creater, &num1);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}