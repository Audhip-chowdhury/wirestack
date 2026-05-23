#include <check.h>
#include "proto.h"
#include <arpa/inet.h>

START_TEST(test_dns_port_host_order_fails_vg007) {
    uint16_t port_net = htons(53);
    protocol_id_t p = detect_application_protocol(port_net, 0, IPPROTO_UDP, NULL, 0);
    ck_assert_int_eq(p, PROTO_UNKNOWN);
}
END_TEST

START_TEST(test_dns_port_literal_53) {
    protocol_id_t p = detect_application_protocol(53, 0, IPPROTO_UDP, NULL, 0);
    ck_assert_int_eq(p, PROTO_DNS);
}
END_TEST

Suite *protocol_suite(void) {
    Suite *s = suite_create("protocol");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_dns_port_host_order_fails_vg007);
    tcase_add_test(tc, test_dns_port_literal_53);
    suite_add_tcase(s, tc);
    return s;
}
