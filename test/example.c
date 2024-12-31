#include "coheader.h"
void* func(const void* arg) {
    printf("A\n");
    coroutine_yield();
    printf("B\n");
    return NULL;
}
int main() {
    coroutine_t co = coroutine_create(func, NULL, 0);
    coroutine_join(co);
    printf("C\n");
}
/*输出：
A
B
C
*/