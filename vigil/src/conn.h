#ifndef VIGIL_CONN_H
#define VIGIL_CONN_H

#include "packet.h"
#include <pthread.h>
#include <netinet/tcp.h>

#define CONN_TABLE_SIZE 65536

typedef enum {
    TCP_STATE_SYN_SENT,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT,
    TCP_STATE_CLOSED
} tcp_state_t;

typedef struct connection {
    uint32_t     src_ip;
    uint32_t     dst_ip;
    uint16_t     src_port;
    uint16_t     dst_port;
    uint8_t      protocol;
    uint64_t     packets;
    uint64_t     bytes;
    time_t       first_seen;
    time_t       last_seen;
    tcp_state_t  tcp_state;
    char         interface[16];
    struct connection *next;
} connection_t;

typedef struct {
    connection_t *table[CONN_TABLE_SIZE];
    uint32_t      count;
    pthread_mutex_t lock;
    int           timeout_sec;
} conn_table_t;

uint32_t conn_hash(uint32_t src_ip, uint32_t dst_ip,
                   uint16_t src_port, uint16_t dst_port, uint8_t proto);
int conn_table_init(conn_table_t *t, int timeout_sec);
void conn_table_free(conn_table_t *t);
connection_t *conn_find_or_create(conn_table_t *table, const packet_info_t *pkt);
void conn_update_tcp_state(connection_t *conn, const struct tcphdr *tcp);
void *conn_timeout_worker(void *arg);
int conn_start_timeout_thread(conn_table_t *t);

#endif
