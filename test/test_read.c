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

char buf[1024];
void *read_test(void *) {
    while (1) {
        printf("please input:\n");
        read(0, buf, 1024);
        printf("Read from cmd:%s\n", buf);
    }
    return NULL;
}
void *testing_read_suspend(void *) {
    while (1) {
        printf("read was not running now!\n");
        sleep(1);
    }
    return NULL;
}
int main() {
    enable_hook();
    coroutine_t reading, suspending;
    //参数并不使用，只是便于调试
    reading = coroutine_create(read_test, "read_test", 0);
    suspending = coroutine_create(testing_read_suspend, "testing_suspend", 0);
    coroutine_join(reading);
    coroutine_join(suspending);
    return 0;
}
/*正确输出：
please input:
然后每隔一秒输出read was not running now!
当键盘输入字符串s时会输出
Read from cmd:s
please input:
之后继续每隔一秒输出read was not running now!
如此循环
*/