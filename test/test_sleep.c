#include "coheader.h"
void* slp(const void*) {
    while (1) {
        sleep(1);
        printf("I sleep 1 second\n");
    }
    return NULL;
}
int main() {
    coroutine_t co;
    co = coroutine_create(slp, "sleep", 0);
    coroutine_join(co);
}
/*正确输出：
每隔一秒输出一句
I sleep 1 second
*/