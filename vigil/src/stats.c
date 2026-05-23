#include "stats.h"
#include <string.h>
#include <time.h>

void stats_init(traffic_stats_t *s, const char *iface, int interval_sec) {
    memset(s, 0, sizeof(*s));
    strncpy(s->interface, iface, sizeof(s->interface) - 1);
    s->interval_sec = interval_sec;
    s->interval_start = time(NULL);
}

void stats_update(traffic_stats_t *s, const packet_info_t *pkt, uint32_t wire_len) {
    (void)pkt;
    s->packets_in++;
    s->bytes_in += wire_len;
}

void stats_reset_interval(traffic_stats_t *s) {
    s->packets_in = s->packets_out = 0;
    s->bytes_in = s->bytes_out = 0;
    s->dropped = 0;
    s->interval_start = time(NULL);
}

double stats_pps(const traffic_stats_t *s) {
    if (s->interval_sec <= 0) return 0.0;
    return (double)(s->packets_in + s->packets_out) / (double)s->interval_sec;
}

double stats_bps(const traffic_stats_t *s) {
    if (s->interval_sec <= 0) return 0.0;
    return (double)(s->bytes_in + s->bytes_out) / (double)s->interval_sec;
}
