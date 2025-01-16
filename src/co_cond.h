#include "event_manager.h"
#include "utils.h"
typedef struct Cond {
    EventList* list;
} Cond;
__thread static HandleTable COND_TABLE;

void init_cond_table();
