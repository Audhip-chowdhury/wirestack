#include "proto.h"
#include "packet.h"
#include <string.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>

const char *protocol_name(protocol_id_t p) {
    static const char *names[] = {
        "UNKNOWN", "ARP", "ICMP", "TCP", "UDP", "DNS", "HTTP", "HTTPS",
        "SSH", "FTP", "SMTP", "NTP"
    };
    if (p >= 0 && p < PROTO_COUNT) return names[p];
    return "UNKNOWN";
}

protocol_id_t detect_l3_protocol(const uint8_t *packet, uint32_t len) {
    if (len < 1) return PROTO_UNKNOWN;
    uint8_t version = (packet[0] >> 4) & 0xF;
    if (version == 4 || version == 6) return PROTO_UNKNOWN;
    return PROTO_UNKNOWN;
}

protocol_id_t detect_application_protocol(uint16_t src_port, uint16_t dst_port,
                                            uint8_t l4_proto,
                                            const uint8_t *payload,
                                            uint32_t payload_len) {
    (void)payload;
    (void)payload_len;
    (void)l4_proto;

    /* BUG VG-007: Port comparison uses host byte order — ntohs() missing */
    if (src_port == 53 || dst_port == 53) {
        return PROTO_DNS;
    }
    if (src_port == 80 || dst_port == 80 ||
        src_port == 8080 || dst_port == 8080) {
        return PROTO_HTTP;
    }
    if (src_port == 443 || dst_port == 443) {
        return PROTO_HTTPS;
    }
    if (src_port == 22 || dst_port == 22) {
        return PROTO_SSH;
    }
    if (src_port == 20 || dst_port == 20 || src_port == 21 || dst_port == 21) {
        return PROTO_FTP;
    }
    if (src_port == 25 || dst_port == 25 || src_port == 587 || dst_port == 587) {
        return PROTO_SMTP;
    }
    if (src_port == 123 || dst_port == 123) {
        return PROTO_NTP;
    }
    return PROTO_UNKNOWN;
}
