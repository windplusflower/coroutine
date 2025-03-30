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

#include <alloca.h>
#include <stdlib.h>
#include "coheader.h"
co_cond_t cond;
co_mutex_t mutex;
void *producer(void *arg) {
    while (1) {
        co_mutex_lock(mutex);
        //模拟切换协程
        sleep(0);
        printf("pro send signal\n");
        co_mutex_unlock(mutex);
        co_cond_signal(cond);
        sleep(3);
    }
    return NULL;
}
void *consumer(void *arg) {
    while (1) {
        co_mutex_lock(mutex);
        int ret = co_cond_timewait(cond, mutex, 2000);
        if (ret == 0)
            printf("success consum!\n");
        else
            printf("timeout!\n");
        co_mutex_unlock(mutex);
    }
    return NULL;
}
int main() {
    enable_hook();
    cond = co_cond_alloc();
    mutex = co_mutex_alloc();
    coroutine_t pro = coroutine_create(producer, NULL, 0);
    coroutine_t con = coroutine_create(consumer, NULL, 0);
    coroutine_join(pro);
    coroutine_join(con);
    return 0;
}
/* 正确输出：
隔2秒，超时
再隔1秒，成功
以此循环
...
*/
