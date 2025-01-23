#include "coheader.h"
#include "coroutine.h"
#include "hook.h"
co_mutex_t mutex;
int num = 0;
void* test_mutex(const void* arg) {
    int id = *(int*)arg;
    while (1) {
        co_mutex_lock(mutex);
        num++;
        printf("co %d:num is %d before yield\n", id, num);
        sleep(1);
        printf("co %d:num is %d after yield\n", id, num);
        co_mutex_unlock(mutex);
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
实际只有一个协程在运行，因为对于这种情况下，只让一个协程执行或是多个协程执行效率相同。
*/