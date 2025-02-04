#include <pthread.h>
#include "coheader.h"

#define MAX_COROUTINES 5000
#define TARGET_VALUE 100000

co_mutex_t mutex;
int counter = 0;
int done = 0;
coroutine_t coroutines[MAX_COROUTINES];

void* increment_counter(const void* arg) {
    while (1) {
        co_mutex_lock(mutex);
        counter++;
        if (counter >= TARGET_VALUE) {
            done = 1;
        }
        co_mutex_unlock(mutex);
        if (done) break;
    }
    return NULL;
}

void* creater(void* arg) {
    int num = *(int*)arg;
    for (int i = num; i < num + MAX_COROUTINES / 2; i++) {
        coroutines[i] = coroutine_create(increment_counter, NULL, 0);
    }
    for (int i = num; i < num + MAX_COROUTINES / 2; i++) {
        coroutine_join(coroutines[i]);
    }
}

int main() {
    mutex = co_mutex_alloc();
    int num0 = 0, num1 = MAX_COROUTINES / 2;
    pthread_t t1, t2;
    pthread_create(&t1, NULL, creater, &num0);
    // pthread_create(&t2, NULL, creater, &num1);
    pthread_join(t1, NULL);
    // pthread_join(t2, NULL);
    return 0;
}