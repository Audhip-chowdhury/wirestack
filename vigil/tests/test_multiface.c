#include <check.h>
#include "multiface.h"
#include "config.h"

START_TEST(test_multiface_init) {
    multiface_ctx_t m;
    ck_assert_int_eq(multiface_init(&m), 0);
    ck_assert_int_eq(m.count, 0);
}
END_TEST

Suite *multiface_suite(void) {
    Suite *s = suite_create("multiface");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_multiface_init);
    suite_add_tcase(s, tc);
    return s;
}
