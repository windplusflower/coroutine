#include "coheader.h"
void* func(const void* arg) {
    printf("A\n");
    coroutine_yield();
    printf("C\n");
    return NULL;
}
int main() {
    coroutine_t co = coroutine_create(func, NULL, 0);
    coroutine_resume(co);
    printf("B\n");
    coroutine_resume(co);
    coroutine_free(co);
}
/*输出：
A
B
C
*/