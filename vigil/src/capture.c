#include "capture.h"
#include "packet.h"
#include "conn.h"
#include "proto.h"
#include "stats.h"
#include <pcap.h>
#include <string.h>

extern conn_table_t g_conn_table;
extern void vigil_on_packet(packet_info_t *info, uint32_t caplen, uint32_t wire_len);

void packet_callback(u_char *user_data, const struct pcap_pkthdr *header,
                     const u_char *packet) {
    capture_ctx_t *ctx = (capture_ctx_t *)user_data;
    traffic_stats_t *stats = &ctx->stats;

    packet_info_t info;
    strncpy(info.interface, ctx->interface, sizeof(info.interface) - 1);

    // BUG VG-001: Missing bounds check on header->caplen
    parse_packet(packet, header->caplen, &info);

    stats->packets_in++;
    stats->bytes_in += header->caplen;  /* BUG VG-001: should use header->len not caplen */

    vigil_on_packet(&info, header->caplen, header->len);
}

int capture_open(capture_ctx_t *ctx, const char *iface) {
    strncpy(ctx->interface, iface, sizeof(ctx->interface) - 1);
    char errbuf[PCAP_ERRBUF_SIZE];
    int promisc = ctx->promiscuous ? 1 : 0;
    ctx->handle = pcap_open_live(iface, ctx->snaplen, promisc, ctx->timeout_ms, errbuf);
    if (!ctx->handle) return -1;
    ctx->running = 1;
    stats_init(&ctx->stats, iface, 1);
    return 0;
}

void capture_close(capture_ctx_t *ctx) {
    ctx->running = 0;
    if (ctx->handle) {
        pcap_breakloop(ctx->handle);
        pcap_close(ctx->handle);
        ctx->handle = NULL;
    }
}

int capture_loop(capture_ctx_t *ctx) {
    return pcap_loop(ctx->handle, -1, packet_callback, (u_char *)ctx);
}
