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
#include "coroutine.h"
#include "hook.h"
co_mutex_t mutex;
int num = 0;
void *test_mutex(void *arg) {
    int id = *(int *)arg;
    while (1) {
        co_mutex_lock(mutex);
        num++;
        printf("co %d:num is %d before yield\n", id, num);
        sleep(1);
        printf("co %d:num is %d after yield\n", id, num);
        co_mutex_unlock(mutex);
        sleep(0);
    }
}
int main() {
    enable_hook();
    mutex = co_mutex_alloc();
    coroutine_t cos[6];
    for (int i = 1; i <= 5; i++) {
        cos[i] = coroutine_create(test_mutex, &cos[i], 0);
        coroutine_detach(cos[i]);
    }
    sleep(-1);
}
/*正确输出：
同一协程的num在before yield和after yield不变
*/