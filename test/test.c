#include <stdio.h>

#include "coroutine.h"

void test_func(void *arg) {
    printf("Coroutine function started: %s\n", (char *)arg);
    coroutine_yield();
    printf("Coroutine function resumed: %s\n", (char *)arg);
}

int main() {
    coroutine_t co;
    coroutine_init(&co, test_func, "Test", 1024);

    printf("Main: Starting coroutine...\n");
    coroutine_resume(&co);
    printf("Main: Coroutine suspended.\n");
    printf("Main: Coroutine finished.\n");

    coroutine_free(&co);
    return 0;
}
