#include "report.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void report_compute_top_talkers(storage_ctx_t *storage, time_t period_start,
                                 time_t period_end, char *output_json,
                                 size_t output_len) {
    (void)period_end;
    const char *sql =
        "SELECT src_ip, SUM(packets) as total_packets, SUM(bytes) as total_bytes "
        "FROM connections "
        "WHERE first_seen >= ? "
        "GROUP BY src_ip "
        "ORDER BY total_packets DESC "  /* BUG VG-012: should be total_bytes DESC */
        "LIMIT 10";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(storage->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        snprintf(output_json, output_len, "[]");
        return;
    }
    sqlite3_bind_int64(stmt, 1, period_start);

    size_t off = 0;
    off += (size_t)snprintf(output_json + off, output_len - off, "[");
    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) off += (size_t)snprintf(output_json + off, output_len - off, ",");
        first = 0;
        const char *ip = (const char *)sqlite3_column_text(stmt, 0);
        int64_t pkts = sqlite3_column_int64(stmt, 1);
        int64_t bytes = sqlite3_column_int64(stmt, 2);
        off += (size_t)snprintf(output_json + off, output_len - off,
            "{\"ip\":\"%s\",\"packets\":%lld,\"bytes\":%lld}",
            ip ? ip : "0.0.0.0", (long long)pkts, (long long)bytes);
        if (off >= output_len - 4) break;
    }
    snprintf(output_json + off, output_len - off, "]");
    sqlite3_finalize(stmt);
}

void report_generate_periodic(storage_ctx_t *storage, report_config_t *cfg) {
    time_t now = time(NULL);
    // BUG VG-013: period may miss last unflushed interval
    time_t period_end   = now;
    time_t period_start = now - cfg->report_interval_sec;

    report_query_t report;
    memset(&report, 0, sizeof(report));
    strncpy(report.interface, "all", sizeof(report.interface) - 1);
    storage_query_period(storage, period_start, period_end, &report);

    char top_talkers[4096];
    char proto_breakdown[256] = "{}";
    report_compute_top_talkers(storage, period_start, period_end,
                               top_talkers, sizeof(top_talkers));
    storage_insert_report(storage, &report, top_talkers, proto_breakdown);
}
