#ifndef VIGIL_LOGGER_H
#define VIGIL_LOGGER_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

typedef struct {
    char  log_path[512];
    FILE *log_file;
    log_level_t min_level;
    int   max_log_size_mb;
    int   rotate_count;
    long  current_size;
} logger_ctx_t;

int logger_init(logger_ctx_t *ctx, const char *path, log_level_t level,
                int max_mb, int rotate_count);
void logger_close(logger_ctx_t *ctx);
void log_write(log_level_t level, const char *fmt, ...);
int logger_rotate(logger_ctx_t *ctx);
void logger_check_rotate(logger_ctx_t *ctx);

extern logger_ctx_t g_logger;

#endif
