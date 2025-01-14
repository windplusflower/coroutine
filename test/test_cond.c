#include <alloca.h>
#include "coheader.h"
#include "coroutine.h"
#include "hook.h"
co_cond_t cond;
void* producer(const void* arg) {
    while (1) {
        sleep(2);
        printf("pro send signal\n");
        co_cond_signal(cond);
    }
    return NULL;
}
void* consumer(const void* arg) {
    while (1) {
        int ret = co_cond_wait(cond, 1000);
        if (ret)
            printf("success consum!\n");
        else
            printf("wait timeout!\n");
    }
    return NULL;
}
int main() {
    enable_hook();
    cond = co_cond_alloc();
    coroutine_t pro = coroutine_create(producer, NULL, 0);
    coroutine_t con = coroutine_create(consumer, NULL, 0);
    coroutine_join(pro);
    coroutine_join(con);
    return 0;
}
/* 正确输出：
相隔一秒交替输出
wait timeout!
和
pro send signal
success consum!
*/
