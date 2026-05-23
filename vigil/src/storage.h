#ifndef VIGIL_STORAGE_H
#define VIGIL_STORAGE_H

#include "stats.h"
#include "conn.h"
#include "proto.h"
#include "alert.h"
#include <sqlite3.h>
#include <time.h>

typedef struct storage_ctx_t {
    sqlite3 *db;
    char     db_path[256];
    int      flush_interval;
    time_t   last_flush;
} storage_ctx_t;

typedef struct {
    time_t period_start;
    time_t period_end;
    char   interface[64];
    uint64_t total_packets;
    uint64_t total_bytes;
    uint64_t peak_pps;
    uint64_t peak_bps;
    int    alert_count;
} report_query_t;

int storage_init(storage_ctx_t *ctx, const char *path, int flush_interval);
void storage_close(storage_ctx_t *ctx);
int storage_init_schema(storage_ctx_t *ctx);
int storage_insert_stats(storage_ctx_t *ctx, const traffic_stats_t *stats);
int storage_insert_connection(storage_ctx_t *ctx, const connection_t *conn);
int storage_insert_proto_stats(storage_ctx_t *ctx, const char *iface, time_t ts,
                               protocol_id_t proto, uint64_t pkts, uint64_t bytes);
int storage_insert_alert(storage_ctx_t *ctx, const alert_t *alert);
int storage_insert_report(storage_ctx_t *ctx, report_query_t *r,
                          const char *top_talkers, const char *proto_breakdown);
int storage_query_period(storage_ctx_t *ctx, time_t start, time_t end, report_query_t *out);
int storage_config_history(storage_ctx_t *ctx, const char *key,
                           const char *old_val, const char *new_val, const char *by);

#endif
