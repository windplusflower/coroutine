#include <stdio.h>

#include "coroutine.h"

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

int main() {
    Coroutine co1, co2;
    coroutine_init(&co1, test_func1, "Test1", STACKSIZE);
    coroutine_init(&co2, test_func2, "Test2", STACKSIZE);
    printf("Main: Starting coroutine...\n");
    start_eventloop();
    while (finished < 2) coroutine_yield();
    printf("Main: Coroutine finished.\n");

    coroutine_free(&co1);
    coroutine_free(&co2);
    return 0;
}
