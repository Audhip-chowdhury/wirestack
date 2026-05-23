#ifndef VIGIL_CONFIG_H
#define VIGIL_CONFIG_H

#include <stdint.h>

#define MAX_CONFIG_STR 512

typedef struct {
    char     interface[64];
    int      snaplen;
    int      promiscuous;
    int      timeout_ms;
    char     db_path[MAX_CONFIG_STR];
    int      max_connections;
    int      flush_interval_sec;
    char     log_path[MAX_CONFIG_STR];
    int      log_level;
    int      max_log_size_mb;
    int      rotate_count;
    int      alerts_enabled;
    uint64_t threshold_pps;
    uint64_t threshold_bps;
    char     alert_command[MAX_CONFIG_STR];
    int      cooldown_sec;
    int      report_interval_sec;
    char     report_output_path[MAX_CONFIG_STR];
    int      anomaly_enabled;
    int      baseline_window_sec;
    double   deviation_threshold;
    char     socket_path[MAX_CONFIG_STR];
    char     pid_file[MAX_CONFIG_STR];
    int      daemon_mode;
} config_t;

void config_set_defaults(config_t *cfg);
int config_parse_file(const char *path, config_t *cfg);
int config_parse_args(int argc, char **argv, config_t *cfg);
void config_reload(config_t *cfg, const char *config_path, void *storage);
uint64_t parse_uint64(const char *value, const char *key);

extern config_t g_config;
extern char g_config_path[512];

#endif
