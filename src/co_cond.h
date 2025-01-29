#include <stdatomic.h>
#include "event_manager.h"
#include "utils.h"
typedef struct Cond {
    atomic_int cnt_signal;
    volatile int cnt_broadcast;
} Cond;

void init_cond_table();
