#include <alloca.h>
#include <stdlib.h>
#include "coheader.h"
co_cond_t cond;
co_mutex_t mutex;
void* producer(const void* arg) {
    while (1) {
        co_mutex_lock(mutex);
        //模拟切换协程
        sleep(0);
        printf("pro send signal\n");
        co_mutex_unlock(mutex);
        co_cond_signal(cond);
        sleep(1);
    }
    return NULL;
}
void* consumer(const void* arg) {
    while (1) {
        co_mutex_lock(mutex);
        int ret = co_cond_timewait(cond, mutex, 1500);
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
pro send signal
pro send signal
success consum!
timeout!
pro send signal
success consum!
pro send signal
success consum!
pro send signal
success consum!
timeout!
...
*/
