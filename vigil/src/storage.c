#include "storage.h"
#include "logger.h"
#include "util.h"
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

static const char *SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS traffic_stats ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "interface TEXT NOT NULL,"
    "timestamp INTEGER NOT NULL,"
    "interval_sec INTEGER NOT NULL,"
    "packets_in INTEGER NOT NULL DEFAULT 0,"
    "packets_out INTEGER NOT NULL DEFAULT 0,"
    "bytes_in INTEGER NOT NULL DEFAULT 0,"
    "bytes_out INTEGER NOT NULL DEFAULT 0,"
    "dropped_packets INTEGER NOT NULL DEFAULT 0);"
    "CREATE INDEX IF NOT EXISTS idx_stats_time ON traffic_stats(timestamp DESC);"
    "CREATE INDEX IF NOT EXISTS idx_stats_iface ON traffic_stats(interface, timestamp DESC);"
    "CREATE TABLE IF NOT EXISTS connections ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "src_ip TEXT NOT NULL, dst_ip TEXT NOT NULL,"
    "src_port INTEGER, dst_port INTEGER, protocol TEXT NOT NULL,"
    "first_seen INTEGER NOT NULL, last_seen INTEGER NOT NULL,"
    "packets INTEGER NOT NULL DEFAULT 0, bytes INTEGER NOT NULL DEFAULT 0,"
    "state TEXT NOT NULL DEFAULT 'ACTIVE', interface TEXT NOT NULL);"
    "CREATE INDEX IF NOT EXISTS idx_conn_src ON connections(src_ip);"
    "CREATE TABLE IF NOT EXISTS protocol_stats ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "interface TEXT NOT NULL, timestamp INTEGER NOT NULL,"
    "protocol TEXT NOT NULL, packet_count INTEGER NOT NULL DEFAULT 0,"
    "byte_count INTEGER NOT NULL DEFAULT 0);"
    "CREATE INDEX IF NOT EXISTS idx_proto_time ON protocol_stats(timestamp DESC);"
    "CREATE TABLE IF NOT EXISTS alerts ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "timestamp INTEGER NOT NULL, interface TEXT NOT NULL,"
    "alert_type TEXT NOT NULL, threshold_value INTEGER NOT NULL,"
    "actual_value INTEGER NOT NULL, message TEXT NOT NULL,"
    "script_exit_code INTEGER, resolved_at INTEGER);"
    "CREATE TABLE IF NOT EXISTS reports ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "generated_at INTEGER NOT NULL, period_start INTEGER NOT NULL,"
    "period_end INTEGER NOT NULL, interface TEXT NOT NULL,"
    "total_packets INTEGER NOT NULL, total_bytes INTEGER NOT NULL,"
    "peak_pps INTEGER NOT NULL, peak_bps INTEGER NOT NULL,"
    "alert_count INTEGER NOT NULL, top_talkers TEXT, protocol_breakdown TEXT);"
    "CREATE TABLE IF NOT EXISTS config_history ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "timestamp INTEGER NOT NULL, config_key TEXT NOT NULL,"
    "old_value TEXT, new_value TEXT NOT NULL, changed_by TEXT NOT NULL);";

int storage_init(storage_ctx_t *ctx, const char *path, int flush_interval) {
    memset(ctx, 0, sizeof(*ctx));
    strncpy(ctx->db_path, path, sizeof(ctx->db_path) - 1);
    ctx->flush_interval = flush_interval;
    ctx->last_flush = time(NULL);

    char dir[512];
    strncpy(dir, path, sizeof(dir) - 1);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        vigil_mkdir_p(dir);
    }

    if (sqlite3_open(path, &ctx->db) != SQLITE_OK) return -1;
    return storage_init_schema(ctx);
}

int storage_init_schema(storage_ctx_t *ctx) {
    char *err = NULL;
    int rc = sqlite3_exec(ctx->db, SCHEMA_SQL, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        if (err) sqlite3_free(err);
        return -1;
    }
    return 0;
}

void storage_close(storage_ctx_t *ctx) {
    if (ctx->db) sqlite3_close(ctx->db);
    ctx->db = NULL;
}

int storage_insert_stats(storage_ctx_t *ctx, const traffic_stats_t *stats) {
    // BUG VG-002: SQL statement is not using prepared statements
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO traffic_stats (interface, timestamp, interval_sec, "
        "packets_in, packets_out, bytes_in, bytes_out, dropped_packets) VALUES "
        "('%s', %ld, %d, %lu, %lu, %lu, %lu, %lu)",
        stats->interface,
        (long)stats->interval_start,
        stats->interval_sec,
        (unsigned long)stats->packets_in,
        (unsigned long)stats->packets_out,
        (unsigned long)stats->bytes_in,
        (unsigned long)stats->bytes_out,
        (unsigned long)stats->dropped);

    char *err_msg = NULL;
    int rc = sqlite3_exec(ctx->db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return -1;
    }
    return 0;
}

