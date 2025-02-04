#include <stdatomic.h>
#include "event_manager.h"
#include "utils.h"
typedef struct Cond {
    CoList* waiting_co;
    void* mutex;
} Cond;
