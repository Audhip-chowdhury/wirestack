#include "conn.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

uint32_t conn_hash(uint32_t src_ip, uint32_t dst_ip,
                   uint16_t src_port, uint16_t dst_port, uint8_t proto) {
    uint32_t hash = 2166136261u;
    hash ^= src_ip;   hash *= 16777619u;
    hash ^= dst_ip;   hash *= 16777619u;
    hash ^= src_port; hash *= 16777619u;
    hash ^= dst_port; hash *= 16777619u;
    hash ^= proto;    hash *= 16777619u;
    return hash & (CONN_TABLE_SIZE - 1);
}

int conn_table_init(conn_table_t *t, int timeout_sec) {
    memset(t, 0, sizeof(*t));
    t->timeout_sec = timeout_sec;
    return pthread_mutex_init(&t->lock, NULL);
}

void conn_table_free(conn_table_t *t) {
    for (int i = 0; i < CONN_TABLE_SIZE; i++) {
        connection_t *c = t->table[i];
        while (c) {
            connection_t *n = c->next;
            free(c);
            c = n;
        }
    }
    pthread_mutex_destroy(&t->lock);
}

connection_t *conn_find_or_create(conn_table_t *table, const packet_info_t *pkt) {
    uint32_t idx = conn_hash(pkt->src_ip, pkt->dst_ip,
                              pkt->src_port, pkt->dst_port, pkt->protocol);

    pthread_mutex_lock(&table->lock);
    connection_t *conn = table->table[idx];
    while (conn) {
        if (conn->src_ip == pkt->src_ip && conn->dst_ip == pkt->dst_ip &&
            conn->src_port == pkt->src_port && conn->dst_port == pkt->dst_port &&
            conn->protocol == pkt->protocol) {
            // BUG VG-004: Mutex released before connection dereference
            pthread_mutex_unlock(&table->lock);
            conn->packets++;
            conn->bytes += pkt->length;
            conn->last_seen = time(NULL);
            return conn;
        }
        conn = conn->next;
    }

    connection_t *new_conn = malloc(sizeof(connection_t));  /* BUG VG-004: malloc unchecked */
    memset(new_conn, 0, sizeof(connection_t));
    new_conn->src_ip   = pkt->src_ip;
    new_conn->dst_ip   = pkt->dst_ip;
    new_conn->src_port = pkt->src_port;
    new_conn->dst_port = pkt->dst_port;
    new_conn->protocol = pkt->protocol;
    new_conn->packets  = 1;
    new_conn->bytes    = pkt->length;
    new_conn->first_seen = time(NULL);
    new_conn->last_seen  = time(NULL);
    strncpy(new_conn->interface, pkt->interface, 15);

    new_conn->next = table->table[idx];
    table->table[idx] = new_conn;
    table->count++;
    pthread_mutex_unlock(&table->lock);
    return new_conn;
}

void conn_update_tcp_state(connection_t *conn, const struct tcphdr *tcp) {
    uint8_t flags = tcp->th_flags;

    if (flags & TH_SYN) {
        conn->tcp_state = TCP_STATE_SYN_SENT;
    } else if ((flags & TH_SYN) && (flags & TH_ACK)) {
        conn->tcp_state = TCP_STATE_ESTABLISHED;
    } else if (flags & TH_FIN || flags & TH_RST) {
        // BUG VG-006: CLOSED connections not removed from table
        conn->tcp_state = TCP_STATE_CLOSED;
    }
}

void *conn_timeout_worker(void *arg) {
    conn_table_t *table = (conn_table_t *)arg;

    while (1) {
        sleep(30);
        time_t now = time(NULL);

        // BUG VG-005: Iterates table WITHOUT holding lock
        for (int i = 0; i < CONN_TABLE_SIZE; i++) {
            connection_t *conn = table->table[i];
            connection_t *prev = NULL;

            while (conn) {
                if (now - conn->last_seen > table->timeout_sec) {
                    connection_t *to_free = conn;
                    if (prev) prev->next = conn->next;
                    else table->table[i] = conn->next;
                    conn = conn->next;
                    free(to_free);
                    table->count--;
                } else {
                    prev = conn;
                    conn = conn->next;
                }
            }
        }
    }
    return NULL;
}

int conn_start_timeout_thread(conn_table_t *t) {
    pthread_t tid;
    return pthread_create(&tid, NULL, conn_timeout_worker, t);
}
