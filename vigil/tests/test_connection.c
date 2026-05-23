#include <check.h>
#include "conn.h"
#include "packet.h"
#include <netinet/in.h>
#include <string.h>

START_TEST(test_conn_hash) {
    uint32_t h1 = conn_hash(1, 2, 80, 443, IPPROTO_TCP);
    uint32_t h2 = conn_hash(1, 2, 80, 443, IPPROTO_TCP);
    ck_assert_uint_eq(h1, h2);
}
END_TEST

START_TEST(test_find_or_create) {
    conn_table_t t;
    conn_table_init(&t, 60);
    packet_info_t p;
    memset(&p, 0, sizeof(p));
    p.src_ip = 0x0a000001;
    p.dst_ip = 0x0a000002;
    p.src_port = 1234;
    p.dst_port = 80;
    p.protocol = IPPROTO_TCP;
    connection_t *c1 = conn_find_or_create(&t, &p);
    connection_t *c2 = conn_find_or_create(&t, &p);
    ck_assert_ptr_eq(c1, c2);
    ck_assert_uint_eq(c1->packets, 2);
    conn_table_free(&t);
}
END_TEST

Suite *connection_suite(void) {
    Suite *s = suite_create("connection");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_conn_hash);
    tcase_add_test(tc, test_find_or_create);
    suite_add_tcase(s, tc);
    return s;
}
