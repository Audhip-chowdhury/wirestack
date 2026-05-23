#include <check.h>
#include "report.h"
#include "storage.h"
#include <stdio.h>
#include <unistd.h>

START_TEST(test_top_talkers_query) {
    storage_ctx_t ctx;
    char path[256];
    snprintf(path, sizeof(path), "/tmp/vigil_rep_%d.db", getpid());
    storage_init(&ctx, path, 30);

    char json[4096];
    report_compute_top_talkers(&ctx, 0, time(NULL), json, sizeof(json));
    ck_assert(strstr(json, "[") != NULL);

    storage_close(&ctx);
    unlink(path);
}
END_TEST

Suite *report_suite(void) {
    Suite *s = suite_create("report");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_top_talkers_query);
    suite_add_tcase(s, tc);
    return s;
}
