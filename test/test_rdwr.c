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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "coheader.h"
#include "hook.h"
int fd[2];
void *read_pipe(void *) {
    int buf[1000];
    while (1) {
        int ret = read(fd[0], buf, 1000);
        printf("%d bytes read finished\n", ret);
    }
    return NULL;
}
void *write_pipe(void *) {
    int buf[10000];
    int cnt = 0;
    while (1) {
        //这里要用阻塞的sleep，不然cpu会让给read，导致无法填满管道
        disable_hook();
        sleep(1);
        enable_hook();
        int ret = write(fd[1], buf, 10000);
        printf("%d bytes write finished %d\n", ret, cnt);
        cnt++;
    }
    return NULL;
}
int main() {
    enable_hook();
    pipe(fd);
    coroutine_t rd = coroutine_create(read_pipe, "readpipe", 0);
    coroutine_t wr = coroutine_create(write_pipe, "writepipe", 0);
    coroutine_join(rd);
    coroutine_join(wr);
}
/*
管道缓冲区为65536，第七次写开始管道容量不足，
管道写容量不足时以页为单位，一次只能写4096的倍数。
正确输出：
10000 bytes write finished 0
read finished
10000 bytes write finished 1
read finished
10000 bytes write finished 2
read finished
10000 bytes write finished 3
read finished
10000 bytes write finished 4
read finished
10000 bytes write finished 5
read finished
8192 bytes write finished 6
read finished
read finished
read finished
4096 bytes write finished 7
read finished
read finished
read finished
4096 bytes write finished 8
read finished
read finished
read finished
read finished
......
*/