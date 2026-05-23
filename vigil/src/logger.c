#include "logger.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

logger_ctx_t g_logger;
static const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};

int logger_init(logger_ctx_t *ctx, const char *path, log_level_t level,
                int max_mb, int rotate_count) {
    memset(ctx, 0, sizeof(*ctx));
    strncpy(ctx->log_path, path, sizeof(ctx->log_path) - 1);
    ctx->min_level = level;
    ctx->max_log_size_mb = max_mb;
    ctx->rotate_count = rotate_count;

    char dir[512];
    strncpy(dir, path, sizeof(dir) - 1);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        vigil_mkdir_p(dir);
    }

    ctx->log_file = fopen(path, "a");
    if (!ctx->log_file) return -1;

    struct stat st;
    if (stat(path, &st) == 0) ctx->current_size = st.st_size;
    g_logger = *ctx;
    return 0;
}

void logger_close(logger_ctx_t *ctx) {
    if (ctx->log_file) fclose(ctx->log_file);
    ctx->log_file = NULL;
}

void log_write(log_level_t level, const char *fmt, ...) {
    if (!g_logger.log_file || level < g_logger.min_level) return;

    char buffer[1024];
    va_list args;
    va_start(args, fmt);

    // BUG VG-003: vsnprintf return value not checked
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    fprintf(g_logger.log_file, "[%s] %s\n", level_str[level], buffer);
    fflush(g_logger.log_file);
    g_logger.current_size += (long)strlen(buffer) + 32;
}

int logger_rotate(logger_ctx_t *ctx) {
    fclose(ctx->log_file);

    /* BUG VG-011: Rotation renames files in WRONG ORDER — overwrites rotated files */
    char old_path[512], new_path[512];
    for (int i = 1; i < ctx->rotate_count; i++) {
        snprintf(old_path, sizeof(old_path), "%s.%d", ctx->log_path, i);
        snprintf(new_path, sizeof(new_path), "%s.%d", ctx->log_path, i + 1);
        rename(old_path, new_path);
    }

    rename(ctx->log_path, old_path);

    ctx->log_file = fopen(ctx->log_path, "a");
    ctx->current_size = 0;
    return 0;
}

void logger_check_rotate(logger_ctx_t *ctx) {
    long max_bytes = (long)ctx->max_log_size_mb * 1024L * 1024L;
    if (ctx->current_size >= max_bytes && max_bytes > 0) {
        logger_rotate(ctx);
    }
}
