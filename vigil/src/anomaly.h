#ifndef VIGIL_ANOMALY_H
#define VIGIL_ANOMALY_H

#define BASELINE_MAX_SAMPLES 3600

typedef struct {
    double   samples[BASELINE_MAX_SAMPLES];
    int      head;
    int      count;
    double   mean;
    double   stddev;
    int      window_sec;
    double   deviation_threshold;
    int      baseline_ready;
    char     interface[16];
} anomaly_ctx_t;

void anomaly_init(anomaly_ctx_t *ctx, const char *iface, int window, double threshold);
void anomaly_add_sample(anomaly_ctx_t *ctx, double rate);
void anomaly_recompute(anomaly_ctx_t *ctx);
int anomaly_check(anomaly_ctx_t *ctx, double current_rate);
double anomaly_score(anomaly_ctx_t *ctx, double current_rate);
void anomaly_reset_baseline(anomaly_ctx_t *ctx);

#endif
