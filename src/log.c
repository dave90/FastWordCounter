#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "log.h"
#include "constants.h"

static FILE* log_file = NULL;
static int print = 0;

int init_logger(const char* filename, int p) {
    log_file = fopen(filename, "a");
    print = p;
    return (log_file != NULL) ? 0 : -1;
}

void log_message(const char* format, ...) {
    if (!log_file) return;
    
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", t);

    va_list args;
    va_start(args, format);
    fprintf(log_file, "[%s] ", time_buf);
    vfprintf(log_file, format, args);
    fprintf(log_file, "\n");
    fflush(log_file);
    if(print){
        printf( "[%s] ", time_buf);
        vprintf(format, args);
        printf( "\n");
    }
    va_end(args);
}

void close_logger() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}
