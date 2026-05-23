#include <check.h>
#include "alert.h"
#include <string.h>
#include <time.h>

START_TEST(test_cooldown_global_vg010) {
    alert_config_t cfg;
    alert_init(&cfg);
    cfg.cooldown_sec = 60;
    cfg.last_alert_time = time(NULL);
    ck_assert_int_eq(alert_should_fire(&cfg, ALERT_PPS_THRESHOLD), 0);
    ck_assert_int_eq(alert_should_fire(&cfg, ALERT_BPS_THRESHOLD), 0);
}
END_TEST

START_TEST(test_threshold_pps) {
    alert_config_t cfg;
    alert_init(&cfg);
    cfg.enabled = 1;
    cfg.pps_threshold = 100;
    alert_t a;
    int r = alert_check_thresholds(&cfg, "eth0", 200.0, 0.0, &a);
    ck_assert_int_eq(r, 1);
}
END_TEST

Suite *alert_suite(void) {
    Suite *s = suite_create("alert");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_cooldown_global_vg010);
    tcase_add_test(tc, test_threshold_pps);
    suite_add_tcase(s, tc);
    return s;
}
