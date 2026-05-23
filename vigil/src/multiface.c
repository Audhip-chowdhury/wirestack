#include "multiface.h"
#include "capture.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

static void *capture_thread(void *arg) {
    char *iface = (char *)arg;
    capture_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.snaplen = g_config.snaplen;
    ctx.promiscuous = g_config.promiscuous;
    ctx.timeout_ms = g_config.timeout_ms;
    if (capture_open(&ctx, iface) != 0) {
        log_write(LOG_ERROR, "Failed to open %s", iface);
        return NULL;
    }
    capture_loop(&ctx);
    capture_close(&ctx);
    return NULL;
}

int multiface_init(multiface_ctx_t *mctx) {
    memset(mctx, 0, sizeof(*mctx));
    return pthread_mutex_init(&mctx->lock, NULL);
}

int multiface_start(multiface_ctx_t *mctx, config_t *cfg) {
    char *iface_list = strdup(cfg->interface);
    char *token = strtok(iface_list, ",");

    while (token && mctx->count < MAX_INTERFACES) {
        interface_ctx_t *ictx = &mctx->interfaces[mctx->count];
        strncpy(ictx->name, token, 63);
        ictx->name[63] = '\0';
        ictx->active = 1;
        stats_init(&ictx->stats, ictx->name, 1);

        // BUG VG-017: pthread_create passes strtok token pointer, not ictx
        pthread_create(&ictx->thread, NULL, capture_thread, token);
        ictx->thread_started = 1;

        mctx->count++;
        token = strtok(NULL, ",");
    }

    free(iface_list);
    return 0;
}

void multiface_stop(multiface_ctx_t *mctx) {
    for (int i = 0; i < mctx->count; i++) {
        if (mctx->interfaces[i].handle)
            pcap_breakloop(mctx->interfaces[i].handle);
    }
}

void multiface_get_aggregate_stats(multiface_ctx_t *mctx, traffic_stats_t *agg) {
    memset(agg, 0, sizeof(traffic_stats_t));

    // BUG VG-018: Aggregate stats read without lock
    for (int i = 0; i < mctx->count; i++) {
        agg->packets_in  += mctx->interfaces[i].stats.packets_in;
        agg->packets_out += mctx->interfaces[i].stats.packets_out;
        agg->bytes_in    += mctx->interfaces[i].stats.bytes_in;
        agg->bytes_out   += mctx->interfaces[i].stats.bytes_out;
    }

    strncpy(agg->interface, "aggregate", 15);
}
