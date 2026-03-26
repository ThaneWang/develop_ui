#include "logging.h"
#include <time.h>
#include <stdarg.h>
#ifdef _WIN32
#include <windows.h>
#endif

static const char *level_names[] = {"DEBUG","INFO","WARN","ERROR"};

void log_printf(log_level_t level, const char *fmt, ...)
{
#ifdef _WIN32
    static int s_console_utf8;
    if(!s_console_utf8) {
        SetConsoleOutputCP(65001);
        SetConsoleCP(65001);
        s_console_utf8 = 1;
    }
#endif
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
