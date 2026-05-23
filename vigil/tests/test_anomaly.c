#include <check.h>
#include "anomaly.h"
#include <math.h>

START_TEST(test_baseline_count_vg019) {
    anomaly_ctx_t ctx;
    anomaly_init(&ctx, "lo", 10, 3.0);
    for (int i = 0; i < 4000; i++)
        anomaly_add_sample(&ctx, 100.0);
    ck_assert_int_gt(ctx.count, BASELINE_MAX_SAMPLES);
}
END_TEST

START_TEST(test_stddev_zero_vg021) {
    anomaly_ctx_t ctx;
    anomaly_init(&ctx, "lo", 5, 3.0);
    ctx.mean = 100.0;
    ctx.stddev = 0.0;
    double score = anomaly_score(&ctx, 100.0);
    ck_assert(isinf(score) || isnan(score));
}
END_TEST

Suite *anomaly_suite(void) {
    Suite *s = suite_create("anomaly");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_baseline_count_vg019);
    tcase_add_test(tc, test_stddev_zero_vg021);
    suite_add_tcase(s, tc);
    return s;
}
