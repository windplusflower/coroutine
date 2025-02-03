#include <stdatomic.h>
#include "event_manager.h"
#include "utils.h"
typedef struct Cond {
    atomic_int cnt_signal;
    volatile int cnt_broadcast;
    atomic_int waiting_co;
} Cond;

void init_cond_table();
