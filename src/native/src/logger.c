#include <stdio.h>
#include <stdarg.h>

void log_info(const char* msg, ...) {
    va_list args;
    fprintf(stdout, "[LudicrousNative/INFO] ");
    va_start(args, msg);
    vfprintf(stdout, msg, args);
    va_end(args);
}

void log_err(const char* msg, ...) {
    va_list args;
    fprintf(stdout, "[LudicrousNative/ERROR] ");
    va_start(args, msg);
    vfprintf(stdout, msg, args);
    va_end(args);
}