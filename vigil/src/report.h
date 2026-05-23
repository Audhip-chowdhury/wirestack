#ifndef VIGIL_REPORT_H
#define VIGIL_REPORT_H

#include "storage.h"

typedef struct {
    int report_interval_sec;
    char report_output_path[512];
} report_config_t;

void report_compute_top_talkers(storage_ctx_t *storage, time_t period_start,
                                 time_t period_end, char *output_json, size_t output_len);
void report_generate_periodic(storage_ctx_t *storage, report_config_t *cfg);

#endif
