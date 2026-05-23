#ifndef VIGIL_PACKET_H
#define VIGIL_PACKET_H

#include <stdint.h>

typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  protocol;
    uint32_t length;
    uint64_t timestamp;
    char     interface[16];
    uint8_t  direction;
} packet_info_t;

int parse_packet(const uint8_t *packet, uint32_t len, packet_info_t *info);
int parse_ip_header(const uint8_t *packet, uint32_t len, packet_info_t *info);

#endif
