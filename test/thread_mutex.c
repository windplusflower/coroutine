#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_THREADS 5000
#define TARGET_VALUE 100000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;
int done = 0;
pthread_t threads[MAX_THREADS];

void* increment_counter(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        counter++;
        if (counter >= TARGET_VALUE) {
            done = 1;
        }
        pthread_mutex_unlock(&mutex);
        if (done) break;
    }
    return NULL;
}

int main() {
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, increment_counter, NULL);
    }
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}