#ifndef VIGIL_ALERT_H
#define VIGIL_ALERT_H

#include <stdint.h>
#include <time.h>

typedef enum {
    ALERT_PPS_THRESHOLD,
    ALERT_BPS_THRESHOLD,
    ALERT_ANOMALY,
    ALERT_PORT_SCAN
} alert_type_t;

typedef struct {
    alert_type_t  type;
    char          interface[16];
    uint64_t      threshold_value;
    uint64_t      actual_value;
    char          message[512];
    time_t        timestamp;
    int           script_exit_code;
} alert_t;

typedef struct {
    uint64_t  pps_threshold;
    uint64_t  bps_threshold;
    char      alert_command[512];
    int       cooldown_sec;
    time_t    last_alert_time;
    int       enabled;
} alert_config_t;

int alert_init(alert_config_t *cfg);
int alert_should_fire(alert_config_t *cfg, alert_type_t type);
int alert_check_thresholds(alert_config_t *cfg, const char *iface,
                           double pps, double bps, alert_t *out);
struct storage_ctx_t;
int alert_fire(alert_config_t *cfg, struct storage_ctx_t *storage, alert_t *alert);
int alert_execute_script(const alert_config_t *cfg, const alert_t *alert);

#endif
