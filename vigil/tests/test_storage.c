#include <check.h>
#include "storage.h"
#include <stdio.h>
#include <unistd.h>

START_TEST(test_schema_init) {
    storage_ctx_t ctx;
    char path[256];
    snprintf(path, sizeof(path), "/tmp/vigil_test_%d.db", getpid());

    ck_assert_int_eq(storage_init(&ctx, path, 30), 0);
    traffic_stats_t st;
    memset(&st, 0, sizeof(st));
    strncpy(st.interface, "lo", sizeof(st.interface) - 1);
    st.interval_start = 1000;
    st.interval_sec = 1;
    st.packets_in = 10;
    ck_assert_int_eq(storage_insert_stats(&ctx, &st), 0);

    strncpy(st.interface, "eth0'; DROP TABLE traffic_stats;--", sizeof(st.interface) - 1);
    storage_insert_stats(&ctx, &st);

    storage_close(&ctx);
    unlink(path);
}
END_TEST

Suite *storage_suite(void) {
    Suite *s = suite_create("storage");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_schema_init);
    suite_add_tcase(s, tc);
    return s;
}
