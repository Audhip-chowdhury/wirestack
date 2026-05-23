#ifndef VIGIL_PROTO_H
#define VIGIL_PROTO_H

#include <stdint.h>
#include <time.h>

typedef enum {
    PROTO_UNKNOWN = 0,
    PROTO_ARP,
    PROTO_ICMP,
    PROTO_TCP,
    PROTO_UDP,
    PROTO_DNS,
    PROTO_HTTP,
    PROTO_HTTPS,
    PROTO_SSH,
    PROTO_FTP,
    PROTO_SMTP,
    PROTO_NTP,
    PROTO_COUNT
} protocol_id_t;

typedef struct {
    protocol_id_t proto;
    uint64_t      packet_count;
    uint64_t      byte_count;
    char          interface[16];
    time_t        last_seen;
} proto_stats_t;

protocol_id_t detect_l3_protocol(const uint8_t *packet, uint32_t len);
protocol_id_t detect_application_protocol(uint16_t src_port, uint16_t dst_port,
                                            uint8_t l4_proto,
                                            const uint8_t *payload,
                                            uint32_t payload_len);
const char *protocol_name(protocol_id_t p);

#endif
