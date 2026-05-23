#ifndef VIGIL_STATS_H
#define VIGIL_STATS_H

#include "packet.h"
#include <stdint.h>
#include <time.h>

typedef struct {
    uint64_t packets_in;
    uint64_t packets_out;
    uint64_t bytes_in;
    uint64_t bytes_out;
    uint64_t dropped;
    time_t   interval_start;
    int      interval_sec;
    char     interface[16];
} traffic_stats_t;

void stats_init(traffic_stats_t *s, const char *iface, int interval_sec);
void stats_update(traffic_stats_t *s, const packet_info_t *pkt, uint32_t wire_len);
void stats_reset_interval(traffic_stats_t *s);
double stats_pps(const traffic_stats_t *s);
double stats_bps(const traffic_stats_t *s);

#endif
