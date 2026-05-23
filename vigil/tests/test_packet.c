#include <check.h>
#include "packet.h"
#include <string.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

static uint8_t sample_tcp_packet[] = {
    0x00,0x11,0x22,0x33,0x44,0x55, 0x66,0x77,0x88,0x99,0xaa,0xbb,
    0x08,0x00,
    0x45,0x00,0x00,0x3c,0x1c,0x46,0x40,0x00,0x40,0x06,0x00,0x00,
    0xc0,0xa8,0x01,0x01, 0xc0,0xa8,0x01,0x02,
    0x30,0x39,0x00,0x50, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x50,0x02,0x20,0x00, 0x00,0x00,0x00,0x00
};

START_TEST(test_parse_valid_tcp) {
    packet_info_t info;
    int r = parse_packet(sample_tcp_packet, sizeof(sample_tcp_packet), &info);
    ck_assert_int_eq(r, 0);
    ck_assert_uint_eq(info.protocol, IPPROTO_TCP);
}
END_TEST

START_TEST(test_parse_truncated) {
    packet_info_t info;
    int r = parse_packet(sample_tcp_packet, 20, &info);
    ck_assert_int_ne(r, 0);
}
END_TEST

Suite *packet_suite(void) {
    Suite *s = suite_create("packet");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_parse_valid_tcp);
    tcase_add_test(tc, test_parse_truncated);
    suite_add_tcase(s, tc);
    return s;
}
