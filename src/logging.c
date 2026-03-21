#include "logging.h"
#include <time.h>
#include <stdarg.h>

static const char *level_names[] = {"DEBUG","INFO","WARN","ERROR"};

void log_printf(log_level_t level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char timestr[32] = {0};
    if (tm_info) strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(stdout, "%s [%s] ", timestr, level_names[level]);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    fflush(stdout);
    va_end(ap);
}
