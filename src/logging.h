// Simple logging module
#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <stdarg.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level_t;

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

void log_printf(log_level_t level, const char *fmt, ...);

#define LOG_DEBUG(...) do { if(LOG_LEVEL<=LOG_LEVEL_DEBUG) log_printf(LOG_LEVEL_DEBUG, __VA_ARGS__); } while(0)
#define LOG_INFO(...)  do { if(LOG_LEVEL<=LOG_LEVEL_INFO)  log_printf(LOG_LEVEL_INFO,  __VA_ARGS__); } while(0)
#define LOG_WARN(...)  do { if(LOG_LEVEL<=LOG_LEVEL_WARN)  log_printf(LOG_LEVEL_WARN,  __VA_ARGS__); } while(0)
#define LOG_ERROR(...) do { if(LOG_LEVEL<=LOG_LEVEL_ERROR) log_printf(LOG_LEVEL_ERROR, __VA_ARGS__); } while(0)

#endif // LOGGING_H
