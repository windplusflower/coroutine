#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 10000
#define NUM_IO_OPERATIONS 100
pthread_t threads[NUM_THREADS];
// 模拟IO操作
void *io_task(void *arg) {
    for (int i = 0; i < NUM_IO_OPERATIONS; i++) {
        usleep(10000);  // 睡眠10毫秒，模拟IO操作
    }
    return NULL;
}

int main() {
    // 创建线程
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, io_task, NULL);
    }

    // 等待所有线程完成
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
