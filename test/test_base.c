#include <stdio.h>

#include "coheader.h"

static int finished = 0;

void test_func1(const void *arg) {
    printf("Coroutine function started: %s\n", (char *)arg);
    coroutine_yield();
    printf("Coroutine function resumed: %s\n", (char *)arg);
    finished++;
}
void test_func2(const void *arg) {
    printf("Coroutine function started: %s\n", (char *)arg);
    coroutine_yield();
    printf("Coroutine function resumed: %s\n", (char *)arg);
    finished++;
}
void test_func3(const void *arg) {
    printf("Coroutine function started: %s\n", (char *)arg);
    coroutine_yield();
    printf("Coroutine function resumed: %s\n", (char *)arg);
    finished++;
}

int main() {
    printf("Main: Starting coroutines......\n");
    int co1, co2, co3;
    co1 = coroutine_create(test_func1, "Test1", 0);
    co2 = coroutine_create(test_func2, "Test2", 0);
    coroutine_join(co1);
    coroutine_join(co2);
    co3 = coroutine_create(test_func3, "Test3", 0);
    coroutine_resume(co3);
    printf("Main: Coroutine3 suspended.\n");
    coroutine_resume(co3);
    printf("Main: All coroutines finished.\n");
    coroutine_free(co3);
    return 0;
}
/*正确输出：

Main: Starting coroutines......
Coroutine function started: Test1
Coroutine function started: Test2
Coroutine function resumed: Test1
Coroutine function resumed: Test2
Coroutine function started: Test3
Main: Coroutine3 suspended.
Coroutine function resumed: Test3
Main: All coroutines finished.

*/
