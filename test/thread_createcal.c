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
#include <time.h>
#include <stdio.h>
#include <unistd.h>

// 计算毫秒级时间差
double get_runtime(struct timespec start, struct timespec end) {
    double seconds = end.tv_sec - start.tv_sec;
    double nanoseconds = end.tv_nsec - start.tv_nsec;
    double elapsed = seconds + nanoseconds * 1e-9;
    return elapsed * 1000; // 转换为毫秒
}

void *fun(void *arg) {
    volatile int dummy = 0;
    while (1) {
        dummy++;
        sleep(0);
    }
    return NULL;
}

int main() {
    pthread_t co;
    int num = 0;
    struct timespec start_time, current_time;

    // 获取程序启动时间
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (1) {
        pthread_create(&co, NULL, fun, &num);
        pthread_detach(co);
        num++;
        if (num % 100 == 0) {
            // 获取当前时间
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            double runtime = get_runtime(start_time, current_time);
            printf("create %d threads, runtime: %.3fs\n", num, runtime / 1000);
            fflush(stdout);
            if (num == 300) break;
        }
    }
    return 0;
}