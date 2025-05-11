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

#include "coheader.h"

#define NUM_COROUTINES 1000
#define NUM_IO_OPERATIONS 100
coroutine_t coroutines[NUM_COROUTINES];

// 模拟IO操作
void *io_task(void *arg) {
    for (int i = 0; i < NUM_IO_OPERATIONS; i++) {
        usleep(10000); // 睡眠10毫秒，模拟IO操作
    }
    return NULL;
}

int main() {
    enable_hook();

    // 创建协程
    for (int i = 0; i < NUM_COROUTINES; i++) { coroutines[i] = coroutine_create(io_task, NULL, 0); }

    // 等待所有协程完成
    for (int i = 0; i < NUM_COROUTINES; i++) { coroutine_join(coroutines[i]); }

    return 0;
}
