#include <stdio.h>

#include "coroutine.h"
#include "utils.h"

static int finished = 0;

void test_func1(void *arg) {
    printf("Coroutine function started: %s\n", (char *)arg);
    coroutine_yield();
    printf("Coroutine function resumed: %s\n", (char *)arg);
    finished++;
}
void test_func2(void *arg) {
    printf("Coroutine function started: %s\n", (char *)arg);
    coroutine_yield();
    printf("Coroutine function resumed: %s\n", (char *)arg);
    finished++;
}
void test_func3(void *arg) {
    printf("Coroutine function started: %s\n", (char *)arg);
    coroutine_yield();
    printf("Coroutine function resumed: %s\n", (char *)arg);
    finished++;
}

int main() {
    log_set_level_from_env();
    printf("Main: Starting coroutines......\n");
    Coroutine co1, co2, co3;
    coroutine_init(&co1, test_func1, "Test1", STACKSIZE);
    coroutine_init(&co2, test_func2, "Test2", STACKSIZE);
    start_eventloop();
    while (finished < 2) coroutine_yield();
    coroutine_init(&co3, test_func2, "Test3", STACKSIZE);
    coroutine_resume(&co3);
    printf("Main: Coroutine3 suspended.\n");
    coroutine_resume(&co3);
    printf("Main: All coroutines finished.\n");

    coroutine_free(&co1);
    coroutine_free(&co2);
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
