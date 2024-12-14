#include "coroutine.h"
#include "hook.h"
#include "utils.h"
void slp(void *) {
    while (1) {
        co_sleep(1);
        printf("I sleep 1 second\n");
    }
}
int main() {
    log_set_level_from_env();
    Coroutine co;
    coroutine_init(&co, slp, "sleep", 0);
    start_eventloop();
    while (1) coroutine_yield();
}
/*正确输出：
每隔一秒输出一句
I sleep 1 second
*/