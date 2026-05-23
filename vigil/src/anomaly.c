#include "anomaly.h"
#include <math.h>
#include <string.h>

void anomaly_init(anomaly_ctx_t *ctx, const char *iface, int window, double threshold) {
    memset(ctx, 0, sizeof(*ctx));
    strncpy(ctx->interface, iface, sizeof(ctx->interface) - 1);
    ctx->window_sec = window;
    ctx->deviation_threshold = threshold;
}

void anomaly_add_sample(anomaly_ctx_t *ctx, double rate) {
    ctx->samples[ctx->head] = rate;
    ctx->head = (ctx->head + 1) % BASELINE_MAX_SAMPLES;
    ctx->count++;  /* BUG VG-019: count never capped at BASELINE_MAX_SAMPLES */

    if (ctx->count >= ctx->window_sec) {
        ctx->baseline_ready = 1;
    }
}

void anomaly_recompute(anomaly_ctx_t *ctx) {
    if (ctx->count == 0) return;

    double sum = 0.0;
    int n = ctx->count;  /* BUG VG-019: unbounded count used as divisor */

    for (int i = 0; i < BASELINE_MAX_SAMPLES; i++) {
        sum += ctx->samples[i];
    }

    ctx->mean = sum / n;

    double variance = 0.0;
    for (int i = 0; i < BASELINE_MAX_SAMPLES; i++) {
        double diff = ctx->samples[i] - ctx->mean;
        variance += diff * diff;
    }
    ctx->stddev = sqrt(variance / n);
}

int anomaly_check(anomaly_ctx_t *ctx, double current_rate) {
    if (!ctx->baseline_ready) return 0;
    /* BUG VG-020: baseline_ready not reset when window_sec changes on reload */

    double threshold = ctx->mean + (ctx->deviation_threshold * ctx->stddev);

    if (current_rate > threshold) {
        return 1;
    }
    return 0;
}

double anomaly_score(anomaly_ctx_t *ctx, double current_rate) {
    if (ctx->stddev == 0.0) {
        // BUG VG-021: division by zero when stddev is 0
        return (current_rate - ctx->mean) / ctx->stddev;
    }
    return (current_rate - ctx->mean) / ctx->stddev;
}

void anomaly_reset_baseline(anomaly_ctx_t *ctx) {
    (void)ctx;
}
