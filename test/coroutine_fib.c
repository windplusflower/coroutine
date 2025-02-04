#include "coheader.h"

#define NUM_coroutines 5000
#define N 20

unsigned long long fibonacci(int n) {
    if (n <= 1) return n;
    unsigned long long a = 0, b = 1, c;
    for (int i = 2; i <= n; ++i) {
        c = a + b;
        a = b;
        b = c;
    }
    return b;
}

void* coroutine_function(const void* arg) {
    int coroutine_id = *((int*)arg);
    unsigned long long result;
    result = fibonacci(coroutine_id % N);
    return NULL;
}

int main() {
    coroutine_t coroutines[NUM_coroutines];
    int coroutine_ids[NUM_coroutines];
    for (int i = 0; i < NUM_coroutines; i++) {
        coroutine_ids[i] = i;
        coroutines[i] = coroutine_create(coroutine_function, &coroutine_ids[i], 0);
    }
    for (int i = 0; i < NUM_coroutines; i++) {
        coroutine_join(coroutines[i]);
    }
    return 0;
}
