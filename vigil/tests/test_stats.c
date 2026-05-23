#include <check.h>
#include "stats.h"
#include <string.h>

START_TEST(test_stats_pps) {
    traffic_stats_t s;
    stats_init(&s, "lo", 10);
    s.packets_in = 100;
    ck_assert_float_eq(stats_pps(&s), 10.0);
}
END_TEST

Suite *stats_suite(void) {
    Suite *s = suite_create("stats");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_stats_pps);
    suite_add_tcase(s, tc);
    return s;
}
