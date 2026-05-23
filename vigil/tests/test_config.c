#include <check.h>
#include "config.h"
#include <stdio.h>
#include <unistd.h>

START_TEST(test_parse_defaults) {
    config_t cfg;
    config_set_defaults(&cfg);
    ck_assert_str_eq(cfg.interface, "eth0");
}
END_TEST

START_TEST(test_uint64_overflow_vg016) {
    uint64_t v = parse_uint64("5000000000", "threshold_bps");
    ck_assert(v < 5000000000ULL);
}
END_TEST

Suite *config_suite(void) {
    Suite *s = suite_create("config");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_parse_defaults);
    tcase_add_test(tc, test_uint64_overflow_vg016);
    suite_add_tcase(s, tc);
    return s;
}
