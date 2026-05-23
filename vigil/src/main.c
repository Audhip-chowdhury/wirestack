#include "config.h"
#include "capture.h"
#include "conn.h"
#include "storage.h"
#include "logger.h"
#include "alert.h"
#include "anomaly.h"
#include "multiface.h"
#include "report.h"
#include "ipc.h"
#include "proto.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netinet/tcp.h>

conn_table_t g_conn_table;
storage_ctx_t g_storage;
alert_config_t g_alert_cfg;
anomaly_ctx_t g_anomaly;
multiface_ctx_t g_multiface;
capture_ctx_t g_capture;
volatile int g_running = 1;

static proto_stats_t g_proto_counts[PROTO_COUNT];

void vigil_on_packet(packet_info_t *info, uint32_t caplen, uint32_t wire_len) {
    (void)caplen;
    connection_t *conn = NULL;
    if (info->protocol == IPPROTO_TCP || info->protocol == IPPROTO_UDP) {
        conn = conn_find_or_create(&g_conn_table, info);
    }

    protocol_id_t app = detect_application_protocol(
        info->src_port, info->dst_port, info->protocol, NULL, 0);
    if (app < PROTO_COUNT) {
        g_proto_counts[app].packet_count++;
        g_proto_counts[app].byte_count += wire_len;
        g_proto_counts[app].proto = app;
        strncpy(g_proto_counts[app].interface, info->interface, 15);
    }

    double pps = stats_pps(&g_capture.stats);
    double bps = stats_bps(&g_capture.stats);

    if (g_config.anomaly_enabled) {
        anomaly_add_sample(&g_anomaly, pps);
        anomaly_recompute(&g_anomaly);
        if (anomaly_check(&g_anomaly, pps)) {
            alert_t a = {0};
            a.type = ALERT_ANOMALY;
            strncpy(a.interface, info->interface, sizeof(a.interface) - 1);
            snprintf(a.message, sizeof(a.message), "Anomaly detected on %s", info->interface);
            a.timestamp = time(NULL);
            if (alert_should_fire(&g_alert_cfg, ALERT_ANOMALY))
                alert_fire(&g_alert_cfg, &g_storage, &a);
        }
    }

    alert_t alert;
    if (g_alert_cfg.enabled && alert_check_thresholds(&g_alert_cfg, info->interface, pps, bps, &alert)) {
        if (alert_should_fire(&g_alert_cfg, alert.type))
            alert_fire(&g_alert_cfg, &g_storage, &alert);
    }

    (void)conn;
}

static void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) g_running = 0;
    if (sig == SIGHUP) config_reload(&g_config, g_config_path, &g_storage);
}

static void flush_stats(void) {
    storage_insert_stats(&g_storage, &g_capture.stats);
    for (int p = 0; p < PROTO_COUNT; p++) {
        if (g_proto_counts[p].packet_count > 0) {
            storage_insert_proto_stats(&g_storage, g_proto_counts[p].interface,
                time(NULL), g_proto_counts[p].proto,
                g_proto_counts[p].packet_count, g_proto_counts[p].byte_count);
        }
    }
    stats_reset_interval(&g_capture.stats);
    g_storage.last_flush = time(NULL);
}

int main(int argc, char **argv) {
    config_set_defaults(&g_config);
    config_parse_file(g_config_path, &g_config);
    config_parse_args(argc, argv, &g_config);

    log_level_t lvl = (log_level_t)g_config.log_level;
    if (logger_init(&g_logger, g_config.log_path, lvl,
                    g_config.max_log_size_mb, g_config.rotate_count) != 0) {
        fprintf(stderr, "Failed to init logger\n");
        return 1;
    }

    if (storage_init(&g_storage, g_config.db_path, g_config.flush_interval_sec) != 0) {
        log_write(LOG_ERROR, "Storage init failed");
        return 1;
    }

    conn_table_init(&g_conn_table, 300);
    conn_start_timeout_thread(&g_conn_table);

    g_alert_cfg.enabled = g_config.alerts_enabled;
    g_alert_cfg.pps_threshold = g_config.threshold_pps;
    g_alert_cfg.bps_threshold = g_config.threshold_bps;
    strncpy(g_alert_cfg.alert_command, g_config.alert_command, sizeof(g_alert_cfg.alert_command) - 1);
    g_alert_cfg.cooldown_sec = g_config.cooldown_sec;

    anomaly_init(&g_anomaly, g_config.interface,
                 g_config.baseline_window_sec, g_config.deviation_threshold);

    ipc_set_context(&g_storage, &g_conn_table);
    ipc_start(g_config.socket_path);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGHUP, handle_signal);

    log_write(LOG_INFO, "vigil starting on %s", g_config.interface);

    int multi = strchr(g_config.interface, ',') != NULL;
    if (multi) {
        multiface_init(&g_multiface);
        multiface_start(&g_multiface, &g_config);
        while (g_running) sleep(1);
        multiface_stop(&g_multiface);
    } else {
        g_capture.snaplen = g_config.snaplen;
        g_capture.promiscuous = g_config.promiscuous;
        g_capture.timeout_ms = g_config.timeout_ms;
        stats_init(&g_capture.stats, g_config.interface, 1);

        if (capture_open(&g_capture, g_config.interface) != 0) {
            log_write(LOG_ERROR, "Capture open failed (need root/CAP_NET_RAW?)");
            return 1;
        }

        time_t last_flush = time(NULL);
        time_t last_report = time(NULL);
        report_config_t rcfg;
        rcfg.report_interval_sec = g_config.report_interval_sec;
        strncpy(rcfg.report_output_path, g_config.report_output_path, sizeof(rcfg.report_output_path) - 1);

        while (g_running) {
            struct pcap_pkthdr *hdr;
            const u_char *data;
            int r = pcap_next_ex(g_capture.handle, &hdr, &data);
            if (r == 1) {
                packet_callback((u_char *)&g_capture, hdr, data);
            } else if (r == -1) break;

            time_t now = time(NULL);
            if (now - last_flush >= g_config.flush_interval_sec) {
                flush_stats();
                last_flush = now;
            }
            if (now - last_report >= g_config.report_interval_sec) {
                report_generate_periodic(&g_storage, &rcfg);
                last_report = now;
            }
            logger_check_rotate(&g_logger);
        }
        capture_close(&g_capture);
    }

    flush_stats();
    ipc_stop();
    logger_close(&g_logger);
    storage_close(&g_storage);
    conn_table_free(&g_conn_table);
    log_write(LOG_INFO, "vigil stopped");
    return 0;
}
