#ifndef VIGIL_CAPTURE_H
#define VIGIL_CAPTURE_H

#include "stats.h"
#include <pcap.h>

typedef struct {
    pcap_t *handle;
    char    interface[64];
    int     snaplen;
    int     promiscuous;
    int     timeout_ms;
    traffic_stats_t stats;
    volatile int running;
} capture_ctx_t;

int capture_open(capture_ctx_t *ctx, const char *iface);
void capture_close(capture_ctx_t *ctx);
int capture_loop(capture_ctx_t *ctx);
void packet_callback(u_char *user_data, const struct pcap_pkthdr *header,
                     const u_char *packet);

#endif
