#include "event_manager.h"
#include "utils.h"
typedef struct Mutex {
    CoList* list;
    bool is_locked;
} Mutex;
__thread static HandleTable MUTEX_TABLE;

void init_mutex_table();
