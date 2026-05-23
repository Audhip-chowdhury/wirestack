#include "packet.h"
#include "proto.h"
#include "util.h"
#include <string.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

int parse_ip_header(const uint8_t *packet, uint32_t len, packet_info_t *info) {
    if (len < 20) return -1;

    uint8_t version = (packet[0] >> 4) & 0xF;

    if (version == 4) {
        struct ip *iph = (struct ip *)packet;
        info->src_ip = ntohl(iph->ip_src.s_addr);
        info->dst_ip = ntohl(iph->ip_dst.s_addr);
        info->protocol = iph->ip_p;
        return (iph->ip_hl * 4);
    } else if (version == 6) {
        /* BUG VG-008: IPv6 addresses truncated to 32 bits in uint32_t fields */
        // are stored as uint32_t — truncates 128-bit IPv6 to 32 bits
        struct ip6_hdr *ip6h = (struct ip6_hdr *)packet;
        memcpy(&info->src_ip, &ip6h->ip6_src.s6_addr[12], 4);
        memcpy(&info->dst_ip, &ip6h->ip6_dst.s6_addr[12], 4);
        info->protocol = ip6h->ip6_nxt;
        return 40;
    }

    return -1;
}

int parse_packet(const uint8_t *packet, uint32_t len, packet_info_t *info) {
    memset(info, 0, sizeof(*info));
    info->timestamp = vigil_timestamp_us();

    if (len < sizeof(struct ether_header)) return -1;

    const struct ether_header *eth = (const struct ether_header *)packet;
    uint16_t ethertype = ntohs(eth->ether_type);
    const uint8_t *payload = packet + sizeof(struct ether_header);
    uint32_t plen = len - (uint32_t)sizeof(struct ether_header);

    if (ethertype != ETHERTYPE_IP && ethertype != ETHERTYPE_IPV6) {
        return -1;
    }

    int ip_hdr_len = parse_ip_header(payload, plen, info);
    if (ip_hdr_len < 0) return -1;

    const uint8_t *l4 = payload + ip_hdr_len;
    uint32_t l4_len = plen - (uint32_t)ip_hdr_len;

    if (info->protocol == IPPROTO_TCP && l4_len >= (uint32_t)sizeof(struct tcphdr)) {
        struct tcphdr *tcp = (struct tcphdr *)l4;
        info->src_port = ntohs(tcp->th_sport);
        info->dst_port = ntohs(tcp->th_dport);
        info->length = len;
    } else if (info->protocol == IPPROTO_UDP && l4_len >= (uint32_t)sizeof(struct udphdr)) {
        struct udphdr *udp = (struct udphdr *)l4;
        info->src_port = ntohs(udp->uh_sport);
        info->dst_port = ntohs(udp->uh_dport);
        info->length = len;
    } else {
        info->length = len;
    }

    return 0;
}