int storage_insert_connection(storage_ctx_t *ctx, const connection_t *conn) {
    char src[32], dst[32];
    ip_to_str(conn->src_ip, src, sizeof(src));
    ip_to_str(conn->dst_ip, dst, sizeof(dst));

    const char *sql =
        "INSERT INTO connections (src_ip,dst_ip,src_port,dst_port,protocol,"
        "first_seen,last_seen,packets,bytes,state,interface) VALUES (?,?,?,?,?,?,?,?,?,?,?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    const char *proto = conn->protocol == IPPROTO_TCP ? "TCP" : "UDP";
    const char *state = conn->tcp_state == TCP_STATE_CLOSED ? "CLOSED" : "ACTIVE";

    sqlite3_bind_text(stmt, 1, src, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, dst, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, conn->src_port);
    sqlite3_bind_int(stmt, 4, conn->dst_port);
    sqlite3_bind_text(stmt, 5, proto, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 6, conn->first_seen);
    sqlite3_bind_int64(stmt, 7, conn->last_seen);
    sqlite3_bind_int64(stmt, 8, (sqlite3_int64)conn->packets);
    sqlite3_bind_int64(stmt, 9, (sqlite3_int64)conn->bytes);
    sqlite3_bind_text(stmt, 10, state, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, conn->interface, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

int storage_insert_proto_stats(storage_ctx_t *ctx, const char *iface, time_t ts,
                               protocol_id_t proto, uint64_t pkts, uint64_t bytes) {
    const char *sql =
        "INSERT INTO protocol_stats (interface,timestamp,protocol,packet_count,byte_count) "
        "VALUES (?,?,?,?,?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, iface, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, ts);
    sqlite3_bind_text(stmt, 3, protocol_name(proto), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)pkts);
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)bytes);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

int storage_insert_alert(storage_ctx_t *ctx, const alert_t *alert) {
    const char *sql =
        "INSERT INTO alerts (timestamp,interface,alert_type,threshold_value,"
        "actual_value,message,script_exit_code) VALUES (?,?,?,?,?,?,?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    const char *types[] = {"pps_threshold", "bps_threshold", "anomaly", "port_scan"};
    sqlite3_bind_int64(stmt, 1, alert->timestamp);
    sqlite3_bind_text(stmt, 2, alert->interface, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, types[alert->type], -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)alert->threshold_value);
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)alert->actual_value);
    sqlite3_bind_text(stmt, 6, alert->message, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, alert->script_exit_code);

    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

int storage_insert_report(storage_ctx_t *ctx, report_query_t *r,
                          const char *top_talkers, const char *proto_breakdown) {
    const char *sql =
        "INSERT INTO reports (generated_at,period_start,period_end,interface,"
        "total_packets,total_bytes,peak_pps,peak_bps,alert_count,top_talkers,protocol_breakdown) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    time_t now = time(NULL);
    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_int64(stmt, 2, r->period_start);
    sqlite3_bind_int64(stmt, 3, r->period_end);
    sqlite3_bind_text(stmt, 4, r->interface, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)r->total_packets);
    sqlite3_bind_int64(stmt, 6, (sqlite3_int64)r->total_bytes);
    sqlite3_bind_int64(stmt, 7, (sqlite3_int64)r->peak_pps);
    sqlite3_bind_int64(stmt, 8, (sqlite3_int64)r->peak_bps);
    sqlite3_bind_int(stmt, 9, r->alert_count);
    sqlite3_bind_text(stmt, 10, top_talkers ? top_talkers : "[]", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, proto_breakdown ? proto_breakdown : "{}", -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

int storage_query_period(storage_ctx_t *ctx, time_t start, time_t end, report_query_t *out) {
    const char *sql =
        "SELECT SUM(packets_in+packets_out), SUM(bytes_in+bytes_out) "
        "FROM traffic_stats WHERE timestamp >= ? AND timestamp <= ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int64(stmt, 1, start);
    sqlite3_bind_int64(stmt, 2, end);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out->total_packets = (uint64_t)sqlite3_column_int64(stmt, 0);
        out->total_bytes = (uint64_t)sqlite3_column_int64(stmt, 1);
    }
    sqlite3_finalize(stmt);
    out->period_start = start;
    out->period_end = end;
    return 0;
}

int storage_config_history(storage_ctx_t *ctx, const char *key,
                           const char *old_val, const char *new_val, const char *by) {
    const char *sql =
        "INSERT INTO config_history (timestamp,config_key,old_value,new_value,changed_by) "
        "VALUES (?,?,?,?,?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int64(stmt, 1, time(NULL));
    sqlite3_bind_text(stmt, 2, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, old_val ? old_val : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, new_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, by, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}
