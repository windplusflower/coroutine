#include "coheader.h"

#define NUM_COROUTINES 5000
#define NUM_IO_OPERATIONS 100
coroutine_t coroutines[NUM_COROUTINES];

// 模拟IO操作
void *io_task(const void *arg) {
    for (int i = 0; i < NUM_IO_OPERATIONS; i++) {
        usleep(10000);  // 睡眠10毫秒，模拟IO操作
    }
    return NULL;
}

int main() {
    enable_hook();

    // 创建协程
    for (int i = 0; i < NUM_COROUTINES; i++) {
        coroutines[i] = coroutine_create(io_task, NULL, 0);
    }

    // 等待所有协程完成
    for (int i = 0; i < NUM_COROUTINES; i++) {
        coroutine_join(coroutines[i]);
    }

    return 0;
}
