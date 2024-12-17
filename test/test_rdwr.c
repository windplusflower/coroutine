
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

// #include "hook.h"
// #include "coroutine.h"
// #include "log.h"
// #include "utils.h"
#include "coheader.h"
int fd[2];
void read_pipe(void *) {
    int buf[1000];
    while (1) {
        int ret = co_read(fd[0], buf, 1000);
        printf("%d bytes read finished\n", ret);
    }
}
void write_pipe(void *) {
    int buf[10000];
    int cnt = 0;
    while (1) {
        sleep(1);
        int ret = co_write(fd[1], buf, 10000);
        printf("%d bytes write finished %d\n", ret, cnt);
        cnt++;
    }
}
int main() {
    log_set_level_from_env();
    pipe(fd);
    Coroutine *rd = malloc(514), *wr = malloc(514);
    coroutine_init(rd, read_pipe, "readpipe", 0);
    coroutine_init(wr, write_pipe, "writepipe", 0);
    start_eventloop();
    while (1) coroutine_yield();
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