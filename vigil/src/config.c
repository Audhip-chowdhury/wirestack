#include "config.h"
#include "ipc.h"
#include "logger.h"
#include "storage.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

config_t g_config;
char g_config_path[512] = "./vigil.conf";

void config_set_defaults(config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    strncpy(cfg->interface, "eth0", sizeof(cfg->interface) - 1);
    cfg->snaplen = 65535;
    cfg->promiscuous = 1;
    cfg->timeout_ms = 1000;
    strncpy(cfg->db_path, "./data/vigil.db", sizeof(cfg->db_path) - 1);
    cfg->max_connections = 10000;
    cfg->flush_interval_sec = 30;
    strncpy(cfg->log_path, "./logs/vigil.log", sizeof(cfg->log_path) - 1);
    cfg->log_level = LOG_INFO;
    cfg->max_log_size_mb = 100;
    cfg->rotate_count = 5;
    cfg->alerts_enabled = 1;
    cfg->threshold_pps = 10000;
    cfg->threshold_bps = 104857600ULL;
    strncpy(cfg->alert_command, "/etc/vigil/alert.sh", sizeof(cfg->alert_command) - 1);
    cfg->cooldown_sec = 60;
    cfg->report_interval_sec = 300;
    strncpy(cfg->report_output_path, "./logs/reports/", sizeof(cfg->report_output_path) - 1);
    cfg->anomaly_enabled = 0;
    cfg->baseline_window_sec = 3600;
    cfg->deviation_threshold = 3.0;
    strncpy(cfg->socket_path, VIGIL_SOCK_DEFAULT, sizeof(cfg->socket_path) - 1);
    strncpy(cfg->pid_file, "/tmp/vigil.pid", sizeof(cfg->pid_file) - 1);
}

uint64_t parse_uint64(const char *value, const char *key) {
    (void)key;
    // BUG VG-016: atoi() for uint64 — overflows above ~2.1 billion
    return (uint64_t)atoi(value);
}

int config_parse_file(const char *path, config_t *cfg) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[256];
    char key[128], value[128];

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '[' || line[0] == '\n') continue;

        if (sscanf(line, "%127[^=]=%127[^\n]", key, value) == 2) {
            char *k = strtrim(key);
            char *v = strtrim(value);

            if (strcmp(k, "interface") == 0) {
                // BUG VG-014: strncpy without explicit null termination at 64 chars
                strncpy(cfg->interface, v, 64);
            } else if (strcmp(k, "snaplen") == 0) {
                cfg->snaplen = atoi(v);
            } else if (strcmp(k, "promiscuous") == 0) {
                cfg->promiscuous = (strcmp(v, "true") == 0);
            } else if (strcmp(k, "timeout_ms") == 0) {
                cfg->timeout_ms = atoi(v);
            } else if (strcmp(k, "db_path") == 0) {
                strncpy(cfg->db_path, v, MAX_CONFIG_STR - 1);
                cfg->db_path[MAX_CONFIG_STR - 1] = '\0';
            } else if (strcmp(k, "flush_interval_sec") == 0) {
                cfg->flush_interval_sec = atoi(v);
            } else if (strcmp(k, "log_path") == 0) {
                strncpy(cfg->log_path, v, MAX_CONFIG_STR - 1);
            } else if (strcmp(k, "log_level") == 0) {
                if (strcmp(v, "DEBUG") == 0) cfg->log_level = LOG_DEBUG;
                else if (strcmp(v, "WARN") == 0) cfg->log_level = LOG_WARN;
                else if (strcmp(v, "ERROR") == 0) cfg->log_level = LOG_ERROR;
                else cfg->log_level = LOG_INFO;
            } else if (strcmp(k, "max_log_size_mb") == 0) {
                cfg->max_log_size_mb = atoi(v);
            } else if (strcmp(k, "rotate_count") == 0) {
                cfg->rotate_count = atoi(v);
            } else if (strcmp(k, "enabled") == 0) {
                cfg->alerts_enabled = (strcmp(v, "true") == 0);
            } else if (strcmp(k, "threshold_pps") == 0) {
                cfg->threshold_pps = parse_uint64(v, k);
            } else if (strcmp(k, "threshold_bps") == 0) {
                cfg->threshold_bps = parse_uint64(v, k);
            } else if (strcmp(k, "alert_command") == 0) {
                strncpy(cfg->alert_command, v, MAX_CONFIG_STR - 1);
                cfg->alert_command[MAX_CONFIG_STR - 1] = '\0';
            } else if (strcmp(k, "cooldown_sec") == 0) {
                cfg->cooldown_sec = atoi(v);
            } else if (strcmp(k, "interval_sec") == 0) {
                cfg->report_interval_sec = atoi(v);
            } else if (strcmp(k, "output_path") == 0) {
                strncpy(cfg->report_output_path, v, MAX_CONFIG_STR - 1);
            } else if (strcmp(k, "baseline_window_sec") == 0) {
                cfg->baseline_window_sec = atoi(v);
            } else if (strcmp(k, "deviation_threshold") == 0) {
                cfg->deviation_threshold = atof(v);
            } else if (strcmp(k, "interfaces") == 0) {
                strncpy(cfg->interface, v, sizeof(cfg->interface) - 1);
            }
        }
    }

    fclose(f);
    return 0;
}

int config_parse_args(int argc, char **argv, config_t *cfg) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            strncpy(g_config_path, argv[++i], sizeof(g_config_path) - 1);
        } else if (strcmp(argv[i], "--interface") == 0 && i + 1 < argc) {
            strncpy(cfg->interface, argv[++i], sizeof(cfg->interface) - 1);
        } else if (strcmp(argv[i], "--db") == 0 && i + 1 < argc) {
            strncpy(cfg->db_path, argv[++i], MAX_CONFIG_STR - 1);
        } else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            strncpy(cfg->log_path, argv[++i], MAX_CONFIG_STR - 1);
        } else if (strcmp(argv[i], "--daemon") == 0) {
            cfg->daemon_mode = 1;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("vigil 1.0.0 WireStack\n");
            exit(0);
        }
    }
    return 0;
}

void config_reload(config_t *cfg, const char *config_path, void *storage) {
    (void)storage;
    config_t new_cfg;
    memset(&new_cfg, 0, sizeof(config_t));
    config_set_defaults(&new_cfg);

    if (config_parse_file(config_path, &new_cfg) != 0) {
        log_write(LOG_ERROR, "Failed to reload config");
        return;
    }

    // BUG VG-015: Non-atomic memcpy reload — partial reads possible
    memcpy(cfg, &new_cfg, sizeof(config_t));
    log_write(LOG_INFO, "Config reloaded");
}
