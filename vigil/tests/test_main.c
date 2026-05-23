#include <check.h>

extern Suite *packet_suite(void);
extern Suite *storage_suite(void);
extern Suite *connection_suite(void);
extern Suite *protocol_suite(void);
extern Suite *alert_suite(void);
extern Suite *stats_suite(void);
extern Suite *report_suite(void);
extern Suite *config_suite(void);
extern Suite *multiface_suite(void);
extern Suite *anomaly_suite(void);

int main(void) {
    int nf = 0;
    SRunner *sr = srunner_create(NULL);

    srunner_add_suite(sr, packet_suite());
    srunner_add_suite(sr, storage_suite());
    srunner_add_suite(sr, connection_suite());
    srunner_add_suite(sr, protocol_suite());
    srunner_add_suite(sr, alert_suite());
    srunner_add_suite(sr, stats_suite());
    srunner_add_suite(sr, report_suite());
    srunner_add_suite(sr, config_suite());
    srunner_add_suite(sr, multiface_suite());
    srunner_add_suite(sr, anomaly_suite());

    srunner_run_all(sr, CK_NORMAL);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (nf == 0) ? 0 : 1;
}
