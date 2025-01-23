#include "event_manager.h"
#include "utils.h"
#include <pthread.h>
// Mutex* m
// m==&m->mutex
// 所以可以把m当&m->mutex用
typedef struct Mutex {
    pthread_mutex_t mutex;
} Mutex;
__thread static HandleTable MUTEX_TABLE;

void init_mutex_table();
