#include "alert.h"
#include "storage.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

int alert_init(alert_config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    return 0;
}

int alert_should_fire(alert_config_t *cfg, alert_type_t type) {
    (void)type;
    time_t now = time(NULL);

    // BUG VG-010: Single global last_alert_time for all types/interfaces
    if (now - cfg->last_alert_time < cfg->cooldown_sec) {
        return 0;
    }

    cfg->last_alert_time = now;
    return 1;
}

int alert_execute_script(const alert_config_t *cfg, const alert_t *alert) {
    if (!cfg->alert_command[0]) return 0;

    char cmd[1024];
    // BUG VG-009: system() with unsanitized interface and message
    snprintf(cmd, sizeof(cmd), "%s \"%s\" \"%s\" %lu %lu",
        cfg->alert_command,
        alert->interface,
        alert->message,
        (unsigned long)alert->threshold_value,
        (unsigned long)alert->actual_value);

    int ret = system(cmd);
    return WEXITSTATUS(ret);
}

int alert_check_thresholds(alert_config_t *cfg, const char *iface,
                           double pps, double bps, alert_t *out) {
    if (!cfg->enabled) return 0;

    if (pps > (double)cfg->pps_threshold) {
        out->type = ALERT_PPS_THRESHOLD;
        out->threshold_value = cfg->pps_threshold;
        out->actual_value = (uint64_t)pps;
        snprintf(out->message, sizeof(out->message),
                 "PPS threshold exceeded on %s", iface);
        strncpy(out->interface, iface, sizeof(out->interface) - 1);
        out->timestamp = time(NULL);
        return 1;
    }
    if (bps > (double)cfg->bps_threshold) {
        out->type = ALERT_BPS_THRESHOLD;
        out->threshold_value = cfg->bps_threshold;
        out->actual_value = (uint64_t)bps;
        snprintf(out->message, sizeof(out->message),
                 "BPS threshold exceeded on %s", iface);
        strncpy(out->interface, iface, sizeof(out->interface) - 1);
        out->timestamp = time(NULL);
        return 1;
    }
    return 0;
}

int alert_fire(alert_config_t *cfg, storage_ctx_t *storage, alert_t *alert) {
    setenv("VIGIL_ALERT_TYPE",
           alert->type == ALERT_PPS_THRESHOLD ? "pps_threshold" : "bps_threshold", 1);
    setenv("VIGIL_ALERT_INTERFACE", alert->interface, 1);
    char buf[64];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)alert->threshold_value);
    setenv("VIGIL_ALERT_THRESHOLD", buf, 1);
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)alert->actual_value);
    setenv("VIGIL_ALERT_ACTUAL", buf, 1);
    setenv("VIGIL_ALERT_MESSAGE", alert->message, 1);
    snprintf(buf, sizeof(buf), "%ld", (long)alert->timestamp);
    setenv("VIGIL_ALERT_TIMESTAMP", buf, 1);

    alert->script_exit_code = alert_execute_script(cfg, alert);
    storage_insert_alert(storage, alert);
    log_write(LOG_WARN, "Alert: %s", alert->message);
    return 0;
}
