#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define TARGET_VALUE 1000
#define MAX_THREADS 10000
#define MUTEX_NUM 100

pthread_mutex_t mutex[MUTEX_NUM];
int counter[MUTEX_NUM];
int done[MUTEX_NUM];
int ids[MAX_THREADS];
pthread_t threads[MAX_THREADS];

void* increment_counter(void* arg) {
    int id = *(int*)arg;
    id %= MUTEX_NUM;
    while (1) {
        pthread_mutex_lock(mutex + id);
        counter[id]++;
        if (counter[id] >= TARGET_VALUE) {
            done[id] = 1;
        }
        pthread_mutex_unlock(mutex + id);
        if (done[id]) break;
    }
    return NULL;
}

int main() {
    for (int i = 0; i < MUTEX_NUM; i++) {
        pthread_mutex_init(mutex + i, NULL);
    }
    for (int i = 0; i < MAX_THREADS; i++) {
        ids[i] = i;
    }
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, increment_counter, ids + i);
    }
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}