#ifndef VIGIL_MULTIFACE_H
#define VIGIL_MULTIFACE_H

#include "capture.h"
#include "config.h"
#include <pthread.h>

#define MAX_INTERFACES 16

typedef struct {
    char           name[64];
    pcap_t        *handle;
    pthread_t      thread;
    traffic_stats_t stats;
    int            active;
    int            thread_started;
} interface_ctx_t;

typedef struct {
    interface_ctx_t interfaces[MAX_INTERFACES];
    int             count;
    pthread_mutex_t lock;
} multiface_ctx_t;

int multiface_init(multiface_ctx_t *mctx);
int multiface_start(multiface_ctx_t *mctx, config_t *cfg);
void multiface_stop(multiface_ctx_t *mctx);
void multiface_get_aggregate_stats(multiface_ctx_t *mctx, traffic_stats_t *agg);

#endif
