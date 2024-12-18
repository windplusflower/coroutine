#include "coheader.h"
void slp(const void *) {
    while (1) {
        co_sleep(1);
        printf("I sleep 1 second\n");
    }
}
int main() {
    log_set_level_from_env();
    coroutine_t co;
    co = coroutine_init(slp, "sleep", 0);
    coroutine_join(co);
}
/*正确输出：
每隔一秒输出一句
I sleep 1 second
*/