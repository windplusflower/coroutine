#include "utils.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"
void log_set_level_from_env() {
    const char *log_level = getenv("LOG_LEVEL");
    if (log_level != NULL) {
        if (strcmp(log_level, "LOG_TRACE") == 0) {
            log_set_level(LOG_TRACE);
        } else if (strcmp(log_level, "LOG_DEBUG") == 0) {
            log_set_level(LOG_DEBUG);
        } else if (strcmp(log_level, "LOG_INFO") == 0) {
            log_set_level(LOG_INFO);
        } else if (strcmp(log_level, "LOG_WARN") == 0) {
            log_set_level(LOG_WARN);
        } else if (strcmp(log_level, "LOG_ERROR") == 0) {
            log_set_level(LOG_ERROR);
        } else if (strcmp(log_level, "LOG_FATAL") == 0) {
            log_set_level(LOG_FATAL);
        }
    }
}