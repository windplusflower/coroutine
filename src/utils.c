#include "utils.h"

//功能跟printf一样，只是输出后会自动刷新缓冲区
void debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    fflush(stdout);
    va_end(args);
}